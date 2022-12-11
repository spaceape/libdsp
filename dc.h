#ifndef dsp_dc_h
#define dsp_dc_h
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

namespace dsp {

/* dc
   dsp device context
*/
class dc
{
  /* vector_base_t
  */
  struct vector_base_t
  {
    fptype*      data;
    int          size;   // requested size
  };

  /* pc_
     process state flags
  */
  static constexpr unsigned int pc_state_suspend = 0u;
  static constexpr unsigned int pc_state_ready = 1u;      // process is active
  static constexpr unsigned int pc_state_busy = 16u;      // process is currently running
  static constexpr unsigned int pc_state_wait = 32u;      // one or more source nodes failed to return

  /* pe_
     process errors
  */
  static constexpr int e_okay =  0;
  static constexpr int e_return_fault = 1;
  static constexpr int e_render_fault = 2;
  static constexpr int e_fail = -1;

  static constexpr int v_invalid = -1;
  static constexpr int v_default = 0;

  /* branch_base_t
  */
  struct branch_base_t
  {
    unsigned int   sample_format;
    int            sample_rate;
    int            return_flags;   
    int            return_vector;
    int            vector_assign_lb;
    int            vector_assign_ub;
    float          gain;              // accumulated gain from all the converged branches
    float          bias;              // accumulated bias from all the converged branches
    branch_base_t* branch_parent;
  };

  /* process_base_t
  */
  struct process_base_t: public branch_base_t
  {
    core*           owner;
    unsigned int    state;
    branch_base_t*  branch_head;
    branch_base_t*  branch_tail;
    float           step_latency;
    float           step_time;
    float           dt;
    float           time;
    float           omega;
  };

  protected:
  /* op_
     render flags
  */
  static constexpr unsigned int op_none = 0;
  static constexpr unsigned int op_render_additive = 1u;

  protected:
  /* dc_t
   * device context backup struct for context switches
  */
  struct dc_t {
    apu*            restore_apu;
    process_base_t* restore_process;
  };

  private:
  static  process_base_t* s_process;
  
  protected:
          int       dsp_get_sample_count() const noexcept;
          int       dsp_get_sample_size() const noexcept;

          fptype*   dsp_get_return_vector() noexcept;
          fptype*   dsp_set_return_vector() noexcept;
          fptype*   dsp_make_scratch_vector() noexcept;
          fptype*   dsp_make_scratch_vector(int) noexcept;
          void      dsp_drop_scratch_vector(fptype*) noexcept;

  static  void      pcm_clr(fptype*, int) noexcept;  
  static  void      pcm_mov(fptype*, fptype, int) noexcept;  
  static  void      pcm_mov(fptype*, fptype*, int) noexcept;  
  static  void      pcm_add(fptype*, fptype*, int) noexcept;
  static  void      pcm_mul(fptype*, fptype*, int) noexcept;

          int      dsp_get_sample_rate() const noexcept;
          unsigned int  dsp_get_sample_format() const noexcept;
          unsigned int  dsp_get_sample_format(unsigned int) const noexcept;
          float    dsp_get_dt() const noexcept;
          float    dsp_get_time() const noexcept;
          float    dsp_get_sample_time() const noexcept;
          float    dsp_get_omega() const noexcept;
          float    dsp_get_omega(float) const noexcept;
          float    dsp_get_sample_omega() const noexcept;
          float    dsp_get_sample_omega(float) const noexcept;
          void*    dsp_get_process_id() const noexcept;

          void     dsp_save(dc_t&, apu*) noexcept;
          void     dsp_restore(dc_t&) noexcept;

  friend class apu;
  public:
          dc() noexcept;
          dc(const dc&) noexcept = delete;
          dc(dc&&) noexcept = delete;
          ~dc();
          dc&  swap(dc&) = delete;
          dc&  operator=(const dc&) noexcept = delete;
          dc&  operator=(dc&&) noexcept = delete;
};


/*namespace dsp*/ }
#endif
