/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATrWordIOTools_utf8.h"
#include "FALimits.h"
#include "FAFsmConst.h"
#include "FAException.h"
#include "FAUtf32Utils.h"
#include "FAMultiMapCA.h"
#include "FAUtf8Utils.h"

namespace BlingFire
{


static inline const int FATrStr2Ow (const int * pStr, const int Len)
{
    int Symbol = 0;
    bool IsX1 = false;
    bool IsX2 = false;
    bool IsAfter = false;

    if (1 == Len && int ('?') == *pStr) {
        return FAFsmConst::HYPH_DONT_CARE;
    }

    /// parse the input string
    for (int i = 0; i < Len; ++i) {

        const int C = pStr [i];

        // before-after delimiter
        if ('=' == C && !IsAfter) {

            IsAfter = true;
            // ignore the priority, e.g. =0
            while (i + 1 < Len && \
                   '0' <= pStr [i + 1] && '9' >= pStr [i + 1]) {
                i++;
            }
        // X
        } else if ('X' == C && 0 == Symbol) {

            if (!IsAfter)
                IsX1 = true;
            else
                IsX2 = true;
        // c
        } else {
            // bad action format
            FAAssert (0 == Symbol, FAMsg::IOError);
            Symbol = C;
        }
    }

    /// build the Ow

    if (!IsX1 && !IsX2) {

        if (0 == Symbol) {
            // [=0]
            return FAFsmConst::HYPH_SIMPLE_HYPH;
        } else {
            // [c=0]
            return int (Symbol << 4) | FAFsmConst::HYPH_ADD_BEFORE;
        }

    } else if (IsX1 && !IsX2) {

        if (0 == Symbol) {
            // [X=0]
            return FAFsmConst::HYPH_DELETE_BEFORE;
        } else {
            // [Xc=0]
            return int (Symbol << 4) | FAFsmConst::HYPH_CHANGE_BEFORE;
        }
    } else if (!IsX1 && IsX2 && Symbol) {

        // [=0Xc]
        return int (Symbol << 4) | FAFsmConst::HYPH_CHANGE_AFTER;

    } else if (IsX1 && IsX2 && Symbol) {

        // [X=0Xc]
        return int (Symbol << 4) | FAFsmConst::HYPH_DEL_AND_CHANGE;

    } else {

        // incorrect procedure
        FAAssert (0, FAMsg::IOError);
        return FAFsmConst::HYPH_SIMPLE_HYPH;
    }
}


const int FATrWordIOTools_utf8::
    Str2IwOw (
        const char * pStr,
        const int StrLen,
        __out_ecount(MaxOutSize) int * pIws,
        __out_ecount(MaxOutSize) int * pOws,
        const int MaxOutSize,
        const bool IgnoreCase,
        const FAMultiMapCA * pCharMap
    )
{
    const int MaxSize = 2 * FALimits::MaxWordLen;
    int Chain [MaxSize];

    const int ChainSize = FAStrUtf8ToArray (pStr, StrLen, Chain, MaxSize);

    if (0 >= ChainSize || ChainSize > MaxSize) {
        throw FAException (FAMsg::IOError, __FILE__, __LINE__);
    }

    int Count = 0;
    int ActLeft = -1;

    for (int i = 0; i < ChainSize; ++i) {

        int C = Chain [i];

        // action sequence begins
        if ('[' == C && -1 == ActLeft) {

            ActLeft = i + 1;

        // action sequence ends
        } else if (']' == C && -1 != ActLeft) {

            // [XX] should be preceeded by the input symbol
            FAAssert (0 < Count, FAMsg::IOError);

            const int ActSize = i - ActLeft;
            DebugLogAssert (0 < ActSize);

            const int Ow = FATrStr2Ow (Chain + ActLeft, ActSize);

            // [XX][YY] in a raw are not allowed
            if (Count - 1 < MaxOutSize && pOws) {
                FAAssert (0 == pOws [Count - 1], FAMsg::IOError);
                pOws [Count - 1] = Ow;
            }

            ActLeft = -1;

        // ordinary symbol
        } else if (-1 == ActLeft) {

            // too big
            FAAssert (Count < FALimits::MaxWordLen, FAMsg::IOError);

            // ignore-case
            if (IgnoreCase) {
                C = FAUtf32ToLower (C);
            }
            // charmap normalization
            if (pCharMap) {
                int Norm;
                const int NewCount = pCharMap->Get (C, &Norm, 1);
                // hyphenator can only use 1:1 mappings, 
                // so that length of the word is always the same
                if (1 == NewCount) {
                    C = Norm;
                }
            }

            if (Count < MaxOutSize) {
                if (pIws) {
                    pIws [Count] = C;
                }
                if (pOws) {
                    pOws [Count] = FAFsmConst::HYPH_NO_HYPH;
                }
            }

            Count++;
        }
    } // of for (int i = 0; i < ChainSize; ...

    return Count;
}


const int FATrWordIOTools_utf8::
    IwOw2Str (
            const int * pIws,
            const int * pOws,
            const int Size,
            __out_ecount (MaxOutSize) char * pOut,
            const int MaxOutSize
        )
{
    const char * pEnd;
    int ChStrLen;
    char ChStr [FAUtf8Const::MAX_CHAR_SIZE];

    int OutSize = 0;

    for (int i = 0; i < Size; ++i) {

        // get input symbol
        const int Iw = pIws [i];

        // copy input symbol out
        pEnd = FAIntToUtf8 (Iw, pOut + OutSize, MaxOutSize - OutSize);
        FAAssert (NULL != pEnd, FAMsg::IOError);
        OutSize = (int) (pEnd - pOut);

        // get output symbol
        const int Ow = pOws [i];

        // decode an operation code
        const int Op = Ow & 0xf;
        FAAssert (FAFsmConst::HYPH_NO_HYPH <= Op && \
            Op < FAFsmConst::HYPH_COUNT, FAMsg::IOError);

        // nothing to do
        if (FAFsmConst::HYPH_NO_HYPH == Op)
            continue;

        // decode an optional character
        const int Ch = Ow >> 4;
        pEnd = FAIntToUtf8 (Ch, ChStr, FAUtf8Const::MAX_CHAR_SIZE);
        FAAssert (NULL != pEnd, FAMsg::IOError);
        ChStrLen = (int) (pEnd - ChStr);

        if (FAFsmConst::HYPH_SIMPLE_HYPH == Op) {
            // [=]
            FAAssert (MaxOutSize - OutSize >= 3, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_ADD_BEFORE == Op) {
            // [c=]
            FAAssert (MaxOutSize - OutSize >= 3 + ChStrLen, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            memcpy (pOut + OutSize, ChStr, ChStrLen);
            OutSize += ChStrLen;
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_CHANGE_BEFORE == Op) {
            // [Xc=]
            FAAssert (MaxOutSize - OutSize >= 4 + ChStrLen, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('X');
            memcpy (pOut + OutSize, ChStr, ChStrLen);
            OutSize += ChStrLen;
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_DELETE_BEFORE == Op) {
            // [X=]
            FAAssert (MaxOutSize - OutSize >= 4, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('X');
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_CHANGE_AFTER == Op) {
            // [=Xc]
            FAAssert (MaxOutSize - OutSize >= 4 + ChStrLen, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int ('X');
            memcpy (pOut + OutSize, ChStr, ChStrLen);
            OutSize += ChStrLen;
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_DEL_AND_CHANGE == Op) {
            // [X=Xc]
            FAAssert (MaxOutSize - OutSize >= 5 + ChStrLen, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('X');
            pOut [OutSize++] = int ('=');
            pOut [OutSize++] = int ('X');
            memcpy (pOut + OutSize, ChStr, ChStrLen);
            OutSize += ChStrLen;
            pOut [OutSize++] = int (']');

        } else if (FAFsmConst::HYPH_DONT_CARE == Op) {
            // [?]
            FAAssert (MaxOutSize - OutSize >= 3, FAMsg::IOError);
            pOut [OutSize++] = int ('[');
            pOut [OutSize++] = int ('?');
            pOut [OutSize++] = int (']');
        }

    } // for (int i = 0; ...

    return OutSize;
}

}
