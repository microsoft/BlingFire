/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSNfa_wo_ro.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSNfa_wo_ro::FARSNfa_wo_ro (FAAllocatorA * pAlloc) :
    m_delta (pAlloc)
{
    m_initials.SetAllocator (pAlloc);
    m_initials.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();
}


FARSNfa_wo_ro::~FARSNfa_wo_ro ()
{}


const int FARSNfa_wo_ro::GetMaxState () const
{
    return m_delta.GetMaxState ();
}


const int FARSNfa_wo_ro::GetMaxIw () const
{
    return m_delta.GetMaxIw ();
}


const int FARSNfa_wo_ro::GetInitials (const int ** ppStates) const
{
    DebugLogAssert (ppStates);

    *ppStates = m_initials.begin ();
    const int Count = m_initials.size ();

    return Count;
}


const int FARSNfa_wo_ro::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (ppStates);

    *ppStates = m_finals.begin ();
    const int Count = m_finals.size ();

    return Count;
}


const bool FARSNfa_wo_ro::IsFinal (const int State) const
{
    const int * pFinals = m_finals.begin ();
    const int Count = m_finals.size ();

    return -1 != FAFind_log (pFinals, Count, State);
}


const bool FARSNfa_wo_ro::IsFinal (const int * pStates, const int Count) const
{
    const int * pFinals = m_finals.begin ();
    const int FinalCount = m_finals.size ();

    for (int i = 0; i < Count; ++i) {

        const int State = pStates [i];
        if (-1 != FAFind_log (pFinals, FinalCount, State)) {
            return true;
        }
    }

    return false;
}


const int FARSNfa_wo_ro::GetIWs (const int State, const int ** ppIws) const
{
    const int Count = m_delta.GetIWs (State, ppIws);
    DebugLogAssert (0 > Count || FAIsSortUniqed (*ppIws, Count));
    return Count;
}


const int FARSNfa_wo_ro::
    GetDest (
            const int State,
            const int Iw,
            const int ** ppIwDstStates
        ) const
{
    const int Count = m_delta.GetDest (State, Iw, ppIwDstStates);
    DebugLogAssert (0 > Count || FAIsSortUniqed (*ppIwDstStates, Count));
    return Count;
}


const int FARSNfa_wo_ro::
    GetDest (
            const int State,
            const int Iw,
            int * pDstStates,
            const int MaxCount
        ) const
{
    const int * pDsts;
    const int Count = m_delta.GetDest (State, Iw, &pDsts);

    if (0 < Count && MaxCount >= Count) {
        memcpy (pDstStates, pDsts, Count * sizeof (int));
    }

    return Count;
}


void FARSNfa_wo_ro::SetMaxState (const int)
{}


void FARSNfa_wo_ro::SetMaxIw (const int)
{}


void FARSNfa_wo_ro::Create ()
{}


void FARSNfa_wo_ro::
    SetTransition (
            const int State,
            const int Iw,
            const int * pDstStates,
            const int Count
        )
{
    if (-1 == Count) {
        return;
    }

    DebugLogAssert (0 < Count && FAIsSortUniqed (pDstStates, Count));

    // do not add DeadState into the destination states
    if (FAFsmConst::NFA_DEAD_STATE != pDstStates [0]) {

        m_delta.AddTransition (State, Iw, pDstStates, Count);

    } else {

        m_delta.AddTransition (State, Iw, pDstStates + 1, Count - 1);
    }
}


void FARSNfa_wo_ro::
    SetTransition (const int State, const int Iw, const int DstState)
{
    m_delta.AddTransition (State, Iw, DstState);
}


void FARSNfa_wo_ro::SetInitials (const int * pStates, const int Count)
{
    DebugLogAssert (0 <= Count && FAIsSortUniqed (pStates, Count));

    m_initials.resize (Count);

    if (0 < Count) {
        DebugLogAssert (pStates);
        memcpy (m_initials.begin (), pStates, Count * sizeof (int));
    }
}


void FARSNfa_wo_ro::SetFinals (const int * pStates, const int Count)
{
    DebugLogAssert (0 <= Count && FAIsSortUniqed (pStates, Count));

    m_finals.resize (Count);

    if (0 < Count) {
        DebugLogAssert (pStates);
        memcpy (m_finals.begin (), pStates, Count * sizeof (int));
    }
}


void FARSNfa_wo_ro::Prepare ()
{
    m_delta.Prepare ();
}


void FARSNfa_wo_ro::Clear ()
{
    m_delta.Clear ();

    m_initials.Clear ();
    m_initials.Create ();

    m_finals.Clear ();
    m_finals.Create ();
}

}
