/** 
    Copyright (c) 2022, wicked systems
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
    conditions are met:
    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
      disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
      disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of wicked systems nor the names of its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/
#include "apu.h"
#include "core.h"
#include <limits>
#include <numbers>

namespace dsp {

      constexpr unsigned int v_size_auto = 0;
      constexpr unsigned int v_flag_none = 0;
      constexpr unsigned int v_flag_persist = 1;

/* op_*
   dsp_descend() operations
*/
      constexpr unsigned int op_null = 0u;
      constexpr unsigned int op_render = 1u;
      constexpr unsigned int op_copy = 2u;
      constexpr unsigned int op_mix = 4u;
      constexpr unsigned int op_sync = 16u;
      constexpr unsigned int op_resync = op_render | op_sync;

/* ff_*
   dsp_fork() flags
*/
      constexpr unsigned int ff_static        = v_flag_persist;           // return vector is not discarded when the branch is folded
      constexpr unsigned int ff_vector_flags  = v_flag_persist;
      constexpr unsigned int ff_default       = 0u;

      apu::apu() noexcept:
      apu(nullptr, nullptr)
{
}

      apu::apu(ppu* pp, mmu* mm) noexcept:
      m_ppu(pp),
      m_mmu(mm),
      m_sample_format(default_sample_format),
      m_sample_rate(default_sample_rate),
      m_control_rate(default_control_rate),
      m_core_head(nullptr),
      m_core_tail(nullptr),
      m_dps_page_head(nullptr),
      m_dps_page_tail(nullptr),
      m_dps_page_count(0),
      m_dss_page_head(nullptr),
      m_dss_page_tail(nullptr),
      m_dss_page_count(0),
      m_dvf_base(nullptr),
      m_dvf_last(nullptr),
      m_dvf_size(0),
      m_process_head(nullptr),
      m_process_tail(nullptr),
      m_iteration_fingerprint(0u),
      m_busy(false)
{
      // attach_variable("time");
      // attach_variable("dt");
}

      apu::~apu()
{
      dsp_dispose_process_list();
      dsg_clear();
      dvf_dispose(false);
      dss_dispose(false);
      dps_dispose(false);
}

/* dsg_find()
*/
bool  apu::dsg_find(core* node) const noexcept
{
      core* i_core = m_core_head;
      if(i_core != nullptr) {
          do {
              if(i_core == node) {
                  return true;
              }
              i_core = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }
      return false;
}

/* dsg_bind()
   insert the node into the root node list
*/
void  apu::dsg_bind(core* node) noexcept
{
      // link at the end of internal tree list
      if(m_core_tail != nullptr) {
          m_core_tail->m_core_next = node;
      } else
          m_core_head = node;
      m_core_tail = node;

      // set the internal state of the newly attached tree
      dsg_acquire(node);
      node->m_core_next = m_core_head;
}

/* dsg_acquire()
*/
void  apu::dsg_acquire(core* node) noexcept
{
      node->m_target = this;
      node->m_dov = v_invalid;
      node->m_dpc = 0;
}

void  apu::dsg_dispose(core* node) noexcept
{
}

void  apu::dsg_release(core* node) noexcept
{
      node->m_dcc = 0;
      node->m_dpc = 0;
      node->m_dov = v_invalid;
      node->m_target = nullptr;
}

void  apu::dsg_drop(core* node) noexcept
{
      core* i_prev = m_core_tail;
      core* i_core = m_core_head;

      // find the node linked into the root node list before the current node
      if(i_core != nullptr) {
          do {
              if(i_core == node) {
                  break;
              }
              i_prev = i_core;
              i_core = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }

      // unlink from the root node list
      if(i_core != i_prev) {
          i_prev->m_core_next = i_core->m_core_next;
          if(m_core_head == i_core) {
              m_core_head = i_core->m_core_next;
          }
          if(m_core_tail == i_core) {
              m_core_tail = i_prev;
          }
      }
      else {
          m_core_head = nullptr;
          m_core_tail = nullptr;
      }

      // reset the internal state of the current node
      i_core->m_core_next = nullptr;
}

/* dsg_lookup_intern
   recursively look for occurences of node within the given root tree, return a count of how many matches were found
*/
int   apu::dsg_lookup_intern(core* root, core* node) noexcept
{
      // if the two nodes differ then look for convergence in root's subtrees
      if(node != root) {
          int    l_count = 0;
          gate*  i_gate = root->m_gate_head;
          while(i_gate != nullptr) {
              core* l_tree  = i_gate->m_source;
              if(l_tree != nullptr) {
                  l_count += dsg_lookup_intern(l_tree, node);
              }
              i_gate = i_gate->m_gate_next;
          }
          return l_count;
      }
      return 1;
}

/* dsg_lookup_global
   recursively look for occurences of node within the globally attached trees, return a count of how many matches were found
*/
int   apu::dsg_lookup_global(core* node) noexcept
{
      int   l_count = 0;
      core* i_core = m_core_head;
      if(i_core != nullptr) {
          do {
              l_count += dsg_lookup_intern(i_core, node);
              i_core   = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }
      return l_count;
}


/* dsg_converge_intern
   recursively descend into tree and lookup occurences of each subtree into the tree identified by root
*/
bool  apu::dsg_converge_intern(core* root, core* tree) noexcept
{
      int   l_converge_count = 0;
      int   l_converge_success = 0;
      gate* i_gate = tree->m_gate_head;
      while(i_gate != nullptr) {
          core* l_tree = i_gate->m_source;
          if(l_tree != nullptr) {
              int l_reference_count = 
                  dsg_lookup_intern(root, l_tree) +
                  dsg_lookup_global(l_tree);
              if(l_reference_count <= 1) {
                  bool l_descend_success = dsg_converge_intern(root, l_tree);
                  if(l_descend_success) {
                      l_converge_success++;
                  }
              } else
              if(l_reference_count > 1) {
                  bool l_attach_success = dsg_converge(l_tree);
                  if(l_attach_success) {
                      l_converge_success++;
                  }
              }
              l_tree->m_dcc = l_reference_count;
              l_converge_count++;
          }
          i_gate = i_gate->m_gate_next;
      }
      dsg_acquire(tree);
      return l_converge_success == l_converge_count;
}

/* dsg_converge_global
*/
bool  apu::dsg_converge_global(core* tree) noexcept
{
      int   l_converge_count = 0;
      int   l_converge_success = 0;
      core* i_core = m_core_head;
      if(i_core != nullptr) {
          do {
              bool l_descend_success = dsg_converge_intern(i_core, tree);
              if(l_descend_success) {
                  l_converge_success++;
              }
              l_converge_count++;
              i_core = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }
      return l_converge_success == l_converge_count;
}

/* dsg_diverge_intern
*/
void  apu::dsg_diverge_intern(core* tree) noexcept
{
      gate* i_gate = tree->m_gate_head;
      while(i_gate != nullptr) {
          core* l_tree = i_gate->m_source;
          if(l_tree != nullptr) {
              dsg_diverge_intern(l_tree);
          }
          i_gate = i_gate->m_gate_next;
      }
      if(tree->m_dcc > 0) {
          tree->m_dcc--;
          if(tree->m_dcc == 0) {
              dsg_dispose(tree);
          }
      }
      if(tree->m_dcc == 0) {
          dsg_release(tree);
      }
}

bool  apu::dsg_converge(core* tree) noexcept
{
      bool l_attached = dsg_find(tree);
      if(l_attached == false) {
          bool l_converge_success = dsg_converge_intern(tree, tree);
          if(l_converge_success) {
              tree->m_dcc++;
          }
          return l_converge_success;
      }
      return true;
}

/* dsg_diverge()
*/
bool  apu::dsg_diverge(core* tree) noexcept
{
      bool l_attached = dsg_find(tree);
      if(l_attached) {
          dsg_diverge_intern(tree);
      }
      return l_attached;
}

/* dsg_clear()
*/
void  apu::dsg_clear() noexcept
{
      core* i_core = m_core_head;
      if(i_core != nullptr) {
          do {
              dsg_diverge(i_core);
              i_core = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }
}

/* dps_make_page()
*/
bool  apu::dps_make_page() noexcept
{
      void* l_page_ptr = malloc(memory_vector_page);
      if(l_page_ptr) {
          auto l_page_base = reinterpret_cast<sample_page_t*>(l_page_ptr);
          l_page_base->head.info.data_used_base = std::addressof(l_page_base->data[0]);
          l_page_base->head.info.data_used_last = 
              l_page_base->head.info.data_used_base + sample_page_t::page_effective_samples;
          l_page_base->head.info.data_used_ptr  = 
              l_page_base->head.info.data_used_base;
          std::memset(l_page_base->head.map.bytes, 0, sample_page_t::map_size);
          l_page_base->head.info.page_next = nullptr;
          if(m_dss_page_tail) {
              m_dss_page_tail->head.info.page_next = l_page_base;
          } else
              m_dss_page_head = l_page_base;
          m_dss_page_tail = l_page_base;
          m_dss_page_count++;
          return true;
      }
      return false;
}

bool  apu::dps_map_get_bit(sample_page_t* page, int bit) noexcept
{
      return page->head.map.bytes[bit >> 3] & (1 << (bit & 3));
}

void  apu::dps_map_set_bit(sample_page_t* page, int bit, bool value) noexcept
{
      int  l_byte = bit / 8;
      int  l_mask = 1 << (bit & 3);
      if(value) {
          page->head.map.bytes[l_byte] |= l_mask;
      } else
          page->head.map.bytes[l_byte] &= (~l_mask);
}

int   apu::dps_map_find_free(sample_page_t* page, int bit_count) noexcept
{
      int i_byte = 0;
      while(i_byte < sample_page_t::map_size) {
          if(page->head.map.bytes[i_byte] != 0xff) {
              int i_bit = i_byte << 3;
              while(i_bit < sample_page_t::map_bits - bit_count) {
                  int  i_free  = i_bit;
                  int  l_last  = i_free + bit_count;
                  bool l_found = true;
                  while(i_free < l_last) {
                      if(dps_map_get_bit(page, i_free) == true) {
                          l_found = false;
                      } else
                      if(l_found == false) {
                         break;
                      }
                      i_free++;
                  }
                  if(l_found) {
                      if(i_free == bit_count) {
                          return i_bit;
                      }
                  }
                  i_bit = i_free;
              }
          }
          i_byte++;
      }
      return -1;
}

void  apu::dps_map_mark_used(sample_page_t* page, int bit_index, int bit_count) noexcept
{
      int i_bit      = bit_index;
      int i_bit_last = bit_index + bit_count;
      while(i_bit < i_bit_last) {
          dps_map_set_bit(page, i_bit, true);
          ++i_bit;
      }
}

void  apu::dps_map_mark_free(sample_page_t* page, int bit_index, int bit_count) noexcept
{
      int i_bit      = bit_index;
      int i_bit_last = bit_index + bit_count;
      while(i_bit < i_bit_last) {
          dps_map_set_bit(page, i_bit, false);
          ++i_bit;
      }
}

fptype* apu::dps_map_get_block_address(sample_page_t* page, int bit_index) noexcept
{
      return page->head.info.data_used_base + bit_index * sample_page_t::map_bit_samples;
}

int   apu::dps_map_get_block_size(int bit_count) noexcept
{
      return bit_count * sample_page_t::map_bit_samples;
}

void  apu::dps_map_clear(sample_page_t* page) noexcept
{
      std::memset(page->head.map.bytes, 0, sample_page_t::map_size);
}

/* dps_acquire()
   request memory on the persistent sample store for the given vector
*/
bool  apu::dps_acquire(fptype*& address, int& capacity, int requested_size) noexcept
{
      // get memory from the current page
      if(m_dps_page_tail != nullptr) {
          int l_map_span = get_div_ub(requested_size, sample_page_t::map_bit_samples);
          int l_map_bit  = dps_map_find_free(m_dss_page_tail, l_map_span);
          if(l_map_bit >= 0) {
              address  = dps_map_get_block_address(m_dps_page_tail, l_map_bit);
              capacity = dps_map_get_block_size(l_map_span);
              dps_map_mark_used(m_dps_page_tail, l_map_bit, l_map_span);
              return true;
          } else
          if(requested_size > m_dps_page_tail->head.info.data_used_last - m_dps_page_tail->head.info.data_used_base) {
              printdbg(
                  "Refusing to supply %d samples for a single vector allocation;\n"
                  "    Please reconfigure `memory_vector_pool` if that value sounds reasonable to you.",
                  __FILE__,
                  __LINE__,
                  requested_size
              );
              return false;
          }
          if(m_dps_page_tail->head.info.page_next != nullptr) {
              m_dps_page_tail =
                  m_dps_page_tail->head.info.page_next;
              dps_map_clear(m_dps_page_tail);
              return dps_acquire(address, capacity, requested_size);
          }
      }  
      // create new page and try again if memory in current page is insufficiend
      bool l_extend_assert = dps_make_page();
      if(l_extend_assert == true) {
          return  dps_acquire(address, capacity, requested_size);
      } else
          return false;
      return true;
}

/* dps_release()
*/
bool  apu::dps_release(fptype*& address, int& capacity) noexcept
{
      auto i_page = m_dps_page_head;
      auto l_found = false;
      while(i_page != nullptr) {
          int l_bit = address - i_page->head.info.data_used_base;
          if((l_bit >= 0) &&
              (l_bit < sample_page_t::map_bits)) {
              address = nullptr;
              capacity = 0;
              dps_map_mark_free(i_page, l_bit, capacity / sample_page_t::map_bit_samples);
              l_found = true;
              break;
          }
          i_page = i_page->head.info.page_next;
      }
      if(l_found == false) {
          printdbg(
              "Vector at offset %p was not supplied by the persistent vector store.",
              __FILE__,
              __LINE__,
              address
          );
          return false;
      }
      return true;
}

/* dps_free_page()
*/
void  apu::dps_free_page(sample_page_t* page) noexcept
{
      free(page);
}

/* dps_clear()
*/
void  apu::dps_clear() noexcept
{
      m_dps_page_tail = m_dps_page_head;
      if(m_dps_page_tail != nullptr) {
          dps_map_clear(m_dps_page_tail);
      }     
}

/* dps_dispose()
*/
void  apu::dps_dispose(bool) noexcept
{
      sample_page_t* i_next;
      sample_page_t* i_page = m_dps_page_head;
      while(i_page != nullptr) {
          i_next = i_page->head.info.page_next;
          dps_free_page(i_page);
          i_page = i_next;
      }
}

/* dds_make_page()
*/
bool  apu::dss_make_page() noexcept
{
      void* l_page_ptr = malloc(memory_vector_page);
      if(l_page_ptr) {
          auto l_page_base = reinterpret_cast<sample_page_t*>(l_page_ptr);
          int  l_page_sample_size = memory_vector_page / sizeof(fptype);
          int  l_page_sample_count = l_page_sample_size - memory_vector_block;
          l_page_base->head.info.data_used_base = std::addressof(l_page_base->data[0]);
          l_page_base->head.info.data_used_last = l_page_base->head.info.data_used_base + l_page_sample_count;
          l_page_base->head.info.data_used_ptr  = l_page_base->head.info.data_used_base;
          l_page_base->head.info.page_next = nullptr;
          if(m_dss_page_tail) {
              m_dss_page_tail->head.info.page_next = l_page_base;
          } else
              m_dss_page_head = l_page_base;
          m_dss_page_tail = l_page_base;
          m_dss_page_count++;
          return true;
      }
      return false;
}

/* dds_acquire()
   request memory of given size from the sample store
*/
bool  apu::dss_acquire(fptype*& address, int& capacity, int requested_size) noexcept
{
      // get memory from the current page
      if(m_dss_page_tail != nullptr) {
          int l_size = get_round_value(requested_size, memory_vector_block);
          int l_samples_available =
              m_dss_page_tail->head.info.data_used_last -
              m_dss_page_tail->head.info.data_used_ptr;
          if(l_size <= l_samples_available) {
              address  = m_dss_page_tail->head.info.data_used_ptr;
              capacity = l_size;
              m_dss_page_tail->head.info.data_used_ptr += l_size;
              return true;
          } else
          if(requested_size > m_dss_page_tail->head.info.data_used_last - m_dss_page_tail->head.info.data_used_base) {
              printdbg(
                  "Refusing to supply %d samples for a single vector allocation;\n"
                  "    Please reconfigure `memory_vector_pool` if that value sounds reasonable to you.",
                  __FILE__,
                  __LINE__,
                  requested_size
              );
              return false;
          }
          if(m_dss_page_tail->head.info.page_next != nullptr) {
              m_dss_page_tail =
                  m_dss_page_tail->head.info.page_next;
              m_dss_page_tail->head.info.data_used_ptr = 
                  m_dss_page_tail->head.info.data_used_base;
              return dss_acquire(address, capacity, requested_size);
          }
      }
      
      // create new page and try again if memory in current page is insufficiend
      bool l_extend_assert = dss_make_page();
      if(l_extend_assert == true) {
          return  dss_acquire(address, capacity, requested_size);
      } else
          return false;
}

/* dds_release()
*/
bool  apu::dss_release(fptype*& address, int& capacity) noexcept
{
      return true;
}

/* dds_free_page()
*/
void  apu::dss_free_page(sample_page_t* page) noexcept
{
      free(page);
}

/* dds_clear()
*/
void  apu::dss_clear() noexcept
{
      m_dss_page_tail = m_dss_page_head;
      if(m_dss_page_tail != nullptr) {
          m_dss_page_tail->head.info.data_used_ptr = m_dss_page_tail->head.info.data_used_base;
      }     
}

/* dds_dispose()
*/
void  apu::dss_dispose(bool) noexcept
{
      sample_page_t* i_next;
      sample_page_t* i_page = m_dss_page_head;
      while(i_page != nullptr) {
          i_next = i_page->head.info.page_next;
          dss_free_page(i_page);
          i_page = i_next;
      }
}

/* dvf_reserve()
   reserve space for the specified number of vectors in the vector file
*/
bool  apu::dvf_reserve(int count) noexcept
{
      if(count > m_dvf_size) {
          int    l_reserve_size = get_round_value(count, global::cache_small_max);
          void*  l_reserve_ptr  = std::realloc(m_dvf_base, l_reserve_size * sizeof(vector_t));
          if(l_reserve_ptr != nullptr) {
              vector_t* l_reserve_base = reinterpret_cast<vector_t*>(l_reserve_ptr);
              vector_t* l_initialise_base = l_reserve_base + m_dvf_size;
              if(std::size_t l_initialise_size = l_reserve_size - m_dvf_size; l_initialise_size > 0u) {
                  std::memset(l_initialise_base, 0, l_initialise_size * sizeof(vector_t));
              }
              m_dvf_base = l_reserve_base;
              m_dvf_last = m_dvf_base + l_reserve_size;
              m_dvf_size = l_reserve_size;
              return true;
          }
          return false;
      }
      return true;
}

/* dvf_lookup()
   find the vector owning the given address
*/
int   apu::dvf_lookup(fptype* address) noexcept
{
      auto l_dvf_iter = dvf_get_ptr(s_process->branch_tail->vector_assign_ub);
      auto l_dvf_last = l_dvf_iter;
      if(l_dvf_iter != nullptr) {
          do {
              if(l_dvf_iter < m_dvf_base) {
                  l_dvf_iter = m_dvf_last - 1;
              }
              if(l_dvf_iter->data != nullptr) {
                  if((l_dvf_iter->data >= address) &&
                      ((l_dvf_iter->data + l_dvf_iter->size) < address)) {
                      return l_dvf_iter - m_dvf_base;
                  }
              }
              l_dvf_iter--;
          }
          while(l_dvf_iter != l_dvf_last);
      }
      return v_invalid;
}

/* dvf_get()
   get the pointer to the vector specified by index
*/
apu::vector_t*  apu::dvf_get_ptr(int index) const noexcept
{
      return m_dvf_base + index;
}

/* dvf_get_data_lazy()
   get a pointer to the specified vector data, but also reserve memory for it if it doesn't have enough (or none at all,
   as it's often the case of a freshly acquired register)
*/
fptype*  apu::dvf_get_data_lazy(int index) const noexcept
{
      vector_t* p_vector = dvf_get_ptr(index);
      return    p_vector->data;
}

/* dvf_get_data_immediate()
   get a pointer to the specified vector data, but also reserve memory for it if it doesn't have enough (or none at all,
   as it's often the case of a freshly acquired vector)
*/
fptype*  apu::dvf_get_data_immediate(int index) noexcept
{
      int        l_size;
      vector_t*  p_vector = dvf_get_ptr(index);

      if(p_vector->r_size < 0) {
          // when size is lower than zero this has been already allocated
          return p_vector->data;
      } else
      if(p_vector->r_size == v_size_auto) {
          // when size is zero compute the buffer size to `sample_rate * sample_size * dt
          l_size = get_round_value(
              dsp_get_sample_count() * dsp_get_sample_size(),
              memory_vector_block
          );
      } else
          l_size = p_vector->r_size;
      
      // reserve memory
      bool  l_acquire_success;
      bool  l_acquire_persist = p_vector->r_keep;
      if(l_acquire_persist) {
          if(p_vector->s_keep_bit) {
              if(p_vector->data != nullptr) {
                  if(p_vector->size <= l_size) {
                      return p_vector->data;
                  }
                  dps_release(p_vector->data, p_vector->size);
              }
          } else
          if(p_vector->data != nullptr) {
              dss_release(p_vector->data, p_vector->size);
          }
          l_acquire_success = dps_acquire(p_vector->data, p_vector->size, l_size);
      } else
      if(true) {
          if(p_vector->s_keep_bit) {
              if(p_vector->data != nullptr) {
                  dps_release(p_vector->data, p_vector->size);
              }
          } else
          if(p_vector->size < l_size) {
              if(p_vector->data != nullptr) {
                  dss_release(p_vector->data, p_vector->size);
              }
          }
          l_acquire_success = dss_acquire(p_vector->data, p_vector->size, l_size);
      }

      if(l_acquire_success) {
          p_vector->s_far_bit = false;
          p_vector->s_keep_bit = l_acquire_persist;
          p_vector->r_size = -1;
          p_vector->r_keep = false;
          return p_vector->data;
      } else
          printdbg(
              "Allocation for vector `v%d` failed.`\n",
              __FILE__,
              __LINE__,
              index
          );

      return nullptr;
}

/* dvf_acquire()
   allocate an unused vector in the current branch;
   No memory is actually reserved for the newly created vector - we want that to be a lazy operation, as the information about
   the buffer size cannot be definitely known at branch-creation time (cores can choose a different sampling rate) and we don't
   want to speculate it.
*/
int   apu::dvf_acquire(int size, unsigned int flags) noexcept
{
      vector_t* p_vector;
      int i_vector = s_process->branch_tail->vector_assign_ub;
      while(i_vector != v_invalid) {
          if(i_vector < m_dvf_size) {
              p_vector = dvf_get_ptr(i_vector);
              if(p_vector->s_used_bit == false) {
                  if(size == v_size_auto) {
                      p_vector->r_size = v_size_auto;
                  } else
                  if((size > 0) &&
                      (size < std::numeric_limits<short int>::max())) {
                      p_vector->r_size = size;
                  } else
                      return v_invalid;
                  p_vector->r_keep =(flags & v_flag_persist) != 0;
                  p_vector->s_used_bit = true;
                  s_process->branch_tail->vector_assign_ub = i_vector + 1;
                  return i_vector;
              }
          } else
          if(m_dvf_size < std::numeric_limits<short int>::max() - global::cache_small_max) {
              bool l_resize_success = dvf_reserve(m_dvf_size + global::cache_small_max);
              if(l_resize_success) {
                  continue;
              } else
                  return v_invalid;
          } else
              return v_invalid;
          i_vector++;
      }
      return v_invalid;
}

/* dvf_release()
*/
void  apu::dvf_release(int index, bool force_persist_release) noexcept
{
      vector_t* p_vector = dvf_get_ptr(index);
      if(p_vector->s_far_bit) {
          p_vector->data = nullptr;
          p_vector->size = 0;
          p_vector->s_far_bit = false;
      } else
      if(p_vector->s_keep_bit) {
          if(force_persist_release) {
              dps_release(p_vector->data, p_vector->size);
              p_vector->s_keep_bit = false;
          }
      }
      p_vector->s_used_bit = false;
      if(s_process->branch_tail->vector_assign_ub == index + 1) {
          s_process->branch_tail->vector_assign_ub = index;
      }
}

/* dvf_clear()
   remove ownership of all vectors in the current branch
*/
void  apu::dvf_clear(bool reset) noexcept
{
      int  i_vector = s_process->branch_tail->vector_assign_ub - 1;
      while(i_vector >= s_process->branch_tail->vector_assign_lb) {
          vector_t* p_vector = dvf_get_ptr(i_vector);
          if(p_vector->s_used_bit) {
              if(p_vector->s_keep_bit == false) {
                  dvf_release(i_vector, true);
              }
          }
          i_vector--;
      }
      if(reset) {
          s_process->branch_tail->vector_assign_ub = s_process->branch_tail->vector_assign_lb;
      }
}

/* dvf_dispose()
   free the memory associated with the vector file
*/
void  apu::dvf_dispose(bool clear) noexcept
{
      free(m_dvf_base);
      if(clear) {
          m_dvf_base = nullptr;
          m_dvf_last = nullptr;
          m_dvf_size = 0;
      }
}

/* dsp_find_process()
   find the process associated with given core_ptr
*/
apu::process_t*   apu::dsp_find_process(core* core_ptr) noexcept
{
    process_t* i_process = m_process_head;
    while(i_process != nullptr) {
        if(i_process->owner == core_ptr) {
            return i_process;
        }
        i_process = i_process->next;
    }
    return nullptr;
}

/* dsp_make_process()
   allocate and initialize a new process structure for the given core_ptr 
*/
apu::process_t*   apu::dsp_make_process(core* core_ptr) noexcept
{
      auto p_process = reinterpret_cast<process_t*>(malloc(sizeof(process_t)));
      if(p_process != nullptr) {
          // initialize the new process
          p_process->sample_format = m_sample_format;
          p_process->sample_rate = m_sample_rate;
          p_process->return_flags = dc::e_okay;
          p_process->return_vector = dc::v_default;
          p_process->vector_assign_lb = 0;
          p_process->vector_assign_ub = 0;
          p_process->gain = 1.0f;
          p_process->bias = 0.0f;
          p_process->branch_parent = nullptr;
          p_process->owner = core_ptr;
          p_process->state = dc::pc_state_ready;
          p_process->branch_head = p_process;
          p_process->branch_tail = p_process;
          p_process->step_latency = 1.0f / static_cast<float>(m_control_rate);
          p_process->step_time = 0.0f;
          p_process->dt = 0.0f;
          p_process->time = 0.0f;
          p_process->omega = 0.0f;
          p_process->prev = m_process_tail;
          p_process->next = nullptr;
          // link the new process into the process list
          if(m_process_tail) {
              m_process_tail->next = p_process;
          } else
              m_process_head = p_process;
          m_process_tail = p_process;
      }
      return p_process;
}

/* dsp_free_process()
*/
apu::process_t*   apu::dsp_free_process(process_t* process) noexcept
{
      if(process->prev) {
          process->prev->next = process->next;
      } else
          m_process_head = process->next;

      if(process->next) {
          process->next->prev = process->prev;
      } else
          m_process_tail = process->prev;
      free(process);
      return nullptr;
}

void  apu::dsp_dispose_process_list() noexcept
{
      process_t* i_process_prev;
      process_t* i_process = m_process_tail;
      while(i_process != nullptr) {
          i_process_prev = i_process->prev;
          dsp_free_process(i_process);
          i_process = i_process_prev;
      }
}

void  apu::dsp_push(process_base_t* process, branch_base_t&  branch, int return_vector) noexcept
{
      // set up the new branch
      branch.sample_format = m_sample_format;
      branch.sample_rate = m_sample_rate;
      branch.return_flags = dc::e_okay;
      branch.return_vector = return_vector;
      branch.vector_assign_lb = process->branch_tail->vector_assign_ub;
      branch.vector_assign_ub = branch.vector_assign_lb;
      branch.gain = 1.0f;
      branch.bias = 0.0f;
      branch.branch_parent = process->branch_tail;

      // push the new branch onto the stack
      process->branch_tail = std::addressof(branch);
}

int   apu::dsp_fork(process_base_t* process, core* target, unsigned int op, unsigned int flags) noexcept
{
      auto     l_op = op;
      auto     l_flags = flags;
      int      l_return_vector = process->branch_tail->return_vector;
      int      l_source_vector;
      int      l_forward_vector;
      bool     l_source_success;
      branch_t l_branch;

      // allocate return vectors for the branch in the current stack
      if(l_op & op_render) {
          if(l_flags & ff_static) {
              if(target->m_dov >= 0) {
                  l_forward_vector = target->m_dov;
              } else
                  l_forward_vector = dvf_acquire(0, l_flags & ff_vector_flags);
          } else
              l_forward_vector = dvf_acquire(0, l_flags & ff_vector_flags);
      } else
          l_forward_vector = l_return_vector;

      // create a new branch, push it on top of the rendering context
      dsp_push(process, l_branch, l_forward_vector);

      // render onto the new branch
      l_source_vector  = dsp_descend(process, target, l_op & op_resync);
      l_source_success = l_source_vector != v_invalid;

      // return to the calling branch
      dsp_pop(process, l_branch);

      if(l_source_success) {
          if(l_op & op_render) {
              // transfer the data from the uplevel branch return vector onto the current branch's return vector and
              // set the return vector accordingly
              if(l_return_vector != l_source_vector) {
                  fptype* p_return_vector = dvf_get_data_immediate(l_return_vector);
                  fptype* p_source_vector = dvf_get_data_immediate(l_source_vector);
                  if(l_op & op_copy) {
                      pcm_mov(p_return_vector, p_source_vector, dsp_get_sample_count());
                  } else
                  if(l_op & op_mix) {
                      pcm_add(p_return_vector, p_source_vector, dsp_get_sample_count());
                  } else
                      l_return_vector = l_source_vector;
              } else
                  l_return_vector = l_source_vector;

              // decrease the render pass counter and release render resources associated with this node when it reaches 0
              if(l_flags & ff_static) {
                  if(target->m_dov >= 0) {
                      if(target->m_dpc > 0) {
                          target->m_dpc--;
                          if(target->m_dpc == 0) {
                              dvf_release(target->m_dov, true);
                          }
                      }
                  }
              }
          }
          return l_return_vector;
      }
      return v_invalid;
}

int   apu::dsp_descend(process_base_t* process, core* target, unsigned int op) noexcept
{
      auto l_op = op;
      int  l_return_vector = target->m_dov;

      if(target->m_dcc > 1) {
          // node is referenced multiple times: fork a new branch such that its result is cached onto the branch return vector
          target->m_dcc = 0 - target->m_dcc;
          l_return_vector = dsp_fork(process, target, l_op | op_copy, ff_static);
          target->m_dcc = 0 - target->m_dcc;
      } else
      if(target->m_hash != m_iteration_fingerprint) {
          int    l_source_count   = 0;
          int    l_source_success = 0;
          gate*  i_gate = target->m_gate_head;
          // pre-visit node setup
          target->m_dov  = process->branch_tail->return_vector;
          target->m_dpc  = abs(target->m_dcc);
          // run through the child nodes in depth-first and dispatch the op
          while(i_gate != nullptr) {
              if(i_gate->m_enable_bit) {
                  if(core* l_source = i_gate->m_source; l_source != nullptr) {
                      int       l_source_vector;
                      vector_t* p_source_vector;
                      if(l_source_count == 0) {
                          l_source_vector = dsp_descend(process, l_source, l_op);
                      } else
                          l_source_vector = dsp_fork(process, l_source, l_op, ff_default);
                      if(l_source_vector != v_invalid) {
                          p_source_vector = dvf_get_ptr(l_source_vector);
                          i_gate->bind(p_source_vector->data);
                          l_source_success++;
                      }
                      l_source_count++;
                  }
              }
              i_gate = i_gate->m_gate_next;
          }
          // perform the processing pertaining to the current node
          if(l_source_success == l_source_count) {
              bool l_render_assert = true;
              bool l_branch_assert = true;
              if(l_op & op_sync) {
                  target->sync(process->dt);
              }
              if(l_op & op_render) {
                  // dispatch render operation
                  if(l_op & op_mix) {
                      l_render_assert = target->render(dc::op_render_additive);
                  } else
                      l_render_assert = target->render(dc::op_none);
                  // set the error flags to the branch
                  if(l_render_assert == false) {
                      process->branch_tail->return_flags |= dc::e_render_fault;
                  }
                  if(process->branch_tail->return_vector < 0) {
                      process->branch_tail->return_flags |= dc::e_return_fault;
                  }
                  // assert branch status
                  l_branch_assert = process->branch_tail->return_flags == dc::e_okay;
              }
              if(l_render_assert &&
                  l_branch_assert) {
                  target->m_hash  = m_iteration_fingerprint;
                  l_return_vector = process->branch_tail->return_vector;
              } else
                  l_return_vector = v_invalid;
          } else
              l_return_vector = v_invalid;
      }
      return l_return_vector;
}

void  apu::dsp_pop(process_base_t* process, branch_base_t& branch) noexcept
{
      if(process->branch_tail == std::addressof(branch)) {
          // free used registers
          dvf_clear();
          // return to the calling branch
          process->branch_tail = branch.branch_parent;
          // propagate the branch return flags back
          process->branch_tail->return_flags |= branch.return_flags;
      } else
          printdbg(
              "Unmatched stack tail `%p` != `%p`\n",
              __FILE__,
              __LINE__,
              std::addressof(branch),
              process->branch_tail
          );
}

void  apu::dsp_sync(core* node, float dt) noexcept
{
      gate* i_gate = node->m_gate_head;
      // run through the child nodes in depth-first and dispatch the sync operation
      while(i_gate != nullptr) {
          if(core* l_source = i_gate->m_source; l_source != nullptr) {
              dsp_sync(l_source, dt);
          }
          i_gate = i_gate->m_gate_next;
      };
      // call node's sync() method
      node->sync(dt);
}

void  apu::dsp_reset_fingerprint() noexcept
{
      m_iteration_fingerprint = 
          m_iteration_fingerprint + 1;
      m_iteration_fingerprint = 
          m_iteration_fingerprint ^ 
          (reinterpret_cast<std::size_t>(this) & std::numeric_limits<unsigned int>::max());
}

void  apu::dsp_join_event(core*) noexcept
{
}

void  apu::dsp_part_event(core*) noexcept
{
}

ppu*  apu::get_ppu() const noexcept
{
      return m_ppu;
}

mmu*  apu::get_mmu() const noexcept
{
      return m_mmu;
}

bool  apu::attach(core* core_ptr) noexcept
{
      if(core_ptr != nullptr) {
          if(core_ptr->m_target == nullptr) {
              if(dsg_converge(core_ptr)) {
                  dsg_bind(core_ptr);
                  dsp_make_process(core_ptr);
                  return true;
              }
          }
      }
      return false;
}

bool  apu::detach(core* core_ptr) noexcept
{
      if(core_ptr != nullptr) {
          if(core_ptr->m_target == this) {
              dsg_diverge(core_ptr);
              dsg_drop(core_ptr);
              if(process_t* p_process = dsp_find_process(core_ptr); p_process != nullptr) {
                  dsp_free_process(p_process);
              } else
                  printdbg(
                      "No process found for DSP core `%p`; Process was created when DSP core was attached.\n",
                      __FILE__,
                      __LINE__,
                      core_ptr
                  );
              return true;
          }
      }
      return false;
}

bool  apu::render() noexcept
{
      return render(0.0f);
}

bool  apu::render(float dt) noexcept
{
      dc_t         l_dc;
      bool         l_rs;
      unsigned int l_op = op_render;
      if(m_process_head != nullptr) {
          l_rs = true;
          s_process = m_process_head;
          m_busy = true;
          dsp_save(l_dc, this);
          if(true) {
              int        l_render_count = 0;
              int        l_render_success = 0;
              bool       l_descend_success;
              process_t* i_process;

              // we have a sync operation included into this render, react as such
              if(dt > 0.0f) {
                  l_op = l_op | op_sync;
              }
              // update fingerprint
              dsp_reset_fingerprint();

              // run through the active processes and render
              while(s_process != nullptr) {
                  if(s_process->state != dc::pc_state_suspend) {
                      if(s_process->dt += dt;
                          s_process->dt >= 0.0f) {
                          s_process->return_flags = dc::e_okay;
                          s_process->return_vector = dvf_acquire();
                          l_descend_success = dsp_descend(s_process, s_process->owner, l_op) != v_invalid;
                          if(l_descend_success) {
                              s_process->time += s_process->dt;
                              if(s_process->time >= 1.0f) {
                                  s_process->time -= 1.0f;
                              }
                              s_process->omega += s_process->dt * std::numbers::pi * 2.0f;
                              if(s_process->omega >= std::numbers::pi * 2.0f) {
                                  s_process->omega -= std::numbers::pi * 2.0f;
                              }
                              s_process->dt = 0.0f;
                              l_render_success++;
                          }
                          dvf_clear(true);
                          dss_clear();
                          l_render_count++;
                      }
                  }
                  i_process = static_cast<process_t*>(s_process);
                  s_process = i_process->next;
              }
              dps_clear();
              l_rs = l_render_success == l_render_count;
          }
          dsp_restore(l_dc);
          m_busy = false;
          s_process = nullptr;
          return l_rs;
      }
      return false;
}

bool  apu::sync(float dt) noexcept
{
      if(dt > 0.0f) {

          // update fingerprint
          dsp_reset_fingerprint();

          // run through all the processes, update the step time accumulator and dispatch the event within the tree
          m_busy = true;
          if(m_process_head != nullptr) {
              process_t* i_process = m_process_head;
              while(i_process != nullptr) {
                  if(i_process->state != dc::pc_state_suspend) {
                      dsp_sync(i_process->owner, dt);
                      i_process->dt += dt;
                  }
                  i_process = i_process->next;
              }
          }
          m_busy = false;
      }
      return true;
}

unsigned int apu::get_sample_format() const noexcept
{
      return m_sample_format;
}

bool  apu::set_sample_format(unsigned int value) noexcept
{
      if((value == fmt_pcm_1) ||
          (value == fmt_pcm_2) ||
          (value == fmt_pcm_4) ||
          (value == fmt_pcm_8) ||
          (value == fmt_apm)) {
          m_sample_format = value;
          return true;
      }
      return false;
}

int   apu::get_sample_rate() const noexcept
{
      return m_sample_rate;
}

bool  apu::set_sample_rate(int value) noexcept
{
      if(value >= min_sample_rate) {
          if(value <= max_sample_rate) {
              return true;
          }
      }
      return false;
}

bool  apu::is_attached(core* core, bool expected_result) const noexcept
{
      bool l_attached;
      bool l_result;
      if(core != nullptr) {
          l_attached = dsg_find(core);
      } else
          l_attached = false;
      l_result = l_attached == expected_result;
      return l_result;
}

#ifdef DEBUG
#ifdef LINUX

void  apu::dump_mount_tree(FILE* file) noexcept
{
      int   l_index = 0;
      core* i_core  = m_core_head;
      printf("\n");
      if(i_core != nullptr) {
          do {
              i_core->dump_tree(file);
              l_index++;
              i_core = i_core->m_core_next;
          }
          while(i_core != m_core_head);
      }
      printf("\n");
}

#endif
#endif

/*namespace dsp*/ }
