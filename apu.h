#ifndef dsp_apu_h
#define dsp_apu_h
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
#include "dsp.h"
#include "dc.h"
#include "config.h"

namespace dsp {

class apu: public dc
{
  ppu*    m_ppu;
  mmu*    m_mmu;

  struct vector_t: public vector_base_t
  {
    short int r_size;         // requested size
    bool      r_keep:1;       // requested persistance attribute
    bool      s_far_bit:1;    // far attribute: memory referenced by the vector is not allocated internally
    bool      s_keep_bit:1;
    bool      s_used_bit:1;
  };

  struct sample_page_t
  {
    struct info_t {
      fptype*         data_used_base;
      fptype*         data_used_last;
      fptype*         data_used_ptr;
      sample_page_t*  page_next;
    };

    static constexpr int info_size   =  sizeof(info_t);
    static constexpr int head_size   =  memory_vector_block * sizeof(fptype);
    static constexpr int map_size    =  head_size - info_size;
    static constexpr int map_bits    =  map_size * 8;
    static constexpr int page_blocks =  memory_vector_page / memory_vector_block;

    // how many blocks a single bit in the map can point to
    static constexpr int map_bit_blocks = (page_blocks / map_bits) ? (page_blocks / map_bits) : 1;
    static constexpr int map_bit_samples = map_bit_blocks * memory_vector_block;

    static constexpr int page_effective_samples = (page_blocks - 1) * memory_vector_block;

    struct map_t {
      char            padding[info_size];
      unsigned char   bytes[map_size];
    };

    union {
      info_t  info;
      map_t   map;
      char    bytes[head_size];
    } head;
    fptype    data[0];
  };

  struct process_t: public process_base_t
  {
    process_t*  prev;
    process_t*  next;
  };

  struct branch_t: public branch_base_t
  {
  };

  unsigned int  m_sample_format;
  int           m_sample_rate;
  int           m_control_rate;

  core*         m_core_head;
  core*         m_core_tail;

  sample_page_t*  m_dps_page_head;
  sample_page_t*  m_dps_page_tail;
  int             m_dps_page_count;

  sample_page_t*  m_dss_page_head;
  sample_page_t*  m_dss_page_tail;
  int             m_dss_page_count;

  vector_t*     m_dvf_base;
  vector_t*     m_dvf_last;
  int           m_dvf_size;

  process_t*    m_process_head;
  process_t*    m_process_tail;

  unsigned int  m_iteration_fingerprint;
  bool          m_busy;

  protected:
          bool        dsg_find(core*) const noexcept;
          void        dsg_bind(core*) noexcept;
          void        dsg_acquire(core*) noexcept;
          void        dsg_dispose(core*) noexcept;
          void        dsg_release(core*) noexcept;
          void        dsg_drop(core*) noexcept;

          int         dsg_lookup_intern(core*, core*) noexcept;
          int         dsg_lookup_global(core*) noexcept;
          bool        dsg_converge_intern(core*, core*) noexcept;
          bool        dsg_converge_global(core*) noexcept;
          void        dsg_diverge_intern(core*) noexcept;
          bool        dsg_converge(core*) noexcept;
          bool        dsg_diverge(core*) noexcept;
          void        dsg_clear() noexcept;

          bool        dps_make_page() noexcept;
          bool        dps_map_get_bit(sample_page_t*, int) noexcept;
          void        dps_map_set_bit(sample_page_t*, int, bool) noexcept;
          int         dps_map_find_free(sample_page_t*, int) noexcept;
          void        dps_map_mark_used(sample_page_t*, int, int) noexcept;
          void        dps_map_mark_free(sample_page_t*, int, int) noexcept;
          fptype*     dps_map_get_block_address(sample_page_t*, int) noexcept;
          int         dps_map_get_block_size(int) noexcept;
          void        dps_map_clear(sample_page_t*) noexcept;
          bool        dps_acquire(fptype*&, int&, int) noexcept;
          bool        dps_release(fptype*&, int&) noexcept;
          void        dps_free_page(sample_page_t*) noexcept;
          void        dps_clear() noexcept;
          void        dps_dispose(bool = true) noexcept;

          bool        dss_make_page() noexcept;
          bool        dss_acquire(fptype*&, int&, int) noexcept;
          bool        dss_release(fptype*&, int&) noexcept;
          void        dss_free_page(sample_page_t*) noexcept;
          void        dss_clear() noexcept;
          void        dss_dispose(bool = true) noexcept;

          bool        dvf_reserve(int) noexcept;
          int         dvf_lookup(fptype*) noexcept;
          vector_t*   dvf_get_ptr(int) const noexcept;
          fptype*     dvf_get_data_lazy(int) const noexcept;
          fptype*     dvf_get_data_immediate(int) noexcept;
          int         dvf_acquire(int = 0, unsigned int = 0) noexcept;
          void        dvf_release(int, bool) noexcept;
          void        dvf_clear(bool = true) noexcept;
          void        dvf_dispose(bool = true) noexcept;

  protected:
          process_t*  dsp_find_process(core*) noexcept;
          process_t*  dsp_make_process(core*) noexcept;
          process_t*  dsp_free_process(process_t*) noexcept;
          void        dsp_dispose_process_list() noexcept;

          void        dsp_push(process_base_t*, branch_base_t&, int) noexcept;
          int         dsp_fork(process_base_t*, core*, unsigned int, unsigned int) noexcept;
          int         dsp_descend(process_base_t*, core*, unsigned int) noexcept;
          void        dsp_pop(process_base_t*, branch_base_t&) noexcept;
          void        dsp_sync(core*, float) noexcept;
          void        dsp_reset_fingerprint() noexcept;

  private:
          void        dsp_join_event(core*) noexcept;
          void        dsp_part_event(core*) noexcept;

  friend class  core;
  friend class  dc;
  public:
          apu() noexcept;
          apu(ppu*, mmu*) noexcept;
          apu(const apu&) noexcept = delete;
          apu(apu&&) noexcept = delete;
          ~apu();

          ppu*  get_ppu() const noexcept;
          mmu*  get_mmu() const noexcept;

          bool  attach_variable(const char*, uniform&) noexcept;
          bool  detach_variable(const char*) noexcept;
          bool  attach_constant(const char*, uniform&) noexcept;
          bool  detach_constant(const char*) noexcept;
          bool  attach_function(const char*, uniform&) noexcept;
          bool  detach_function(const char*) noexcept;

          bool  attach(core*) noexcept;
          bool  detach(core*) noexcept;

          bool  render() noexcept;
          bool  render(float) noexcept;
          bool  sync(float) noexcept;
          unsigned int get_sample_format() const noexcept;
          bool  set_sample_format(unsigned int) noexcept;
          int   get_sample_rate() const noexcept;
          bool  set_sample_rate(int) noexcept;

          bool  is_attached(core*, bool = true) const noexcept;

 #ifdef DEBUG
 #ifdef LINUX
          void  dump_mount_tree(FILE*) noexcept;
 #endif
 #endif


          apu&  swap(apu&) noexcept = delete;
          apu&  operator=(const apu&) noexcept = delete;
          apu&  operator=(apu&&) noexcept = delete;
};

/*namespace dsp*/ }
#endif
