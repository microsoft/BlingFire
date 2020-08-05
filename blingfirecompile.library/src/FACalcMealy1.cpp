/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACalcMealy1.h"
#include "FARSNfaA.h"
#include "FAUtils.h"

namespace BlingFire
{


FACalcMealy1::FACalcMealy1 (FAAllocatorA * pAlloc) :
    m_pInNfa (NULL),
    m_pS2C (NULL),
    m_MapSize (0),
    m_MaxColor (0),
    m_in_nfa (pAlloc),
    m_OutDfa (pAlloc),
    m_OutSigma (pAlloc),
    m_nfa2dfa (pAlloc)
{
    m_c2s.SetAllocator (pAlloc);

    m_nfa2dfa.SetNFA (&m_in_nfa);
    m_nfa2dfa.SetDFA (&m_OutDfa);

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_tmp2.SetAllocator (pAlloc);
    m_tmp2.Create ();

    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();
}


void FACalcMealy1::Clear ()
{
    m_MaxColor = 0;

    m_c2s.Clear ();
    m_in_nfa.Clear ();
    m_OutDfa.Clear ();
    m_OutSigma.Clear ();
    m_nfa2dfa.Clear ();

    m_tmp.Clear ();
    m_tmp.Create ();
    m_tmp2.Clear ();
    m_tmp2.Create ();
    m_alphabet.Clear ();
    m_alphabet.Create ();
}


void FACalcMealy1::SetInNfa (const FARSNfaA * pInNfa)
{
    m_pInNfa = pInNfa;
}


void FACalcMealy1::SetColorMap (const int * pS2C, const int Size)
{
    m_pS2C = pS2C;
    m_MapSize = Size;
}


const FARSDfaA * FACalcMealy1::GetDfa () const
{
  return & m_OutDfa;
}


const FAMealyDfaA * FACalcMealy1::GetSigma () const
{
  return & m_OutSigma;
}


void FACalcMealy1::Prepare ()
{
    DebugLogAssert (0 < m_MapSize && m_pS2C);

    m_OutDfa.Clear ();
    m_in_nfa.Clear ();
    m_OutSigma.Clear ();
    m_c2s.Clear ();

    m_MaxColor = 0;

    for (int j = 0; j < m_MapSize; ++j) {

        const int C = m_pS2C [j];

        if (-1 != C) {

            if (m_MaxColor < C) {
                m_MaxColor = C;
            }

            m_c2s.Add (C, j);
        }
    }
    m_c2s.SortUniq ();
}


const int FACalcMealy1::
    Remap (const int * pStates, const int Count, const int ** ppStates)
{
    DebugLogAssert (ppStates);

    if (0 == Count) {
        return 0;
    }

    DebugLogAssert (0 < Count && pStates);

    m_tmp.resize (Count);
    int * pTmp = m_tmp.begin ();

    for (int i = 0; i < Count; ++i) {

        const int State = pStates [i];
        DebugLogAssert (m_MapSize > State);

        const int NewState = m_pS2C [State];
        DebugLogAssert (0 <= NewState);

        pTmp [i] = NewState;
    }

    if (1 < Count) {
        const int NewSize = FASortUniq (pTmp, pTmp + Count);
        m_tmp.resize (NewSize);
    }

    *ppStates = m_tmp.begin ();
    return m_tmp.size ();
}


void FACalcMealy1::RemapNfa ()
{
    DebugLogAssert (m_pInNfa && 0 < m_MapSize && m_pS2C);

    int Count;
    int NewCount;
    const int * pStates;
    const int * pNewStates;

    m_in_nfa.SetMaxState (m_MaxColor);
    const int MaxIw = m_pInNfa->GetMaxIw ();
    m_in_nfa.SetMaxIw (MaxIw);
    m_in_nfa.Create ();

    // setup initials
    Count = m_pInNfa->GetInitials (&pStates);
    NewCount = Remap (pStates, Count, &pNewStates);
    DebugLogAssert (0 < NewCount && pNewStates);
    m_in_nfa.SetInitials (pNewStates, NewCount);

    // setup finals
    Count = m_pInNfa->GetFinals (&pStates);
    NewCount = Remap (pStates, Count, &pNewStates);
    DebugLogAssert (0 < NewCount && pNewStates);
    m_in_nfa.SetFinals (pNewStates, NewCount);

    // calculate the alphabet
    FAGetAlphabet (m_pInNfa, &m_alphabet);

    const int * pIws = m_alphabet.begin ();
    const int IwsCount = m_alphabet.size ();

    // make iteration thru the states of input automaton
    for (int NewState = 0; NewState <= m_MaxColor; ++NewState) {

        Count = m_c2s.Get (NewState, &pStates);
        DebugLogAssert (0 < Count && pStates);

        for (int j = 0; j < IwsCount; ++j) {

            const int Iw = pIws [j];

            m_tmp2.resize (0);

            for (int i = 0; i < Count; ++i) {

                const int State = pStates [i];
                DebugLogAssert (-1 != State);

                const int * pDsts;
                const int Dsts = m_pInNfa->GetDest (State, Iw, &pDsts);

                if (0 < Dsts) {
                    const int OldSize = m_tmp2.size ();
                    m_tmp2.resize (OldSize + Dsts);
                    memcpy (m_tmp2.begin () + OldSize, pDsts, sizeof (int)*Dsts);
                }
            }

            // build a set from all old destination states
            const int NewTmpSize = \
                FASortUniq (m_tmp2.begin (), m_tmp2.end ());
            m_tmp2.resize (NewTmpSize);

            // remap this set
            NewCount = Remap (m_tmp2.begin (), NewTmpSize, &pNewStates);

            // setup a transition
            if (0 < NewCount) {
                m_in_nfa.SetTransition (NewState, Iw, pNewStates, NewCount);
            }

        } // of for (int Iw = 0; ...
    } // of for (int State = 0; ...

    m_in_nfa.Prepare ();
}


void FACalcMealy1::Process ()
{
    Prepare ();

    RemapNfa ();

    m_nfa2dfa.Process ();

    /// build sigma function

    int m_LastOw = -1;

    const int MaxState = m_OutDfa.GetMaxState ();
    DebugLogAssert (0 <= MaxState);

    const int * pIws;
    const int IwCount = m_OutDfa.GetIWs (&pIws);
    DebugLogAssert (0 < IwCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwCount));

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < IwCount; ++i) {

            const int Iw = pIws [i];
            const int Dst = m_OutDfa.GetDest (State, Iw);

            if (0 <= Dst) {
                m_OutSigma.SetOw (State, Iw, ++m_LastOw);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...
}

}
