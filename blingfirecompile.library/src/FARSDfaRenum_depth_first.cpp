/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfaRenum_depth_first.h"
#include "FARSDfaA.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FARSDfaRenum_depth_first::FARSDfaRenum_depth_first (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_MaxState (-1),
    m_IwsCount (-1),
    m_pIws (NULL),
    m_LastState (-1)
{
    m_old2new.SetAllocator (pAlloc);
    m_old2new.Create ();

    m_tmp_dsts.SetAllocator (pAlloc);
    m_tmp_dsts.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_iw2f.SetAllocator (pAlloc);
    m_iw2f.Create ();

    m_idx2f.SetAllocator (pAlloc);
    m_idx2f.Create ();

    m_tr.SetAllocator (pAlloc);
    m_tr.Create ();
}


const int * FARSDfaRenum_depth_first::GetOld2NewMap () const
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert ((unsigned int) (m_pDfa->GetMaxState () + 1) == m_old2new.size ());

    return m_old2new.begin ();
}


void FARSDfaRenum_depth_first::SetDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FARSDfaRenum_depth_first::Prepare ()
{
    DebugLogAssert (m_pDfa);

    const int MaxIw = m_pDfa->GetMaxIw ();
    DebugLogAssert (0 <= MaxIw);

    m_iw2f.resize (MaxIw + 1);

    for (int iw = 0; iw <= MaxIw; ++iw) {
        m_iw2f [iw] = 0;
    }

    m_MaxState = m_pDfa->GetMaxState ();
    DebugLogAssert (0 < m_MaxState);

    m_IwsCount = m_pDfa->GetIWs (&m_pIws);
    DebugLogAssert (0 < m_IwsCount && m_pIws);

    m_old2new.resize (m_MaxState + 1);

    for (int State = 0; State <= m_MaxState; ++State) {

        // setup initial mapping value
        m_old2new [State] = -1;

        for (int i = 0; i < m_IwsCount; ++i) {

            const int Iw = m_pIws [i];
            const int Dst = m_pDfa->GetDest (State, Iw);

            // update Iw --> Freq mapping
            if (-1 != Dst) {
                m_iw2f [Iw] = m_iw2f [Iw] + 1;
            }
        }
    }

    m_LastState = -1;
}


void FARSDfaRenum_depth_first::build_dsts (const int State)
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_pIws);

    int iw_idx;

    m_tmp_dsts.resize (0);

    for (iw_idx = 0; iw_idx < m_IwsCount; ++iw_idx) {

        const int Iw = m_pIws [iw_idx];
        const int DstState = m_pDfa->GetDest (State, Iw);

        if (0 <= DstState) {
            m_tmp_dsts.push_back (DstState);
        }
    }

    const int NewSize = FASortUniq (m_tmp_dsts.begin (), m_tmp_dsts.end ());
    m_tmp_dsts.resize (NewSize);

    if (1 < NewSize) {

        int i;
        const int DstCount = NewSize;
        const int * pDst = m_tmp_dsts.begin ();
        DebugLogAssert (pDst);

        m_idx2f.resize (DstCount);
        m_tr.resize (DstCount);

        for (i = 0; i < DstCount; ++i) {
            m_idx2f [i] = 0;
            m_tr [i] = i;
        }

        for (iw_idx = 0; iw_idx < m_IwsCount; ++iw_idx) {

            const int Iw = m_pIws [iw_idx];
            const int DstState = m_pDfa->GetDest (State, Iw);

            if (-1 != DstState) {

                const int Idx = FAFind_log (pDst, DstCount, DstState);
                DebugLogAssert (0 <= Idx && DstCount > Idx);

                DebugLogAssert (0 <= m_iw2f [Iw]);
                m_idx2f [Idx] += m_iw2f [Iw];
            }
        }

        int * pTrBegin = m_tr.begin ();
        int * pTrEnd = m_tr.end ();
        const int * pIdx2Freq = m_idx2f.begin ();

        std::sort (pTrBegin, pTrEnd, FAIdxCmp_s2b (pIdx2Freq));

        // m_idx2f will be reused, as we don't need it here any more
        m_idx2f.resize (DstCount);
        int * pOldDsts = m_idx2f.begin ();
        memcpy (pOldDsts, pDst, DstCount * sizeof (int));

        // make transposition
        for (i = 0; i < DstCount; ++i) {
            const int j = pTrBegin [i];
            m_tmp_dsts [i] = pOldDsts [j];
        }
    } // of if (1 < NewSize) ...
}


void FARSDfaRenum_depth_first::Process ()
{
    DebugLogAssert (m_pDfa);

    Prepare ();

    const int InitialState = m_pDfa->GetInitial ();
    m_stack.push_back (InitialState);

    while (0 < m_stack.size ()) {

        const int State = m_stack [m_stack.size () - 1];
        m_stack.pop_back ();

        if (-1 == m_old2new [State]) {

            m_LastState++;
            m_old2new [State] = m_LastState;

            build_dsts (State);

            const int DstsSize = m_tmp_dsts.size ();

            for (int i = 0; i < DstsSize; ++i) {

                const int DstState = m_tmp_dsts [i];
                m_stack.push_back (DstState);
            }
        }
    } // of while (0 < m_stack.size ()) ...
}

}
