/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSNfa2RevNfa.h"
#include "FARSNfaA.h"
#include "FAUtils.h"
#include "FAFsmConst.h"
#include "FASetUtils.h"

namespace BlingFire
{


FARSNfa2RevNfa::FARSNfa2RevNfa (FAAllocatorA * pAlloc) :
  m_pInNfa (NULL),
  m_pOutNfa (NULL),
  m_set_utils (pAlloc),
  m_any_iw (-1),
  m_process_any (false),
  m_tmp_nfa (pAlloc)
{
  m_sets.SetAllocator (pAlloc);
  m_sets.Create ();

  m_set_sizes.SetAllocator (pAlloc);
  m_set_sizes.Create ();

  m_iws.SetAllocator (pAlloc);
  m_iws.Create ();
}


void FARSNfa2RevNfa::Clear ()
{
  m_tmp_nfa.Clear ();
  m_set_utils.Clear ();
  m_sets.Clear ();
  m_sets.Create ();
  m_set_sizes.Clear ();
  m_set_sizes.Create ();
  m_iws.Clear ();
  m_iws.Create ();
}


void FARSNfa2RevNfa::SetAnyIw (const int AnyIw)
{
  m_any_iw = AnyIw;
  m_process_any = true;
}


void FARSNfa2RevNfa::SetInNfa (const FARSNfaA * pInNfa)
{
  m_pInNfa = pInNfa;
}


void FARSNfa2RevNfa::SetOutNfa (FARSNfaA * pOutNfa)
{
  m_pOutNfa = pOutNfa;
}


void FARSNfa2RevNfa::RevTrivial (FARSNfaA * pOutNfa)
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (pOutNfa);

    const int * pStates;
    int Count;

    // make the output automaton ready
    const int MaxState = m_pInNfa->GetMaxState ();
    pOutNfa->SetMaxState (MaxState);

    const int MaxIw = m_pInNfa->GetMaxIw ();
    pOutNfa->SetMaxIw (MaxIw);

    pOutNfa->Create ();

    // convert input final states into output initial one
    Count = m_pInNfa->GetFinals (&pStates);

    if (0 < Count) {

        pOutNfa->SetInitials (pStates, Count);

        // convert input initial states into output final one
        Count = m_pInNfa->GetInitials (&pStates);

        if (0 < Count) {

            pOutNfa->SetFinals (pStates, Count);

            // convert transitions

            const int * pIws;
            int IwCount;

            for (int State = 0; State <= MaxState; ++State) {

                IwCount = m_pInNfa->GetIWs (State, &pIws);

                for (int i = 0; i < IwCount; ++i) {

                    DebugLogAssert (pIws);

                    const int Iw = pIws [i];

                    Count = m_pInNfa->GetDest (State, Iw, &pStates);

                    for (int j = 0; j < Count; ++j) {

                        DebugLogAssert (pStates);

                        const int DstState = pStates [j];

                        // set up a new transition
                        pOutNfa->SetTransition (DstState, Iw, State);

                    } // of for (int j = 0

                } // of for (int i = 0

            } // of for (int State = 0

        } // of if
    } // of if
}


/// The algorithm works as follows:
///
/// foreach q \in RevNfa.Q, s.t.
///   0 < |Dst|, Dst = {d_1, ..., d_n}, where (q, ANY, d) \in RevNfa.E do
///
///   // calculate the local alphabet of the state q at RevNfa
///   IWs = RevNfa.q.iws
///   foreach d \in Dst do
///     IWs = IWs U Nfa.d.iws;
///
///   // build additional transitions
///   foreach d \in Dst do {
///
///     IWs_d = Nfa.d.iws
///
///     foreach iw \in IWs - IWs_d
///       RevNfa.E.push (q, iw, d)
///   }
///
///   // remove some
///   foreach iw \in IWs - RevNfa.q.iws do
///     RevNfa.E.push (q, iw, DeadState)
///
void FARSNfa2RevNfa::RevAny ()
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (m_pOutNfa);

    int i;
    int j;

    // make the iteration thru all the states of the output NFA
    // process those which have ANY-transitions

    const int MaxState = m_tmp_nfa.GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        // see whether there are ANY-transitions

        const int * pDstStates;
        const int Count = m_tmp_nfa.GetDest (State, m_any_iw, &pDstStates);

        // proceed if they are
        if (0 < Count) {

            DebugLogAssert (pDstStates);

            // Step 1. calculate the local alphabet of the state q at RevNfa

            m_sets.resize (Count + 1);
            m_set_sizes.resize (Count + 1);

            const int * pIws;
            int IwsSize;

            for (i = 0; i < Count; ++i) {

                const int DstState = pDstStates [i];

                IwsSize = m_pInNfa->GetIWs (DstState, &pIws);
                m_sets [i] = pIws;
                m_set_sizes [i] = IwsSize;
            }

            IwsSize = m_tmp_nfa.GetIWs (State, &pIws);
            m_sets [Count] = pIws;
            m_set_sizes [Count] = IwsSize;

            // copy State's Iws into the m_iws (we'll need them further)
            m_iws.resize (IwsSize);
            memcpy (m_iws.begin (), pIws, IwsSize * sizeof (int));

            // calc the union
            m_set_utils.UnionN (m_sets.begin (), m_set_sizes.begin (), Count + 1, 2);
            const int * pAlphabet;
            const int AlphabetSize = m_set_utils.GetRes (&pAlphabet, 2);

            // Step 2. build additional transitions

            for (i = 0; i < Count; ++i) {

                const int DstState = pDstStates [i];

                // get ANY-definition set from the input NFA
                const int * pD;
                const int DSize = m_pInNfa->GetIWs (DstState, &pD);

                // calc the difference : pAlphabet - pD
                m_set_utils.Difference (pAlphabet, AlphabetSize, 0,
                                        pD, DSize, 1);

                const int * pDiff;
                const int DiffSize = m_set_utils.GetRes (&pDiff, 0);

                for (j = 0; j < DiffSize; ++j) {

                    DebugLogAssert (pDiff);

                    const int Iw = pDiff [j];

                    // add transition
                    m_pOutNfa->SetTransition (State, Iw, DstState);

                    // collect State's Iws
                    m_iws.push_back (Iw);
                }
            }

            // Step 3. bulid transitions to the dead state

            // calc State's Iws set (remove duplicates and sort Iws)
            const int NewSize = FASortUniq (m_iws.begin (), m_iws.end ());
            m_iws.resize (NewSize);

            // calc the difference : pAlphabet - m_iws
            m_set_utils.Difference (pAlphabet, AlphabetSize, 0,
                                    m_iws.begin (), m_iws.size (), 1);

            const int * pDiff;
            const int DiffSize = m_set_utils.GetRes (&pDiff, 0);

            for (j = 0; j < DiffSize; ++j) {

                DebugLogAssert (pDiff);

                const int Iw = pDiff [j];
                m_pOutNfa->SetTransition (State, Iw, FAFsmConst::NFA_DEAD_STATE);
            }

        } // of if (0 < Count) ...

    } // of for (int State = 0; ...
}


void FARSNfa2RevNfa::Process ()
{
    if (false == m_process_any) {

        RevTrivial (m_pOutNfa);
        m_pOutNfa->Prepare ();

    } else {

        RevTrivial (&m_tmp_nfa);
        m_tmp_nfa.Prepare ();

        RevTrivial (m_pOutNfa);
        RevAny ();
        m_pOutNfa->Prepare ();
    }

    FARSNfa2RevNfa::Clear ();
}

}
