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
#include "argument.h"
#include "runtime.h"

namespace dsp {

      argument::argument() noexcept:
      m_code_head(nullptr),
      m_code_tail(nullptr)
{
}

      argument::argument(micro* code_head, micro* code_tail) noexcept:
      m_code_head(code_head),
      m_code_tail(code_tail)
{
}

      argument::argument(const argument& copy) noexcept:
      m_code_head(copy.m_code_head),
      m_code_tail(copy.m_code_tail)
{
}

      argument::argument(argument&& copy) noexcept:
      m_code_head(copy.m_code_head),
      m_code_tail(copy.m_code_tail)
{
      copy.m_code_head = nullptr;
      copy.m_code_tail = nullptr;
}

      argument::~argument()
{
}

void  argument::bind(micro* code_head, micro* code_tail) noexcept
{
      m_code_head = code_head;
      m_code_tail = code_tail;
}

void  argument::unbind() noexcept
{
      m_code_head = nullptr;
      m_code_tail = nullptr;
}

int   argument::load(micro*& code_head, micro*& code_tail) const noexcept
{
      if(m_code_head) {
          int l_code_size = m_code_tail - m_code_head;
          if(l_code_size > 0) {
              code_head = m_code_head;
              code_tail = m_code_tail;
              return l_code_size;
          }
      }
      code_head = nullptr;
      code_tail = nullptr;
      return 0;
}

bool  argument::is_bound() const noexcept
{
      return m_code_head != nullptr;
}

argument& argument::swap(argument& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          micro* l_code_head = rhs.m_code_head;
          micro* l_code_tail = rhs.m_code_tail;
          rhs.m_code_head = m_code_head;
          rhs.m_code_tail = m_code_tail;
          m_code_head = l_code_head;
          m_code_tail = l_code_tail;
      }
      return *this;
}

argument& argument::operator=(const argument& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          m_code_head = rhs.m_code_head;
          m_code_tail = rhs.m_code_tail;
      }
      return *this;
}

argument& argument::operator=(argument&& rhs) noexcept
{
      if(std::addressof(rhs) != this) {
          m_code_head = rhs.m_code_head;
          m_code_tail = rhs.m_code_tail;
          rhs.m_code_head = nullptr;
          rhs.m_code_tail = nullptr;
      }
      return *this;
}

/*namespace dsp*/ }
