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
#include "core.h"
#include "apu.h"

namespace dsp {

/* gate
*/
      gate::gate(core* owner) noexcept:
      m_owner(nullptr),
      m_source(nullptr),
      m_gate_next(nullptr),
      m_value_ptr(nullptr),
      m_enable_bit(false)
{
      if(owner != nullptr) {
          owner->bind(this);
      }
}

      gate::~gate()
{
      if(m_owner != nullptr) {
          m_owner->drop(this);
      }
}

void  gate::bind(fptype* address) noexcept
{
      m_value_ptr = address;
}

void  gate::unbind() noexcept
{
      m_value_ptr = nullptr;
}

bool  gate::attach(core* source_ptr) noexcept
{
      if(m_owner != nullptr) {
          if(m_source != nullptr) {
              if(m_source == source_ptr) {
                  return true;
              }
              if(m_owner->part(m_source, this) == false) {
                  return false;
              }
              m_source = nullptr;
          }
          if(m_owner->join(source_ptr, this)) {
              m_source = source_ptr;
              return  true;
          }
      }
      return false;
}

bool  gate::attach(core& source) noexcept
{
      return attach(std::addressof(source));
}

bool  gate::detach() noexcept
{
      if(m_owner != nullptr) {
          if(m_source != nullptr) {
              if(m_owner->part(m_source, this)) {
                  return  true;
              }
          }
      }
      return false;
}

fptype* gate::get_return_vector() const noexcept
{
      return m_value_ptr;
}

fptype* gate::get_return_vector(int offset) const noexcept
{
      return m_value_ptr + offset;
}

bool  gate::is_attached_to(core* core_ptr) const noexcept
{
      return m_source == core_ptr;
}

bool  gate::is_attached(bool expected) const noexcept
{
      return (m_source != nullptr) == expected;
}

bool  gate::is_detached() const noexcept
{
      return m_source == nullptr;
}

/* core
*/
      core::core(unsigned int option) noexcept:
      m_target(nullptr),
      m_core_next(nullptr),
      m_gate_head(nullptr),
      m_gate_tail(nullptr),
      m_d_base(nullptr),
      m_i_base(nullptr),
      m_d_size(0),
      m_i_size(0),
      m_dov(-1),
      m_dpc(0),
      m_dcc(0),
      m_argv(nullptr),
      m_argc(0),
      m_variable_count(0),
      m_register_count(0),
      m_instruction_count(0),
      m_option(option),
      m_hash(0)
{
}

      core::~core()
{
      dispose();
}

bool  core::bind(gate* gate_ptr) noexcept
{
      if(gate_ptr != nullptr) {
          if(gate_ptr->m_owner == nullptr) {
              if(m_gate_tail != nullptr) {
                  m_gate_tail->m_gate_next = gate_ptr;
              } else
                  m_gate_head = gate_ptr;
              m_gate_tail = gate_ptr;
              gate_ptr->m_owner = this;
              gate_ptr->m_gate_next = nullptr;
              gate_ptr->m_enable_bit = true;
              return true;
          } else
          if(gate_ptr->m_owner == this) {
              return true;
          }
      }
      return false;
}

bool  core::drop(gate* gate_ptr) noexcept
{
      if(gate_ptr != nullptr) {
          if(gate_ptr->m_owner != nullptr) {
              gate* i_prev = nullptr;
              gate* i_gate = m_gate_head;
              while(i_gate != nullptr) {
                  if(i_gate == gate_ptr) {
                      if(m_gate_head == gate_ptr) {
                          m_gate_head = gate_ptr->m_gate_next;
                      }
                      if(m_gate_tail == gate_ptr) {
                          m_gate_tail = i_prev;
                      }
                      if(i_prev != nullptr) {
                          i_prev->m_gate_next = gate_ptr->m_gate_next;
                      }
                      gate_ptr->m_owner = nullptr;
                      gate_ptr->m_gate_next = nullptr;
                      gate_ptr->m_enable_bit = false;
                      return true;
                  }
                  i_prev = i_gate;
                  i_gate = i_gate->m_gate_next;
              }
          }
      }
      return false;
}

bool  core::join(core* core_ptr, gate* gate_ptr) noexcept
{
      bool l_can_join = can_join(core_ptr);
      if(l_can_join) {
          bool l_allow_join = (m_option & o_enable_join_event) ? join_event(gate_ptr, core_ptr) : true;
          if(l_allow_join) {
              if(m_target != nullptr) {
                  m_target->dsp_join_event(core_ptr);
              }
              return true;
          }
      }
      return false;
}

bool  core::can_join(core* core_ptr) noexcept
{
      // verify that adding the new node doesn't trigger an immediate recursion
      if(core_ptr == this) {
          return false;
      }
      return true;
}

bool  core::part(core* core_ptr, gate* gate_ptr) noexcept
{
      bool l_allow_part = (m_option & o_enable_part_event) ? part_event(gate_ptr, core_ptr) : true;
      if(l_allow_part) {
          if(m_target != nullptr) {
              m_target->dsp_part_event(core_ptr);
          }
      }
      return false;
}

void  core::move(core& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          dispose();
          m_d_base = rhs.m_d_base;
          m_i_base = rhs.m_i_base;
          m_d_size = rhs.m_d_size;
          m_i_size = rhs.m_i_size;
          m_argv = rhs.m_argv;
          m_argc = rhs.m_argc;
          m_arg_size = rhs.m_arg_size;
          m_variable_count = rhs.m_variable_count;
          m_register_count = rhs.m_register_count;
          m_instruction_count = rhs.m_instruction_count;
          rhs.release();
      }
}

void  core::release() noexcept
{
      m_d_base = nullptr;
      m_i_base = nullptr;
      m_argc   = 0;
      m_argv   = nullptr;
}

void  core::dispose() noexcept
{
      if(m_argv != nullptr) {
          while(m_argc >= 0) {
              --m_argc;
              m_argv[m_argc].~argument();
          }
          free(m_argv);
          m_argv = nullptr;
      }
      if(m_i_base) {
          free(m_i_base);
          m_i_base = nullptr;
      }
      if(m_d_base) {
          free(m_d_base);
          m_d_base = nullptr;
      }
}

bool  core::join_event(gate*, core*) noexcept
{
      return true;
}

bool  core::part_event(gate*, core*) noexcept
{
      return true;
}

bool  core::render(unsigned int) noexcept
{
      return true;
}

void  core::sync(float) noexcept
{
}

apu*  core::get_target() const noexcept
{
      return m_target;
}

bool  core::attach(core* core_ptr) noexcept
{
      // by default - bind core_ptr to the next available gate
      // core derivates can alter this behaviour to better suit their function by overriding this function
      if(core_ptr != nullptr) {
          gate* i_gate = m_gate_head;
          while(i_gate != nullptr) {
              if(i_gate->is_attached(false)) {
                  if(i_gate->attach(core_ptr)) {
                      return true;
                  }
              }
              i_gate = i_gate->m_gate_next;
          }
      }
      return false;
}

bool  core::detach(core* core_ptr, bool drop_all) noexcept
{
      int l_core_found = 0;
      int l_core_dropped = 0;
      if(core_ptr != nullptr) {
          gate* i_gate = m_gate_head;
          while(i_gate != nullptr) {
              if(i_gate->is_attached_to(core_ptr)) {
                  l_core_found++;
                  if(i_gate->detach()) {
                      l_core_dropped++;
                  }
                  if(drop_all == false) {
                      break;
                  }
              }
              i_gate = i_gate->m_gate_next;
          }
      }
      return (l_core_dropped > 0) && (l_core_dropped == l_core_found);
}

unsigned int core::get_sample_format() const noexcept
{
      return m_target->get_sample_format();
}

bool  core::set_sample_format(unsigned int value) noexcept
{
      return value == m_target->get_sample_format();
}

int   core::get_sample_rate() const noexcept
{
      return m_target->get_sample_rate();
}

bool  core::set_sample_rate(int value) noexcept
{
      return value == m_target->get_sample_rate();
}

#ifdef DEBUG
#ifdef LINUX

void  core::dump_tree(FILE* file, core* tree, int index, int depth) noexcept
{
      for(int l_space = 0; l_space != depth; l_space++) {
          fprintf(file, "    ");
      }
      fprintf(file, "[%d] ", index);

      if(tree != nullptr) {
          int    l_index = 0;
          gate*  i_gate  = tree->m_gate_head;
          char   l_target_char = '-';
          if(tree->m_target != nullptr) {
              l_target_char = '+';
          }
          fprintf(file, "%p %d %c\n", tree, tree->m_dcc, l_target_char);
          while(i_gate != nullptr) {
              core* l_tree = i_gate->m_source;
              if(l_tree != nullptr) {
                  dump_tree(file, l_tree, l_index, depth + 1);
              }
              l_index++;
              i_gate = i_gate->m_gate_next;
          }
      } else
          fprintf(file, "\n");
}

void  core::dump_tree(FILE* file) noexcept
{
      printf("\n");
      dump_tree(file, this, 0, 0);
      printf("\n");
}

void  core::dump_code(FILE* file, int column_count, int column_width) noexcept
{
      int  l_row_count;
      int  l_column_count;
      // int  l_column_width = column_width;
    
      std::fprintf(file, "\n");
      std::fprintf(file, "Stacks:\t%d\n", m_argc);
      std::fprintf(file, "Registers:\t%d\n", m_register_count);
      std::fprintf(file, "Variables:\t%d\n", m_variable_count);
      std::fprintf(file, "\n");

      if(m_argc) {
          l_column_count = column_count;
          l_row_count    = m_argc / l_column_count;
          if(m_argc % l_column_count) {
              l_row_count = l_row_count + 1;
          }

          for(int l_row = 0; l_row < l_row_count; l_row++) {
              int l_stack_base = l_row * l_column_count;
              int l_line = 0;
              // int l_column = 0;
              // int l_line = 0;
              int l_show;
              do {
                  l_show = 0;
                  std::fprintf(file, "\e[37;1m%.4d\e[0m: ", l_line);
                  for(int l_column = 0; l_column < l_column_count; l_column++) {
                      int l_stack = l_stack_base + l_column;
                      if(l_stack < m_argc) {
                          auto& l_argument = m_argv[l_stack];
                          micro* l_code_head;
                          micro* l_code_tail;
                          int    l_code_size = l_argument.load(l_code_head, l_code_tail);

                          if(l_line < l_code_size) {
                              micro* l_code_line = l_code_head + l_line;
                              char   l_i_flags[4];
                              char   l_i_op[8];
                              char   l_i_dst[32];
                              char   l_i_src[32];

                              l_i_flags[0] = '-';
                              l_i_flags[1] = '-';
                              l_i_flags[2] = '-';
                              if(l_code_line->bit_const) {
                                  l_i_flags[0] = 'c';
                              }
                              if(l_code_line->bit_halt) {
                                  l_i_flags[1] = 'h';
                              }
                              if(l_code_line->bit_return) {
                                  l_i_flags[2] = 'r';
                              }
                              l_i_flags[3] = 0;

                              switch(l_code_line->op_code) {
                                  case micro::op_code_ret:
                                      std::strncpy(l_i_op, "ret", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_imm:
                                      std::strncpy(l_i_op, "imm", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_mov:
                                      std::strncpy(l_i_op, "mov", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_pos:
                                      std::strncpy(l_i_op, "pos", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_neg:
                                      std::strncpy(l_i_op, "neg", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_add:
                                      std::strncpy(l_i_op, "add", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_sub:
                                      std::strncpy(l_i_op, "sub", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_mul:
                                      std::strncpy(l_i_op, "mul", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_div:
                                      std::strncpy(l_i_op, "div", sizeof(l_i_op));
                                      break;
                                  case micro::op_code_nop:
                                      std::strncpy(l_i_op, "nop", sizeof(l_i_op));
                                      break;
                                  default:
                                      std::strncpy(l_i_op, "???", sizeof(l_i_op));
                                      break;
                              };

                              switch(l_code_line->op_dst) {
                                  case micro::op_dst_no:
                                      std::strncpy(l_i_dst, "", sizeof(l_i_dst));
                                      break;
                                  case micro::op_dst_r:
                                      std::snprintf(l_i_dst, sizeof(l_i_dst), "r%d", l_code_line->dst.r);
                                      break;
                                  default:
                                      std::snprintf(l_i_dst, sizeof(l_i_dst), "???");
                                      break;
                              };


                              switch(l_code_line->op_src) {
                                  case micro::op_src_no:
                                      std::strncpy(l_i_src, "", sizeof(l_i_src));
                                      break;
                                  case micro::op_src_r:
                                      std::snprintf(l_i_src, sizeof(l_i_src), "r%d", l_code_line->src.r);
                                      break;
                                  case micro::op_src_p:
                                      std::snprintf(l_i_src, sizeof(l_i_src), "%p", l_code_line->src.p);
                                      break;
                                  default:
                                      std::snprintf(l_i_dst, sizeof(l_i_dst), "???");
                                      break;
                              };

                              std::fprintf(file, "\e[30;1m%-3s\e[0m \e[32;1m%-6s\e[0m %-3s %-12s\t", l_i_flags, l_i_op, l_i_dst, l_i_src);
                              l_show++;
                          }
                      }
                  }
                  std::fprintf(file, "\n");
                  l_line++;
              }
              while(l_show > 0);
              std::fprintf(file, "\n");
          }
      }
}

#endif
#endif

/*namespace dsp*/ }
