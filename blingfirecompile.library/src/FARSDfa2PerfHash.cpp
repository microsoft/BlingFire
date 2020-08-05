/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa2PerfHash.h"
#include "FARSDfaA.h"
#include "FAState2OwsA.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSDfa2PerfHash::FARSDfa2PerfHash (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pState2Ows (NULL),
    m_FinalCount (0),
    m_pFinals (NULL),
    m_Graph (pAlloc),
    m_sorter (pAlloc)
{
    m_sorter.SetGraph (&m_Graph);

    m_state2count.SetAllocator (pAlloc);
    m_state2count.Create ();

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();
}


void FARSDfa2PerfHash::SetRsDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FARSDfa2PerfHash::SetState2Ows(FAState2OwsA * pState2Ows)
{
    m_pState2Ows = pState2Ows;
}


void FARSDfa2PerfHash::Prepare ()
{
    DebugLogAssert (m_pDfa);

    /// get final states (does not rely on FARSDfaCA::IsFinal)
    m_FinalCount = m_pDfa->GetFinals (&m_pFinals);
    DebugLogAssert (0 < m_FinalCount && m_pFinals);

    /// initialize state -> count map
    const int StateCount = m_pDfa->GetMaxState () + 1;
    DebugLogAssert (0 < StateCount);

    m_state2count.resize (StateCount);
    int * pCounts = m_state2count.begin ();
    memset (pCounts, 0, sizeof (int) * StateCount);
}


void FARSDfa2PerfHash::TopoSort ()
{
    m_Graph.SetDfa (m_pDfa);
    m_sorter.Process ();
}


inline bool FARSDfa2PerfHash::IsFinal (const int State) const
{
    DebugLogAssert (0 < m_FinalCount && m_pFinals);

    return -1 != FAFind_log (m_pFinals, m_FinalCount, State);
}


void FARSDfa2PerfHash::CalcCds ()
{
    DebugLogAssert (m_pDfa && m_pState2Ows);

    int Cd;

    // get the alphabet
    const int * pIws;
    const int IwCount = m_pDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwCount));

    // traverse states in the reverse topological order
    const int * pOrder;
    const int StateCount = m_sorter.GetTopoOrder (&pOrder);

    for (int i = 1; i <= StateCount; ++i) {

        DebugLogAssert (pOrder);
        const int State = pOrder [StateCount - i];

        m_ows.resize (0);

        if (!IsFinal (State)) {
            Cd = 0;
        } else {
            Cd = 1;
        }

        for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = m_pDfa->GetDest (State, Iw);

            // see whether transition exist
            if (-1 != DstState) {
                // add cardinality for the current transition
                m_ows.push_back (Cd);
                // get destination state cardinality
                const int DstCd = m_state2count [DstState];
                DebugLogAssert (0 < DstCd);
                // update current state cardinality
                Cd += DstCd;
            }

        } // of for (int iw_idx = 0; ...

        // update state -> cardinality map
        m_state2count [State] = Cd;

        const int OwsCount = m_ows.size ();

        if (0 < OwsCount) {
            const int * pOws = m_ows.begin ();
            m_pState2Ows->SetOws (State, pOws, OwsCount);
        }
    } // of for (int i = 1; ...
}


void FARSDfa2PerfHash::Process ()
{
    DebugLogAssert (m_pDfa && m_pState2Ows);

    Prepare ();
    TopoSort ();
    CalcCds ();
}

}
