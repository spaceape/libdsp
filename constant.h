#ifndef dsp_constant_h
#define dsp_constant_h
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
#include "lattice/primitive.h"

namespace dsp {

class constant: public symbol
{
  fptype  m_value;

  public:
  static constexpr int used_variable_count = 1;
  static constexpr int used_register_count = 1;
  static constexpr int used_instruction_count = 1;
  
  public:
          constant() noexcept;
          constant(fptype) noexcept;
          constant(const constant&) noexcept;
          constant(constant&&) noexcept;
          ~constant();

          fptype    get_value() const noexcept;
          
          constant  operator+(const constant&) noexcept;
          constant  operator-(const constant&) noexcept;
          constant  operator*(const constant&) noexcept;
          constant  operator/(const constant&) noexcept;

          constant& operator+=(const constant&) noexcept;
          constant& operator-=(const constant&) noexcept;
          constant& operator*=(const constant&) noexcept;
          constant& operator/=(const constant&) noexcept;

          bool      operator<(const constant&) noexcept;
          bool      operator<=(const constant&) noexcept;
          bool      operator>(const constant&) noexcept;
          bool      operator>=(const constant&) noexcept;
          bool      operator==(const constant&) noexcept;
          bool      operator!=(const constant&) noexcept;
          int       operator<=>(const constant&) noexcept;

          operator  fptype() const noexcept;
          
          constant& swap(constant&) noexcept;

          constant& operator=(const constant&) noexcept;
          constant& operator=(constant&&) noexcept;
};

/*namespace dsp*/ }
#endif
