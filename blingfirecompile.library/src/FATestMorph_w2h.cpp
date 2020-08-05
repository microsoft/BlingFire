/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph_w2h.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf32Utils.h"
#include "FATrWordIOTools_utf8.h"

#include <string>
#include <algorithm>

namespace BlingFire
{


FATestMorph_w2h::FATestMorph_w2h (FAAllocatorA * pAlloc) :
    FATestMorph (pAlloc),
    m_pHyph (NULL),
    m_UseAltW2H (false)
{}


void FATestMorph_w2h::SetW2H (FAHyphInterpreter_t < int > * pHyph)
{
    m_pHyph = pHyph;
}


void FATestMorph_w2h::SetUseAltW2H (const bool UseAltW2H)
{
    m_UseAltW2H = UseAltW2H;
}


inline const int FATestMorph_w2h::Chain2Set (int * pChain, const int Size)
{
    DebugLogAssert (0 <= Size && FALimits::MaxWordLen > Size);

    int * pOut = pChain;

    for (int Pos = 0; Pos < Size; ++Pos) {

        const int HypId = pChain [Pos];

        if (0 < HypId) {
            DebugLogAssert (0x7fffffff / FALimits::MaxWordLen > HypId);
            const int Val = (HypId * FALimits::MaxWordLen) + Pos;
            *pOut++ = Val;
        }
    }

    std::sort (pChain, pOut);

    const int OutSize = int (pOut - pChain);
    return OutSize;
}


void FATestMorph_w2h::Test (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (m_pHyph);
    DebugLogAssert (0 < LineLen && pLineStr);

    std::ostream * pDbgOs = GetOutStream ();

    // parse the input
    int ChainSize = FATrWordIOTools_utf8::Str2IwOw (pLineStr, LineLen, \
        m_ChainBuffer, m_OutChain2Buffer, MaxChainSize - 2);

    if (0 > ChainSize || MaxChainSize - 2 < ChainSize) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2H ERROR: BAD INPUT\t" << Line << '\n';
        }
        return;
    }

    // process the input
    const int OutSize = \
        m_pHyph->Process (
                m_ChainBuffer,
                ChainSize,
                m_OutChainBuffer,
                MaxChainSize,
                m_UseAltW2H
            );

    if (-1 == OutSize) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2H ERROR: BAD INPUT\t" << Line << '\n';
        }
        return;
    }

    DebugLogAssert (OutSize == ChainSize);

    // make chain --> set convertion in-place
    const int Size = Chain2Set (m_OutChainBuffer, OutSize);
    const int Size2 = Chain2Set (m_OutChain2Buffer, ChainSize);

    // calculate the difference between input and result
    const int ErrType = \
        UpdateCounts (
            m_OutChainBuffer,
            Size,
            m_OutChain2Buffer,
            Size2
        );

    if (pDbgOs && ERR_NO_ERROR != ErrType) {

        (*pDbgOs) << "W2H ERROR: ";

        if (ERR_BIGGER == ErrType) {
            (*pDbgOs) << "BIGGER\t";
        } else if (ERR_SMALLER == ErrType) {
            (*pDbgOs) << "SMALLER\t";
        } else if (ERR_DIFFERENT == ErrType) {
            (*pDbgOs) << "DIFFERENT\t";
        }

        // print the word
        FAPrintWord (*pDbgOs, m_ChainBuffer, ChainSize);
        // print the expected results
        (*pDbgOs) << '\t' << std::string (pLineStr, LineLen) << '\t';
        // process the input (again)
        m_pHyph->Process (m_ChainBuffer, ChainSize, \
            m_OutChainBuffer, MaxChainSize, m_UseAltW2H);
        // print the hyphenator's output 
        FAPrintHyphWord (*pDbgOs, m_ChainBuffer, m_OutChainBuffer, ChainSize);
        (*pDbgOs) << '\n';
    }
}

}
