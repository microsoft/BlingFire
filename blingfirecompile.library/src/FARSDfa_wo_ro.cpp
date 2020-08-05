/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa_wo_ro.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FARSDfa_wo_ro::_TTrCmp::_TTrCmp (const int * pTrs, const int Count) :
    m_pTrs (pTrs),
    m_Count (Count)
{}


const bool FARSDfa_wo_ro::_TTrCmp::
    operator () (const int i1, const int i2) const
{
    DebugLogAssert (0 <= i1 && m_Count > i1);
    DebugLogAssert (0 <= i2 && m_Count > i2);

    const int * pTr1 = m_pTrs + (i1 * 3);
    const int * pTr2 = m_pTrs + (i2 * 3);

    if (pTr1 [0] < pTr2 [0]) {

        return true;

    } else if (pTr1 [0] == pTr2 [0]) {

        // duplicates are normal
        if (pTr1 [1] == pTr2 [1] && pTr1 [2] == pTr2 [2]) {
            return false;
        }

        // non-deterministic transitions are not allowed
        DebugLogAssert (pTr1 [1] != pTr2 [1]);

        if (pTr1 [1] < pTr2 [1]) {
            return true;
        } else {
            return false;
        }

    } else {

        return false;
    }
}


FARSDfa_wo_ro::FARSDfa_wo_ro (FAAllocatorA * pAlloc) :
    m_ro_dfa (pAlloc),
    m_IsWo (true)
{
    m_tmp_tr_list.SetAllocator (pAlloc);
    m_tmp_tr_list.Create ();

    m_tmp_tr_idx.SetAllocator (pAlloc);
    m_tmp_tr_idx.Create ();
}


FARSDfa_wo_ro::~FARSDfa_wo_ro ()
{}


/// optional -- will be auto-calculated
void FARSDfa_wo_ro::SetMaxState (const int MaxState)
{
    m_ro_dfa.SetMaxState (MaxState);
}


/// optional -- will be auto-calculated
void FARSDfa_wo_ro::SetMaxIw (const int MaxIw)
{
    m_ro_dfa.SetMaxIw (MaxIw);
}


void FARSDfa_wo_ro::Create ()
{}


void FARSDfa_wo_ro::SetIWs (const int *, const int)
{}


void FARSDfa_wo_ro::SetInitial (const int State)
{
    m_ro_dfa.SetInitial (State);
}


void FARSDfa_wo_ro::SetFinals (const int * pStates, const int Count)
{
    m_ro_dfa.SetFinals (pStates, Count);
}


void FARSDfa_wo_ro::
    SetTransition (
            const int State, 
            const int * pIws, 
            const int * pDsts, 
            const int Count
        )
{
    DebugLogAssert (m_IsWo);
    DebugLogAssert (0 <= State);

    if (0 < Count) {

        DebugLogAssert (pIws && pDsts);

        const int OldSize = m_tmp_tr_list.size ();
        DebugLogAssert (0 == OldSize % 3);

        m_tmp_tr_list.resize (OldSize + (3 * Count), OldSize);
        int * pOut = m_tmp_tr_list.begin () + OldSize;

        for (int i = 0; i < Count; ++i) {

            const int Iw = pIws [i];
            const int Dst = pDsts [i];

            DebugLogAssert (0 <= Iw);
            DebugLogAssert (0 <= Dst || FAFsmConst::DFA_DEAD_STATE == Dst);

            *pOut++ = State;
            *pOut++ = Iw;
            *pOut++ = Dst;
        }
    } // of if (0 < Count) ...
}


void FARSDfa_wo_ro::
    SetTransition (const int State, const int Iw, const int Dst)
{
    DebugLogAssert (m_IsWo);
    DebugLogAssert (0 <= State && 0 <= Iw);
    DebugLogAssert (0 <= Dst || FAFsmConst::DFA_DEAD_STATE == Dst);

    const int OldSize = m_tmp_tr_list.size ();
    DebugLogAssert (0 == OldSize % 3);

    m_tmp_tr_list.resize (OldSize + 3, OldSize); // logN rellocations

    int * pOut = m_tmp_tr_list.begin () + OldSize;

    *pOut++ = State;
    *pOut++ = Iw;
    *pOut = Dst;
}


void FARSDfa_wo_ro::Prepare ()
{
    int i;

    if (!m_IsWo) {
        return;
    }

    const int TrCount = m_tmp_tr_list.size () / 3;
    m_tmp_tr_idx.resize (TrCount);

    int * pIdx = m_tmp_tr_idx.begin ();
    DebugLogAssert (pIdx);
    for (i = 0; i < TrCount; ++i) {
        pIdx [i] = i;
    }

    const int * pTrs = m_tmp_tr_list.begin ();
    std::sort (pIdx, pIdx + TrCount, _TTrCmp (pTrs, TrCount));

    int PrevState = -1;
    int PrevIw = -1;
    int PrevDst = -1;

    for (i = 0; i < TrCount; ++i) {

        const int Idx = pIdx [i];
        const int j = 3 * Idx;

        const int State = pTrs [j];
        const int Iw = pTrs [j + 1];
        const int Dst = pTrs [j + 2];

        if (PrevState != State || PrevIw != Iw || PrevDst != Dst) {

            m_ro_dfa.SetTransition (State, Iw, Dst);

            PrevState = State;
            PrevIw = Iw;
            PrevDst = Dst;
        }
    }

    m_tmp_tr_list.Clear ();
    m_tmp_tr_idx.Clear ();
    m_ro_dfa.Prepare ();

    m_IsWo = false;
}


void FARSDfa_wo_ro::Clear ()
{
    if (false == m_IsWo) {
        m_tmp_tr_list.Create ();
        m_tmp_tr_idx.Create ();
    } else {
        m_tmp_tr_list.resize (0);
        m_tmp_tr_idx.resize (0);
    }
    m_ro_dfa.Clear ();

    m_IsWo = true;
}


const int FARSDfa_wo_ro::GetInitial () const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetInitial ();
}


const bool FARSDfa_wo_ro::IsFinal (const int State) const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.IsFinal (State);
}


const int FARSDfa_wo_ro::GetIWs (const int ** ppIws) const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetIWs (ppIws);
}


const int FARSDfa_wo_ro::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    DebugLogAssert (!m_IsWo);

    const int * pIws2;
    const int IwCount = m_ro_dfa.GetIWs (&pIws2);

    if (0 < IwCount && IwCount <= MaxIwCount) {
        memcpy (pIws, pIws2, sizeof (int) * IwCount);
    }

    return IwCount;
}


const int FARSDfa_wo_ro::GetDest (const int State, const int Iw) const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetDest (State, Iw);
}


const int FARSDfa_wo_ro::GetMaxState () const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetMaxState ();
}


const int FARSDfa_wo_ro::GetMaxIw () const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetMaxIw ();
}


const int FARSDfa_wo_ro::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (!m_IsWo);
    return m_ro_dfa.GetFinals (ppStates);
}

}

