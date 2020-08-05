/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa_renum.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSDfa_renum::FARSDfa_renum (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pOld2NewState (NULL),
    m_NewMaxState (-1)
{
    m_new2old.SetAllocator (pAlloc);
    m_new2old.Create ();

    m_new_finals.SetAllocator (pAlloc);
    m_new_finals.Create ();
}


FARSDfa_renum::~FARSDfa_renum ()
{}


void FARSDfa_renum::SetOldDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FARSDfa_renum::SetOld2New (const int * pOld2NewState)
{
    m_pOld2NewState = pOld2NewState;
}


void FARSDfa_renum::Prepare ()
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_pOld2NewState);

    m_NewMaxState = -1;
    m_new2old.resize (0);
    m_new_finals.resize (0);

    const int OldMaxState = m_pDfa->GetMaxState ();
    for (int OldState = 0; OldState <= OldMaxState; ++OldState) {

        const int NewState = m_pOld2NewState [OldState];

        if (-1 == NewState)
            continue;

        if (NewState > m_NewMaxState) {
            m_NewMaxState = NewState;
            m_new2old.resize (m_NewMaxState + 1);
        }

        m_new2old [NewState] = OldState;
    }

    const int * pOldFinals;
    const int FinalsCount = m_pDfa->GetFinals (&pOldFinals);
    DebugLogAssert (0 < FinalsCount && pOldFinals);

    for (int i = 0; i < FinalsCount; ++i) {

        const int OldFinal = pOldFinals [i];
        const int NewFinal = m_pOld2NewState [OldFinal];

        if (-1 == NewFinal)
            continue;

        m_new_finals.push_back (NewFinal);
    }

    const int NewSize = FASortUniq (m_new_finals.begin (), m_new_finals.end ());
    m_new_finals.resize (NewSize);
}


void FARSDfa_renum::Clear ()
{
    m_new2old.Clear ();
    m_new2old.Create ();
    
    m_new_finals.Clear ();
    m_new_finals.Create ();
}


const int FARSDfa_renum::GetMaxState () const
{
    return m_NewMaxState;
}


const int FARSDfa_renum::GetMaxIw () const
{
    DebugLogAssert (m_pDfa);

    return m_pDfa->GetMaxIw ();
}


const int FARSDfa_renum::GetInitial () const
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_pOld2NewState);

    const int OldInitial = m_pDfa->GetInitial ();

    if (-1 != OldInitial) {
        return m_pOld2NewState [OldInitial];
    }

    return -1;
}


const int FARSDfa_renum::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (ppStates);

    *ppStates = m_new_finals.begin ();
    return m_new_finals.size ();
}


const int FARSDfa_renum::GetIWs (const int ** ppIws) const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->GetIWs (ppIws);
}


const int FARSDfa_renum::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->GetIWs (pIws, MaxIwCount);
}


const bool FARSDfa_renum::IsFinal (const int State) const
{
    DebugLogAssert (m_pDfa);

    const int OldState = m_new2old [State];
    return m_pDfa->IsFinal (OldState);
}


const int FARSDfa_renum::GetDest (const int State, const int Iw) const
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_pOld2NewState);

    if (FAFsmConst::DFA_DEAD_STATE == State) {
        return -1;
    }

    const int OldState = m_new2old [State];
    const int OldDstState = m_pDfa->GetDest (OldState, Iw);

    if (-1 == OldDstState) {
        return -1;
    } else if (FAFsmConst::DFA_DEAD_STATE == OldDstState) {
        return FAFsmConst::DFA_DEAD_STATE;
    } else {
        DebugLogAssert (0 <= OldDstState);
        return m_pOld2NewState [OldDstState];
    }
}


void FARSDfa_renum::SetMaxState (const int /*MaxState*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetMaxIw (const int /*MaxIw*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::Create ()
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetInitial (const int /*State*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetFinals (const int * /*pStates*/, const int /*StateCount*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetIWs (const int * /*pIws*/, const int /*IwsCount*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetTransition (const int /*FromState*/,
                                  const int /*Iw*/,
                                  const int /*DstState*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum::SetTransition (const int /*FromState*/,
                                  const int * /*pIws*/,
                                  const int * /*pDstStates*/,
                                  const int /*Count*/)
{
    // not implemented
    DebugLogAssert (0);
}

}
