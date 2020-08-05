/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph_w2t.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf32Utils.h"
#include "FAStringTokenizer.h"
#include "FATagSet.h"
#include "FAFsmConst.h"

#include <string>

namespace BlingFire
{


FATestMorph_w2t::FATestMorph_w2t (FAAllocatorA * pAlloc) :
    FATestMorph (pAlloc),
    m_Func (FAFsmConst::FUNC_W2T),
    m_pTagSet (NULL)
{}


void FATestMorph_w2t::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FATestMorph_w2t::SetFunc (const int Func)
{
    DebugLogAssert (FAFsmConst::FUNC_W2T == Func || FAFsmConst::FUNC_B2T == Func);

    m_Func = Func;
}


void FATestMorph_w2t::Test (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (m_pPRM && m_pTagSet);
    DebugLogAssert (0 < LineLen && pLineStr);

    std::ostream * pDbgOs = GetOutStream ();

    const char * pTmpStr;
    int TmpStrLen;

    FAStringTokenizer tokenizer;
    tokenizer.SetString (pLineStr, LineLen);
    tokenizer.SetSpaces ("\t");

    // read the first token
    if (!tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2T ERROR: BAD INPUT\t" << Line << '\n';
        }
        return;
    }

    // convert UTF-8 input word into array of integers (UTF-32)
    const int ChainSize = \
        FAStrUtf8ToArray (
            pTmpStr, 
            TmpStrLen, 
            m_ChainBuffer, 
            MaxChainSize - 1
        );

    if (0 > ChainSize || MaxChainSize - 1 < ChainSize) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2T ERROR: BAD INPUT\t" << Line << '\n';
        }
        return;
    }

    // make input in the lower case, if needed
    if (m_IgnoreCase) {
        FAUtf32StrLower (m_ChainBuffer, ChainSize);
    }

    int InTagCount = 0;

    while (tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {

        const int Tag = m_pTagSet->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == Tag) {
            if (pDbgOs) {
                const std::string Line (pLineStr, LineLen);
                (*pDbgOs) << "W2T ERROR: UNKNOWN TAG\t" << Line << '\n';
            }
            return;
        }

        m_OutChain2Buffer [InTagCount++] = Tag;
    }

    if (0 == InTagCount) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2T ERROR: NO TAGS\t" << Line << '\n';
        }
        return;
    }
    // make them sorted and uniqued
    InTagCount = \
        FASortUniq (m_OutChain2Buffer, m_OutChain2Buffer + InTagCount);

    // generate a set of tags with word-guesser
    const int * pOutTags = NULL;
    int OutTagCount;

    if (FAFsmConst::FUNC_B2T != m_Func) {
        OutTagCount = m_pPRM->ProcessW2T (m_ChainBuffer, ChainSize, &pOutTags);
    } else {
        OutTagCount = m_pPRM->ProcessB2T (m_ChainBuffer, ChainSize, &pOutTags);
    }

    DebugLogAssert (OutTagCount <= MaxChainSize);

    if (0 < OutTagCount) {

        for (int i = 0; i < OutTagCount; ++i) {
            DebugLogAssert (pOutTags);
            m_OutChainBuffer [i] = pOutTags [i];
        }
        OutTagCount = \
            FASortUniq (m_OutChainBuffer, m_OutChainBuffer + OutTagCount);

    } else if (-1 == OutTagCount) {
        OutTagCount = 0;
    }

    // calculate the difference between input and result
    const int ErrType = \
        UpdateCounts (
            m_OutChainBuffer, 
            OutTagCount, 
            m_OutChain2Buffer, 
            InTagCount
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

        // print the word
        FAPrintWordList (*pDbgOs, m_ChainBuffer, ChainSize);
        (*pDbgOs) << '\t';
        // print the output tags
        FAPrintTagList (*pDbgOs, m_pTagSet, m_OutChainBuffer, OutTagCount);
        (*pDbgOs) << "\tvs\t";
        // print the expected tags
        FAPrintTagList (*pDbgOs, m_pTagSet, m_OutChain2Buffer, InTagCount);
        (*pDbgOs) << '\n';
    }
}

}
