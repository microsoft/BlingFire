/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph_w2s.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf32Utils.h"

#include <string>

namespace BlingFire
{


FATestMorph_w2s::FATestMorph_w2s (FAAllocatorA * pAlloc) :
    FATestMorph (pAlloc)
{}


inline const int FATestMorph_w2s::
    BuildInputSegs (const int * pInBuff, const int InBuffSize)
{
    DebugLogAssert (pInBuff && 0 < InBuffSize);

    int SegCount = 0;

    for (int i = 0; i < InBuffSize; ++i) {
        if ('\t' == pInBuff [i]) {
            m_OutChain2Buffer [SegCount] = (i - 1) - SegCount;
            SegCount++;
        }
    }

    m_OutChain2Buffer [SegCount] = (InBuffSize - 1) - SegCount;
    SegCount++;

    return SegCount;
}


inline const bool FATestMorph_w2s::
    IsValid (const int * pB, const int BSize) const
{
    if (!pB || 2 > BSize) {
        return false;
    }

    if (!FAIsSortUniqed (pB, BSize)) {
        return false;
    }

    for (int i = 0; i < BSize; ++i) {
        if (0 > pB [i]) {
            return false;
        }
    }

    return true;
}


void FATestMorph_w2s::Test (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (m_pPRM);
    DebugLogAssert (0 < LineLen && pLineStr);

    std::ostream * pDbgOs = GetOutStream ();

    // convert UTF-8 string into array of integers (UTF-32)
    const int ChainSize = \
        FAStrUtf8ToArray (
            pLineStr, 
            LineLen, 
            m_ChainBuffer, 
            MaxChainSize - 1
        );

    if (0 > ChainSize || MaxChainSize - 1 < ChainSize) {
        if (pDbgOs) {
            std::string line (pLineStr, LineLen);
            (*pDbgOs) << "W2S ERROR: Bad input : \"" << line << "\"\n";
        }
        return;
    }

    // make input in the lower case, if needed
    if (m_IgnoreCase) {
        FAUtf32StrLower (m_ChainBuffer, ChainSize);
    }

    // find delimiter
    int DelimPos = FindDelim (m_ChainBuffer, ChainSize);

    if (-1 == DelimPos) {
        if (pDbgOs) {
            std::string line (pLineStr, LineLen);
            (*pDbgOs) << "W2S ERROR: Bad input : \"" << line << "\"\n";
        }
        return;
    }

    // get input segmentation
    const int * pInSegsBuff = m_ChainBuffer + DelimPos + 1;
    const int InSegBuffSize = ChainSize - (DelimPos + 1);
    const int InSegCount = BuildInputSegs (pInSegsBuff, InSegBuffSize);

    const int * pComp = m_ChainBuffer;
    const int CompLen = DelimPos;

    // make sure the output buffer does not contain any data
    memset (m_OutChainBuffer, 0, MaxChainSize * sizeof (int));

    // split word into segments
    int OutSegCount = \
        m_pPRM->ProcessW2S (pComp, CompLen, m_OutChainBuffer, MaxChainSize);
    DebugLogAssert (OutSegCount <= MaxChainSize);

    if (-1 == OutSegCount) {
        OutSegCount = 0;
    }

    // do not compare if input data are not valid
    if (!IsValid (m_OutChain2Buffer, InSegCount)) {
        return;
    }

    // see how output correlates with expected output
    const int ErrType = \
        UpdateCounts (
            m_OutChainBuffer, 
            OutSegCount, 
            m_OutChain2Buffer, 
            InSegCount
        );

    if (pDbgOs && ERR_NO_ERROR != ErrType) {

        (*pDbgOs) << "W2T ERROR: ";

        if (ERR_BIGGER == ErrType) {
            (*pDbgOs) << "BIGGER\t";
        } else if (ERR_SMALLER == ErrType) {
            (*pDbgOs) << "SMALLER\t";
        } else if (ERR_DIFFERENT == ErrType) {
            (*pDbgOs) << "DIFFERENT\t";
        }

        // print the output splitting
        FAPrintSplitting (
            *pDbgOs, 
            m_ChainBuffer, 
            ChainSize, 
            m_OutChainBuffer, 
            OutSegCount
        );
        (*pDbgOs) << "\tvs\t";

        // print the expexted splitting
        FAPrintSplitting (
            *pDbgOs, 
            m_ChainBuffer, 
            ChainSize,
            m_OutChain2Buffer, 
            InSegCount
        );
        (*pDbgOs) << '\n';
    }
}

}

