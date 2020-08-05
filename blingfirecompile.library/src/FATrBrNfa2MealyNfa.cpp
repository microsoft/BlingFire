/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATrBrNfa2MealyNfa.h"
#include "FARSNfaA.h"
#include "FAMealyNfaA.h"
#include "FAFsmConst.h"
#include "FAChain2NumA.h"
#include "FAUtils.h"

namespace BlingFire
{


FATrBrNfa2MealyNfa::FATrBrNfa2MealyNfa (FAAllocatorA * pAlloc) :
    m_pInNfa (NULL),
    m_pOutNfa (NULL),
    m_pOutOws (NULL),
    m_TrBrBaseIw (0),
    m_TrBrMaxIw (0),
    m_EpsilonIw (FAFsmConst::IW_EPSILON),
    m_erm (pAlloc),
    m_g (pAlloc),
    m_tmp_nfa (pAlloc),
    m_tmp_ows1 (pAlloc),
    m_tmp_ows2 (pAlloc)
{
    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_g.SetEpsilonIw (m_EpsilonIw);
    m_g.SetInNfa (&m_tmp_nfa);
    m_g.SetInMealy (&m_tmp_ows1);
    m_g.SetOutMealy (&m_tmp_ows2);

    m_erm.SetEpsilonGraph (&m_g);
    m_erm.SetEpsilonIw (m_EpsilonIw);
    m_erm.SetInNfa (&m_tmp_nfa);
    m_erm.SetOutNfa (m_pOutNfa);

    m_ow2trbrs.SetAllocator (pAlloc);
}


const FAMultiMapA * FATrBrNfa2MealyNfa::GetOw2TrBrMap () const
{
    return & m_ow2trbrs;
}


void FATrBrNfa2MealyNfa::SetTrBrBaseIw (const int TrBrBaseIw)
{
    m_TrBrBaseIw = TrBrBaseIw;
}


void FATrBrNfa2MealyNfa::SetTrBrMaxIw (const int TrBrMaxIw)
{
    m_TrBrMaxIw = TrBrMaxIw;
}


void FATrBrNfa2MealyNfa::SetEpsilonIw (const int EpsilonIw)
{
    m_EpsilonIw = EpsilonIw;

    m_g.SetEpsilonIw (m_EpsilonIw);
    m_erm.SetEpsilonIw (m_EpsilonIw);
}


void FATrBrNfa2MealyNfa::SetInNfa (const FARSNfaA * pInNfa)
{
    m_pInNfa = pInNfa;
}


void FATrBrNfa2MealyNfa::SetOutNfa (FARSNfaA * pOutNfa, FAMealyNfaA * pOutOws)
{
    m_pOutNfa = pOutNfa;
    m_pOutOws = pOutOws;

    m_g.SetOutNfa (m_pOutNfa);
    m_erm.SetOutNfa (m_pOutNfa);
}


inline void FATrBrNfa2MealyNfa::BuildEpsilonMealy ()
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (m_pOutNfa);

    int i;
    const int * pStates;
    int Count;

    const int MaxState = m_pInNfa->GetMaxState ();
    const int MaxIw = m_pInNfa->GetMaxIw ();

    int NewMaxIw = -1;
    const int NewInitial = MaxState + 1;
    const int NewFinal = MaxState + 2;

    m_tmp_nfa.SetMaxState (MaxState + 2);
    m_tmp_nfa.SetMaxIw (MaxIw);

    // make the output automaton ready    
    m_tmp_nfa.Create ();

    // set up initial states
    Count = m_pInNfa->GetInitials (&pStates);
    if (0 == Count)
        return;

    for (i = 0; i < Count; ++i) {
        DebugLogAssert (pStates);
        const int State = pStates [i];
        m_tmp_nfa.SetTransition (NewInitial, FAFsmConst::IW_EOS, &State, 1);
    }

    m_tmp_nfa.SetInitials (&NewInitial, 1);
    m_tmp_nfa.SetFinals (&NewFinal, 1);

    // make iteration thru the states of input automaton
    for (int State = 0; State <= MaxState; ++State) {

        // get input weights
        const int * pIws;
        const int IwsCount = m_pInNfa->GetIWs (State, &pIws);

        m_tmp.resize (0);

        // make iteration thru the input weights
        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            DebugLogAssert (pIws);
            const int Iw = pIws [iw_idx];

            // get the destination states
            Count = m_pInNfa->GetDest (State, Iw, &pStates);

            // see whether it is a trbr transition
            if (Iw >= m_TrBrBaseIw && Iw <= m_TrBrMaxIw) {

                for (i = 0; i < Count; ++i) {

                    DebugLogAssert (pStates);
                    const int Dst = pStates [i];
                    const int Ow = Iw - m_TrBrBaseIw;

                    m_tmp_ows1.SetOw (State, m_EpsilonIw, Dst, Ow);
                    m_tmp.push_back (Dst);
                }

            } else {

                if (-1 != Count) {
                    m_tmp_nfa.SetTransition (State, Iw, pStates, Count);
                }
                if (NewMaxIw < Iw) {
                    NewMaxIw = Iw;
                }
            }

        } // of for (int iw_idx = 0; ...

        // setup epsilon transition in Mealy NFA, if needed

        const int OldSize = m_tmp.size ();

        if (0 < OldSize) {

            const int NewSize = FASortUniq (m_tmp.begin (), m_tmp.end ());
            m_tmp.resize (NewSize);

            const int * pEpsDsts = m_tmp.begin ();
            m_tmp_nfa.SetTransition (State, m_EpsilonIw, pEpsDsts, NewSize);
        }

    } // of for (int State = 0; ...

    DebugLogAssert (-1 != NewMaxIw);
    m_tmp_nfa.SetMaxIw (NewMaxIw);

    // set up transitions from old final states into NewFinal by IW_EOS
    Count = m_pInNfa->GetFinals (&pStates);

    for (i = 0; i < Count; ++i) {
        DebugLogAssert (pStates);
        const int State = pStates [i];
        m_tmp_nfa.SetTransition (State, FAFsmConst::IW_EOS, NewFinal);
    }

    m_tmp_nfa.Prepare ();
}


inline void FATrBrNfa2MealyNfa::RemoveEpsilons ()
{
    m_erm.Process ();
}


inline void FATrBrNfa2MealyNfa::RemapOws ()
{
    // keeps mapping old-Ow --> new-Ow
    m_tmp.resize (0);
    int OwCount = 0;

    const FAChain2NumA * pOw2TrBrs = m_g.GetOwsMap ();
    DebugLogAssert (pOw2TrBrs);

    const int MaxState = m_pOutNfa->GetMaxState ();

    // make iteration thru the states of input automaton
    for (int State = 0; State <= MaxState; ++State) {

        // get input weights
        const int * pIws;
        const int IwsCount = m_pOutNfa->GetIWs (State, &pIws);

        // make iteration thru the input weights
        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            DebugLogAssert (pIws);
            const int Iw = pIws [iw_idx];

            // get the destination states
            const int * pDst;
            const int Count = m_pOutNfa->GetDest (State, Iw, &pDst);

            for (int j = 0; j < Count; ++j) {

                DebugLogAssert (pDst);
                const int Dst = pDst [j];
                const int Ow = m_tmp_ows2.GetOw (State, Iw, Dst);

                if (-1 != Ow) {

                    // ensure Ow -> NewOw defined
                    const int OldSize = m_tmp.size ();
                    if (OldSize <= Ow) {
                        m_tmp.resize (Ow + 1);
                        const int NewCount = (Ow - OldSize) + 1;
                        int * pNew = m_tmp.begin () + OldSize;
                        memset (pNew, -1, sizeof (int) * NewCount);
                    }

                    // get NewOW
                    int NewOw = m_tmp [Ow];

                    // see whether it is not known
                    if (-1 == NewOw) {

                        NewOw = OwCount++;
                        m_tmp [Ow] = NewOw;

                        const int * pTrBrs;
                        const int TrBrCount = pOw2TrBrs->GetChain (Ow, &pTrBrs);
                        DebugLogAssert (0 < TrBrCount && pTrBrs);

                        m_ow2trbrs.Set (NewOw, pTrBrs, TrBrCount);
                    }

                    // setup m_pOutOws entry
                    m_pOutOws->SetOw (State, Iw, Dst, NewOw);
                }

            } // of for (int j = 0; ...

        } // of for (int iw_idx = 0; ...

    } // of for (int State = 0; ...
}


inline void FATrBrNfa2MealyNfa::Clear ()
{
    m_tmp_ows1.Clear ();
    m_tmp_ows2.Clear ();
    m_tmp_nfa.Clear ();
    m_g.Clear ();
    m_tmp.Clear ();
    m_tmp.Create ();
}


void FATrBrNfa2MealyNfa::Process ()
{
    DebugLogAssert (m_pInNfa && m_pOutNfa && m_pOutOws);

    m_ow2trbrs.Clear ();

    BuildEpsilonMealy ();

    if (m_pOutNfa == m_pInNfa) {
        m_pOutNfa->Clear ();
    }

    RemoveEpsilons ();

    RemapOws ();

    Clear ();
}

}

