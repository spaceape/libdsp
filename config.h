#ifndef dsp_config_h
#define dsp_config_h
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
#include <global.h>
#include "format.h"
#include <limits>

namespace dsp {

/* enable_cache
 * enable dsp node caching in the apu 
*/
constexpr bool enable_cache = true;

/* memory_register_page
 * how many registers to reserve in a register page"
*/
constexpr int  memory_register_page = 16;  

/* memory_instruction_page
 * how many instructions to reserve in an instruction page"
*/
constexpr int  memory_instruction_page = 64;

/* memory_vector_block
*/
constexpr int  memory_vector_block = 64;

static_assert((memory_vector_block % fpu::pts) == 0, "vector block size must be a multiple of fpu::pts");

/* memory_vector_page
 * suggestion of how many samples a pool vector allocator should allocate
*/
constexpr int  memory_vector_page = 131072;

static_assert((memory_vector_page % memory_vector_block) == 0, "vector page size must be a multiple of memory_vector_block");
static_assert(memory_vector_page > memory_vector_block * 2, "vector page size must be at least twice the size of memory_vector_block");

/* default sample rate
 * default sample rate to initialize atoms with
*/
constexpr int  default_sample_rate = 48000;


/* sample rate min/max
*/
constexpr int  min_sample_rate = 256;
static_assert(default_sample_rate >= min_sample_rate, "Default sample rate must be at least min_sample_rate");
constexpr int  max_sample_rate = std::numeric_limits<int>::max();
static_assert(default_sample_rate <= max_sample_rate, "Default sample rate must be at most max_sample_rate");

/* default control rate
 * default control rate to initialize atoms with
*/
constexpr int  default_control_rate = 100;

/* default sample format
 * default sample format to initialize atoms with
*/
constexpr int  default_sample_format = fmt_pcm;

/*namespace dsp*/ }
#endif
