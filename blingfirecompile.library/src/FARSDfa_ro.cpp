/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa_ro.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FARSDfa_ro::FARSDfa_ro (FAAllocatorA * pAlloc) :
    m_Initial (-1),
    m_MaxState (-1),
    m_MaxIw (-1),
    m_TmpState (-1)
{
    m_State2Sets.SetAllocator (pAlloc);
    m_State2Sets.Create ();

    m_Sets.SetAllocator (pAlloc);
    m_Sets.SetCopyChains (true);

    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();

    m_tmp_iws.SetAllocator (pAlloc);
    m_tmp_iws.Create ();

    m_tmp_dsts.SetAllocator (pAlloc);
    m_tmp_dsts.Create ();
}


FARSDfa_ro::~FARSDfa_ro ()
{}


/// optional -- will be auto-calculated
void FARSDfa_ro::SetMaxState (const int MaxState)
{
    if (MaxState > m_MaxState)
        m_MaxState = MaxState;
}


/// optional -- will be auto-calculated
void FARSDfa_ro::SetMaxIw (const int MaxIw)
{
    if (MaxIw > m_MaxIw)
        m_MaxIw = MaxIw;
}


void FARSDfa_ro::Create ()
{}


void FARSDfa_ro::SetInitial (const int State)
{
    DebugLogAssert (0 <= State);
    m_Initial = State;
}


void FARSDfa_ro::SetFinals (const int * pStates, const int Count)
{
    DebugLogAssert (0 < Count && pStates);

    m_finals.resize (Count);
    memcpy (m_finals.begin (), pStates, Count * sizeof (int));
}


// auto-calculated
void FARSDfa_ro::SetIWs (const int *, const int)
{}


void FARSDfa_ro::
    SetTransition (
            const int State, 
            const int * pIws, 
            const int * pDsts, 
            const int Count
        )
{
    DebugLogAssert (m_tmp_iws.size () == m_tmp_dsts.size ());
    DebugLogAssert (0 < Count && pDsts && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, Count));
    DebugLogAssert (0 == m_State2Sets.size () % 2);
    DebugLogAssert (0 <= State);

    /// update Max State/Iw

    const int MaxIw = pIws [Count - 1];
    if (MaxIw > m_MaxIw) {
        m_MaxIw = MaxIw;
    }
    if (State > m_MaxState) {
        m_MaxState = State;
    }
    for (int j = 0; j < Count; ++j) {
        const int Dst = pDsts [j];
        if (Dst > m_MaxState)
            m_MaxState = Dst;
    }

    /// add all State's arcs

    const int Idx1 = m_Sets.Add (pIws, Count, 0);
    const int Idx2 = m_Sets.Add (pDsts, Count, 0);

    const int OldSize = m_State2Sets.size ();
    const int I = State << 1;

    if (OldSize <= I) {
        m_State2Sets.resize (I + 2, OldSize);
        const int GapSize = int (I - OldSize);
        if (0 < GapSize) {
            int * pOut = m_State2Sets.begin ();
            memset (pOut + OldSize, -1, GapSize * sizeof (int));
        }
    }

    m_State2Sets [I] = Idx1;
    m_State2Sets [I + 1] = Idx2;

    /// update alphabet

    const int OldIwsSize = m_iws.size ();

    for (int i = 0; i < Count; ++i) {
        const int Iw = pIws [i];
        if (-1 == FAFind_log (m_iws.begin (), OldIwsSize, Iw)) {
            m_iws.push_back (Iw);
        }
    }

    const int NewIwsSize = m_iws.size ();

    if (NewIwsSize != OldIwsSize) {
        std::sort (m_iws.begin (), m_iws.end ());
    }
}


void FARSDfa_ro::SetTransition (const int State, const int Iw, const int Dst)
{
    DebugLogAssert (m_tmp_iws.size () == m_tmp_dsts.size ());
    DebugLogAssert (0 <= State);

    if (State != m_TmpState) {

        if (-1 != m_TmpState) {

            const int * pIws = m_tmp_iws.begin ();
            const int * pDsts = m_tmp_dsts.begin ();
            const int Count = m_tmp_dsts.size ();
            DebugLogAssert (FAIsSortUniqed (pIws, Count));

            FARSDfa_ro::SetTransition (m_TmpState, pIws, pDsts, Count);
        }

        m_TmpState = State;
        m_tmp_iws.resize (0);
        m_tmp_dsts.resize (0);
    }

    m_tmp_iws.push_back (Iw);
    m_tmp_dsts.push_back (Dst);
}


void FARSDfa_ro::Prepare ()
{
    if (-1 != m_TmpState) {

        const int * pIws = m_tmp_iws.begin ();
        const int * pDsts = m_tmp_dsts.begin ();
        const int Count = m_tmp_dsts.size ();
        DebugLogAssert (0 < Count && FAIsSortUniqed (pIws, Count));

        FARSDfa_ro::SetTransition (m_TmpState, pIws, pDsts, Count);

        m_TmpState = -1;
        m_tmp_iws.Clear ();
        m_tmp_iws.Create ();
        m_tmp_dsts.Clear ();
        m_tmp_dsts.Create ();
    }

    int * pFinals = m_finals.begin ();
    const int OldCount = m_finals.size ();

    if (!FAIsSortUniqed (pFinals, OldCount)) {
        const int NewCount = FASortUniq (pFinals, pFinals + OldCount);
        m_finals.resize (NewCount);
    }
}


void FARSDfa_ro::Clear ()
{
    m_Initial = -1;
    m_MaxState = -1;
    m_MaxIw = -1;
    m_TmpState = -1;

    m_finals.Clear ();
    m_finals.Create ();
    m_iws.Clear ();
    m_iws.Create ();
    m_tmp_iws.Clear ();
    m_tmp_iws.Create ();
    m_tmp_dsts.Clear ();
    m_tmp_dsts.Create ();
    m_State2Sets.Clear ();
    m_State2Sets.Create ();
    m_Sets.Clear ();
}


const int FARSDfa_ro::GetInitial () const
{
    return m_Initial;
}


const bool FARSDfa_ro::IsFinal (const int State) const
{
    DebugLogAssert (FAIsSortUniqed (m_finals.begin (), m_finals.size ()));
    return -1 != FAFind_log (m_finals.begin (), m_finals.size (), State);
}


const int FARSDfa_ro::GetIWs (const int ** ppIws) const
{
    DebugLogAssert (ppIws);

    *ppIws = m_iws.begin ();
    const int IwsCount = m_iws.size ();

    return IwsCount;
}


const int FARSDfa_ro::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    const int * pIws2;
    const int IwCount = GetIWs (&pIws2);

    if (0 < IwCount && IwCount <= MaxIwCount) {
        memcpy (pIws, pIws2, sizeof (int) * IwCount);
    }

    return IwCount;
}


const int FARSDfa_ro::GetDest (const int State, const int Iw) const
{
    DebugLogAssert (0 <= State);

    if (FAFsmConst::DFA_DEAD_STATE == State) {
        return -1;
    }

    const int I = State << 1;

    if ((unsigned int) I < m_State2Sets.size ()) {

        const int Idx1 = m_State2Sets [I];
        const int Idx2 = m_State2Sets [I + 1];

        if (-1 == Idx1) {
            DebugLogAssert (-1 == Idx2);
            return -1;
        }

        const int * pIws;
        const int Count = m_Sets.GetChain(Idx1, &pIws);
        DebugLogAssert (0 < Count && FAIsSortUniqed (pIws, Count));

        const int DstIdx = FAFind_log (pIws, Count, Iw);

        if (-1 != DstIdx) {

            DebugLogAssert (DstIdx < Count);

            const int * pDsts;
#ifndef NDEBUG
            const int Count2 = 
#endif
                m_Sets.GetChain (Idx2, &pDsts);
            DebugLogAssert (Count2 == Count && pDsts);

            return pDsts [DstIdx];
        }
    }

    return -1;
}


const int FARSDfa_ro::GetMaxState () const
{
    return m_MaxState;
}


const int FARSDfa_ro::GetMaxIw () const
{
    return m_MaxIw;
}


const int FARSDfa_ro::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (ppStates);

    *ppStates = m_finals.begin ();
    const int Count = m_finals.size ();

    return Count;
}


}
