/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa_renum_iws.h"
#include "FAMapA.h"
#include "FAUtils.h"
#include "FALimits.h"

namespace BlingFire
{


FARSDfa_renum_iws::FARSDfa_renum_iws (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pNewIw2Old (NULL),
    m_MaxIw (-1)
{
    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();
}


FARSDfa_renum_iws::~FARSDfa_renum_iws ()
{}


void FARSDfa_renum_iws::SetOldDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FARSDfa_renum_iws::SetNew2Old (const FAMapA * pNewIw2Old)
{
    m_pNewIw2Old = pNewIw2Old;
}


void FARSDfa_renum_iws::Clear ()
{
    m_iws.Clear ();
    m_iws.Create ();

    m_MaxIw = -1;
}


void FARSDfa_renum_iws::Prepare ()
{
    DebugLogAssert (m_pDfa && m_pNewIw2Old);

    m_iws.resize (0);
    m_MaxIw = -1;

    int Iw = -1;
    const int * pOldIw = m_pNewIw2Old->Prev (&Iw);

    while (NULL != pOldIw) {

        DebugLogAssert (0 <= Iw && Iw < FALimits::MaxIw);

        if (m_MaxIw < Iw) {
            m_MaxIw = Iw;
        }

        m_iws.push_back (Iw);

        pOldIw = m_pNewIw2Old->Prev (&Iw);

    } // of while (NULL != pOldIw) ...

    const int NewSize = FASortUniq (m_iws.begin (), m_iws.end ());
    m_iws.resize (NewSize);
}


const int FARSDfa_renum_iws::GetMaxState () const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->GetMaxState ();
}


const int FARSDfa_renum_iws::GetMaxIw () const
{
    return m_MaxIw;
}


const int FARSDfa_renum_iws::GetInitial () const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->GetInitial ();
}


const int FARSDfa_renum_iws::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->GetFinals (ppStates);
}


const int FARSDfa_renum_iws::GetIWs (const int ** ppIws) const
{
    DebugLogAssert (ppIws);
    *ppIws = m_iws.begin ();
    return m_iws.size ();
}


const int FARSDfa_renum_iws::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    const int * pIws2;
    const int IwCount = GetIWs (&pIws2);

    if (0 < IwCount && IwCount <= MaxIwCount) {
        memcpy (pIws, pIws2, sizeof (int) * IwCount);
    }

    return IwCount;
}


const bool FARSDfa_renum_iws::IsFinal (const int State) const
{
    DebugLogAssert (m_pDfa);
    return m_pDfa->IsFinal (State);
}


const int FARSDfa_renum_iws::GetDest (const int State, const int NewIw) const
{
    DebugLogAssert (m_pDfa);

    const int * pOldIw = m_pNewIw2Old->Get (NewIw);

    if (NULL == pOldIw) {
        return -1;
    }

    const int OldIw = *pOldIw;
    DebugLogAssert (0 <= OldIw && OldIw < FALimits::MaxIw);

    return m_pDfa->GetDest (State, OldIw);
}


void FARSDfa_renum_iws::SetMaxState (const int /*MaxState*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetMaxIw (const int /*MaxIw*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::Create ()
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetInitial (const int /*State*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetFinals (const int * /*pStates*/, const int /*StateCount*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetIWs (const int * /*pIws*/, const int /*IwsCount*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetTransition (const int /*FromState*/,
                                       const int /*Iw*/,
                                       const int /*DstState*/)
{
    // not implemented
    DebugLogAssert (0);
}

void FARSDfa_renum_iws::SetTransition (const int /*FromState*/,
                                       const int * /*pIws*/,
                                       const int * /*pDstStates*/,
                                       const int /*Count*/)
{
    // not implemented
    DebugLogAssert (0);
}

}
