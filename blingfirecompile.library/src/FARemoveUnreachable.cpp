/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARemoveUnreachable.h"
#include "FAAllocatorA.h"
#include "FARSNfaA.h"

namespace BlingFire
{


FARemoveUnreachable::FARemoveUnreachable (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_RemoveIniUnreach (true)
{
    m_state2info.SetAllocator (pAlloc);
    m_state2info.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_arcs.SetAllocator (pAlloc);

    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();
}


void FARemoveUnreachable::SetRemoveIniUnreach (const bool RemoveIniUnreach)
{
    m_RemoveIniUnreach = RemoveIniUnreach;
}


void FARemoveUnreachable::SetNfa (FARSNfaA * pNfa)
{
    m_pNfa = pNfa;
}


void FARemoveUnreachable::Clear ()
{
    DebugLogAssert (m_pNfa);
    DebugLogAssert (0 == m_stack.size ());

    const int PrevSize = m_state2info.size ();
    const int MaxState = m_pNfa->GetMaxState ();

    m_state2info.resize (MaxState + 1);

    for (int State = PrevSize; State <= MaxState; ++State) {
        m_state2info [State] = 0;
    }

    m_arcs.Clear ();
}


inline void FARemoveUnreachable::BuildArcs ()
{
    DebugLogAssert (m_pNfa);

    const int MaxState = m_pNfa->GetMaxState ();
    DebugLogAssert (0 <= MaxState);

    for (int i = 0; i <= MaxState; ++i) {

        const int * pIws;
        const int IwCount = m_pNfa->GetIWs (i, &pIws);

        for (int j = 0; j < IwCount; ++j) {

            const int Iw = pIws [j];

            const int * pStates;
            const int Count = m_pNfa->GetDest (i, Iw, &pStates);
            //// DebugLogAssert (0 < Count && pStates);

            for (int k = 0; k < Count; ++k) {

                const int Dst = pStates [k];
                DebugLogAssert (0 <= Dst);

                if (m_RemoveIniUnreach) {
                    m_arcs.Add (i, Dst);
                } else {
                    m_arcs.Add (Dst, i);
                }
            }
        }
    }

    m_arcs.SortUniq ();
}


void FARemoveUnreachable::FindUnreach ()
{
    DebugLogAssert (m_pNfa);
    DebugLogAssert (0 == m_stack.size ());

    int i;
    int Count;
    const int * pStates;

    if (m_RemoveIniUnreach) {
        Count = m_pNfa->GetInitials (&pStates);
    } else {
        Count = m_pNfa->GetFinals (&pStates);
    }

    m_stack.resize (Count);

    for (i = 0; i < Count; ++i) {

        DebugLogAssert (pStates);

        const int State = pStates [i];
        m_stack [i] = State;
        m_state2info [State] = 1;
    }

    while (0 != m_stack.size ()) {

        const int State = m_stack [m_stack.size () - 1];
        m_stack.pop_back ();

        /// get all destination/source states
        Count = m_arcs.Get (State, &pStates);

        for (i = 0; i < Count; ++i) {

            DebugLogAssert (pStates);
            const int DstState = pStates [i];

            if (0 == m_state2info [DstState]) {

                m_stack.push_back (DstState);
                m_state2info [DstState] = 1;
            }

        } // of iteration thru the destination states
    }
}


void FARemoveUnreachable::RemoveTransitions ()
{
    const int MaxState = m_pNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        /// remove all out-going transitions, if state is unreachable
        if (0 == m_state2info [State]) {

            /// remove all out-going transitions
            const int * pIws;
            const int IwCount = m_pNfa->GetIWs (State, &pIws);

            if (0 < IwCount) {
                DebugLogAssert (pIws);
                m_iws.resize (IwCount);
                memcpy (m_iws.begin (), pIws, sizeof (int) * IwCount);
            }

            for (int i = 0; i < IwCount; ++i) {
                const int Iw = m_iws [i];
                m_pNfa->SetTransition (State, Iw, NULL, -1);
            }

            m_iws.resize (0);

        /// remove all out-going transitions to unreachable states
        } else {

            const int * pIws;
            const int IwCount = m_pNfa->GetIWs (State, &pIws);

            for (int j = 0; j < IwCount; ++j) {

                DebugLogAssert (pIws);
                const int Iw = pIws [j];

                const int * pDsts;
                const int DstCount = m_pNfa->GetDest (State, Iw, &pDsts);

                if (0 < DstCount) {

                    // reuse stack container for new destination set
                    m_stack.resize (0);

                    for (int k = 0; k < DstCount; ++k) {

                        const int Dst = pDsts [k];

                        if (0 != m_state2info [Dst]) {
                            m_stack.push_back (Dst);
                        }
                    }

                    int NewDstCount = m_stack.size ();

                    // delete all transitions, if empty
                    if (0 == NewDstCount) {
                        NewDstCount = -1;
                    }

                    if (NewDstCount < DstCount) {
                        const int * pNewDsts = m_stack.begin ();
                        m_pNfa->SetTransition (State, Iw, pNewDsts, NewDstCount);
                    }
                }

            } // of for (int j = 0; j < IwCount; ...

        } // of if (0 == m_state2info [State]) ...

    } // of for (int State = 0; State <= MaxState; ...

    const int Size = m_state2info.size ();

    if (0 < Size) {
        memset (m_state2info.begin (), 0, Size * sizeof (m_state2info [0]));
    }

    m_stack.resize (0);
}


void FARemoveUnreachable::Process ()
{
    DebugLogAssert (m_pNfa);
    DebugLogAssert (0 == m_stack.size ());

    Clear ();

    BuildArcs ();
    FindUnreach ();
    RemoveTransitions ();

    m_pNfa->Prepare ();
}

}

