/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph_w2b.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf32Utils.h"
#include "FAStringTokenizer.h"

#include <string>

namespace BlingFire
{


FATestMorph_w2b::FATestMorph_w2b (FAAllocatorA * pAlloc) :
    FATestMorph (pAlloc),
    m_MaxId (0)
{
    m_word2id.SetAllocator (pAlloc);
}


const int FATestMorph_w2b::Word2Id (const int * pWord, const int WordLen)
{
    DebugLogAssert (pWord && 0 < WordLen);

    // see whether such word already exist
    const int * pId = m_word2id.Get (pWord, WordLen);

    if (pId) {

        return *pId;

    } else {
        // add a new word
        m_word2id.Add (pWord, WordLen, m_MaxId++);
        return m_MaxId - 1;
    }
}


void FATestMorph_w2b::Test (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (m_pPRM);
    DebugLogAssert (0 < LineLen && pLineStr);

    m_MaxId = 0;
    m_word2id.Clear ();

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
            (*pDbgOs) << "W2B ERROR: BAD INPUT\t" << Line << '\n';
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
            (*pDbgOs) << "W2B ERROR: BAD INPUT\t" << Line << '\n';
        }
        return;
    }

    // make input in the lower case, if needed
    if (m_IgnoreCase) {
        FAUtf32StrLower (m_ChainBuffer, ChainSize);
    }

    // build a set of ids from the input set of base-forms
    int InCount = 0;
    while (tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {

        const int WordSize = \
            FAStrUtf8ToArray (
                pTmpStr, 
                TmpStrLen,
                m_OutChainBuffer,
                MaxChainSize - 1
            );

        if (0 > WordSize || MaxChainSize - 1 < WordSize) {
            if (pDbgOs) {
                const std::string Line (pLineStr, LineLen);
                (*pDbgOs) << "W2B ERROR: BAD INPUT\t" << Line << '\n';
            }
            return;
        }

        if (m_IgnoreCase) {
            FAUtf32StrLower (m_OutChainBuffer, WordSize);
        }

        const int Id = Word2Id (m_OutChainBuffer, WordSize);
        m_OutChain2Buffer [InCount++] = Id;
    }

    if (0 == InCount) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "W2B ERROR: NO BASES\t" << Line << '\n';
        }
        return;
    }
    // make them sorted and uniqued
    InCount = FASortUniq (m_OutChain2Buffer, m_OutChain2Buffer + InCount);

    // generate base forms
     const int OutChainSize = \
        m_pPRM->ProcessW2B (
            m_ChainBuffer, 
            ChainSize, 
            m_OutChainBuffer, 
            MaxChainSize
        );
    DebugLogAssert (OutChainSize <= MaxChainSize);

    // build a set of ids from the generated list of base-forms
    int OutCount = 0;
    int Pos = 0;
    int CurrWordPos = 0;

    while (Pos < OutChainSize) {

        if (0 == m_OutChainBuffer [Pos]) {

            const int CurrWordSize = Pos - CurrWordPos;
            DebugLogAssert (0 < CurrWordSize);

            const int Id = \
                Word2Id (m_OutChainBuffer + CurrWordPos, CurrWordSize);
            m_ChainBuffer [OutCount++] = Id;

            CurrWordPos = Pos + 1;

        } // of if (0 == m_OutChainBuffer [Pos]) ...

        Pos++;
    }
    // make them sorted and uniqued
    OutCount = FASortUniq (m_ChainBuffer, m_ChainBuffer + OutCount);

    // calculate the difference between input and result
    const int ErrType = \
        UpdateCounts (m_ChainBuffer, OutCount, m_OutChain2Buffer, InCount);

    // print error log, if needed
    if (pDbgOs && ERR_NO_ERROR != ErrType) {

        (*pDbgOs) << "W2B ERROR: ";

        if (ERR_BIGGER == ErrType) {
            (*pDbgOs) << "BIGGER\t";
        } else if (ERR_SMALLER == ErrType) {
            (*pDbgOs) << "SMALLER\t";
        } else if (ERR_DIFFERENT == ErrType) {
            (*pDbgOs) << "DIFFERENT\t";
        }

        std::string line (pLineStr, LineLen);
        (*pDbgOs) << line << "\t\t";
        FAPrintWordList (*pDbgOs, m_OutChainBuffer, OutChainSize);
        (*pDbgOs) << '\n';
    }
}

}
