#ifndef dsp_format_h
#define dsp_format_h
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

/* format flags: [ - F F F  - - S S ]
*/
static constexpr unsigned int fmt_undef = 0;
static constexpr unsigned int fmt_mode_bits = 0x70;
static constexpr unsigned int fmt_size_bits = 0x02;

static constexpr unsigned int mode_pcm  = 0x10;           // PCM encoding; size bits indicate the number of channels (1..8)
static constexpr unsigned int mode_apm  = 0x20;           // Amplitude/Phase encoding

static constexpr unsigned int fmt_pcm   = mode_pcm;
static constexpr unsigned int fmt_pcm_1 = fmt_pcm;        // PCM single channel
static constexpr unsigned int fmt_pcm_2 = mode_pcm | 1;   // PCM 2 channels interleaved
static constexpr unsigned int fmt_pcm_4 = mode_pcm | 2;   // PCM 4 channels interleaved
static constexpr unsigned int fmt_pcm_8 = mode_pcm | 4;   // PCM 8 channels interleaved
static constexpr unsigned int fmt_apm   = mode_apm | 1;

/*namespace dsp*/ }
#endif
