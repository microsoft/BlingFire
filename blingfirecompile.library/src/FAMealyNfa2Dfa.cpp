/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMealyNfa2Dfa.h"
#include "FARSNfaA.h"
#include "FAMealyNfaA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMealyNfa2Dfa::_TEqGraph::
    _TEqGraph (const int * pV, const int Count, const FANfa2EqPairs * pE) :
    m_pV (pV),
    m_Count (Count),
    m_pE (pE)
{}

const int FAMealyNfa2Dfa::_TEqGraph::
    GetVertices (const int ** ppV) const
{
    DebugLogAssert (0 < m_Count && m_pV);
    DebugLogAssert (ppV);

    *ppV = m_pV;
    return m_Count;
}

const int FAMealyNfa2Dfa::_TEqGraph::
    GetArcCount () const
{
    DebugLogAssert (m_pE);

    const int Count = m_pE->GetPairCount ();
    return Count;
}

void FAMealyNfa2Dfa::_TEqGraph::
    GetArc (const int Num, int * pFrom, int * pTo) const
{
    DebugLogAssert (m_pE);
    DebugLogAssert (pFrom && pTo);

    m_pE->GetPair (Num, pFrom, pTo);
}


FAMealyNfa2Dfa::FAMealyNfa2Dfa (FAAllocatorA * pAlloc) :
    m_pInNfa (NULL), 
    m_pInOws (NULL),
    m_pFsm1Ows (NULL),
    m_pFsm1Dfa (NULL),
    m_pFsm2Ows (NULL),
    m_pFsm2Dfa (NULL),
    m_eq_pairs (pAlloc),
    m_q2c (pAlloc),
    m_split_states (pAlloc),
    m_rev (pAlloc),
    m_rev_nfa (pAlloc),
    m_calc_fsm1 (pAlloc),
    m_calc_fsm2 (pAlloc),
    m_UseBiMachine (false)
{
    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_rev.SetOutNfa (&m_rev_nfa);

    m_calc_fsm1.SetInNfa (&m_rev_nfa);
    m_calc_fsm2.SetInNfa (&m_rev_nfa);
}


void FAMealyNfa2Dfa::Clear ()
{
    m_tmp.Clear ();
    m_tmp.Create ();
    m_eq_pairs.Clear ();
    m_q2c.Clear ();
    m_split_states.Clear ();
    m_rev.Clear ();
    m_rev_nfa.Clear ();
    m_calc_fsm1.Clear ();
    m_calc_fsm2.Clear ();
}


void FAMealyNfa2Dfa::SetOutFsm1 (FARSDfaA * pFsm1Dfa, FAMealyDfaA * pFsm1Ows)
{
    m_pFsm1Ows = pFsm1Ows;
    m_pFsm1Dfa = pFsm1Dfa;
}


void FAMealyNfa2Dfa::SetOutFsm2 (FARSDfaA * pFsm2Dfa, FAMealyDfaA * pFsm2Ows)
{
    m_pFsm2Ows = pFsm2Ows;
    m_pFsm2Dfa = pFsm2Dfa;
}


void FAMealyNfa2Dfa::SetUseBiMachine (const bool UseBiMachine)
{
    m_UseBiMachine = UseBiMachine;
}


void FAMealyNfa2Dfa::
    SetInNfa (const FARSNfaA * pInNfa, const FAMealyNfaA * pInOws)
{
    m_pInNfa = pInNfa;
    m_pInOws = pInOws;

    m_eq_pairs.SetNfa (m_pInNfa);
    m_rev.SetInNfa (m_pInNfa);

    m_calc_fsm2.SetSigma (pInOws);
}


void FAMealyNfa2Dfa::CreateDfa_triv ()
{
    DebugLogAssert (m_pInNfa && m_pInOws);
    DebugLogAssert (m_pFsm1Dfa && m_pFsm1Ows);

    const int * pStates;
    int Count;

    const int MaxState = m_pInNfa->GetMaxState ();
    m_pFsm1Dfa->SetMaxState (MaxState);

    const int MaxIw = m_pInNfa->GetMaxIw ();
    m_pFsm1Dfa->SetMaxIw (MaxIw);

    // make the output automaton ready    
    m_pFsm1Dfa->Create ();

    // set up initial states
    Count = m_pInNfa->GetInitials (&pStates);
    DebugLogAssert (1 == Count && pStates);
    m_pFsm1Dfa->SetInitial (*pStates);

    // set up final states
    Count = m_pInNfa->GetFinals (&pStates);
    DebugLogAssert (0 < Count && pStates);
    m_pFsm1Dfa->SetFinals (pStates, Count);

    // make iteration thru the states of input automaton
    for (int State = 0; State <= MaxState; ++State) {

        // get input weights
        const int * pIws;
        const int IwsCount = m_pInNfa->GetIWs (State, &pIws);

        // make iteration thru the input weights
        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            DebugLogAssert (pIws);
            const int Iw = pIws [iw_idx];

            // copy the destination states
            Count = m_pInNfa->GetDest (State, Iw, &pStates);
            DebugLogAssert (-1 == Count || 1 == Count);

            if (-1 != Count) {

                DebugLogAssert (pStates);
                const int Dst = *pStates;

                m_pFsm1Dfa->SetTransition (State, Iw, Dst);

                const int Ow = m_pInOws->GetOw (State, Iw, Dst);

                if (-1 != Ow) {
                    m_pFsm1Ows->SetOw (State, Iw, Ow);
                }
            }
        } // of for (int iw_idx = 0; ...
    } // of for (int State = 0; ...

    m_pFsm1Dfa->Prepare ();
}


void FAMealyNfa2Dfa::CalcEqPairs ()
{
    m_eq_pairs.Process ();
    // this should always be true unless FAIsDfa () has failed
    DebugLogAssert (0 < m_eq_pairs.GetPairCount ());
}


void FAMealyNfa2Dfa::ColorGraph ()
{
    const int MaxState = m_pInNfa->GetMaxState ();
    m_tmp.resize (MaxState + 1);
    for (int k = 0; k <= MaxState; ++k) {
        m_tmp [k] = k;
    }

    // we won't need g longer than m_q2c.Process () works
    _TEqGraph g (m_tmp.begin (), m_tmp.size (), &m_eq_pairs);
    m_q2c.SetGraph (&g);
    m_q2c.Process ();

    const int * pS2C;
    const int Size = m_q2c.GetColorMap (&pS2C);

    m_calc_fsm1.SetColorMap (pS2C, Size);
}


void FAMealyNfa2Dfa::CalcRevNfa ()
{
    m_rev.Process ();
}


void FAMealyNfa2Dfa::SplitStates ()
{
    const int * pS2C;
    const int Size = m_q2c.GetColorMap (&pS2C);

    m_split_states.SetNfa (&m_rev_nfa);
    m_split_states.SetS2C (pS2C, Size);

    m_split_states.Process ();

    const int * pS2C_new;
    const int Size_new = m_split_states.GetS2C (&pS2C_new);
    DebugLogAssert (Size_new == Size);

    m_calc_fsm1.SetColorMap (pS2C_new, Size_new);
}


void FAMealyNfa2Dfa::SetMaxClasses ()
{
    const int MaxState = m_rev_nfa.GetMaxState ();
    m_tmp.resize (MaxState + 1);
    for (int State = 0; State <= MaxState; ++State) {
        m_tmp [State] = State;
    }

    m_calc_fsm1.SetColorMap (m_tmp.begin (), m_tmp.size ());
}


void FAMealyNfa2Dfa::CalcFsm1 ()
{
    m_calc_fsm1.Process ();
}


void FAMealyNfa2Dfa::CalcFsm2 ()
{
    DebugLogAssert (m_pFsm2Dfa && m_pFsm2Ows);

    const FARSDfaA * pDfa1 = m_calc_fsm1.GetDfa ();
    DebugLogAssert (pDfa1);

    const FAMealyDfaA * pOws1 = m_calc_fsm1.GetSigma ();
    DebugLogAssert (pOws1);

    m_calc_fsm2.SetMealy1 (pDfa1, pOws1);
    m_calc_fsm2.Process ();

    const FARSNfaA * pNfa2 = m_calc_fsm2.GetOutNfa ();
    const FAMealyNfaA * pOws2 = m_calc_fsm2.GetSigma ();

    if (FAIsDfa (pNfa2)) {

        FACopyNfa2Dfa (m_pFsm2Dfa, pNfa2);

        const int * pIws;
        const int IwsCount = m_pFsm2Dfa->GetIWs (&pIws);
        const int MaxState = m_pFsm2Dfa->GetMaxState ();

        for (int i = 0; i <= MaxState; ++i) {
            for (int j = 0; j < IwsCount; ++j) {

                const int Iw = pIws [j];
                const int Dst = m_pFsm2Dfa->GetDest (i, Iw);

                if (-1 != Dst) {
                    const int Ow = pOws2->GetOw (i, Iw, Dst);
                    if (-1 != Ow) {
                        m_pFsm2Ows->SetOw (i, Iw, Ow);
                    }
                }

            } // of for (int j = 0; j < IwsCount; ...
        } // of for (int i = 0; i <= MaxState; ...
    }
}


void FAMealyNfa2Dfa::RmArcs1 ()
{
    DebugLogAssert (m_pFsm1Dfa && m_pFsm1Ows);
    DebugLogAssert (m_pFsm2Dfa);

    const FARSDfaA * pDfa1 = m_calc_fsm1.GetDfa ();
    DebugLogAssert (pDfa1);

    const FAMealyDfaA * pOws1 = m_calc_fsm1.GetSigma ();
    DebugLogAssert (pOws1);

    const FARSNfaA * pNfa2 = m_calc_fsm2.GetOutNfa ();
    m_tmp.resize (0);
    FAGetAlphabet (pNfa2, & m_tmp);
    const int * pIws2 = m_tmp.begin ();
    const int Iws2 = m_tmp.size ();
    DebugLogAssert (0 < Iws2 && pIws2);
    DebugLogAssert (FAIsSortUniqed (pIws2, Iws2));

    const int * pIws1;
    const int Iws1 = pDfa1->GetIWs (&pIws1);
    DebugLogAssert (0 < Iws1 && pIws1);
    DebugLogAssert (FAIsSortUniqed (pIws1, Iws1));

    const int MaxState = pDfa1->GetMaxState ();
    m_pFsm1Dfa->SetMaxState (MaxState);

    const int MaxIw = pDfa1->GetMaxIw ();
    m_pFsm1Dfa->SetMaxIw (MaxIw);

    m_pFsm1Dfa->Create ();

    const int Initial = pDfa1->GetInitial ();
    m_pFsm1Dfa->SetInitial (Initial);

    const int * pFinals;
    const int Finals = pDfa1->GetFinals (&pFinals);
    DebugLogAssert (0 < Finals && pFinals);
    m_pFsm1Dfa->SetFinals (pFinals, Finals);

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < Iws1; ++i) {

            const int Iw1 = pIws1 [i];
            const int Dst = pDfa1->GetDest (State, Iw1);

            if (-1 == Dst)
                continue;

            const int Ow = pOws1->GetOw (State, Iw1);
            DebugLogAssert (-1 != Ow);

            if (-1 != FAFind_log (pIws2, Iws2, Ow)) {

                m_pFsm1Dfa->SetTransition (State, Iw1, Dst);
                m_pFsm1Ows->SetOw (State, Iw1, Ow);
            }
        } // of for (int i = 0; ...
    } // of for (int State = 0; ...

    m_pFsm1Dfa->Prepare ();
}



const bool FAMealyNfa2Dfa::IsNonDet () const
{
    const FARSNfaA * pNfa2 = m_calc_fsm2.GetOutNfa ();
    return !FAIsDfa (pNfa2);
}

const FARSNfaA * FAMealyNfa2Dfa::GetNfa2 () const
{
    const FARSNfaA * pNfa2 = m_calc_fsm2.GetOutNfa ();
    return pNfa2;
}

const FAMealyNfaA * FAMealyNfa2Dfa::GetSigma2 () const
{
    const FAMealyNfaA * pOws2 = m_calc_fsm2.GetSigma ();
    return pOws2;
}


void FAMealyNfa2Dfa::Process ()
{
    DebugLogAssert (m_pInNfa && m_pInOws);

    // see whether no decomposition is needed
    if (FAIsDfa (m_pInNfa)) {

        CreateDfa_triv ();

    } else {

        if (!m_UseBiMachine) {

            CalcEqPairs ();
            ColorGraph ();
            CalcRevNfa ();
            SplitStates ();

        } else {

            CalcRevNfa ();
            SetMaxClasses ();
        }

        CalcFsm1 ();
        CalcFsm2 ();
        RmArcs1 ();
    }
}

}
