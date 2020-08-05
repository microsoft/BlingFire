/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACalcMealy2.h"
#include "FARSDfaA.h"
#include "FAMealyDfaA.h"

namespace BlingFire
{


FACalcMealy2::FACalcMealy2 (FAAllocatorA * pAlloc) :
    m_pInRevNfa (NULL),
    m_pOrigSigma (NULL),
    m_pInDfa (NULL),
    m_pInSigma (NULL),
    m_tmp_out (pAlloc),
    m_rev (pAlloc),
    m_OutNfa (pAlloc),
    m_OutSigma (pAlloc)
{
    m_s.SetAllocator (pAlloc);
    m_s.Create ();

    m_rev.SetInNfa (&m_tmp_out);
    m_rev.SetOutNfa (&m_OutNfa);
}


void FACalcMealy2::Clear ()
{
    m_s.Clear ();
    m_s.Create ();
    m_P.clear ();
    m_tmp_out.Clear ();
    m_rev.Clear ();
    m_OutNfa.Clear ();
    m_OutSigma.Clear ();
}


void FACalcMealy2::SetInNfa (const FARSNfaA * pInRevNfa)
{
    m_pInRevNfa = pInRevNfa;
}


void FACalcMealy2::SetSigma (const FAMealyNfaA * pOrigSigma)
{
    m_pOrigSigma = pOrigSigma;
}


void FACalcMealy2::
    SetMealy1 (const FARSDfaA * pInDfa, const FAMealyDfaA * pInSigma)
{
    m_pInDfa = pInDfa;
    m_pInSigma = pInSigma;
}


inline void FACalcMealy2::Prepare ()
{
    DebugLogAssert (m_pInRevNfa);

    m_tmp_out.Clear ();
    m_OutNfa.Clear ();
    m_OutSigma.Clear ();

    const int MaxState = m_pInRevNfa->GetMaxState ();
    m_tmp_out.SetMaxState (MaxState);
    m_tmp_out.SetMaxIw (0);
    m_tmp_out.Create ();

    int Count;
    const int * pStates;

    Count = m_pInRevNfa->GetInitials (&pStates);
    DebugLogAssert (0 < Count && pStates);
    m_tmp_out.SetInitials (pStates, Count);

    Count = m_pInRevNfa->GetFinals (&pStates);
    DebugLogAssert (0 < Count && pStates);
    m_tmp_out.SetFinals (pStates, Count);
}


inline void FACalcMealy2::EnqueueInitials ()
{
    DebugLogAssert (m_pInDfa && m_pInRevNfa);

    m_P.clear ();

    const int q1 = m_pInDfa->GetInitial ();

    const int * pStates;
    const int Count = m_pInRevNfa->GetInitials (&pStates);

    for (int i = 0; i < Count; ++i) {

        DebugLogAssert (pStates);
        const int q2 = pStates [i];

        // add <q1, q2> pair
        if (m_P.end () == m_P.find (std::pair <int, int> (q1, q2))) {
            m_P.insert (std::pair <int, int> (q1, q2));
            m_s.push_back (q2);
            m_s.push_back (q1);
        }
    }
}


inline void FACalcMealy2::SubstNfa ()
{
    DebugLogAssert (m_pInRevNfa && m_pOrigSigma);
    DebugLogAssert (m_pInDfa && m_pInSigma);
    DebugLogAssert (0 == m_s.size ());

    EnqueueInitials ();

    int SS = m_s.size ();

    while (0 != SS) {

        DebugLogAssert (0 == SS % 2);

        // state from InDfa
        const int q1 = m_s [SS - 1];
        // state from RevNfa
        const int q2 = m_s [SS - 2];

        SS -= 2;
        m_s.resize (SS);

        const int * pRevNfaIws;
        const int RevNfaIws = m_pInRevNfa->GetIWs (q2, &pRevNfaIws);

        for (int i = 0; i < RevNfaIws; ++i) {

            const int Iw = pRevNfaIws [i];

            // see whether InDfa has <q1, Iw, X> transition
            const int D1 = m_pInDfa->GetDest (q1, Iw);

            if (0 <= D1) {

                // get input Mealy DFA output symbol
                const int Ow = m_pInSigma->GetOw (q1, Iw);
                DebugLogAssert (-1 != Ow);

                // get the set of destinations of RevNfa
                const int * pDs2;
                const int Ds2 = m_pInRevNfa->GetDest (q2, Iw, &pDs2);
                DebugLogAssert (0 < Ds2 && pDs2);

                for (int j = 0; j < Ds2; ++j) {

                    const int D2 = pDs2 [j];

                    m_tmp_out.SetTransition (q2, Ow, D2);

                    // see whether original Mealy NFA had <D2, Iw, q2> -> Ow
                    const int OutOw = m_pOrigSigma->GetOw (D2, Iw, q2);

                    if (-1 != OutOw) {
                        m_OutSigma.SetOw (D2, Ow, q2, OutOw);
                    }

                    // add <q1, q2> pair if not in m_P
                    if (m_P.end () == m_P.find (std::pair <int, int> (D1, D2))) {
                        m_P.insert (std::pair <int, int> (D1, D2));
                        m_s.push_back (D2);
                        m_s.push_back (D1);
                        SS += 2;
                    }
                }

            } // of if (0 <= D1) ...

        } // of for (i = 0; i < RevNfaIws; ...

    } // of while (0 != SS) ...

    m_tmp_out.Prepare ();
}


inline void FACalcMealy2::Reverse ()
{
    m_rev.Process ();
}


void FACalcMealy2::Process ()
{
    DebugLogAssert (m_pInRevNfa);
    DebugLogAssert (m_pInDfa && m_pInSigma);

    Prepare ();
    SubstNfa ();
    Reverse ();
}


const FARSNfaA * FACalcMealy2::GetOutNfa () const
{
    return & m_OutNfa;
}


const FAMealyNfaA * FACalcMealy2::GetSigma () const
{
    return & m_OutSigma;
}

}
