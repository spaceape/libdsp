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
#include "factory.h"

namespace dsp {

      factory::factory() noexcept:
      core(o_none),
      m_result(true)
{
}

      factory::~factory()
{
}

/* d_get_raw()
 * reserve a data register
*/
fptype* factory::d_get_raw(symbol* symbol) noexcept
{
      // if a symbol is specified, search the alias pool in order to verify if the symbol
      // is already bound to a data register
      if(symbol != nullptr) {
          for(int i_alias = 0; i_alias < m_alias_count; i_alias++) {
              if(m_alias_pool[i_alias].reference == symbol) {
                  return m_alias_pool[i_alias].address;
              }
          }
      }

      // get the next unused register within the data pool and save it to the alias pool if
      // a symbol is
      if(fptype* l_data_ptr = m_d_last; l_data_ptr < m_d_base + m_d_size) {
          if(symbol != nullptr) {
              m_alias_pool[m_alias_count].reference = symbol;
              m_alias_pool[m_alias_count].address   = l_data_ptr;
              m_alias_count++;
          }
          m_d_last += fpu::pts;
          return l_data_ptr;
      }
      return nullptr;
}

/* d_get_constant()
 * reserve a data register and set its initial value
*/
fptype* factory::d_get_constant(const constant& symbol) noexcept
{
      fptype* l_data_ptr = d_get_raw(nullptr);
      if(l_data_ptr != nullptr) {
          fpu::reg_mov(l_data_ptr, symbol.get_value());
      }
      return l_data_ptr;
}

/* r_get_variable()
 * reserve a data register and bind it to the given uniform
*/
fptype* factory::d_get_variable(uniform& symbol) noexcept
{
      fptype* l_data_ptr = d_get_raw(std::addressof(symbol));
      if(l_data_ptr != nullptr) {
          symbol.bind(l_data_ptr);
      }
      return l_data_ptr;
}

/* r_get_ptr()
   lookup given register in the live pool
*/
factory::reg* factory::r_get_ptr(int rx) noexcept
{
      return m_live_pool + (rx / fpu::pts);
}

/* r_get_scratch()
*/
int   factory::r_get_scratch() noexcept
{
      int  i_register = m_r_last;
      reg* p_register;
      // iterate through the m_r_lb..m_r_ub range and check if i_register is in use
      while(i_register < m_r_ub) {
          if(p_register = r_get_ptr(i_register); p_register->i_load == nullptr) {
              break;
          }
          m_r_last    = i_register;
          i_register += fpu::pts;
      }
      // found free register, return;
      if(i_register < m_r_ub) {
          p_register->i_load  = m_i_last;
          p_register->b_owner = m_b_tail;
          return i_register;
      }
      return -1;
}

/* r_drop_scratch()
*/
void  factory::r_drop_scratch(int rx) noexcept
{
      reg* p_register = r_get_ptr(rx);
      // unset the register in the table
      if(p_register->i_load) {
          p_register->i_load  = nullptr;
          p_register->b_owner = nullptr;
      }
      // signal that there is a free register waiting to be allocated
      if(rx < m_r_last) {
          m_r_last = rx;
      }
}

micro& factory::i_emit_generic(unsigned int op) noexcept
{
      micro* l_micro = m_i_last++;
      if(l_micro < m_i_base + m_i_size) {
          l_micro->op_code = op;
          l_micro->bit_flags = 0u;
          l_micro->bit_const = 0u;
          l_micro->bit_volatile = 0u;
          l_micro->bit_halt = 0u;
          l_micro->bit_return = 0u;
          return *l_micro;
      }
      return i_emit_error();
}

micro& factory::i_emit_generic(unsigned int op, int lhs, int rhs) noexcept
{
      micro& l_micro = i_emit_generic(op);
      l_micro.op_dst = micro::op_dst_r;
      l_micro.dst.r = lhs;
      l_micro.op_src = micro::op_src_r;
      l_micro.src.r = rhs;
      return l_micro;
}

micro& factory::i_emit_generic(unsigned int op, int lhs, fptype* rhs) noexcept
{
      micro& l_micro = i_emit_generic(op);
      l_micro.op_dst = micro::op_dst_r;
      l_micro.dst.r = lhs;
      if(rhs != nullptr) {
          l_micro.op_src = micro::op_src_p;
          l_micro.src.p = rhs;
      }
      else {
          l_micro.op_src = micro::op_src_no;
          l_micro.src.p = nullptr;
      }
      return l_micro;
}

micro& factory::i_emit_constant_load(const constant& symbol) noexcept
{
      micro& l_micro = i_emit_generic(micro::op_code_mov, r_get_scratch(), d_get_constant(symbol));
      l_micro.bit_const = 1u;
      return l_micro;
}

micro& factory::i_emit_variable_load(uniform& symbol) noexcept
{
      micro& l_micro = i_emit_generic(micro::op_code_mov, r_get_scratch(), d_get_variable(symbol));
      l_micro.bit_volatile = 1u;
      return l_micro;
}

micro& factory::i_emit_operation(unsigned int op, micro& lhs) noexcept
{
      if(op == micro::op_code_pos) {
          return lhs;
      } else
      if(op == micro::op_code_neg) {
          if(lhs.op_dst == micro::op_dst_r) {
              return i_emit_generic(micro::op_code_neg, lhs.dst.r, nullptr);
          } else
              return i_emit_error();
      } else
          return i_emit_error();
}

micro& factory::i_emit_operation(unsigned int op, micro& lhs, micro& rhs) noexcept
{
      if(lhs.op_dst == micro::op_dst_r) {
          if(rhs.op_dst == micro::op_dst_r) {
              micro& l_micro = i_emit_generic(op, lhs.dst.r, rhs.dst.r);
              // set the operation const and volatile bits
              if((lhs.bit_volatile == 1u) ||
                  (rhs.bit_volatile == 1u)) {
                  l_micro.bit_volatile = 1u;
              } else
              if((lhs.bit_const == 1u) &&
                  (rhs.bit_const == 1u)) {
                  l_micro.bit_const = 1u;
              }
              // drop rhs register
              r_drop_scratch(rhs.dst.r);
              return l_micro;
          }
      }
      return i_emit_error();
}

micro& factory::i_emit_error() noexcept
{
      micro* l_micro = m_i_error;
      l_micro->op_code = micro::op_code_nop;
      l_micro->op_dst  = micro::op_dst_no;
      l_micro->op_src  = micro::op_src_no;
      l_micro->dst.r = -1;
      l_micro->src.p = nullptr;
      l_micro->bit_flags = 0u;
      l_micro->bit_const = 1u;
      l_micro->bit_volatile = 0u;
      l_micro->bit_halt = 1u;
      l_micro->bit_return = 0u;
      return *l_micro;
}

void  factory::i_drop() noexcept
{
      m_i_last--;
}

bool   factory::push(sub& b) noexcept
{
        b.b_parent = m_b_tail;
        b.r_lb     = m_r_lb;
        b.r_ub     = m_r_ub;
        m_b_tail = std::addressof(b);
        if(m_b_head == nullptr) {
            m_b_head = m_b_tail;
        }
        return true;
}

void   factory::pop() noexcept
{
        // drop live registers within the range of the current branch
        for(int i_register = m_b_tail->r_lb; i_register < m_b_tail->r_ub; i_register++) {
            r_drop_scratch(i_register);
        }
        // restore the state saved during the matching push() call
        m_r_ub = m_b_tail->r_ub;
        m_r_lb = m_b_tail->r_lb;
        m_b_tail = m_b_tail->b_parent;
        if(m_b_tail == nullptr) {
            m_b_head = nullptr;
        }
}

void  factory::build() noexcept
{
}

bool  factory::get_return_status() const noexcept
{
      return m_result;
}

/*namespace dsp*/ }
