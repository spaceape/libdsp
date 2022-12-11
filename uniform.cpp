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
#include "uniform.h"

namespace dsp {

      uniform::uniform() noexcept:
      m_value_ptr(nullptr)
{
}

      uniform::uniform(const uniform& copy) noexcept:
      m_value_ptr(copy.m_value_ptr)
{
}

      uniform::uniform(uniform&& copy) noexcept:
      m_value_ptr(copy.m_value_ptr)
{
      copy.m_value_ptr = nullptr;
}

      uniform::~uniform()
{
}

void  uniform::bind(fptype* address) noexcept
{
      m_value_ptr = address;
}

void  uniform::unbind() noexcept
{
      m_value_ptr = nullptr;
}

fptype uniform::get_value() const noexcept
{
      return m_value_ptr[0];
}

fptype uniform::get_value(int index) const noexcept
{
      return m_value_ptr[index];
}

bool  uniform::is_bound() const noexcept
{
      return m_value_ptr != nullptr;
}

fptype uniform::operator[](int index) const noexcept
{
      return m_value_ptr[index];
}

      uniform::operator fptype() const noexcept
{
      return m_value_ptr[0];
}

      uniform::operator fptype*() const noexcept
{
      return m_value_ptr;
}

uniform& uniform::swap(uniform& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          fptype* l_lhs = rhs.m_value_ptr;
          rhs.m_value_ptr = m_value_ptr;
          m_value_ptr = l_lhs;
      }
      return *this;
}

uniform& uniform::operator=(const uniform& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          m_value_ptr = rhs.m_value_ptr;
      }
      return *this;
}

uniform& uniform::operator=(uniform&& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          m_value_ptr = rhs.m_value_ptr;
          rhs.m_value_ptr = nullptr;
      }
      return *this;
}

/*namespace dsp*/ }
