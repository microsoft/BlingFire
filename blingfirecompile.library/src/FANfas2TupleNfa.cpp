/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfas2TupleNfa.h"
#include "FARSNfaA.h"
#include "FAException.h"
#include "FAUtils.h"

namespace BlingFire
{


FANfas2TupleNfa::FANfas2TupleNfa (FAAllocatorA * pAlloc) :
    m_pNfaArr (NULL),
    m_Count (0),
    m_pOutNfa (NULL),
    m_IgnoreBase (-1),
    m_IgnoreMax (-1)
{
    m_NfaArr.SetAllocator (pAlloc);
    m_NfaArr.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_state2state.SetAllocator (pAlloc);
    m_state2state.SetEncoder (&m_encoder);
}


void FANfas2TupleNfa::SetIgnoreBase (const int IgnoreBase)
{
    m_IgnoreBase = IgnoreBase;
}


void FANfas2TupleNfa::SetIgnoreMax (const int IgnoreMax)
{
    m_IgnoreMax = IgnoreMax;
}


void FANfas2TupleNfa::AddNfa (const FARSNfaA * pNfa)
{
    m_NfaArr.push_back (pNfa);

    m_pNfaArr = m_NfaArr.begin ();
    m_Count = m_NfaArr.size ();
}


void FANfas2TupleNfa::Clear ()
{
    m_NfaArr.resize (0);
    m_pNfaArr = NULL;
    m_Count = 0;
}


void FANfas2TupleNfa::SetOutNfa (FARSNfaA * pNfa)
{
    m_pOutNfa = pNfa;
}


inline const int FANfas2TupleNfa::GetMaxState () const
{
    DebugLogAssert (m_pNfaArr && 0 < m_Count);

    const FARSNfaA * pNfa = m_pNfaArr [0];
    DebugLogAssert (pNfa);

    const int MaxState = pNfa->GetMaxState ();

    for (int i = 1; i < m_Count; ++i) {

        pNfa = m_pNfaArr [i];
        DebugLogAssert (pNfa);

        const int CurrMaxState = pNfa->GetMaxState ();

        if (MaxState != CurrMaxState) {
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }
    }

    return MaxState;
}


inline const int FANfas2TupleNfa::GetMaxIw () const
{
    DebugLogAssert (m_pNfaArr && 0 < m_Count);

    const FARSNfaA * pNfa = m_pNfaArr [0];
    DebugLogAssert (pNfa);

    int MaxIw = pNfa->GetMaxIw ();

    for (int i = 1; i < m_Count; ++i) {

        pNfa = m_pNfaArr [i];
        DebugLogAssert (pNfa);

        const int CurrMaxIw = pNfa->GetMaxIw ();

        if (MaxIw < CurrMaxIw) {
            MaxIw = CurrMaxIw;
        }
    }

    return MaxIw;
}


inline const int FANfas2TupleNfa::GetInitials (const int ** ppStates) const
{
    DebugLogAssert (ppStates);
    DebugLogAssert (m_pNfaArr && 0 < m_Count);

    const FARSNfaA * pNfa = m_pNfaArr [0];
    DebugLogAssert (pNfa);

    const int Count = pNfa->GetInitials (ppStates);

    for (int i = 1; i < m_Count; ++i) {

        pNfa = m_pNfaArr [i];
        DebugLogAssert (pNfa);

        const int * pCurrStates;
        const int CurrCount = pNfa->GetInitials (&pCurrStates);

        if (Count != CurrCount || \
            0 != memcmp (*ppStates, pCurrStates, sizeof (int) * Count)) {
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }
    }

    return Count;
}


inline const int FANfas2TupleNfa::GetFinals (const int ** ppStates) const
{
    DebugLogAssert (ppStates);
    DebugLogAssert (m_pNfaArr && 0 < m_Count);

    const FARSNfaA * pNfa = m_pNfaArr [0];
    DebugLogAssert (pNfa);

    const int Count = pNfa->GetFinals (ppStates);

    for (int i = 1; i < m_Count; ++i) {

        pNfa = m_pNfaArr [i];
        DebugLogAssert (pNfa);

        const int * pCurrStates;
        const int CurrCount = pNfa->GetFinals (&pCurrStates);

        if (Count != CurrCount || \
            0 != memcmp (*ppStates, pCurrStates, sizeof (int) * Count)) {
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }
    }

    return Count;
}


inline const bool FANfas2TupleNfa::Ignore (const int Iw) const
{
    if (-1 == m_IgnoreBase && -1 == m_IgnoreMax) {

        return false;

    } else if (-1 == m_IgnoreBase) {

        DebugLogAssert (-1 != m_IgnoreMax);
        return Iw <= m_IgnoreMax;

    } else if (-1 == m_IgnoreMax) {

        DebugLogAssert (-1 != m_IgnoreBase);
        return Iw >= m_IgnoreBase;

    } else {

        DebugLogAssert (-1 != m_IgnoreBase && -1 != m_IgnoreMax);
        return Iw >= m_IgnoreBase && Iw <= m_IgnoreMax;
    }
}


inline void FANfas2TupleNfa::RemapStates (
        const int * pStates, 
        const int Count, 
        const int ** ppOutStates
    )
{
    DebugLogAssert (ppOutStates);
    DebugLogAssert (0 < Count && pStates);

    m_tmp.resize (Count);
    int * pOutStates = m_tmp.begin ();

    for (int i = 0; i < Count; ++i) {

        const int State = pStates [i];

        const int * pNewState = m_state2state.Get (&State, 1);
        DebugLogAssert (pNewState);

        pOutStates [i] = *pNewState;
    }

    *ppOutStates = pOutStates;
}


inline void FANfas2TupleNfa::RemapStates2 (
        const int FromState,
        const int Idx,
        const int * pStates, 
        const int Count, 
        const int ** ppOutStates
    )
{
    DebugLogAssert (ppOutStates);
    DebugLogAssert (0 < Count && pStates);
    DebugLogAssert (0 <= Idx && Idx < m_Count);

    int Triplet [3];
    Triplet [0] = FromState;
    Triplet [2] = Idx;

    m_tmp.resize (Count);
    int * pOutStates = m_tmp.begin ();

    for (int i = 0; i < Count; ++i) {

        const int State = pStates [i];
        Triplet [1] = State;

        const int * pNewState = m_state2state.Get (Triplet, 3);
        DebugLogAssert (pNewState);

        pOutStates [i] = *pNewState;
    }

    *ppOutStates = pOutStates;
}


void FANfas2TupleNfa::AddTransitions (const int Idx, const int State)
{
    DebugLogAssert (m_pOutNfa);
    DebugLogAssert (0 <= Idx && Idx < m_Count);

    const FARSNfaA * pNfa = m_pNfaArr [Idx];
    DebugLogAssert (pNfa);

    const int * pIWs;
    const int IwCount = pNfa->GetIWs (State, &pIWs);

    // first tuple constituent
    if (0 == Idx) {

        const int * pNewState = m_state2state.Get (&State, 1);
        DebugLogAssert (pNewState);
        const int NewState = *pNewState;

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIWs);
            const int Iw = pIWs [i];

            if (Ignore (Iw)) {
                continue;
            }

            const int * pDsts;
            const int DstCount = pNfa->GetDest (State, Iw, &pDsts);

            const int * pNewDsts;
            RemapStates2 (State, Idx + 1, pDsts, DstCount, &pNewDsts);

            m_pOutNfa->SetTransition (NewState, Iw, pNewDsts, DstCount);

        } // of for (int i = 0; i < IwCount; ...

    // last tuple constituent
    } else if (m_Count - 1 == Idx) {

        int Triplet [3];
        Triplet [0] = State;
        Triplet [2] = Idx;

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIWs);
            const int Iw = pIWs [i];

            if (Ignore (Iw)) {
                continue;
            }

            const int * pDsts;
            const int DstCount = pNfa->GetDest (State, Iw, &pDsts);

            for (int j = 0; j < DstCount; ++j) {

                DebugLogAssert (pDsts);
                const int Dst = pDsts [j];
                Triplet [1] = Dst;

                const int * pNewState = m_state2state.Get (Triplet, 3);
                DebugLogAssert (pNewState);
                const int NewState = *pNewState;

                const int * pNewDst = m_state2state.Get (&Dst, 1);
                DebugLogAssert (pNewDst);
                const int NewDst = *pNewDst;

                m_pOutNfa->SetTransition (NewState, Iw, &NewDst, 1);
            }
        } // of for (int i = 0; i < IwCount; ...

    // intermediate constituents
    } else {
        int Triplet1 [3];
        Triplet1 [0] = State;
        Triplet1 [2] = Idx;

        int Triplet2 [3];
        Triplet2 [0] = State;
        Triplet2 [2] = Idx + 1;

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIWs);
            const int Iw = pIWs [i];

            if (Ignore (Iw)) {
                continue;
            }

            const int * pDsts;
            const int DstCount = pNfa->GetDest (State, Iw, &pDsts);

            for (int j = 0; j < DstCount; ++j) {

                DebugLogAssert (pDsts);
                const int Dst = pDsts [j];

                Triplet1 [1] = Dst;
                Triplet2 [1] = Dst;

                const int * pNewState = m_state2state.Get (Triplet1, 3);
                DebugLogAssert (pNewState);
                const int NewState = *pNewState;

                const int * pNewDst = m_state2state.Get (Triplet2, 3);
                DebugLogAssert (pNewDst);
                const int NewDst = *pNewDst;

                m_pOutNfa->SetTransition (NewState, Iw, &NewDst, 1);
            }
        } // of for (int i = 0; i < IwCount; ...
    }
}


void FANfas2TupleNfa::CalcStateMap ()
{
    int Triplet [3] ;

    m_NewMaxState = -1;
    const int MaxState = GetMaxState ();

    const FARSNfaA * pNfa = m_pNfaArr [0];
    DebugLogAssert (pNfa);

    for (int State = 0; State <= MaxState; ++State) {

        /// map State itself
        DebugLogAssert (!m_state2state.Get (&State, 1));
        m_state2state.Add (&State, 1, ++m_NewMaxState);

        /// collect all destination states
        m_tmp.resize (0);
        const int * pIWs;
        const int IwCount = pNfa->GetIWs (State, &pIWs);

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIWs);
            const int Iw = pIWs [i];

            if (Ignore (Iw)) {
                continue;
            }

            const int * pDsts;
            const int DstCount = pNfa->GetDest (State, Iw, &pDsts);

            if (0 < DstCount) {

                const int OldSize = m_tmp.size ();
                m_tmp.resize (OldSize + DstCount);

                int * pTmp = m_tmp.begin () + OldSize;
                memcpy (pTmp, pDsts, sizeof (int) * DstCount);
            }
        } // of for (int i = 0; i < IwCount; ...

        const int NewSize = FASortUniq (m_tmp.begin (), m_tmp.end ());
        m_tmp.resize (NewSize);

        const int * pUniqDsts = m_tmp.begin ();
        const int UniqDstCount = m_tmp.size ();

        /// map intermediate states one per each < from, to, Idx > triplet
        Triplet [0] = State;

        for (int j = 0; j < UniqDstCount; ++j) {

            DebugLogAssert (pUniqDsts);
            const int DstState = pUniqDsts [j];
            Triplet [1] = DstState;

            // 1 as transitions start from State and end in DstState
            for (int Idx = 1; Idx < m_Count; ++Idx) {

                Triplet [2] = Idx;

                DebugLogAssert (!m_state2state.Get (Triplet, 3));
                m_state2state.Add (Triplet, 3, ++m_NewMaxState);

            } // of for (int Idx = 1; Idx < m_Count; ...
        } // of for (int j = 0; j < UniqDstCount; ...

    } // of for (int State = 0; State <= MaxState; ...
}


void FANfas2TupleNfa::AddIgnoredTransitions (const int State)
{
    const FARSNfaA * pNfa = m_NfaArr [0];
    DebugLogAssert (pNfa);

    const int * pIWs;
    const int IwCount = pNfa->GetIWs (State, &pIWs);

    for (int i = 0; i < IwCount; ++i) {

        const int Iw = pIWs [i];

        if (!Ignore (Iw)) {
            continue;
        }

        const int * pNewState = m_state2state.Get (&State, 1);
        DebugLogAssert (pNewState);
        const int NewState = *pNewState;

        const int * pDsts;
        const int Dsts = pNfa->GetDest (State, Iw, &pDsts);

        m_tmp.resize (Dsts);
        int * pOutStates = m_tmp.begin ();

        for (int j = 0; j < Dsts; ++j) {

            const int Dst = pDsts [j];

            pNewState = m_state2state.Get (&Dst, 1);
            DebugLogAssert (pNewState);
            const int NewDst = *pNewState;

            pOutStates [j] = NewDst;
        }

        if (0 < Dsts) {
            m_pOutNfa->SetTransition (NewState, Iw, pOutStates, Dsts);
        }
    }
}


void FANfas2TupleNfa::Process ()
{
    DebugLogAssert (m_pOutNfa && m_pNfaArr && 0 < m_Count);

    CalcStateMap ();

    const int MaxIw = GetMaxIw ();
    const int MaxState = GetMaxState ();

    // prepare the output automaton
    m_pOutNfa->SetMaxState (m_NewMaxState);
    m_pOutNfa->SetMaxIw (MaxIw);
    m_pOutNfa->Create ();

    const int * pStates;
    const int * pOutStates;
    int Count;

    // add initials
    Count = GetInitials (&pStates);
    RemapStates (pStates, Count, &pOutStates);
    m_pOutNfa->SetInitials (pOutStates, Count);

    // add finals
    Count = GetFinals (&pStates);
    RemapStates (pStates, Count, &pOutStates);
    m_pOutNfa->SetFinals (pOutStates, Count);


    // add transitions
    for (int State = 0; State <= MaxState; ++State) {

        // set up transitions from/into remapped states
        for (int Idx = 0; Idx < m_Count; ++Idx) {
            AddTransitions (Idx, State);
        }

        // set up preserved transitions
        if (-1 != m_IgnoreBase && -1 != m_IgnoreMax) {
            AddIgnoredTransitions (State);
        }
    }

    // make it ready to use
    m_pOutNfa->Prepare ();
}

}
