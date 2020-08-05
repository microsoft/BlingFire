/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAChains2MinDfa.h"

#include <algorithm>

namespace BlingFire
{


FAChains2MinDfa::_TChainCmp::_TChainCmp (FAArray_t < int > * pD) : 
    m_pD (pD)
{}


const bool FAChains2MinDfa::_TChainCmp::
    operator () (const int B1, const int B2) const
{
    DebugLogAssert (m_pD);

    if (B1 == B2) {
        return false;
    }

    DebugLogAssert (0 <= B1 && m_pD->size () > (unsigned int) B1);
    DebugLogAssert (0 <= B2 && m_pD->size () > (unsigned int) B2);

    const int Size1 = (*m_pD) [B1];
    const int Size2 = (*m_pD) [B2];

    int MinSize = Size1;
    if (Size2 < MinSize)
        MinSize = Size2;

    MinSize++; // skips sizes, see header for description
    for (int i = 1; i < MinSize; ++i) {

        const int D1 = (*m_pD) [B1 + i];
        const int D2 = (*m_pD) [B2 + i];

        if (D1 < D2) {
            return true;
        } else if (D1 > D2) {
            return false;
        }
    }

    // the shortest is smaller, if they are equal then -- false
    if (Size1 < Size2) {
        return true;
    } else {
        return false;
    }
}


FAChains2MinDfa::FAChains2MinDfa (FAAllocatorA * pAlloc) :
    m_chains2dfa (pAlloc)
{
    m_data.SetAllocator (pAlloc);
    m_data.Create ();

    m_b.SetAllocator (pAlloc);
    m_b.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();
}


void FAChains2MinDfa::AddChain (const int * pChain, const int Size)
{
    if (0 < Size && pChain) {

        m_b.push_back (m_data.size ());

        m_data.push_back (Size);

        for (int i = 0; i < Size; ++i) {
            const int D = pChain [i];
            m_data.push_back (D);
        }
    }
}


void FAChains2MinDfa::Process ()
{
    // destroy prev. automaton
    m_chains2dfa.Clear ();

    // sort chains
    std::sort (m_b.begin (), m_b.end (), _TChainCmp (&m_data));

    // build Min DFA
    const int Count = m_b.size ();

    for (int i = 0; i < Count; ++i) {

        const int B = m_b [i];
        DebugLogAssert (0 <= B && m_data.size () > (unsigned int) B);

        const int Size = m_data [B];

        m_tmp.resize (Size);
        int * pTmp = m_tmp.begin ();

        for (int j = 0; j < Size; ++j) {
            pTmp [j] = m_data [B + j + 1];
        }

        m_chains2dfa.AddChain (pTmp, Size);
    }

    // make the interface ready
    m_chains2dfa.Prepare ();

    // remove chains data
    m_data.Clear ();
    m_data.Create ();
    m_b.Clear ();
    m_b.Create ();
    m_tmp.Clear ();
    m_tmp.Create ();
}


const FARSDfaA * FAChains2MinDfa::GetRsDfa () const
{
    return & m_chains2dfa;
}

}
