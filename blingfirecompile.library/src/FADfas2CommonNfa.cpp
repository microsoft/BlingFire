/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADfas2CommonNfa.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FARSDfaA.h"
#include "FAException.h"

namespace BlingFire
{


FADfas2CommonNfa::FADfas2CommonNfa (FAAllocatorA * pAlloc) :
    m_nfa (pAlloc),
    m_StateCount (0),
    m_D (0),
    m_pIws (NULL),
    m_IwsCount (0)
{
    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();
}


void FADfas2CommonNfa::Clear ()
{
    m_nfa.Clear ();
    m_StateCount = 0;

    m_finals.Clear ();
    m_finals.Create ();
}


inline const bool FADfas2CommonNfa::IsReachable (const FARSDfaA * pDfa) const
{
    DebugLogAssert (pDfa);
    DebugLogAssert (m_pIws && 0 < m_IwsCount);

    const int max_state = pDfa->GetMaxState ();

    const int initial = pDfa->GetInitial ();
    DebugLogAssert (-1 != initial);

    for (int state = 0; state <= max_state; ++state) {

        for (int i = 0; i < m_IwsCount; ++i) {

            const int Iw = m_pIws [i];
            const int dst = pDfa->GetDest (state, Iw);

            if (initial == dst) {
                return true;
            }

        } // of for (int i = 0; ...
    } // of for (int state = 0; ...

    return false;
}


inline void FADfas2CommonNfa::
    AddTransitions (const FARSDfaA * pDfa, const int State, const int state)
{
    DebugLogAssert (pDfa);
    DebugLogAssert (m_pIws && 0 < m_IwsCount);

    for (int i = 0; i < m_IwsCount; ++i) {

        const int Iw = m_pIws [i];
        const int dst = pDfa->GetDest (state, Iw);

        if (FAFsmConst::DFA_DEAD_STATE == dst) {

            // add a new transition into the NFA
            m_nfa.SetTransition (State, Iw, FAFsmConst::NFA_DEAD_STATE);

        } else if (0 <= dst) {

            // map dst into a new state-space
            const int Dst = m_StateCount + m_D + dst;
            DebugLogAssert (0 <= Dst);

            // add a new transition into the NFA
            m_nfa.SetTransition (State, Iw, Dst);
        }

    } // of for (int i = 0; ...
}


void FADfas2CommonNfa::AddDfa (const FARSDfaA * pDfa)
{
    FAAssert (FAIsValidDfa (pDfa), FAMsg::InvalidParameters);

    // the initial state of the common Nfa
    const int Initial = 0;

    // see if that is the first Dfa in the list
    if (0 == m_StateCount) {
        m_nfa.SetInitials (&Initial, 1);
        m_StateCount++;
        DebugLogAssert (0 == m_finals.size ());
    }

    // get the alphabet
    m_pIws = NULL;
    m_IwsCount = pDfa->GetIWs (&m_pIws);

    // see if DFA's initial state is needed (after merge)
    const bool fNeedIni = IsReachable (pDfa);

    // calc the state-space delta value
    m_D = fNeedIni ? 0 : -1 ;

    // get the max state
    const int max_state = pDfa->GetMaxState ();

    // get the initial state
    const int initial = pDfa->GetInitial ();

    for (int state = 0; state <= max_state; ++state) {

        // map state into a new state-space
        const int State = m_StateCount + m_D + state;

        // check if the state is initial (in the smaller automaton)
        if (state == initial) {
            AddTransitions (pDfa, Initial, state);
            if (fNeedIni) {
                AddTransitions (pDfa, State, state);
            }
        } else {
            AddTransitions (pDfa, State, state);
        }
    } // of for (int state = 0; ...

    // add final states
    const int * pFinals;
    const int FinalCount = pDfa->GetFinals (&pFinals);
    DebugLogAssert (0 < FinalCount && pFinals);
    DebugLogAssert (FAIsSortUniqed (pFinals, FinalCount));

    for (int i = 0; i < FinalCount; ++i) {

        const int state = pFinals [i];
        DebugLogAssert (0 <= state);

        const int State = m_StateCount + m_D + state;
        DebugLogAssert (0 <= State);

        if (initial == state) {
            m_finals.push_back (Initial);
            if (fNeedIni) {
                m_finals.push_back (State);
            }
        } else {
            m_finals.push_back (State);
        }
    } // of for (int i = 0; ...

    const int Size = FASortUniq (m_finals.begin (), m_finals.end ());
    DebugLogAssert (0 < Size);
    m_finals.resize (Size);

    // update the common NFA state count
    m_StateCount += (max_state + 1 + m_D);
}


void FADfas2CommonNfa::Process ()
{
    m_nfa.SetFinals (m_finals.begin (), m_finals.size ());
    m_nfa.Prepare ();
}


const FARSNfaA * FADfas2CommonNfa::GetCommonNfa () const
{
    return & m_nfa;
}

}
