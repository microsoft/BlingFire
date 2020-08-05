/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestMorph.h"

namespace BlingFire
{


FATestMorph::FATestMorph (FAAllocatorA * pAlloc) :
    m_pOs (NULL),
    m_pPRM (NULL),
    m_IgnoreCase (false),
    m_Count (0),
    m_CorrectCount (0),
    m_MatchedEventCount (0),
    m_EventCount (0),
    m_ExpectedEventCount (0),
    m_NoEventCount (0),
    m_MaxEventCount (0),
    m_ExpectedMaxEventCount (0),
    m_MinEventCount (INT_MAX),
    m_ExpectedMinEventCount (INT_MAX),
    m_ChainBuffer (NULL),
    m_OutChainBuffer (NULL),
    m_OutChain2Buffer (NULL),
    m_set_utils (pAlloc)
{
    m_in_chain.SetAllocator (pAlloc);
    m_in_chain.Create ();
    m_in_chain.resize (MaxChainSize);

    m_out_chain.SetAllocator (pAlloc);
    m_out_chain.Create ();
    m_out_chain.resize (MaxChainSize);

    m_out2_chain.SetAllocator (pAlloc);
    m_out2_chain.Create ();
    m_out2_chain.resize (MaxChainSize);

    m_ChainBuffer = m_in_chain.begin ();
    m_OutChainBuffer = m_out_chain.begin ();
    m_OutChain2Buffer = m_out2_chain.begin ();
}


FATestMorph::~FATestMorph ()
{}


void FATestMorph::SetOutStream (std::ostream * pOs)
{
    m_pOs = pOs;
}


std::ostream * FATestMorph::GetOutStream () const
{
    return m_pOs;
}


void FATestMorph::Clear ()
{
    m_Count = 0;
    m_CorrectCount = 0;
    m_MatchedEventCount = 0;
    m_EventCount = 0;
    m_ExpectedEventCount = 0;
    m_NoEventCount = 0;
    m_MaxEventCount = 0;
    m_ExpectedMaxEventCount = 0;
    m_MinEventCount = INT_MAX;
    m_ExpectedMinEventCount = INT_MAX;
}


void FATestMorph::SetPRM (FAPrmInterpreter_t < int > * pPRM)
{
    m_pPRM = pPRM;
}


void FATestMorph::SetIgnoreCase (const bool IgnoreCase)
{
    m_IgnoreCase = IgnoreCase;
}


const int FATestMorph::UpdateCounts (
        const int * pA, 
        const int ASize,
        const int * pB, 
        const int BSize
    )
{
    DebugLogAssert (pA && 0 <= ASize);
    DebugLogAssert (pB && 0 <= BSize);

    int ErrType = ERR_NO_ERROR;

    m_Count++;

    if (0 == ASize) {
        m_NoEventCount++;
    }

    if (0 < BSize || 0 < ASize) {

        /// Res0 := A - B 
        /// Res1 := B - A 
        m_set_utils.Difference (pA, ASize, 0, pB, BSize, 1);

        /// Res2 := A & B
        m_set_utils.Intersect (pA, ASize, pB, BSize, 2);

        const int * pA_B;
        const int A_B_Size = m_set_utils.GetRes (&pA_B, 0);
        DebugLogAssert (0 <= A_B_Size);

        const int * pB_A;
        const int B_A_Size = m_set_utils.GetRes (&pB_A, 1);
        DebugLogAssert (0 <= B_A_Size);

        const int * pAB;
        const int ABSize = m_set_utils.GetRes (&pAB, 2);
        DebugLogAssert (0 <= ABSize);

        /// update counts
        m_MatchedEventCount += ABSize;
        m_EventCount += ASize;
        m_ExpectedEventCount += BSize;

        /// update error information
        if (0 < A_B_Size && 0 == B_A_Size) {
            ErrType = ERR_BIGGER;
        } else if (0 == A_B_Size && 0 < B_A_Size) {
            ErrType = ERR_SMALLER;
        } else if (0 < A_B_Size && 0 < B_A_Size) {
            ErrType = ERR_DIFFERENT;
        }
    }

    if (ERR_NO_ERROR == ErrType) {
        m_CorrectCount++;
    }

    if (ASize > m_MaxEventCount) {
        m_MaxEventCount = ASize;
    }
    if (ASize < m_MinEventCount) {
        m_MinEventCount = ASize;
    }

    if (BSize > m_ExpectedMaxEventCount) {
        m_ExpectedMaxEventCount = BSize;
    }
    if (BSize < m_ExpectedMinEventCount) {
        m_ExpectedMinEventCount = BSize;
    }

    return ErrType;
}


const int FATestMorph::FindDelim (const int * pChain, const int ChainSize)
{
    DebugLogAssert (0 < ChainSize && pChain);

    for (int i = 0; i < ChainSize; ++i) {
        if ((unsigned char)'\t' == pChain [i]) {
            return i;
        }
    }
    return -1;
}


const int FATestMorph::WordCount (const int * pWordList, const int Size)
{
    DebugLogAssert (0 < Size && pWordList);

    int Count = 0;

    for (int i = 0; i < Size; ++i) {
        if (0 == pWordList [i]) {
            Count++;
        }
    }

    return Count;
}


void FATestMorph::PrintReport (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (0 < m_Count && 0 < m_EventCount && 0 < m_ExpectedEventCount) {
 
       (*pOs) \
            << "  Input count: " 
            << m_Count
            << '\n'
            << "  Correct outputs: "
            << 100 * (float (m_CorrectCount)/float (m_Count))
            << " %\n"
            << "  Precision: " 
            << 100 * (float (m_MatchedEventCount)/float (m_EventCount))
            << " %\n"
            << "  Recall: " 
            << 100 * (float(m_MatchedEventCount)/float(m_ExpectedEventCount))
            << " %\n"
            << "  Correct: " 
            << m_MatchedEventCount
            << '\n'
            << "  Incorrect: " 
            << (m_EventCount - m_MatchedEventCount)
            << '\n'
            << "  Empty: " 
            << m_NoEventCount
            << '\n'
            << "  Total: " 
            << m_EventCount
            << '\n'
            << "  Expected Total: " 
            << m_ExpectedEventCount
            << '\n'
            << "  Output / Input count: " 
            << float (m_EventCount)/float (m_Count)
            << '\n'
            << "  Expected Output / Input count: " 
            << float (m_ExpectedEventCount)/float (m_Count)
            << '\n'
            << "  Min Output: " 
            << m_MinEventCount
            << '\n'
            << "  Max Output: " 
            << m_MaxEventCount
            << '\n'
            << "  Expected Min Output: " 
            << m_ExpectedMinEventCount
            << '\n'
            << "  Expected Max Output: " 
            << m_ExpectedMaxEventCount
            << '\n' ;

    } else {

        (*pOs) \
            << "  Input count: " 
            << m_Count
            << '\n'
            << "  Total: " 
            << m_EventCount
            << '\n'
            << "  Expected Total: " 
            << m_ExpectedEventCount
            << '\n' ;
    }
}

}
