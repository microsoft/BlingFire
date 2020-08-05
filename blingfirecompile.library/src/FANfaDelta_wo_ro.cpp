/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaDelta_wo_ro.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FANfaDelta_wo_ro::_TTrCmp::_TTrCmp (const int * pTrs, const int Count) :
    m_pTrs (pTrs),
    m_Count (Count)
{}


const bool FANfaDelta_wo_ro::_TTrCmp::
    operator () (const int i1, const int i2) const
{
    DebugLogAssert (0 <= i1 && m_Count > i1);
    DebugLogAssert (0 <= i2 && m_Count > i2);

    const int * pTr1 = m_pTrs + (i1 * 3);
    const int * pTr2 = m_pTrs + (i2 * 3);

    if (pTr1 [0] < pTr2 [0]) {

        return true;

    } else if (pTr1 [0] == pTr2 [0]) {

        if (pTr1 [1] < pTr2 [1]) {
            return true;
        } else if (pTr1 [1] == pTr2 [1] && pTr1 [2] < pTr2 [2]) {
            return true;
        } else {
            return false;
        }

    } else {

        return false;
    }
}


FANfaDelta_wo_ro::FANfaDelta_wo_ro (FAAllocatorA * pAlloc) :
    m_delta_const (pAlloc),
    m_MaxIw (-1),
    m_MaxState (-1),
    m_IsWo (true)
{
    m_tmp_tr_list.SetAllocator (pAlloc);
    m_tmp_tr_list.Create ();

    m_tmp_tr_idx.SetAllocator (pAlloc);
    m_tmp_tr_idx.Create ();

    m_tmp_dst.SetAllocator (pAlloc);
    m_tmp_dst.Create ();
}


void FANfaDelta_wo_ro::
    AddTransition (const int State, const int Iw, const int Dst)
{
    DebugLogAssert (m_IsWo);
    DebugLogAssert (0 <= State && 0 <= Iw);
    DebugLogAssert (0 <= Dst || FAFsmConst::NFA_DEAD_STATE == Dst);

    const int OldSize = m_tmp_tr_list.size ();
    DebugLogAssert (0 == OldSize % 3);

    m_tmp_tr_list.resize (OldSize + 3, OldSize); // logN rellocations
    int * pOut = m_tmp_tr_list.begin () + OldSize;

    *pOut++ = State;
    *pOut++ = Iw;
    *pOut = Dst;
}


void FANfaDelta_wo_ro::
    AddTransition (
        const int State, 
        const int Iw, 
        const int * pDsts, 
        const int Count
    )
{
    DebugLogAssert (0 <= Count);
    DebugLogAssert (0 <= State && 0 <= Iw);
    DebugLogAssert (m_IsWo);

    if (0 < Count) {

        const int OldSize = m_tmp_tr_list.size ();
        DebugLogAssert (0 == OldSize % 3);

        m_tmp_tr_list.resize (OldSize + (3 * Count), OldSize);
        int * pOut = m_tmp_tr_list.begin () + OldSize;

        for (int i = 0; i < Count; ++i) {

            const int Dst = pDsts [i];
            DebugLogAssert (0 <= Dst);

            *pOut++ = State;
            *pOut++ = Iw;
            *pOut++ = Dst;
        }

    } else {

        DebugLogAssert (0 == Count);

        FANfaDelta_wo_ro::AddTransition (State, Iw, FAFsmConst::NFA_DEAD_STATE);

    } // of if (0 < Count) ...
}


void FANfaDelta_wo_ro::Prepare ()
{
    int i;

    if (false == m_IsWo) {
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

    int PrevFrom = -1;
    int PrevIw = -1;

    for (i = 0; i < TrCount; ++i) {

        const int Idx = pIdx [i];

        const int j = 3 * Idx;
        const int State = pTrs [j];
        const int Iw = pTrs [j + 1];
        const int Dst = pTrs [j + 2];

        const int MaxState = State > Dst ? State : Dst;

        if (m_MaxState < MaxState) {
            m_MaxState = MaxState;
        }
        if (m_MaxIw < Iw) {
            m_MaxIw = Iw;
        }
        if (State != PrevFrom || Iw != PrevIw) {

            if (-1 != PrevFrom) {
                DebugLogAssert (-1 != PrevIw);
                int * pDsts = m_tmp_dst.begin ();
                int * pEnd = m_tmp_dst.end ();
                const int NewCount = int (std::unique (pDsts, pEnd) - pDsts);
                m_delta_const.AddTransition (PrevFrom, PrevIw, pDsts, NewCount);
            }
            m_tmp_dst.resize (0);
            PrevFrom = State;
            PrevIw = Iw;
        }
        m_tmp_dst.push_back (Dst);
    }

    if (-1 != PrevFrom) {
        DebugLogAssert (-1 != PrevIw);
        int * pDsts = m_tmp_dst.begin ();
        int * pEnd = m_tmp_dst.end ();
        const int NewCount = int (std::unique (pDsts, pEnd) - pDsts);
        m_delta_const.AddTransition (PrevFrom, PrevIw, pDsts, NewCount);
    }

    m_tmp_tr_list.Clear ();
    m_tmp_tr_idx.Clear ();
    m_tmp_dst.Clear ();

    m_IsWo = false;
}


void FANfaDelta_wo_ro::Clear ()
{
    if (false == m_IsWo) {
        m_tmp_tr_list.Create ();
        m_tmp_tr_idx.Create ();
        m_tmp_dst.Create ();
    } else {
        m_tmp_tr_list.resize (0);
        m_tmp_tr_idx.resize (0);
        m_tmp_dst.resize (0);
    }

    m_delta_const.Clear ();

    m_MaxIw = -1;
    m_MaxState = -1;

    m_IsWo = true;
}


const int FANfaDelta_wo_ro::GetMaxState () const
{
    DebugLogAssert (!m_IsWo);
    return m_MaxState;
}


const int FANfaDelta_wo_ro::GetMaxIw () const
{
    DebugLogAssert (!m_IsWo);
    return m_MaxIw;
}


const int FANfaDelta_wo_ro::
    GetIWs (const int State,  const int ** ppIws) const
{
    DebugLogAssert (!m_IsWo);
    return m_delta_const.GetIWs (State,  ppIws);
}


const int FANfaDelta_wo_ro::
    GetDest (const int State, const int Iw, const int ** ppDsts) const
{
    DebugLogAssert (!m_IsWo);
    return m_delta_const.GetDest (State, Iw, ppDsts);
}

}

