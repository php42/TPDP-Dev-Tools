/*
    Copyright 2021 php42

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once
#include <cstdint>
#include <cstddef>
#include <intrin.h>

// faster memcmp implementation for cases where
// we only want a boolean parity test
// returns 0 when block1 and block2 are equal, nonzero otherwise
static inline int sse2_memcmp(const void *block1, const void *block2, std::size_t sz)
{
    auto pos1 = (const uint8_t*)block1;
    auto pos2 = (const uint8_t*)block2;

    while(sz > 0)
    {
        if(sz >= 32) // branch avoidance and pipelining (no avx for compat)
        {
            auto b1 = _mm_loadu_si128((const __m128i*)pos1);
            auto b2 = _mm_loadu_si128((const __m128i*)pos2);
            auto cmp1 = (uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(b1, b2));

            auto b3 = _mm_loadu_si128((const __m128i*)&pos1[16]);
            auto b4 = _mm_loadu_si128((const __m128i*)&pos2[16]);
            auto cmp2 = (uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(b3, b4));

            if((cmp1 & cmp2) != 0xffffu)
                return -1;

            pos1 += 32;
            pos2 += 32;
            sz -= 32;
        }
        else if(sz >= 16) // following cases could be implemented as jump table for further branch avoidance
        {
            auto b1 = _mm_loadu_si128((const __m128i*)pos1);
            auto b2 = _mm_loadu_si128((const __m128i*)pos2);
            auto cmp = (uint16_t)_mm_movemask_epi8(_mm_cmpeq_epi8(b1, b2));

            if(cmp != 0xffffu)
                return -1;

            pos1 += 16;
            pos2 += 16;
            sz -= 16;
        }
        else if(sz >= 8)
        {
            if(*(const uint64_t*)pos1 != *(const uint64_t*)pos2)
                return -1;

            pos1 += 8;
            pos2 += 8;
            sz -= 8;
        }
        else if(sz >= 4)
        {
            if(*(const uint32_t*)pos1 != *(const uint32_t*)pos2)
                return -1;

            pos1 += 4;
            pos2 += 4;
            sz -= 4;
        }
        else
        {
            if(*pos1 != *pos2)
                return -1;

            ++pos1;
            ++pos2;
            --sz;
        }
    }

    return 0;
}
