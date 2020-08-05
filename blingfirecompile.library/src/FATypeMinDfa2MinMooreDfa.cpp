/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATypeMinDfa2MinMooreDfa.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAMultiMapA.h"
#include "FAAllocatorA.h"
#include "FAUtils.h"

namespace BlingFire
{


FATypeMinDfa2MinMooreDfa::FATypeMinDfa2MinMooreDfa (FAAllocatorA * pAlloc) :
    m_InitialTypeIw (0),
    m_pInDfa (NULL),
    m_pOutDfa (NULL),
    m_pState2Ow (NULL),
    m_pType2Ows (NULL),
    m_MaxOw (0),
    m_InitialOw (0)
{
    m_set2id.SetAllocator (pAlloc);
    m_chain.SetAllocator (pAlloc);
    m_chain.Create ();
}


void FATypeMinDfa2MinMooreDfa::SetType2OwsMap (FAMultiMapA * pType2Ows)
{
    m_pType2Ows = pType2Ows;
}


void FATypeMinDfa2MinMooreDfa::SetInitialTypeIw (const int InitialTypeIw)
{
    m_InitialTypeIw = InitialTypeIw;
}


void FATypeMinDfa2MinMooreDfa::SetInitialOw (const int Ow)
{
    m_InitialOw = Ow;
}


void FATypeMinDfa2MinMooreDfa::SetInDfa (const FARSDfaA * pInDfa)
{
    m_pInDfa = pInDfa;
}


void FATypeMinDfa2MinMooreDfa::SetOutMooreDfa (FARSDfaA * pOutDfa, FAState2OwA * pState2Ow)
{
    m_pOutDfa = pOutDfa;
    m_pState2Ow = pState2Ow;
}


void FATypeMinDfa2MinMooreDfa::Clear ()
{
    m_MaxOw = m_InitialOw;
    m_set2id.Clear ();
}


const int FATypeMinDfa2MinMooreDfa::BuildOw ()
{
    int Ow = m_MaxOw;

    const int * pOw = m_set2id.Get (m_chain.begin (), m_chain.size ());

    if (NULL != pOw) {

        Ow = *pOw;

    } else {

        m_set2id.Add (m_chain.begin (), m_chain.size (), Ow);
        m_MaxOw++;
    }

    return Ow;
}


void FATypeMinDfa2MinMooreDfa::UpdateType2Ows (const int Ow)
{
    DebugLogAssert (m_pType2Ows);

    /// make iteration thru the m_chain
    const int ChainSize = m_chain.size ();
    for (int i = 0; i < ChainSize; ++i) {

        const int TypeIw = m_chain [i];

        /// +1 makes TypeNums to be 1-based
        const int TypeNum = TypeIw - m_InitialTypeIw + 1;
        DebugLogAssert (0 < TypeNum);

        m_pType2Ows->Add (TypeNum, Ow);
    }
}


void FATypeMinDfa2MinMooreDfa::ProcessState (const int State)
{
    DebugLogAssert (m_pInDfa);
    DebugLogAssert (m_pOutDfa);
    DebugLogAssert (m_pState2Ow);

    m_chain.resize (0);

    const int * pIws;
    const int IwCount = m_pInDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwCount && pIws);
    DebugLogAssert (true == FAIsSortUniqed (pIws, IwCount));

    /// copy non-type transitions and build the chain
    for (int i = 0; i < IwCount; ++i) {

        const int Iw = pIws [i];
        const int DstState = m_pInDfa->GetDest (State, Iw);

        if (-1 == DstState)
            continue;

        // just copy the transition or build the set of types == chain
        if (Iw < m_InitialTypeIw)
            m_pOutDfa->SetTransition (State, Iw, DstState);
        else
            m_chain.push_back (Iw);

    } // of for (int i = 0; ...

    /// check whether we should add an Ow
    if (0 < m_chain.size ()) {

        /// this situation may happen only in the final state
        DebugLogAssert (m_pInDfa->IsFinal (State));
        /// build new Ow, or find existing
        const int Ow = BuildOw ();
        /// set up output function for Moore automaton
        m_pState2Ow->SetOw (State, Ow);
        /// update Type -> { Ow } mapping
        UpdateType2Ows (Ow);
    }
}


void FATypeMinDfa2MinMooreDfa::Process ()
{
    DebugLogAssert (m_pInDfa);
    DebugLogAssert (m_pOutDfa);
    DebugLogAssert (m_pState2Ow);
    DebugLogAssert (m_pType2Ows);

    FATypeMinDfa2MinMooreDfa::Clear ();

    // prepare the output Dfa
    const int MaxState = m_pInDfa->GetMaxState ();
    m_pOutDfa->SetMaxState (MaxState);
    const int MaxIw = m_pInDfa->GetMaxIw ();
    m_pOutDfa->SetMaxIw (MaxIw);
    m_pOutDfa->Create ();

    // copy initial state
    const int InitialState = m_pInDfa->GetInitial ();
    m_pOutDfa->SetInitial (InitialState);

    // copy final states
    const int * pFinals;
    const int FinalsCount = m_pInDfa->GetFinals (&pFinals);
    m_pOutDfa->SetFinals (pFinals, FinalsCount);

    // process intermediate states
    for (int State = 0; State <= MaxState; ++State) {

        ProcessState (State);
    }
    // make the output automaton ready
    m_pOutDfa->Prepare ();
}

}
