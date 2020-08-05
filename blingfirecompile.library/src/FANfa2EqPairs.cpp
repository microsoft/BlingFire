/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfa2EqPairs.h"
#include "FAUtils.h"

namespace BlingFire
{


FANfa2EqPairs::FANfa2EqPairs (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_nfa2dfa (pAlloc)
{
    m_pairs.SetAllocator (pAlloc);
    m_pairs.SetCopyChains (true);

    m_nfa2dfa.SetDFA (&m_dfa);
}


FANfa2EqPairs::~FANfa2EqPairs ()
{
}


void FANfa2EqPairs::Clear ()
{
    m_pairs.Clear ();
    m_nfa2dfa.Clear ();
}


void FANfa2EqPairs::SetNfa (const FARSNfaA * pNfa)
{
    m_pNfa = pNfa;

    m_nfa2dfa.SetNew2Old (this);
    m_nfa2dfa.SetNFA (pNfa);
}


void FANfa2EqPairs::Process ()
{
    DebugLogAssert (m_pNfa);

    m_pairs.Clear ();

    m_nfa2dfa.Process ();
}


const int FANfa2EqPairs::GetPairCount () const
{
    const int Count = m_pairs.GetChainCount ();
    return Count;
}


void FANfa2EqPairs::GetPair (const int Num, int * pQ1, int * pQ2) const
{
    DebugLogAssert (pQ1 && pQ2);

    const int * pPair;
#ifndef NDEBUG
    const int Size = 
#endif
        m_pairs.GetChain (Num, &pPair);
    DebugLogAssert (2 == Size);
    DebugLogAssert (pPair [0] != pPair [1]);

    *pQ1 = pPair [0];
    *pQ2 = pPair [1];
}


void FANfa2EqPairs::
    SetOws (const int, const int * pOldStates, const int Count)
{
    int P [2];

    if (1 < Count) {

        DebugLogAssert (pOldStates);
        DebugLogAssert (FAIsSortUniqed (pOldStates, Count));

        for (int i = 0; i < Count; ++i) {

            P [0] = pOldStates [i];

            for (int j = i + 1; j < Count; ++j) {

                P [1] = pOldStates [j];
                // be sure it is a set
                DebugLogAssert (P [0] < P [1]);

                const int Num = m_pairs.GetIdx (&(P [0]), 2);
                if (-1 == Num) {
                    m_pairs.Add (&(P [0]), 2, 0);
                }

            } // of for (int j = 0; ...
        } // of for (int i = 0; ...
    } // of if (1 < Count) ...
}


/// stubs

const int FANfa2EqPairs::GetOws (const int, int *, const int) const
{
    DebugLogAssert (0);
    return -1;
}

const int FANfa2EqPairs::GetOws (const int, const int **) const
{
    DebugLogAssert (0);
    return -1;
}

const int FANfa2EqPairs::GetMaxOwsCount () const
{
    DebugLogAssert (0);
    return -1;
}

void FANfa2EqPairs::TDfaStub::
    SetTransition (const int, const int *, const int *, const int)
{}

void FANfa2EqPairs::TDfaStub::
    SetTransition (const int, const int, const int)
{}

void FANfa2EqPairs::TDfaStub::
    SetInitial (const int)
{}

void FANfa2EqPairs::TDfaStub::
    SetFinals (const int * , const int)
{}

void FANfa2EqPairs::TDfaStub::
    Prepare ()
{}

}
