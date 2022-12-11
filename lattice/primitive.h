#ifndef dsp_lattice_primitive_h
#define dsp_lattice_primitive_h
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
#include <dsp.h>
#include <dsp/config.h>
#include <util.h>

namespace dsp {

struct null
{
  static constexpr int used_variable_count = 0;
  static constexpr int used_register_count = 0;
  static constexpr int used_instruction_count = 0;
};

struct error
{
  static constexpr int used_variable_count = 0;
  static constexpr int used_register_count = 0;
  static constexpr int used_instruction_count = 1;
};

struct symbol
{
  static constexpr int used_variable_count = 0;
  static constexpr int used_register_count = 0;
  static constexpr int used_instruction_count = 0;
};

template<typename Lt>
struct reference
{
  Lt& lhs;

  public:
  static constexpr int used_variable_count = Lt::used_variable_count;
  static constexpr int used_register_count = Lt::used_register_count;
  static constexpr int used_instruction_count = Lt::used_instruction_count;

  public:
  constexpr reference(Lt& l) noexcept:
            lhs(l) {
  }

  constexpr reference(const reference&) noexcept = default;
  
  constexpr reference(reference&&) noexcept = default;
  
  constexpr ~reference() = default;

  constexpr reference& operator=(const reference&) noexcept = default;
  
  constexpr reference& operator=(reference&&) noexcept = default;
};

template<typename Lt = null, typename Rt = null>
struct statement
{
  Lt  lhs;
  Rt  rhs;

  public:
  static constexpr int used_variable_count = Lt::used_variable_count + Rt::used_variable_count;
  static constexpr int used_register_count = Lt::used_register_count + Rt::used_register_count + 1;
  static constexpr int used_instruction_count = Lt::used_instruction_count + Rt::used_instruction_count + 1;

  public:
  constexpr statement() noexcept = default;

  constexpr statement(const Lt& l) noexcept:
            lhs(l) {
  }

  constexpr statement(const Lt& l, const Rt& r) noexcept:
            lhs(l),
            rhs(r) {
  }
  
  constexpr statement(const statement&) noexcept = default;
  
  constexpr statement(statement&&) noexcept = default;
  
  constexpr ~statement() = default;

  constexpr statement& operator=(const statement&) noexcept = default;
  
  constexpr statement& operator=(statement&&) noexcept = default;
};

template<typename Lt>
struct statement<Lt, null>
{
  Lt lhs;

  public:
  static constexpr int used_variable_count = Lt::used_variable_count;
  static constexpr int used_register_count = Lt::used_register_count + 1;
  static constexpr int used_instruction_count = Lt::used_instruction_count + 1;

  public:
  constexpr statement() noexcept = default;

  constexpr statement(const Lt& l) noexcept:
            lhs(l) {
  }

  constexpr statement(const statement&) noexcept = default;
  
  constexpr statement(statement&&) noexcept = default;
  
  constexpr ~statement() = default;

  constexpr statement& operator=(const statement&) noexcept = default;
  
  constexpr statement& operator=(statement&&) noexcept = default;
};

template<>
struct statement<null, null>
{
  public:
  static constexpr int used_variable_count = 0;
  static constexpr int used_register_count = 0;
  static constexpr int used_instruction_count = 1;

  public:
  constexpr statement() noexcept = default;
  
  constexpr statement(const statement&) noexcept = default;
  
  constexpr statement(statement&&) noexcept = default;
  
  constexpr ~statement() = default;

  constexpr statement& operator=(const statement&) noexcept = default;
  
  constexpr statement& operator=(statement&&) noexcept = default;
};

/*namespace dsp*/ }
#endif
