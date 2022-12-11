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
#include "dc.h"
#include "format.h"
#include "apu.h"
#include <cmath>
#include <numbers>

namespace dsp {

static thread_local apu*  s_apu;

      dc::process_base_t* dc::s_process;

      dc::dc() noexcept
{
}

      dc::~dc()
{
}

int   dc::dsp_get_sample_count() const noexcept
{
      return std::roundf(static_cast<float>(dsp_get_sample_rate()) * dsp_get_dt());
}

int   dc::dsp_get_sample_size() const noexcept
{
      return 1 << dsp_get_sample_format(fmt_size_bits);
}

fptype*  dc::dsp_get_return_vector() noexcept
{
      return s_apu->dvf_get_data_immediate(s_process->branch_tail->return_vector);
}

fptype*  dc::dsp_set_return_vector() noexcept
{
      return nullptr;
}

fptype*  dc::dsp_make_scratch_vector() noexcept
{
      int  l_vector_index = s_apu->dvf_acquire();
      if(l_vector_index >= 0) {
          return s_apu->dvf_get_data_immediate(l_vector_index);
      }
      return nullptr;
}

fptype* dc::dsp_make_scratch_vector(int size) noexcept
{
      int  l_vector_index = s_apu->dvf_acquire(size);
      if(l_vector_index >= 0) {
          return s_apu->dvf_get_data_immediate(l_vector_index);
      }
      return nullptr;
}

void  dc::dsp_drop_scratch_vector(fptype* address) noexcept
{
      int  l_vector_index = s_apu->dvf_lookup(address);
      if(l_vector_index >= 0) {
          s_apu->dvf_release(l_vector_index, false);
      }
}

void  dc::pcm_clr(fptype* dp, int size) noexcept
{
      fptype* l_base = dp;
      fptype* l_last = l_base + size;
      while(l_base < l_last) {
          *l_base = 0.0f;
          ++l_base;
      }
}

void  dc::pcm_mov(fptype* dp, fptype imm, int size) noexcept
{
      fptype* l_base = dp;
      fptype* l_last = l_base + size;
      while(l_base < l_last) {
          *l_base = imm;
          ++l_base;
      }
}

void  dc::pcm_mov(fptype* dp, fptype* sp, int size) noexcept
{
        fptype* l_dst_base = dp;
        fptype* l_dst_last = dp + size;
        fptype* l_src_base = sp;
        while(l_dst_base < l_dst_last) {
            *l_dst_base = *l_src_base;
            ++l_src_base;
            ++l_dst_base;
        }
}

void  dc::pcm_add(fptype* dp, fptype* sp, int size) noexcept
{
        fptype* l_dst_base = dp;
        fptype* l_dst_last = dp + size;
        fptype* l_src_base = sp;
        while(l_dst_base < l_dst_last) {
            *l_dst_base = *l_dst_base + *l_src_base;
            ++l_src_base;
            ++l_dst_base;
        }
}

void  dc::pcm_mul(fptype* dp, fptype* sp, int size) noexcept
{
        fptype* l_dst_base = dp;
        fptype* l_dst_last = dp + size;
        fptype* l_src_base = sp;
        while(l_dst_base < l_dst_last) {
            *l_dst_base = *l_dst_base * *l_src_base;
            ++l_src_base;
            ++l_dst_base;
        }
}

int   dc::dsp_get_sample_rate() const noexcept
{
      return s_process->branch_tail->sample_rate;
}

unsigned int  dc::dsp_get_sample_format() const noexcept
{
      return s_process->branch_tail->sample_format;
}

unsigned int  dc::dsp_get_sample_format(unsigned int sample_format_bits) const noexcept
{
      return dsp_get_sample_format() & sample_format_bits;
}

float dc::dsp_get_dt() const noexcept
{
      return s_process->dt;
}

float dc::dsp_get_time() const noexcept
{
      return s_process->time;
}

float dc::dsp_get_sample_time() const noexcept
{
      return 1.0f / static_cast<float>(dsp_get_sample_rate());
}

float dc::dsp_get_omega() const noexcept
{
      return s_process->omega;
}

float dc::dsp_get_omega(float t) const noexcept
{
      return t * s_process->omega;
}

float dc::dsp_get_sample_omega() const noexcept
{
      return std::numbers::pi * 2.0f / static_cast<float>(dsp_get_sample_rate());
}

float dc::dsp_get_sample_omega(float t) const noexcept
{
      return t * std::numbers::pi * 2.0f / static_cast<float>(dsp_get_sample_rate());
}

void* dc::dsp_get_process_id() const noexcept
{
      return s_process;
}

/* dsp_save()
   pack the current context variables to dc and create a new context from apu
*/
void  dc::dsp_save(dc_t& dc, apu* ap) noexcept
{
      // store current context
      dc.restore_apu     = s_apu;
      dc.restore_process = s_process;

      // apply new context
      s_apu = ap;
}

void  dc::dsp_restore(dc_t& dc) noexcept
{
      s_process = dc.restore_process;
      s_apu     = dc.restore_apu;
}

/*namespace dsp*/ }
