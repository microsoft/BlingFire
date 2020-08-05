/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfas2CommonENfa.h"
#include "FAAllocatorA.h"
#include "FAFsmConst.h"
#include "FARSNfaA.h"
#include "FAUtils.h"

namespace BlingFire
{


FANfas2CommonENfa::FANfas2CommonENfa (FAAllocatorA * pAlloc) :
    m_empty (true),
    m_pInNfa (NULL),
    m_EpsilonIw (0),
    m_common_nfa (pAlloc),
    m_AddNfaNums (false),
    m_NfaNumBase (0),
    m_NfaCount (0)
{
    m_states.SetAllocator (pAlloc);
    m_states.Create ();

    m_eps_dst.SetAllocator (pAlloc);
    m_eps_dst.Create ();

    m_fsnum2nfanum.SetAllocator (pAlloc);
    m_fsnum2nfanum.Create ();
}


void FANfas2CommonENfa::SetEpsilonIw (const int EpsilonIw)
{
    m_EpsilonIw = EpsilonIw;
}


void FANfas2CommonENfa::SetAddNfaNums (const bool AddNfaNums)
{
    m_AddNfaNums = AddNfaNums;
}


void FANfas2CommonENfa::SetNfaNumBase (const int NfaNumBase)
{
    m_NfaNumBase = NfaNumBase;
}


const FARSNfaA * FANfas2CommonENfa::GetCommonNfa () const
{
    DebugLogAssert (false == m_empty);
    return & m_common_nfa;
}


void FANfas2CommonENfa::Clear ()
{
    m_common_nfa.Clear ();
    m_empty = true;
    m_states.resize (0);
    m_eps_dst.resize (0);
    m_NfaCount = 0;
    m_fsnum2nfanum.resize (0);
}


void FANfas2CommonENfa::AddTransitions (const int StateOffset)
{
    DebugLogAssert (NULL != m_pInNfa);

    const int InMaxState = m_pInNfa->GetMaxState ();

    for (int State = 0; State < InMaxState; ++State) {

        const int * pIw;
        const int IwCount = m_pInNfa->GetIWs (State, &pIw);

        const int NewFromState = StateOffset + State;

        for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

            DebugLogAssert (pIw);
            const int Iw = pIw [iw_idx];

            const int * pDstStates;
            const int DstCount =  m_pInNfa->GetDest (State, Iw, &pDstStates);

            if (0 < DstCount) {

                DebugLogAssert (NULL != pDstStates);

                /// make renumeration
                m_states.resize (DstCount);
                for (int j = 0; j < DstCount; ++j) {
                    const int NewDstState = StateOffset + pDstStates [j];
                    m_states [j] = NewDstState;
                }

                /// setup a new transition
                m_common_nfa.SetTransition (NewFromState, Iw, m_states.begin (), DstCount);

            } else if (0 == DstCount) {

                // add a transition into a dead-state
                const int DeadState = FAFsmConst::NFA_DEAD_STATE;
                m_common_nfa.SetTransition (NewFromState, Iw, &DeadState, 1);
            }

        } // of iteration thru the input weights
    } // of iteration thru the states
}


void FANfas2CommonENfa::AddFinals (const int StateOffset)
{
    DebugLogAssert (NULL != m_pInNfa);

    int i;

    const int *pOldFinals;
    const int OldFinalsCount = m_common_nfa.GetFinals (&pOldFinals);

    DebugLogAssert (m_fsnum2nfanum.size () == (unsigned int) OldFinalsCount);

    const int *pNewFinals;
    const int NewFinalsCount = m_pInNfa->GetFinals (&pNewFinals);

    m_states.resize (NewFinalsCount + OldFinalsCount);
    m_fsnum2nfanum.resize (NewFinalsCount + OldFinalsCount);

    for (i = 0; i < OldFinalsCount; ++i) {

        DebugLogAssert (NULL != pOldFinals);
        m_states [i] = pOldFinals [i];
    }

    for (i = 0; i < NewFinalsCount; ++i) {

        DebugLogAssert (NULL != pNewFinals);

        const int NewFinal = StateOffset + pNewFinals [i];
        m_states [i + OldFinalsCount] = NewFinal;

        m_fsnum2nfanum [i + OldFinalsCount] = m_NfaCount;
    }

    int * pBegin = m_states.begin ();
    int * pEnd = pBegin + m_states.size ();
    const int FinalsCount = FASortUniq (pBegin, pEnd);
    m_states.resize (FinalsCount);

    m_common_nfa.SetFinals (m_states.begin (), m_states.size ());
}


void FANfas2CommonENfa::AddInitials (const int StateOffset)
{
    DebugLogAssert (NULL != m_pInNfa);

    const int * pInitials;
    const int InitialsCount = m_pInNfa->GetInitials (&pInitials);

    for (int i = 0; i < InitialsCount; ++i) {

        DebugLogAssert (pInitials);

        const int EpsDstState = StateOffset + pInitials [i];
        m_eps_dst.push_back (EpsDstState);
    }
}


void FANfas2CommonENfa::AddNfa (const FARSNfaA * pNfa)
{
    DebugLogAssert (NULL != pNfa);

    m_pInNfa = pNfa;

    /// make common nfa ready

    const int InMaxState = m_pInNfa->GetMaxState ();
    const int InMaxIw = m_pInNfa->GetMaxIw ();

    int StateCount = m_common_nfa.GetMaxState () + 1;

    if (true == m_empty) {

        /// keep place for the initial state
        StateCount = 1;

        /// create an empty automaton 
        m_common_nfa.SetMaxState (InMaxState + 1);
        m_common_nfa.SetMaxIw (InMaxIw);
        m_common_nfa.Create ();

        /// add 0 - initial state
        int initial = 0;
        m_common_nfa.SetInitials (&initial, 1);

        /// it is not empty any more
        m_empty = false;

    } else {

        /// add states
        m_common_nfa.AddStateCount (1 + InMaxState);

        /// add iw counts
        const int MaxIw = m_common_nfa.GetMaxIw ();
        if (MaxIw < InMaxIw) {
            m_common_nfa.AddIwCount (InMaxIw - MaxIw);
        }
    }

    AddTransitions (StateCount);
    AddFinals (StateCount);
    AddInitials (StateCount);

    m_NfaCount++;
}


void FANfas2CommonENfa::AddNfaNums ()
{
    const int * pFinals;
    const int FinalsCount = m_common_nfa.GetFinals (&pFinals);

    DebugLogAssert (0 < m_fsnum2nfanum.size ());
    DebugLogAssert (m_fsnum2nfanum.size () == (unsigned int) FinalsCount);

    // adjust MaxIw
    const int MaxIw = m_common_nfa.GetMaxIw ();
    const int NewMaxIw = m_NfaNumBase + m_fsnum2nfanum [m_fsnum2nfanum.size () - 1];

    if (MaxIw < NewMaxIw) {

        m_common_nfa.AddIwCount (NewMaxIw - MaxIw);
    }

    // add one more state
    m_common_nfa.AddStateCount (1);
    const int MaxState = m_common_nfa.GetMaxState ();

    // draw transitions from final states to the added one
    for (int i = 0; i < FinalsCount; ++i) {

        DebugLogAssert (pFinals);

        const int State = pFinals [i];
        const int Iw = m_NfaNumBase + m_fsnum2nfanum [i];

        m_common_nfa.SetTransition (State, Iw, MaxState);
    }

    // make MaxState be final
    const int *pOldFinals;
    const int OldFinalsCount = m_common_nfa.GetFinals (&pOldFinals);

    // add ome more final state
    m_states.resize (OldFinalsCount + 1);
    memcpy (m_states.begin (), pOldFinals, sizeof (int) * OldFinalsCount);
    m_states [OldFinalsCount] = MaxState;

    // set up new finals
    m_common_nfa.SetFinals (m_states.begin (), m_states.size ());
}


void FANfas2CommonENfa::Process ()
{
    /// calc epsilon transitions
    int * pBegin = m_eps_dst.begin ();
    int * pEnd = pBegin + m_eps_dst.size ();
    const int NewSize = FASortUniq (pBegin, pEnd);
    m_eps_dst.resize (NewSize);

    /// setup epsilon transitions
    m_common_nfa.SetTransition (0, m_EpsilonIw, m_eps_dst.begin (), NewSize);

    /// add NfaNum transitions, if asked
    if (m_AddNfaNums) {

        AddNfaNums ();
    }

    /// make the automaton ready
    m_common_nfa.Prepare ();
}

}
