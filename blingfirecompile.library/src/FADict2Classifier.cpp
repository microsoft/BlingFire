/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADict2Classifier.h"
#include "FARSDfaA.h"
#include "FAState2OwsA.h"
#include "FAAllocatorA.h"
#include "FALimits.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FADict2Classifier::FADict2Classifier (FAAllocatorA * pAlloc):
    m_pInDfa (NULL),
    m_pInState2Ows (NULL),
    m_MinDepth (DefMinDepth),
    m_ExtendState2Ows (true),
    m_MergeType (0),
    m_ExtendFinals (false),
    m_pOutDfa (NULL),
    m_pOutState2Ows (NULL),
    m_Graph (pAlloc),
    m_sorter (pAlloc),
    m_sets (pAlloc),
    m_pOw2Freq (NULL),
    m_Ow2FreqCount (0),
    m_OwsBound (100),
    m_MinFreqBound (0),
    m_pAlloc (pAlloc)
{
    m_state2fins.SetAllocator (pAlloc);
    m_state2fins.Create ();

    m_state2fin.SetAllocator (pAlloc);
    m_state2fin.Create ();

    m_tmp_dsts.SetAllocator (pAlloc);
    m_tmp_dsts.Create ();

    m_tmp_arr.SetAllocator (pAlloc);
    m_tmp_arr.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_seen_states.SetAllocator (pAlloc);
    m_seen_states.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();

    m_sorter.SetGraph (&m_Graph);

    m_State2Depth.SetAllocator (pAlloc);
    m_State2Depth.Create ();
}


void FADict2Classifier::
    SetInMooreDfa (const FARSDfaA * pInDfa, const FAState2OwsA * pInState2Ows)
{
    m_pInDfa = pInDfa;
    m_pInState2Ows = pInState2Ows;
}


void FADict2Classifier::
    SetOutMooreDfa (FARSDfaA * pOutDfa, FAState2OwsA * pOutState2Ows)
{
    m_pOutDfa = pOutDfa;
    m_pOutState2Ows = pOutState2Ows;
}


void FADict2Classifier::
    SetOw2Freq (const int * pOw2Freq, const int Count)
{
    m_pOw2Freq = pOw2Freq;
    m_Ow2FreqCount = Count;
}


void FADict2Classifier::SetOwsBound (const int OwsBound)
{
    m_OwsBound = OwsBound;
}


void FADict2Classifier::SetMinDepth (const int MinDepth)
{
    m_MinDepth = MinDepth;
}


void FADict2Classifier::SetExtendState2Ows (const bool ExtendState2Ows)
{
    m_ExtendState2Ows = ExtendState2Ows;
}


void FADict2Classifier::SetOwsMergeType (const int MergeType)
{
    m_MergeType = MergeType;
}


void FADict2Classifier::SetExtendFinals (const bool ExtendFinals)
{
    m_ExtendFinals = ExtendFinals;
}


void FADict2Classifier::Prepare ()
{
    DebugLogAssert (m_pInDfa);
    const int MaxState = m_pInDfa->GetMaxState ();

    m_state2fins.resize (MaxState + 1);
    m_state2fin.resize (MaxState + 1);
    m_seen_states.resize (MaxState + 1);

    for (int i = 0; i <= MaxState; ++i) {
        m_state2fin [i] = -1;
        m_state2fins [i] = 0;
    }

    m_seen_states.set_bits (0, MaxState, false);
    m_sets.Clear ();

    if (m_pOw2Freq && 0 < m_Ow2FreqCount) {

        if (100 <= m_OwsBound) {

            m_MinFreqBound = 0;

        } else {

            m_tmp_arr.resize (m_Ow2FreqCount);
            int * pBegin = m_tmp_arr.begin ();
            memcpy (pBegin, m_pOw2Freq, sizeof (int) * m_Ow2FreqCount);

            std::sort (pBegin, pBegin + m_Ow2FreqCount);

            if (0 >= m_OwsBound) {

                m_MinFreqBound = pBegin [m_Ow2FreqCount - 1];

            } else {

                int Pos = m_Ow2FreqCount - 1 - \
                    ((m_OwsBound * m_Ow2FreqCount) / 100);

                if (0 > Pos) {
                    Pos = 0;
                } else if (m_Ow2FreqCount - 1 < Pos) {
                    Pos = m_Ow2FreqCount - 1;
                }

                m_MinFreqBound = pBegin [Pos];
            }
        }

    } else {

        m_OwsBound = 100;
        m_MinFreqBound = 0;
    }
}


void FADict2Classifier::BuildDstStates (const FARSDfaA * pDfa, const int State)
{
    DebugLogAssert (pDfa);

    m_tmp_dsts.resize (0);

    const int * pIws;
    const int IwCount = pDfa->GetIWs (&pIws);

    for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

        DebugLogAssert (pIws);
        const int Iw = pIws [iw_idx];
        const int DstState = pDfa->GetDest (State, Iw);

        if (-1 != DstState) {
            m_tmp_dsts.push_back (DstState);
        }
    }

    int * pBegin = m_tmp_dsts.begin ();
    const int Size = m_tmp_dsts.size ();

    if (!FAIsSortUniqed (pBegin, Size)) {
        const int NewSize = FASortUniq (pBegin, pBegin + Size);
        m_tmp_dsts.resize (NewSize);
    }
}


void FADict2Classifier::ProcessState (const int State)
{
    /// build destination states
    BuildDstStates (m_pInDfa, State);

    const int * pDstStates = m_tmp_dsts.begin ();
    const int DstStatesCount = m_tmp_dsts.size ();

    /// count final states reachable from State
    m_tmp_arr.resize (0);

    for (int i = 0; i < DstStatesCount; ++i) {

        DebugLogAssert (pDstStates);
        const int DstState = pDstStates [i];

        if (1 == m_state2fins [DstState]) {

            m_state2fins [State] = 1;
            return;

        } else if (-1 != m_state2fin [DstState]) {

            m_tmp_arr.push_back (m_state2fin [DstState]);
        }
    }

    const int * pOws;
    const int OwsSize = m_pInState2Ows->GetOws (State, &pOws);

    if (0 < OwsSize) {
        m_tmp_arr.push_back (State);
    }

    const int Size = FASortUniq (m_tmp_arr.begin (), m_tmp_arr.end ());
    m_tmp_arr.resize (Size);

    if (1 < Size) {

        m_state2fins [State] = 1;

    } else if (1 == Size) {

        const int FinalState = m_tmp_arr [0];
        m_state2fin [State] = FinalState;
    }
}


void FADict2Classifier::BuildOutput ()
{
    DebugLogAssert (m_pInDfa && m_pInState2Ows);
    DebugLogAssert (m_pOutDfa && m_pOutState2Ows);
    DebugLogAssert (m_state2fins.size () == m_state2fin.size ());
    DebugLogAssert (m_state2fins.size () == m_seen_states.size ());

    const int MaxState = m_pInDfa->GetMaxState ();
    const int MaxIw = m_pInDfa->GetMaxIw ();

    m_pOutDfa->SetMaxState (MaxState);
    m_pOutDfa->SetMaxIw (MaxIw);
    m_pOutDfa->Create ();

    const int * pIws;
    const int IwCount = m_pInDfa->GetIWs (&pIws);
    DebugLogAssert (pIws);

    const int InitialState = m_pInDfa->GetInitial ();
    m_pOutDfa->SetInitial (InitialState);

    m_stack.push_back (InitialState);
    m_seen_states.set_bit (InitialState, true);

    while (0 < m_stack.size ()) {

        const int State = m_stack [m_stack.size () - 1];
        m_stack.pop_back ();

        const int * pOws;
        const int OwsSize = m_pInState2Ows->GetOws (State, &pOws);

        if (0 < OwsSize) {
            m_finals.push_back (State);
            m_pOutState2Ows->SetOws (State, pOws, OwsSize);
        }

        for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            int DstState = m_pInDfa->GetDest (State, Iw);

            if (-1 != DstState) {
                // check whether DstState can be substituted with 
                // any _single_ final state
                if (0 == m_state2fins [DstState]) {
                    // get the resulting final state
                    const int FinalState = m_state2fin [DstState];
                    DebugLogAssert (-1 != FinalState);
                    // change the destination state
                    DstState = FinalState;
                }
                // add state to stack if has not been seen yet
                if (false == m_seen_states.get_bit (DstState)) {
                    m_stack.push_back (DstState);
                    m_seen_states.set_bit (DstState, true);
                }
                // setup a transition
                m_pOutDfa->SetTransition (State, Iw, DstState);
            }
        } // of for (int iw_idx = 0; ...
    } // of while (0 < m_stack.size ()) ...

    // setup final states
    const int Size = FASortUniq (m_finals.begin (), m_finals.end ());
    m_finals.resize (Size);
    m_pOutDfa->SetFinals (m_finals.begin (), Size);

    // make it ready to use
    m_pOutDfa->Prepare ();
}


void FADict2Classifier::CalcMaxDepth ()
{
    DebugLogAssert (m_pOutDfa && m_pOutState2Ows);

    int i, j;

    const int * pOrder;
    const int StateCount = m_sorter.GetTopoOrder (&pOrder);
    DebugLogAssert (StateCount == 1 + m_pOutDfa->GetMaxState ());

    m_State2Depth.resize (StateCount);
    int * pMapBegin = m_State2Depth.begin ();
    memset (pMapBegin, 0, StateCount * sizeof (int));

    // traverse states in topological order and build map
    for (i = 0; i < StateCount; ++i) {

        DebugLogAssert (pOrder);
        const int State = pOrder [i];
        const int Depth = m_State2Depth [State];

        // get set of destination states
        BuildDstStates (m_pOutDfa, State);

        // calc depth for destination states
        const int * pDstStates = m_tmp_dsts.begin ();
        const int DstStateCount = m_tmp_dsts.size ();

        for (j = 0; j < DstStateCount; ++j) {

            DebugLogAssert (pDstStates);
            const int DstState = pDstStates [j];
            const int DstDepth = m_State2Depth [DstState];

            if (DstDepth < 1 + Depth) {
                m_State2Depth [DstState] = 1 + Depth;
            }
        }
    }
}


void FADict2Classifier::CalcMinDepth ()
{
    DebugLogAssert (m_pOutDfa && m_pOutState2Ows);

    int i, j;

    const int * pOrder;
    const int StateCount = m_sorter.GetTopoOrder (&pOrder);
    if (0 == StateCount) {
        return;
    }

    DebugLogAssert (StateCount == 1 + m_pOutDfa->GetMaxState ());
    DebugLogAssert (pOrder);

    m_State2Depth.resize (StateCount);
    int * pState2Depth = m_State2Depth.begin ();
    for (i = 0; i < StateCount; ++i) {
        pState2Depth [i] = FALimits::MaxStateVal ;
    }
    DebugLogAssert (pOrder [0] == m_pOutDfa->GetInitial ()); // due to DFA property
    pState2Depth [pOrder [0]] = 0;

    // traverse states in topological order and build map
    for (i = 0; i < StateCount; ++i) {

        DebugLogAssert (pOrder);
        const int State = pOrder [i];
        const int Depth = m_State2Depth [State];

        // get set of destination states
        BuildDstStates (m_pOutDfa, State);

        // calc depth for destination states
        const int * pDstStates = m_tmp_dsts.begin ();
        const int DstStateCount = m_tmp_dsts.size ();

        for (j = 0; j < DstStateCount; ++j) {

            DebugLogAssert (pDstStates);
            const int DstState = pDstStates [j];
            const int DstDepth = m_State2Depth [DstState];

            if (DstDepth > 1 + Depth) {
                m_State2Depth [DstState] = 1 + Depth;
            }
        }
    }
}


const int FADict2Classifier::FilterOws (const int ** ppOws)
{
    DebugLogAssert (*ppOws);

    const int * pOws = NULL;
    const int InCount = m_sets.GetRes (&pOws, 0);

    if (0 >= InCount) {
        return 0;
    }

    if (0 >= m_MinFreqBound) {

        *ppOws = pOws;
        return InCount;

    } else {

        m_tmp_arr.resize (InCount);
        int * pBegin = m_tmp_arr.begin ();

        memcpy (pBegin, pOws, sizeof (int) * InCount);
        std::sort (pBegin, pBegin + InCount, FAIdxCmp_b2s (m_pOw2Freq));

        int OutCount;

        for (OutCount = 0; OutCount < InCount; ++OutCount) {

            const int Ow = pBegin [OutCount];
            DebugLogAssert (0 <= Ow && m_Ow2FreqCount > Ow);

            if (m_pOw2Freq [Ow] < m_MinFreqBound) {
                break;
            }
        }
        if (1 < OutCount) {
            std::sort (pBegin, pBegin + OutCount);
        }

        *ppOws = pBegin;
        return OutCount;
    }
}


void FADict2Classifier::ExtendFsm ()
{
    DebugLogAssert (m_pOutDfa && m_pOutState2Ows);

    int i, j;

    const int * pOrder;
    const int StateCount = m_sorter.GetTopoOrder (&pOrder);
    DebugLogAssert (StateCount == 1 + m_pOutDfa->GetMaxState ());

    // traverse states in reversed topological order and extend reactions
    for (i = StateCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int State = pOrder [i];
        const int Depth = m_State2Depth [State];

        // see whether the state is deep enough
        if (Depth < m_MinDepth) {
            continue;
        }

        const int * pOws;
        const int OwsCount = m_pOutState2Ows->GetOws (State, &pOws);

        // check whether reaction is already exist
        if (0 < OwsCount) {
            continue;
        }

        // get set of destination states
        BuildDstStates (m_pOutDfa, State);

        // merge reactions of destination states, if any
        const int * pDstStates = m_tmp_dsts.begin ();
        const int DstStateCount = m_tmp_dsts.size ();

        if (0 < DstStateCount) {

            DebugLogAssert (pDstStates);
            const int Dst = pDstStates [0];

            const int * pDstOws;
            const int DstOwsCount = m_pOutState2Ows->GetOws (Dst, &pDstOws);
            DebugLogAssert (0 <= DstOwsCount); // the set can be empty

            m_sets.SetRes (pDstOws, DstOwsCount, 0);
        }
        for (j = 1; j < DstStateCount; ++j) {

            DebugLogAssert (pDstStates);
            const int DstState = pDstStates [j];

            const int * pDstOws;
            const int DstOwsCount = m_pOutState2Ows->GetOws (DstState, &pDstOws);
            DebugLogAssert (0 <= DstOwsCount); // the set can be empty

            if (1 == m_MergeType) {
                m_sets.SelfIntersect (pDstOws, DstOwsCount, 0);
            } else {
                m_sets.SelfUnion (pDstOws, DstOwsCount, 0);
            }
        }

        // apply filter to the resulting Ows
        const int * pRes;
        const int ResSize = FilterOws (&pRes);
        DebugLogAssert (FAIsSortUniqed (pRes, ResSize));
        m_sets.SetRes (NULL, 0, 0);

        // setup the reaction
        m_pOutState2Ows->SetOws (State, pRes, ResSize);

        if (m_ExtendFinals) {
            m_finals.push_back (State);
        }

    } // of for (i = StateCount - 1; i >= 0; --i) ...

    if (m_ExtendFinals) {

        const int Size = FASortUniq (m_finals.begin (), m_finals.end ());
        m_finals.resize (Size);

        // setup final states
        m_pOutDfa->SetFinals (m_finals.begin (), Size);

        m_pOutDfa->Prepare ();
    }
}


void FADict2Classifier::Process ()
{
    DebugLogAssert (m_pInDfa && m_pInState2Ows);
    DebugLogAssert (m_pOutDfa && m_pOutState2Ows);

    Prepare ();

    // make topological sorting of the input automaton
    m_Graph.SetDfa (m_pInDfa);
    m_sorter.Process ();

    // traverse states in the reverse topological order and build maps
    const int * pOrder;
    const int StateCount = m_sorter.GetTopoOrder (&pOrder);

    for (int i = 1; i <= StateCount; ++i) {

        DebugLogAssert (pOrder);
        const int State = pOrder [StateCount - i];
        ProcessState (State);
    }

    BuildOutput ();

    if (m_ExtendState2Ows) {

        // make topological sorting of the output automaton
        m_Graph.SetDfa (m_pOutDfa);
        m_sorter.Process ();
        // calc State -> Depth
        CalcMaxDepth ();
        //// CalcMinDepth (); // experiment
        // extend reactions and final states, if needed
        ExtendFsm ();
    }
}

}
