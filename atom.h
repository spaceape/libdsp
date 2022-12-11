#ifndef dsp_atom_h
#define dsp_atom_h
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
#include "factory.h"
#include <functional>

namespace dsp {

using call_t = std::function<bool(unsigned int)>;

/* atom
*/
class atom: public core
{
  call_t       m_call;

  public:
  /* ff_*
   * function flags
  */
  static constexpr unsigned int ff_none = 0u;
  static constexpr unsigned int ff_source = 1u;
  static constexpr unsigned int ff_pass = 2u;
  static constexpr unsigned int ff_sink = 3u;

  private:
  template<typename Ot, typename Ft, int... Is>
  inline  bool  call_impl(Ot object, Ft function, unsigned int op, std::integer_sequence<int, Is...> sequence) noexcept {
          return (object->*function)(op, m_argv[Is]...);
  }

  protected:
  template<typename Ot, typename Ft, typename... Args>
  inline  bool    make_call(Ot* object, Ft function, Args&&... arguments) noexcept {
          factory l_factory(std::forward<Args>(arguments)...);
          if(l_factory.get_return_status()) {
              move(l_factory);
              m_call = [object,function,this](unsigned int op) noexcept -> bool {
                  return call_impl(object, function, op, std::make_integer_sequence<int, sizeof...(Args)>());
              };
              return true;
          }
          return false;
  }

  inline  bool  render(unsigned int op) noexcept override {
          return m_call(op);
  }

  public:
          atom(unsigned int) noexcept;
          atom(const atom&) noexcept = delete;
          atom(atom&&) noexcept = delete;
  virtual ~atom();
          atom& operator=(const atom&) noexcept = delete;
          atom& operator=(atom&&) noexcept = delete;
};

/*namespace dsp*/ }
#endif
