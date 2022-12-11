#ifndef dsp_factory_h
#define dsp_factory_h
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
#include "core.h"
#include <util.h>
#include <limits>

namespace dsp {

/* factory
   construct dsp core according to given parameters
   contains the internal state necessary to build the dsp microcode
*/
class factory: public core
{
  fptype*   m_d_last;
  micro*    m_i_last;
  micro*    m_i_error;
  micro*    m_return;
  micro*    m_fault;
  int       m_r_lb;
  int       m_r_ub;
  int       m_r_last;

  private:
  struct reg;
  struct sub;

  struct alias
  {
    symbol* reference;
    fptype* address;
  };
 
  struct reg
  {
    sub*    b_owner;    // owning branch
    micro*  i_load;     // instruction where the register is loaded
  };

  struct sub
  {
    sub*    b_parent;   // parent branch
    int     r_lb;
    int     r_ub;
  };

  private:
  sub*      m_b_head;
  sub*      m_b_tail;

  alias*    m_alias_pool;
  int       m_alias_count;
  reg*      m_live_pool;
  int       m_live_count;

  bool      m_result;

  private:
          fptype*  d_get_raw(symbol*) noexcept;
          fptype*  d_get_constant(const constant&) noexcept;
          fptype*  d_get_variable(uniform&) noexcept;

          int      r_get_scratch() noexcept;
          reg*     r_get_ptr(int) noexcept;
          void     r_drop_scratch(int) noexcept;

          micro&   i_emit_generic(unsigned int) noexcept;
          micro&   i_emit_generic(unsigned int, int, int) noexcept;
          micro&   i_emit_generic(unsigned int, int, fptype*) noexcept;
          micro&   i_emit_constant_load(const constant&) noexcept;
          micro&   i_emit_variable_load(uniform&) noexcept;
          micro&   i_emit_operation(unsigned int, micro&) noexcept;
          micro&   i_emit_operation(unsigned int, micro&, micro&) noexcept;
          micro&   i_emit_error() noexcept;
          void     i_drop() noexcept;

  protected:
          bool   push(sub&) noexcept;
          void   pop() noexcept;
          void   build() noexcept;

  /* build <constant>
  */
  inline  micro& build(const constant& expr) noexcept {
          return i_emit_constant_load(expr);
  }

  /* build <uniform>
  */
  inline  micro& build(const reference<uniform>& expr) noexcept {
          return i_emit_variable_load(expr.lhs);
  }

  /* build +<lhs>
  */
  template<typename Rt>
  inline  micro& build(const op_pos<Rt>& expr) noexcept {
          return build(expr.lhs);
  }

  /* build -<const>
  */
  inline  micro& build(const op_neg<constant>& expr) noexcept {
        if(expr.lhs.get_value() != 0.0) {
            if(expr.lhs.get_value() != NAN) {
                return i_emit_constant_load(0.0f - expr.lhs.get_value());
            } else
                return i_emit_constant_load(NAN);
        } else
            return i_emit_constant_load(0.0f);
  }

  /* optimize -(-<lhs>) -> <lhs>
  */
  template<typename Lt>
  inline  micro& build(const op_neg<op_neg<Lt>>& expr) noexcept {
          return build(expr.lhs.lhs);
  }

  /* build -<lhs>
  */
  template<typename Lt>
  inline  micro& build(const op_neg<Lt>& expr) noexcept {
          micro& li = build(expr.lhs);
          if(is_return_micro(li)) {
              return i_emit_operation(micro::op_code_neg, li);
          }
          return i_emit_error();
  }

  /* compose
     compose binary operation
  */
  template<typename Lt, typename Rt>
  inline  micro& compose(unsigned int op_code, const Lt& lhs, const Rt& rhs) noexcept {
          sub bb;
          micro& li = build(lhs);
          if(is_return_micro(li)) {
              if(push(bb)) {
                  micro& ri = build(rhs);
                  if(is_return_micro(ri)) {
                      micro& re = i_emit_operation(op_code, li, ri);
                      pop();
                      return re;
                  }
              }
          }
          return i_emit_error();
  }

  /* build <lhs> + <rhs>
  */
  template<typename Lt, typename Rt>
  inline  micro& build(const op_add<Lt, Rt>& expr) noexcept {
          return compose(expr, expr.lhs, expr.rhs);
  }

  /* build <lhs> - <rhs>
  */
  template<typename Lt, typename Rt>
  inline  micro& build(const op_sub<Lt, Rt>& expr) noexcept {
          return compose(expr, expr.lhs, expr.rhs);
  }
  
  /* build <lhs> * <rhs>
  */
  template<typename Lt, typename Rt>
  inline  micro& build(const op_mul<Lt, Rt>& expr) noexcept {
          return compose(expr, expr.lhs, expr.rhs);
  }

  /* build <lhs> / <rhs>
  */
  template<typename Lt, typename Rt>
  inline  micro& build(const op_div<Lt, Rt>& expr) noexcept {
          return compose(expr, expr.lhs, expr.rhs);
  }

  inline  bool  make_argument(int) noexcept {
          return true;
  }

  template<typename Expr, typename... Next>
  inline  bool  make_argument(int index, Expr&& expr, Next&&... next) noexcept {
          sub   l_ab;
          bool  l_result = false;
          if(push(l_ab)) {
              auto  l_code_head = m_i_last;
              auto& l_return    = build(expr);
              auto  l_code_tail = m_i_last;
              auto  l_argument  = new(m_argv + index) argument(l_code_head, l_code_tail);
              if(l_argument) {
                  if(l_return.op_dst == micro::op_dst_r) {
                      if(l_return.op_code != micro::op_code_nop) {
                          l_return.bit_halt = 1u;
                          l_return.bit_return = 1u;
                          l_result = true;
                      } else
                          i_emit_error();
                  } else
                      i_emit_error();
              }
              m_i_error = m_i_last;
              pop();
          }
          return make_argument(index + 1, std::forward<Next>(next)...) && l_result;
  }

  public:
          factory() noexcept;

  template<typename... Args>
  inline  factory(Args&&... arguments) noexcept:
          factory() {
          if(m_argc = sizeof...(Args);
              (m_argc > 0) &&
              (m_argc < std::numeric_limits<short int>::max())) {

              if(int
                  l_variable_count = get_variable_count_ub(std::forward<Args>(arguments)...);
                  l_variable_count <= std::numeric_limits<short int>::max()) {
                  m_variable_count = l_variable_count;
              } else
                  return;

              if(int
                  l_register_count = get_register_count_ub(std::forward<Args>(arguments)...);
                  l_register_count <= std::numeric_limits<short int>::max()) {
                  m_register_count = l_register_count;
              } else
                  return;

              if(int 
                  l_instruction_count = get_instruction_count_ub(std::forward<Args>(arguments)...);
                  l_instruction_count <= std::numeric_limits<short int>::max()) {
                  m_instruction_count = l_instruction_count;
              } else
                  return;

              m_arg_size = get_list_size_ub(std::forward<Args>(arguments)...) * sizeof(argument);
              if(m_argv = reinterpret_cast<argument*>(malloc(m_arg_size));
                  m_argv != nullptr) {
                  m_d_size = m_variable_count * fpu::pts * sizeof(fptype);
                  if(m_d_base = reinterpret_cast<fptype*>(malloc(m_d_size));
                      m_d_base != nullptr) {
                      m_i_size = m_instruction_count * sizeof(micro);
                      if(m_i_base = reinterpret_cast<micro*>(malloc(m_i_size));
                          m_i_base != nullptr) {
                          alias l_alias_pool[m_variable_count];
                          reg   l_live_pool[m_register_count];

                          m_d_last = m_d_base;
                          m_i_last = m_i_base;
                          m_i_error = m_i_base;
                          m_fault  = nullptr;
                          m_return = nullptr;
                          m_r_lb   = 0;
                          m_r_ub   = m_register_count;
                          m_r_last = m_r_lb;

                          m_b_head = nullptr;
                          m_b_tail = nullptr;

                          m_alias_pool = std::addressof(l_alias_pool[0]);
                          m_alias_count = 0;
                          m_live_pool = std::addressof(l_live_pool[0]);
                          m_live_count = 0;
                          std::memset(m_live_pool, 0, m_register_count * sizeof(reg));

                          m_result = make_argument(0, std::forward<Args>(arguments)...);
                      }
                  }
              }
          }
  }

          ~factory();
 
  bool get_return_status() const noexcept;
};

/* operator <uniform>
*/
template<typename Lt>
constexpr auto operator+(uniform& lhs) noexcept
{
      return reference<uniform>(lhs);
}

template<typename Lt>
constexpr auto operator+(const Lt& lhs) noexcept
{
      return lhs;
}

/* operator -<expr>
*/
template<typename Lt>
constexpr auto operator-(uniform& lhs) noexcept
{
      return op_neg<reference<uniform>>(lhs);
}

template<typename Lt>
constexpr auto operator-(const Lt& lhs) noexcept
{
      return op_neg<Lt>(lhs);
}

/* operator <expr> + <expr>
*/
template<typename Lt>
constexpr auto operator+(const Lt& lhs, float rhs) noexcept
{
      return op_add<Lt, constant>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator+(float lhs, const Rt& rhs) noexcept
{
      return op_add<constant, Rt>(lhs, rhs);
}

template<typename Lt>
constexpr auto operator+(const Lt& lhs, uniform& rhs) noexcept
{
      return op_add<Lt, reference<uniform>>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator+(uniform& lhs, const Rt& rhs) noexcept
{
      return op_add<reference<uniform>, Rt>(lhs, rhs);
}

inline auto operator+(uniform& lhs, float rhs) noexcept
{
      return op_add<reference<uniform>, constant>(lhs, rhs);
}

inline auto operator+(float lhs, uniform& rhs) noexcept
{
      return op_add<constant, reference<uniform>>(lhs, rhs);
}

template<typename Lt, typename Rt>
constexpr auto operator+(const Lt& lhs, const Rt& rhs) noexcept
{
      return op_add<Lt, Rt>(lhs, rhs);
}

/* operator <expr> - <expr>
*/
template<typename Lt>
constexpr auto operator-(const Lt& lhs, float rhs) noexcept
{
      return op_sub<Lt, constant>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator-(float lhs, const Rt& rhs) noexcept
{
      return op_sub<constant, Rt>(lhs, rhs);
}

template<typename Lt>
constexpr auto operator-(const Lt& lhs, uniform& rhs) noexcept
{
      return op_sub<Lt, reference<uniform>>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator-(uniform& lhs, const Rt& rhs) noexcept
{
      return op_sub<reference<uniform>, Rt>(lhs, rhs);
}

inline auto operator-(uniform& lhs, float rhs) noexcept
{
      return op_sub<reference<uniform>, constant>(lhs, rhs);
}

inline auto operator-(float lhs, uniform& rhs) noexcept
{
      return op_sub<constant, reference<uniform>>(lhs, rhs);
}

template<typename Lt, typename Rt>
constexpr auto operator-(const Lt& lhs, const Rt& rhs) noexcept
{
      return op_sub<Lt, Rt>(lhs, rhs);
}

/* operator <expr> * <expr>
*/
template<typename Lt>
constexpr auto operator*(const Lt& lhs, float rhs) noexcept
{
      return op_mul<Lt, constant>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator*(float lhs, const Rt& rhs) noexcept
{
      return op_mul<constant, Rt>(lhs, rhs);
}

template<typename Lt>
constexpr auto operator*(const Lt& lhs, uniform& rhs) noexcept
{
      return op_mul<Lt, reference<uniform>>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator*(uniform& lhs, const Rt& rhs) noexcept
{
      return op_mul<reference<uniform>, Rt>(lhs, rhs);
}

inline auto operator*(uniform& lhs, float rhs) noexcept
{
      return op_mul<reference<uniform>, constant>(lhs, rhs);
}

inline auto operator*(float lhs, uniform& rhs) noexcept
{
      return op_mul<constant, reference<uniform>>(lhs, rhs);
}

template<typename Lt, typename Rt>
constexpr auto operator*(const Lt& lhs, const Rt& rhs) noexcept
{
      return op_mul<Lt, Rt>(lhs, rhs);
}

/* operator <expr> / <expr>
*/
template<typename Lt>
constexpr auto operator/(const Lt& lhs, float rhs) noexcept
{
      return op_div<Lt, constant>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator/(float lhs, const Rt& rhs) noexcept
{
      return op_div<constant, Rt>(lhs, rhs);
}

template<typename Lt>
constexpr auto operator/(const Lt& lhs, uniform& rhs) noexcept
{
      return op_div<Lt, reference<uniform>>(lhs, rhs);
}

template<typename Rt>
constexpr auto operator/(uniform& lhs, const Rt& rhs) noexcept
{
      return op_div<reference<uniform>, Rt>(lhs, rhs);
}

inline auto operator/(uniform& lhs, float rhs) noexcept
{
      return op_div<reference<uniform>, constant>(lhs, rhs);
}

inline auto operator/(float lhs, uniform& rhs) noexcept
{
      return op_div<constant, reference<uniform>>(lhs, rhs);
}

template<typename Lt, typename Rt>
constexpr auto operator/(const Lt& lhs, const Rt& rhs) noexcept
{
      return op_div<Lt, Rt>(lhs, rhs);
}

namespace util {

constexpr int get_variable_count() noexcept
{
          return 0;
}

template<typename Xt, typename... Next>
constexpr int get_variable_count(const Xt& expr, Next&&... next) noexcept {
          return Xt::used_variable_count + get_variable_count(std::forward<Next>(next)...);
}

constexpr int get_register_count() noexcept
{
          return 0;
}

template<typename Xt, typename... Next>
constexpr int get_register_count(const Xt& expr, Next&&... next) noexcept {
          int expr_register_count = Xt::used_register_count;
          int next_register_count = get_register_count(std::forward<Next>(next)...);
          // return the max() of the expr vs next register counts
          if(expr_register_count >= next_register_count) {
              return expr_register_count;
          } else
              return next_register_count;
}

constexpr int get_instruction_count() noexcept
{
          return 0;
}

template<typename Xt, typename... Next>
constexpr int get_instruction_count(const Xt& expr, Next&&... next) noexcept {
          return Xt::used_instruction_count + get_instruction_count(std::forward<Next>(next)...);
}

/*namespace util*/ }

template<typename... Args>
constexpr int get_variable_count_ub(Args&&... args) noexcept {
          return get_round_value(util::get_variable_count(std::forward<Args>(args)...), memory_register_page);
}

template<typename... Args>
constexpr int get_register_count_ub(Args&&... args) noexcept {
          return get_round_value(util::get_register_count(std::forward<Args>(args)...), memory_register_page);
}

template<typename... Args>
constexpr int  get_instruction_count_ub(Args&&... args) noexcept {
          return get_round_value(util::get_instruction_count(std::forward<Args>(args)...), memory_instruction_page);
}

template<typename... Args>
constexpr int  get_list_size_ub(Args&&...) noexcept {
          return get_round_value(static_cast<int>(sizeof...(Args)), memory_register_page);
}

/*namespace dsp*/ }
#endif
