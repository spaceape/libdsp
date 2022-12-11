#ifndef dsp_lattice_expr_h
#define dsp_lattice_expr_h
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
#include "primitive.h"
#include <dsp/runtime.h>
#include <dsp/runtime.h>
#include <dsp/runtime.h>

namespace dsp {

template<typename Lt>
struct op_pos: public statement<Lt>
{
  constexpr op_pos(const Lt& l) noexcept:
            statement<Lt>(l) {
  }
  constexpr op_pos(const op_pos&) noexcept = default;
  constexpr op_pos(op_pos&&) noexcept = default;
  constexpr ~op_pos() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_pos;
  }

  constexpr op_pos& operator=(const op_pos&) noexcept = default;
  constexpr op_pos& operator=(op_pos&&) noexcept = default;
};

template<typename Lt>
struct op_neg: public statement<Lt>
{
  constexpr op_neg(const Lt& l) noexcept:
            statement<Lt>(l) {
  }

  constexpr op_neg(const op_neg&) noexcept = default;
  constexpr op_neg(op_neg&&) noexcept = default;
  constexpr ~op_neg() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_neg;
  }

  constexpr op_neg& operator=(const op_neg&) noexcept = default;
  constexpr op_neg& operator=(op_neg&&) noexcept = default;
};

template<typename Lt, typename Rt>
struct op_add: public statement<Lt, Rt>
{
  constexpr op_add(const Lt& l, const Rt& r) noexcept:
            statement<Lt, Rt>(l, r) {
  }
  constexpr op_add(const op_add&) noexcept = default;
  constexpr op_add(op_add&&) noexcept = default;
  constexpr ~op_add() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_add;
  }
  constexpr op_add& operator=(const op_add&) noexcept = default;
  constexpr op_add& operator=(op_add&&) noexcept = default;
};

template<typename Lt, typename Rt>
struct op_sub: public statement<Lt, Rt>
{
  constexpr op_sub(const Lt& l, const Rt& r) noexcept:
            statement<Lt, Rt>(l, r) {
  }
  constexpr op_sub(const op_sub&) noexcept = default;
  constexpr op_sub(op_sub&&) noexcept = default;
  constexpr ~op_sub() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_sub;
  }
  constexpr op_sub& operator=(const op_sub&) noexcept = default;
  constexpr op_sub& operator=(op_sub&&) noexcept = default;
};

template<typename Lt, typename Rt>
struct op_mul: public statement<Lt, Rt>
{
  constexpr op_mul(const Lt& l, const Rt& r) noexcept:
            statement<Lt, Rt>(l, r) {
  }
  constexpr op_mul(const op_mul&) noexcept = default;
  constexpr op_mul(op_mul&&) noexcept = default;
  constexpr ~op_mul() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_mul;
  }

  constexpr op_mul& operator=(const op_mul&) noexcept = default;
  constexpr op_mul& operator=(op_mul&&) noexcept = default;
};

template<typename Lt, typename Rt>
struct op_div: public statement<Lt, Rt>
{
  constexpr op_div(const Lt& l, const Rt& r) noexcept:
            statement<Lt, Rt>(l, r) {
  }
  constexpr op_div(const op_div&) noexcept = default;
  constexpr op_div(op_div&&) noexcept = default;
  constexpr ~op_div() = default;

  constexpr operator unsigned int() const noexcept {
          return micro::op_code_div;
  }
  constexpr op_div& operator=(const op_div&) noexcept = default;
  constexpr op_div& operator=(op_div&&) noexcept = default;
};

/*namespace dsp*/ }
#endif
