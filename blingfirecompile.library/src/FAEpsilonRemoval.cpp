/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAAllocatorA.h"
#include "FAEpsilonRemoval.h"
#include "FARSNfaA.h"
#include "FAFsmConst.h"

namespace BlingFire
{


FAEpsilonRemoval::FAEpsilonRemoval (FAAllocatorA * pAlloc) :
    m_has_epsilon (false),
    m_pInNfa (NULL),
    m_pOutNfa (NULL),
    m_EpsilonIw (-1),
    m_eps_graph (pAlloc),
    m_tr_closure (pAlloc),
    m_remover (pAlloc),
    m_process_any (false),
    m_any_iw (-1),
    m_set_utils (pAlloc),
    m_pG (NULL)
{
    m_tr_closure.SetInGraph (&m_eps_graph);
    m_tr_closure.SetOutGraph (&m_eps_graph);
}


void FAEpsilonRemoval::Clear ()
{
    m_set_utils.Clear ();
}


void FAEpsilonRemoval::SetEpsilonGraph (FAEpsilonGraph * pG)
{
    m_tr_closure.SetInGraph (pG);
    m_tr_closure.SetOutGraph (pG);
}


void FAEpsilonRemoval::SetInNfa (const FARSNfaA * pInNfa)
{
    m_pInNfa = pInNfa;
    m_eps_graph.SetInNfa (pInNfa);
}


void FAEpsilonRemoval::SetAnyIw (const int AnyIw)
{
    m_process_any = true;
    m_any_iw = AnyIw;
}


void FAEpsilonRemoval::SetOutNfa (FARSNfaA * pOutNfa)
{
    m_pOutNfa = pOutNfa;
    m_eps_graph.SetOutNfa (pOutNfa);
    m_remover.SetNfa (pOutNfa);
}


void FAEpsilonRemoval::SetEpsilonIw (const int EpsilonIw)
{
    m_EpsilonIw = EpsilonIw;
    m_eps_graph.SetEpsilonIw (EpsilonIw);
}


void FAEpsilonRemoval::CopyNoEps ()
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (m_pOutNfa);

    // assume there are no epsilon transitions
    m_has_epsilon = false;

    // check if we use the input as an output
    if (m_pInNfa == m_pOutNfa) {
        m_has_epsilon = true;
        return;
    }

    const int * pStates;
    int Count;

    const int MaxState = m_pInNfa->GetMaxState ();
    m_pOutNfa->SetMaxState (MaxState);

    const int MaxIw = m_pInNfa->GetMaxIw ();
    m_pOutNfa->SetMaxIw (MaxIw);

    // make the output automaton ready    
    m_pOutNfa->Create ();

    // set up initial states
    Count = m_pInNfa->GetInitials (&pStates);

    if (0 == Count)
        return;
    else
        m_pOutNfa->SetInitials (pStates, Count);

    // set up final states
    Count = m_pInNfa->GetFinals (&pStates);

    if (0 == Count)
        return;
    else
        m_pOutNfa->SetFinals (pStates, Count);

    // make iteration thru the states of input automaton
    for (int State = 0; State <= MaxState; ++State) {

        // get input weights
        const int * pIws;
        const int IwsCount = m_pInNfa->GetIWs (State, &pIws);

        // make iteration thru the input weights
        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            DebugLogAssert (pIws);
            const int Iw = pIws [iw_idx];

            // it should not be the epsilon transition
            if (Iw == m_EpsilonIw) {
                m_has_epsilon = true;
                continue;
            }

            // copy the destination states
            Count = m_pInNfa->GetDest (State, Iw, &pStates);

            if (0 < Count) {
                m_pOutNfa->SetTransition (State, Iw, pStates, Count);
            } else if (0 == Count) {
                const int DeadState = FAFsmConst::NFA_DEAD_STATE;
                m_pOutNfa->SetTransition (State, Iw, &DeadState, 1);
            }
        }
    }
}


void FAEpsilonRemoval::RemoveEps ()
{
    DebugLogAssert (m_pOutNfa);

    const int * pStates;
    int Count;

    const int MaxState = m_pOutNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        // get Epsilon-transitions
        Count = m_pOutNfa->GetDest (State, m_EpsilonIw, &pStates);

        // see whether there are some transitions
        if (-1 != Count) {
            m_pOutNfa->SetTransition (State, m_EpsilonIw, NULL, -1);
        }
    }
}


void FAEpsilonRemoval::ProcessAnyOther ()
{
    DebugLogAssert (m_pOutNfa);

    const int MaxState = m_pOutNfa->GetMaxState ();

    /// make iteration thru all the states
    for (int State = 0; State <= MaxState; ++State) {

        // get destination set for Epsilon input weight
        const int * pEpsDstStates;
        const int EpsDstCount = m_pOutNfa->GetDest (State, m_EpsilonIw, &pEpsDstStates);

        // get destination set for Any input weight
        const int * pAnyDstSet;
        const int AnyDstCount = m_pOutNfa->GetDest (State, m_any_iw, &pAnyDstSet);

        // skip states without epslion transitions and without any-transitions
        if (0 < EpsDstCount && 0 < AnyDstCount) {

            // get State's alphabet
            const int * pIws;
            const int IwsCount =  m_pOutNfa->GetIWs (State, &pIws);

            // make iteration thru the alphabet
            for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

                DebugLogAssert (pIws);
                const int Iw = pIws [iw_idx];

                // skip m_any_iw and m_EpsilonIw
                if (Iw == m_any_iw || Iw == m_EpsilonIw)
	                continue;

                // get destination set
                const int * pDstSet;
                const int DstCount = m_pOutNfa->GetDest (State, Iw, &pDstSet);

                // calc : DstSet + AnyDstSet
                m_set_utils.Union (pDstSet, DstCount, pAnyDstSet, AnyDstCount, 0);
                const int * pUnion;
                const int UnionSize = m_set_utils.GetRes (&pUnion, 0);

                // set up new destination set
                m_pOutNfa->SetTransition (State, Iw, pUnion, UnionSize);
            }
        }

    } // of for (int State = 0; ...

    m_pOutNfa->Prepare ();
}


void FAEpsilonRemoval::Process ()
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (m_pOutNfa);

    // copy m_pInNfa into m_pOutNfa without epsilon transitions
    CopyNoEps ();

    // add transistive closure over m_EpsilonIw into the m_pOutNfa
    // and change epsilon transitions to the ordinary
    m_tr_closure.Process ();

    // AnyOther processing
    if (m_process_any) {
        m_pOutNfa->Prepare ();
        ProcessAnyOther ();
    }

    // remove epsilons
    RemoveEps ();

    // make all sets correct
    m_pOutNfa->Prepare ();

    // remove unreachable states
    m_remover.Process ();
}

}

