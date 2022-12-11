#ifndef dsp_runtime_h
#define dsp_runtime_h
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

/* micro
 * assembled list instrucion
*/
struct micro
{
  static constexpr unsigned int  op_code_ret = 0x00;
  static constexpr unsigned int  op_code_imm = 0x01;
  static constexpr unsigned int  op_code_mov = 0x02;
  static constexpr unsigned int  op_code_pos = 0x03;
  static constexpr unsigned int  op_code_neg = 0x04;
  static constexpr unsigned int  op_code_add = 0x08;
  static constexpr unsigned int  op_code_sub = 0x09;
  static constexpr unsigned int  op_code_mul = 0x0a;
  static constexpr unsigned int  op_code_div = 0x0b;
  static constexpr unsigned int  op_code_nop = 0xff;

  unsigned int op_code:8;

  static constexpr unsigned int  op_dst_no = 0u;
  static constexpr unsigned int  op_dst_r = 1u;       // src is encoded as an offset into the register table

  unsigned int op_dst:4;

  static constexpr unsigned int  op_src_no = 0u;
  static constexpr unsigned int  op_src_r = 1u;       // src is encoded as an offset into the register table
  static constexpr unsigned int  op_src_d = 2u;       // src is encoded as an offset into the data pointer
  static constexpr unsigned int  op_src_sp = 3u;      // src is encoded as a stack pointer offset
  static constexpr unsigned int  op_src_p = 7u;
  static constexpr unsigned int  op_src_i = 15u;

  unsigned int op_src:4;

  unsigned int bit_flags:4;

  unsigned int bit_const:1;
  unsigned int bit_volatile:1;
  unsigned int bit_halt:1;
  unsigned int bit_return:1;

  union {
    int r;
  } dst;

  union {
    int r;
    fptype* p;
  } src;
};

inline bool is_return_micro(micro& inst) noexcept {
      return inst.op_code != micro::op_code_nop;
}

/*namespace dsp*/ }
#endif
