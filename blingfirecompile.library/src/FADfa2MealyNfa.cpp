/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADfa2MealyNfa.h"
#include "FARSNfaA.h"
#include "FAMealyNfaA.h"
#include "FARSDfaA.h"
#include "FAUtils.h"

namespace BlingFire
{


FADfa2MealyNfa::FADfa2MealyNfa (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pOutNfa (NULL),
    m_pOutOws (NULL)
{
    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_state2state.SetAllocator (pAlloc);
    m_state2state.Create ();
}


void FADfa2MealyNfa::SetInDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FADfa2MealyNfa::SetOutNfa (FARSNfaA * pOutNfa, FAMealyNfaA * pOutOws)
{
    m_pOutNfa = pOutNfa;
    m_pOutOws = pOutOws;
}


void FADfa2MealyNfa::Process ()
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_pOutNfa && m_pOutOws);

    m_pOutNfa->Clear ();
    m_pOutOws->Clear ();

    const int * pIws;
    const int Count = m_pDfa->GetIWs (&pIws);
    DebugLogAssert (0 < Count && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, Count));

    const int MaxState = m_pDfa->GetMaxState ();
    m_state2state.resize (MaxState + 1);
    for (int k = 0; k <= MaxState; ++k) {
        m_state2state [k] = -1;
    }

    int MaxOutState = 0;

    const int Initial = m_pDfa->GetInitial ();
    m_stack.resize (0);
    m_stack.push_back (Initial);
    m_state2state [Initial] = MaxOutState++;

    m_finals.resize (0);

    while (!m_stack.empty ()) {

        const int Size = m_stack.size ();
        const int State = m_stack [Size - 1];
        m_stack.resize (Size - 1);

        // get the source state in the new automaton
        const int FromState = m_state2state [State];
        DebugLogAssert (-1 != FromState);

        if (Initial == State) {
            // due to Dfa properties
            m_pOutNfa->SetInitials (&FromState, 1);
        }
        if (m_pDfa->IsFinal (State)) {
            m_finals.push_back (FromState);
        }

        for (int i = 0; i < Count; ++i) {

            const int Iw = pIws [i];
            const int Dst = m_pDfa->GetDest (State, Iw);

            if (-1 == Dst)
                continue;

            DebugLogAssert (0 < Dst);

            for (int j = 0; j < Count; ++j) {

                const int Ow = pIws [j];
                const int Dst2 = m_pDfa->GetDest (Dst, Ow);

                if (-1 == Dst2)
                    continue;

                DebugLogAssert (0 < Dst2);

                if (-1 == m_state2state [Dst2]) {
                    m_stack.push_back (Dst2);
                    m_state2state [Dst2] = MaxOutState++;
                }

                const int ToState = m_state2state [Dst2];

                m_pOutNfa->SetTransition (FromState, Iw, ToState);
                m_pOutOws->SetOw (FromState, Iw, ToState, Ow);

            } // of for (int j = 0; ...
        } // of for (int i = 0; ...
    } // of while (!m_stack.empty ()) ...

    const int NewSize = FASortUniq (m_finals.begin (), m_finals.end ());
    m_pOutNfa->SetFinals (m_finals.begin (), NewSize);

    m_finals.Clear ();
    m_finals.Create ();

    m_stack.Clear ();
    m_stack.Create ();

    m_state2state.Clear ();
    m_state2state.Create ();

    m_pOutNfa->Prepare ();
}

}
