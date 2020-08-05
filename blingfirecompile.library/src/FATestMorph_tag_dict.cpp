/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph_tag_dict.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf32Utils.h"
#include "FAStringTokenizer.h"
#include "FATagSet.h"

#include <string>

namespace BlingFire
{


FATestMorph_tag_dict::FATestMorph_tag_dict (FAAllocatorA * pAlloc) :
    FATestMorph (pAlloc),
    m_pTagSet (NULL)
{}


void FATestMorph_tag_dict::
    SetTagDict (const FADictInterpreter_t < int > * pDict)
{
    m_pDict = pDict;
}


void FATestMorph_tag_dict::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FATestMorph_tag_dict::Test (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (m_pDict && m_pTagSet);
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
            (*pDbgOs) << "DICT ERROR: BAD INPUT\t" << Line << '\n';
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
            (*pDbgOs) << "DICT ERROR: BAD INPUT\t" << Line << '\n';
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
                (*pDbgOs) << "DICT ERROR: UNKNOWN TAG\t" << Line << '\n';
            }
            return;
        }

        m_OutChain2Buffer [InTagCount++] = Tag;
    }

    if (0 == InTagCount) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "DICT ERROR: NO TAGS\t" << Line << '\n';
        }
        return;
    }
    // make them sorted and uniqued
    InTagCount = FASortUniq (
            m_OutChain2Buffer, 
            m_OutChain2Buffer + InTagCount
        );

    const int MaxInfoSize = m_pDict->GetMaxInfoSize ();

    // get a set of tags from the dictionary
    int OutTagCount = m_pDict->GetInfo (
            m_ChainBuffer, 
            ChainSize, 
            m_OutChainBuffer, 
            MaxChainSize
        );
    DebugLogAssert (OutTagCount < MaxChainSize);

    if (-1 != MaxInfoSize && MaxInfoSize < OutTagCount) {
        if (pDbgOs) {
            const std::string Line (pLineStr, LineLen);
            (*pDbgOs) << "DICT ERROR: The output exceeded max size\t" << Line << '\n';
        }
        return;
    }

    if (0 < OutTagCount) {

        OutTagCount = FASortUniq (
                m_OutChainBuffer, 
                m_OutChainBuffer + OutTagCount
            );

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

        (*pDbgOs) << "DICT ERROR: ";

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
