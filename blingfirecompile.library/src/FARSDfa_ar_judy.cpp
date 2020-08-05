/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa_ar_judy.h"
#include "FAAllocatorA.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSDfa_ar_judy::FARSDfa_ar_judy (FAAllocatorA * pAlloc) :
    m_MaxIw (-1),
    m_StateCount (0),
    m_initial (0),
    m_min_final (0),
    m_max_final (0),
    m_pAlloc (pAlloc)
{
    m_state_iw2dst.SetAllocator (m_pAlloc);
    m_state_iw2dst.Create ();

    m_finals.SetAllocator (m_pAlloc);
    m_finals.Create ();

    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();

    m_iw2bool.SetAllocator (pAlloc);
    m_iw2bool.Create ();
}


FARSDfa_ar_judy::~FARSDfa_ar_judy ()
{
    FARSDfa_ar_judy::Clear ();
}


void FARSDfa_ar_judy::Clear ()
{
    DebugLogAssert (m_StateCount == m_state_iw2dst.size ());

    for (unsigned int State = 0; State < m_StateCount; ++State) {

        FAMap_judy * pIw2Dst = m_state_iw2dst [State];
        DebugLogAssert (pIw2Dst);

        pIw2Dst->Clear ();
        delete pIw2Dst;
    }

    m_state_iw2dst.resize (0);
    m_finals.resize (0, 0);
    m_alphabet.resize (0);
    m_iw2bool.resize (0);

    m_MaxIw = -1;
    m_StateCount = 0;
    m_initial = 0;
    m_min_final = 0;
    m_max_final = 0;
}


void FARSDfa_ar_judy::SetMaxState (const int MaxState)
{
    if (0 != m_StateCount) {
        FARSDfa_ar_judy::Clear ();
    }

    m_StateCount = MaxState + 1;
}


void FARSDfa_ar_judy::SetMaxIw (const int MaxIw)
{
    m_MaxIw = MaxIw;
}


void FARSDfa_ar_judy::Create ()
{
    m_state_iw2dst.resize (m_StateCount);

    m_alphabet.resize (0);

    m_iw2bool.resize (m_MaxIw + 1);
    m_iw2bool.set_bits (0, m_MaxIw, false);

    for (unsigned int State = 0; State < m_StateCount; ++State) {

        create_state (State);
    }
}


void FARSDfa_ar_judy::create_state (const int State)
{
    DebugLogAssert (m_pAlloc);
    DebugLogAssert (m_StateCount == m_state_iw2dst.size ());
    DebugLogAssert (0 <= State && (unsigned int) State < m_StateCount);

    FAMap_judy * pIw2Dst = NEW FAMap_judy;
    DebugLogAssert (pIw2Dst);

    m_state_iw2dst [State] = pIw2Dst;
}


void FARSDfa_ar_judy::AddStateCount (const int StatesCount)
{
    DebugLogAssert (m_StateCount == m_state_iw2dst.size ());
    DebugLogAssert (0 < StatesCount);

    const int OldSize = m_StateCount;
    m_StateCount += StatesCount;

    m_state_iw2dst.resize (m_StateCount);

    for (unsigned int State = OldSize; State < m_StateCount; ++State) {

        create_state (State);
    }
}


void FARSDfa_ar_judy::AddIwCount (const int IwCount)
{
    DebugLogAssert (0 < IwCount);

    const int OldIwCount = m_MaxIw + 1;
    m_MaxIw += IwCount;

    m_iw2bool.resize (m_MaxIw + 1);
    m_iw2bool.set_bits (OldIwCount, m_MaxIw, false);
}


void FARSDfa_ar_judy::SetInitial (const int State)
{
    m_initial = State;
}


void FARSDfa_ar_judy::SetFinals (const int * pStates, const int Size)
{
    m_min_final = m_StateCount;
    m_max_final = 0;

    for (int i = 0; i < Size; ++i) {

        const int State = pStates [i];

        DebugLogAssert (0 <= State && (unsigned int) State < m_StateCount);

        if (m_min_final > State)
            m_min_final = State;
        if (m_max_final < State)
            m_max_final = State;

        m_finals.push_back (State);
    }
}


void FARSDfa_ar_judy::SetIWs (const int * pIws, const int IwsCount)
{
    DebugLogAssert (0 < IwsCount && pIws);
    DebugLogAssert (FAIsSorted (pIws, IwsCount));

    m_MaxIw = pIws [IwsCount - 1];

    m_alphabet.resize (IwsCount, 0);

    m_iw2bool.resize (m_MaxIw + 1);
    m_iw2bool.set_bits (0, m_MaxIw, false);

    for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

        const int Iw = pIws [iw_idx];

        m_iw2bool.set_bit (Iw, true);
        m_alphabet [iw_idx] = Iw;
    }
}


void FARSDfa_ar_judy::SetTransition (const int FromState,
                                     const int Iw,
                                     const int DstState)
{
    DebugLogAssert (0 <= FromState && (unsigned int) FromState < m_StateCount);

    if (-1 != DstState) {

        if (false == m_iw2bool.get_bit (Iw)) {

            m_iw2bool.set_bit (Iw, true);
            m_alphabet.push_back (Iw, 10);
        }

        FAMap_judy * pIw2Dst = m_state_iw2dst [FromState];
        DebugLogAssert (pIw2Dst);

        pIw2Dst->Set (Iw, DstState);
    }
}


void FARSDfa_ar_judy::SetTransition (const int FromState,
                                     const int * pIws,
                                     const int * pDstStates,
                                     const int Count)
{
    DebugLogAssert (0 <= FromState && (unsigned int) FromState < m_StateCount);
    DebugLogAssert (pIws);
    DebugLogAssert (pDstStates);

    FAMap_judy * pIw2Dst = m_state_iw2dst [FromState];
    DebugLogAssert (pIw2Dst);

    for (int i = 0; i < Count; ++i) {

        int DstState = pDstStates [i];

        if (-1 == DstState) {
            continue;
        }

        const int Iw = pIws [i];
        DebugLogAssert (0 <= Iw && Iw <= m_MaxIw);

        if (false == m_iw2bool.get_bit (Iw)) {

            m_iw2bool.set_bit (Iw, 1);
            m_alphabet.push_back (Iw, 10);
        }

        pIw2Dst->Set (Iw, DstState);
    }
}


void FARSDfa_ar_judy::Prepare ()
{
    int NewSize;

    NewSize = FASortUniq (m_alphabet.begin (), m_alphabet.end ());
    m_alphabet.resize (NewSize, 0);

    NewSize = FASortUniq (m_finals.begin (), m_finals.end ());
    m_finals.resize (NewSize, 0);
}


const int FARSDfa_ar_judy::GetMaxState () const
{
    return m_StateCount - 1;
}


const int FARSDfa_ar_judy::GetMaxIw () const
{
    return m_MaxIw;
}


const int FARSDfa_ar_judy::GetInitial () const
{
    return m_initial;
}


const int FARSDfa_ar_judy::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (ppStates);

    *ppStates = m_finals.begin ();
    return m_finals.size ();
}


const bool FARSDfa_ar_judy::IsFinal (const int State) const
{
    return State >= m_min_final && State <= m_max_final;
}


const int FARSDfa_ar_judy::GetIWs (const int ** ppIws) const
{
    DebugLogAssert (ppIws);
    *ppIws = m_alphabet.begin ();
    return m_alphabet.size ();
}


const int FARSDfa_ar_judy::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    const int * pIws2;
    const int IwCount = GetIWs (&pIws2);

    if (0 < IwCount && IwCount <= MaxIwCount) {
        memcpy (pIws, pIws2, sizeof (int) * IwCount);
    }

    return IwCount;
}


const int FARSDfa_ar_judy::GetDest (const int State, const int Iw) const
{
    if (FAFsmConst::DFA_DEAD_STATE == State) {
        return -1;
    }

    DebugLogAssert (0 <= State && (unsigned int) State < m_StateCount);

    const FAMap_judy * pIw2Dst = m_state_iw2dst [State];
    DebugLogAssert (pIw2Dst);

    const int * pDst = pIw2Dst->Get (Iw);

    if (pDst)
        return *pDst;
    else
        return -1;
}

}
