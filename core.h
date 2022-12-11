#ifndef dsp_core_h
#define dsp_core_h
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
#include "lattice/expr.h"
#include "constant.h"
#include "uniform.h"
#include "argument.h"
#include "dc.h"

namespace dsp {

class gate
{
  core*   m_owner;
  core*   m_source;             // input node
  gate*   m_gate_next;
  fptype* m_value_ptr;
  bool    m_enable_bit;

  protected:
          void   bind(fptype*) noexcept;
          void   unbind() noexcept;

  friend class core;
  friend class apu;
  public:
          gate(core*) noexcept;
          gate(const gate&) noexcept = delete;
          gate(gate&&) noexcept = delete;
          ~gate();

          bool   attach(core*) noexcept;
          bool   attach(core&) noexcept;
          bool   detach() noexcept;

          fptype* get_return_vector() const noexcept;
          fptype* get_return_vector(int) const noexcept;

          bool   is_attached_to(core*) const noexcept;
          bool   is_attached(bool = true) const noexcept;
          bool   is_detached() const noexcept;

          gate&  operator=(const gate&) noexcept = delete;
          gate&  operator=(gate&&) noexcept = delete;
};

/* core
   base for dsp functor and factory classes
*/
class core: public dc
{
  apu*          m_target;
  core*         m_core_next;
  gate*         m_gate_head;
  gate*         m_gate_tail;

  protected:
  fptype*       m_d_base;             // memory for data registers
  micro*        m_i_base;             // memory for the execution stacks
  short int     m_d_size;             // size of the memory region for data registers
  short int     m_i_size;             // size of the memory region for the instructions
  short int     m_dov;                // dynamic output vector
  int           m_dpc;                // dynamic pass count
  int           m_dcc;                // dynamic convergence counter: number of paths that converge to this node

  argument*     m_argv;
  short int     m_argc;
  short int     m_arg_size;
  short int     m_variable_count;
  short int     m_register_count;
  short int     m_instruction_count;
  short int     m_option;
  unsigned int  m_hash; 

  friend class  gate;
  friend class  apu;

  private:
          bool  bind(gate*) noexcept;
          bool  drop(gate*) noexcept;
          bool  join(core*, gate*) noexcept;
          bool  can_join(core*) noexcept;
          bool  part(core*, gate*) noexcept;

  protected:
          void  move(core&) noexcept;
          void  release() noexcept;
          void  dispose() noexcept;

  virtual bool  join_event(gate*, core*) noexcept;
  virtual bool  part_event(gate*, core*) noexcept;

  virtual bool  render(unsigned int) noexcept;
  virtual void  sync(float) noexcept;

  public:
  static constexpr short int o_none = 0u;
  static constexpr short int o_enable_join_event = 256;
  static constexpr short int o_enable_part_event = 512;

  public:
          core(unsigned int) noexcept;
          core(const core&) noexcept = delete;
          core(core&&) noexcept = delete;
  virtual ~core();

          apu*  get_target() const noexcept;

  virtual bool  attach(core*) noexcept;
          bool  detach(core*, bool = false) noexcept;

  virtual unsigned int get_sample_format() const noexcept;
  virtual bool  set_sample_format(unsigned int) noexcept;
  virtual int   get_sample_rate() const noexcept;
  virtual bool  set_sample_rate(int) noexcept;

          core& operator=(const core&) noexcept = delete;
          core& operator=(core&&) noexcept = delete;

  #ifdef DEBUG
  #ifdef LINUX
          void  dump_tree(FILE*, core*, int, int) noexcept;
          void  dump_tree(FILE*) noexcept;
          void  dump_code(FILE*, int = 4, int = 32) noexcept;
  #endif
  #endif
};

/*namespace dsp*/ }
#endif
