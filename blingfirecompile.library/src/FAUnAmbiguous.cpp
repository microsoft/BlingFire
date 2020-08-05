/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAUnAmbiguous.h"
#include "FARSNfaA.h"
#include "FAMealyNfaA.h"
#include "FALessA.h"
#include "FAUtils.h"

namespace BlingFire
{

FAUnAmbiguous::FAUnAmbiguous (FAAllocatorA * pAlloc) :
    m_pLess (NULL),
    m_pInNfa (NULL),
    m_pInSigma (NULL),
    m_OutNfa (pAlloc),
    m_OutSigma (pAlloc),
    m_MaxQ (-1),
    m_rm (pAlloc)
{
    m_q2xs.SetAllocator (pAlloc);
    m_q2xs.SetCopyChains (true);

    m_xs.SetAllocator (pAlloc);
    m_xs.Create ();

    m_initials.SetAllocator (pAlloc);
    m_initials.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();

    m_rm.SetNfa (&m_OutNfa);
    m_rm.SetRemoveIniUnreach (false);
}


void FAUnAmbiguous::Clear ()
{
    m_MaxQ = -1;
    m_OutNfa.Clear ();
    m_OutSigma.Clear ();
    m_q2xs.Clear ();
    m_xs.Clear ();
    m_xs.Create ();
    m_initials.Clear ();
    m_initials.Create ();
    m_finals.Clear ();
    m_finals.Create ();
}


void FAUnAmbiguous::SetLess (const FALessA * pLess)
{
    m_pLess = pLess;
}


void FAUnAmbiguous::
    SetInMealy (const FARSNfaA * pInNfa, const FAMealyNfaA * pInSigma)
{
    m_pInNfa = pInNfa;
    m_pInSigma = pInSigma;
}


const FARSNfaA * FAUnAmbiguous::GetNfa () const
{
    return & m_OutNfa;
}


const FAMealyNfaA * FAUnAmbiguous::GetSigma () const
{
    return & m_OutSigma;
}


void FAUnAmbiguous::Prepare ()
{
    DebugLogAssert (m_pInNfa);

    m_OutNfa.Clear ();
    m_OutSigma.Clear ();
    m_q2xs.Clear ();
    m_MaxQ = -1;

    m_initials.resize (0);
    m_finals.resize (0);

    const int MaxIw = m_pInNfa->GetMaxIw ();
    m_OutNfa.SetMaxIw (MaxIw);
    m_OutNfa.SetMaxState (0);
    m_OutNfa.Create ();
}


inline const int FAUnAmbiguous::AddState (const int * pXS, const int Size)
{
    DebugLogAssert (0 < Size && pXS);

    int Q = m_q2xs.GetIdx (pXS, Size);

    if (-1 == Q) {
        Q = m_q2xs.Add (pXS, Size, 0);
        DebugLogAssert (m_MaxQ + 1 == Q);
        m_OutNfa.AddStateCount (1);
        m_MaxQ = Q;
    }

    DebugLogAssert (0 <= Q && Q <= m_MaxQ);
    return Q;
}


inline const int FAUnAmbiguous::GetXS (const int Q, const int ** ppXS) const
{
    DebugLogAssert (0 <= Q && Q <= m_MaxQ);
    const int Size = m_q2xs.GetChain (Q, ppXS);
    return Size;
}


inline const bool FAUnAmbiguous::IsInitial (const int State) const
{
    const int * pInitials;
    const int Count = m_pInNfa->GetInitials (&pInitials);
    DebugLogAssert (0 < Count && pInitials);
    DebugLogAssert (FAIsSortUniqed (pInitials, Count));

    return -1 != FAFind_log (pInitials, Count, State);
}


inline const bool FAUnAmbiguous::IsFinal (const int State) const
{
    const int * pFinals;
    const int Count = m_pInNfa->GetFinals (&pFinals);
    DebugLogAssert (0 < Count && pFinals);
    DebugLogAssert (FAIsSortUniqed (pFinals, Count));

    return -1 != FAFind_log (pFinals, Count, State);
}


const bool FAUnAmbiguous::
    CalcNewState (
        const int X1,
        const int Iw,
        const int Y1,
        const int Ow1,
        const int * pS,
        const int SSize,
        const int * pDst,
        const int Count
    )
{
    DebugLogAssert (m_pInNfa && m_pInSigma);
    DebugLogAssert (0 < Count && pDst);

    int k;
    m_xs.resize (0);
    m_xs.push_back (Y1);

    /// add better paths from the current source state X1

    for (k = 0; k < Count; ++k) {

        const int Y2 = pDst [k];
        const int Ow2 = m_pInSigma->GetOw (X1, Iw, Y2);

        if ((m_pLess && m_pLess->Less (Ow2, Ow1)) || \
            (!m_pLess && Ow2 < Ow1)) {

            DebugLogAssert (Y1 != Y2);
            m_xs.push_back (Y2);
        }

        /// if (Y2 < Y1) {
        ///    m_xs.push_back (Y2);
        /// }
    }

    /// add better paths from the corresponding pS set

    for (k = 0; k < SSize; ++k) {

        const int X2 = pS [k];

        const int * pDst2;
        const int Count2 = m_pInNfa->GetDest (X2, Iw, &pDst2);

        for (int i = 0; i < Count2; ++i) {

            const int Dst2 = pDst2 [i];

            if (Y1 != Dst2) {
                m_xs.push_back (Dst2);
            } else {
                return false;
            }
        }
    }

    const int NewSize = FASortUniq (m_xs.begin () + 1, m_xs.end ());
    m_xs.resize (NewSize + 1);

    return true;
}


void FAUnAmbiguous::PrepareOutNfa ()
{
    const int IniCount = FASortUniq (m_initials.begin (), m_initials.end ());
    m_initials.resize (IniCount);
    DebugLogAssert (0 < IniCount);

    m_OutNfa.SetInitials (m_initials.begin (), IniCount);

    const int FinCount = FASortUniq (m_finals.begin (), m_finals.end ());
    m_finals.resize (FinCount);
    DebugLogAssert (0 < FinCount);

    m_OutNfa.SetFinals (m_finals.begin (), FinCount);

    m_OutNfa.Prepare ();
}


void FAUnAmbiguous::CreateOutNfa ()
{
    DebugLogAssert (m_pInNfa && m_pInSigma);

    const int * pStates;
    int Count;

    Count = m_pInNfa->GetInitials (&pStates);
    DebugLogAssert (0 < Count && pStates);

    for (int i = 0; i < Count; ++i) {
        const int Initial = pStates [i];
        AddState (&Initial, 1);
    }

    int Q = 0;

    while (Q <= m_MaxQ) {

        const int * pXS;
        const int SSize = GetXS (Q, &pXS) - 1;
        DebugLogAssert (0 <= SSize && pXS);

        const int X1 = *pXS;
        const int * pS = pXS + 1;
        DebugLogAssert (0 <= X1);

        // check whether it is initial
        if (IsInitial (X1)) {
            m_initials.push_back (Q);
        }

        // check whether it is final state
        // and there is no other better paths leading to it
        if (IsFinal (X1)) {
            int i;
            for (i = 0; i < SSize; ++i) {
                const int S = pS [i];
                if (IsFinal (S)) {
                    break;
                }
            }
            if (i == SSize) {
                m_finals.push_back (Q);
            }
        } // of if (IsFinal (X1)) ...

        // make iteration thru the all outgoing arcs from x1
        const int * pIws;
        const int IwCount = m_pInNfa->GetIWs (X1, &pIws);

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIws);
            const int Iw = pIws [i];

            Count = m_pInNfa->GetDest (X1, Iw, &pStates);
            DebugLogAssert (0 < Count && pStates);

            for (int j = 0; j < Count; ++j) {

                const int Y1 = pStates [j];
                const int Ow = m_pInSigma->GetOw (X1, Iw, Y1);

                // build a transition to a new state 
                if (CalcNewState (X1, Iw, Y1, Ow, pS, SSize, pStates, Count)) {

                    const int NewState = AddState (m_xs.begin (), m_xs.size ());
                    DebugLogAssert (-1 != NewState);

                    m_OutNfa.SetTransition (Q, Iw, NewState);

                    if (-1 != Ow) {
                        m_OutSigma.SetOw (Q, Iw, NewState, Ow);
                    }
                }

            } // of for (int j = 0; j < Count;  ...
        } // of for (int i = 0; i < IwCount; ...

        Q++;
    }

    PrepareOutNfa ();
}


void FAUnAmbiguous::RmUnreach ()
{
    m_rm.Process ();
}


void FAUnAmbiguous::Process ()
{
    DebugLogAssert (m_pInNfa && m_pInSigma);

    Prepare ();
    CreateOutNfa ();
    RmUnreach ();
}

}
