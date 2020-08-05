/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAUtf16Utils.h"

namespace BlingFire
{

inline static int NormalizeWord (int Word, bool fBE)
{
    return fBE ? ((Word & 0x00FF) << 8) | ((Word & 0xFF00) >> 8) : Word;
}


const wchar_t * FAUtf16ToInt (
        const wchar_t * ptr, 
        int * result, 
        bool fBE
    )
{ 
    int ret;
    int ch;

    DebugLogAssert (ptr);
    DebugLogAssert (result);
    
    ch = NormalizeWord (*ptr++, fBE);

    if (ch < 0xD800 || ch > 0xDFFF)
    {
        // not member of a surrogate pair
        *result = ch;
    }
    else
    {
        if ((ch & 0xFC00) == 0xD800)
        {
            ch &= ~0xFC00;
        }
        else
        {
            // first word not initialized with lower bracket
            return NULL;
        }

        ret = ch << 10;
        ch = NormalizeWord (*ptr++, fBE);
        
        if ((ch & 0xFC00) == 0xDC00)
        {
            ch &= ~0xFC00;
        }
        else
        {
            // second word not initialized with higher bracket
            return NULL;
        }

        ret |= ch;
        *result = ret + 0x10000;
    }

    return ptr;
}


const int FAStrUtf16ToArray (
        const wchar_t * pStr,
        const int Len,
        __out_ecount(MaxSize) int * pArray,
        const int MaxSize,
        bool fBE
    )
{
    DebugLogAssert (pStr);
    DebugLogAssert (pArray);

    const wchar_t * pEnd = pStr + Len;
    const int * pArrayEnd = pArray + MaxSize;

    // process symbol sequence
    int i = 0;
    while (pStr < pEnd && pArray < pArrayEnd) {

        pStr = FAUtf16ToInt (pStr, pArray, fBE);

        if (NULL == pStr) {
            // invalid input sequence
            return -1;
        }

        pArray++;
        i++;
    }

    return i;
}


wchar_t * FAIntToUtf16 (
        int Symbol, 
        __out_ecount(MaxSize) wchar_t * ptr, 
        const int MaxSize,
        bool fBE
    )
{
    DebugLogAssert (ptr);
    DebugLogAssert (0 <= Symbol && 0xFFFFF >= Symbol);

    if (0xFFFF >= (unsigned int) Symbol && 0 < MaxSize)
    {
        *ptr = (wchar_t) NormalizeWord (Symbol, fBE);
        ptr++;
        return ptr;
    }
    else if (1 < MaxSize)
    {
        Symbol -= 0x10000;
        // initialize first word with lower bracket
        *ptr = (wchar_t) NormalizeWord (0xD800 | (Symbol >> 10), fBE);
        ptr++;
        // initialize second word with higher bracket
        *ptr = (wchar_t) NormalizeWord (0xDC00 | (Symbol & 0x3FF), fBE);
        ptr++;
        return ptr;
    }

    return NULL;
}


const int FAArrayToStrUtf16 (
        const int * pArray, 
        const int Size, 
        __out_ecount(MaxStrSize) wchar_t * pStr, 
        const int MaxStrSize,
        bool fBE
    )
{
    DebugLogAssert (pStr);
    DebugLogAssert (pArray && 0 < Size);

    wchar_t * ptr = pStr;

    for (int i = 0; i < Size; ++i) {

        const int Symbol = pArray [i];

        const int CurrSize = (const int) (ptr - pStr);
        ptr = FAIntToUtf16 (Symbol, ptr, MaxStrSize - CurrSize, fBE);

        if (NULL == ptr) {
            // invalid input sequence
            return -1;
        }
    }

    const int CurrSize = (const int) (ptr - pStr);
    return CurrSize;
}


const bool FAIsUtf16LeEnc (const char * pEncName)
{
    bool IsUtf16 = false;

    if (pEncName) {

		if (0 == strcmp ("1200", pEncName) || 
			0 == strcmp ("UTF-16", pEncName) ||
			0 == strcmp ("UTF16", pEncName) ||
			0 == strcmp ("UTF-16LE", pEncName) ||
			0 == strcmp ("UTF16LE", pEncName)) {
            IsUtf16 = true;
        }
    }

    return IsUtf16;
}


const bool FAIsUtf16BeEnc (const char * pEncName)
{
    bool IsUtf16Be = false;

    if (pEncName) {

		if (0 == strcmp ("1201", pEncName) || 
			0 == strcmp ("UTF-16BE", pEncName) ||
			0 == strcmp ("UTF16BE", pEncName)) {
            IsUtf16Be = true;
        }
    }

    return IsUtf16Be;
}

}

