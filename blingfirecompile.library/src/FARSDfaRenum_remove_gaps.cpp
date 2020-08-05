/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfaRenum_remove_gaps.h"
#include "FARSDfaA.h"

namespace BlingFire
{


FARSDfaRenum_remove_gaps::FARSDfaRenum_remove_gaps (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_MaxNewState (-1)
{
    m_old2new.SetAllocator (pAlloc);
    m_old2new.Create ();

    m_is_final.SetAllocator (pAlloc);
    m_is_final.Create ();
}

void FARSDfaRenum_remove_gaps::Clear ()
{
    m_is_final.Clear ();
    m_is_final.Create ();
    m_old2new.Clear ();
    m_old2new.Create ();
}

void FARSDfaRenum_remove_gaps::SetDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}

const int * FARSDfaRenum_remove_gaps::GetOld2NewMap () const
{
    return m_old2new.begin ();
}

void FARSDfaRenum_remove_gaps::Prepare ()
{
    DebugLogAssert (m_pDfa);

    m_MaxNewState = -1;

    const int MaxState = m_pDfa->GetMaxState ();
    DebugLogAssert (0 < MaxState);

    m_old2new.resize (MaxState + 1);
    m_is_final.resize (MaxState + 1);

    m_is_final.set_bits (0, MaxState, false);

    const int * pFinals;
    const int FinalsCount = m_pDfa->GetFinals (&pFinals);

    for (int i = 0; i < FinalsCount; ++i) {

        DebugLogAssert (pFinals);

        const int FinalState = pFinals [i];
        m_is_final.set_bit (FinalState, true);
    }
}

//
// State is used in one of two reasons
//
// 1. it has outgoing transitions
// 2. it is final
//
// TODO: Add 0-reachability criterion instead, as dead-state(s) and 
// unreachable states are possible in general case.
//
const bool FARSDfaRenum_remove_gaps::IsUsed (const int State) const
{
    DebugLogAssert (m_pDfa);

    if (m_is_final.get_bit (State))
        return true;

    const int * pIws;
    const int IwsCount = m_pDfa->GetIWs (&pIws);

    for (int i = 0; i < IwsCount; ++i) {

        DebugLogAssert (pIws);

        const int Iw = pIws [i];
        const int DstState = m_pDfa->GetDest (State, Iw);

        if (-1 != DstState)
            return true;
    }

    return false;
}

void FARSDfaRenum_remove_gaps::Process ()
{
    DebugLogAssert (m_pDfa);

    Prepare ();

    const int MaxState = m_pDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        if (IsUsed (State)) {

            m_MaxNewState++;
            m_old2new [State] = m_MaxNewState;

        } else {

            m_old2new [State] = -1;
        }
    }
}

}

