/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSDfa2MooreDfa.h"
#include "FAAllocatorA.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAState2OwsA.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSDfa2MooreDfa::FARSDfa2MooreDfa (FAAllocatorA * pAlloc) :
    m_pInDfa (NULL),
    m_pOutDfa (NULL),
    m_pState2Ow (NULL),
    m_pState2Ows (NULL),
    m_BaseOw (0),
    m_MaxOw (0),
    m_KeepOws (false),
    m_pAlphabet (NULL),
    m_AlphabetSize (0)
{
    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();
}


void FARSDfa2MooreDfa::Clear ()
{
    m_ows.Clear ();
    m_ows.Create ();

    m_finals.Clear ();
    m_finals.Create ();
}


void FARSDfa2MooreDfa::SetRSDfa (const FARSDfaA * pInDfa)
{
    m_pInDfa = pInDfa;
}


void FARSDfa2MooreDfa::SetMooreDfa (FARSDfaA * pOutDfa)
{
    m_pOutDfa = pOutDfa;
}


void FARSDfa2MooreDfa::SetState2Ow (FAState2OwA * pState2Ow)
{
    m_pState2Ow = pState2Ow;
}


void FARSDfa2MooreDfa::SetState2Ows (FAState2OwsA * pState2Ows)
{
    m_pState2Ows = pState2Ows;
}


void FARSDfa2MooreDfa::SetOwsRange (const int BaseOw, const int MaxOw)
{
    m_BaseOw = BaseOw;
    m_MaxOw = MaxOw;
}


void FARSDfa2MooreDfa::SetKeepOws (const bool KeepOws)
{
    m_KeepOws = KeepOws;
}


void FARSDfa2MooreDfa::ProcessState (const int State)
{
    DebugLogAssert (m_pInDfa);
    DebugLogAssert (m_pOutDfa && (m_pState2Ows || m_pState2Ow));
    DebugLogAssert (m_pAlphabet);

    m_ows.resize (0);

    for (int iw_idx = 0; iw_idx < m_AlphabetSize; ++iw_idx) {

        const int Iw = m_pAlphabet [iw_idx];
        const int DstState = m_pInDfa->GetDest (State, Iw);

        if (-1 == DstState)
            continue;

        if (m_BaseOw <= Iw && Iw <= m_MaxOw) {

            int Ow = Iw;
            if (false == m_KeepOws) {
                Ow -= m_BaseOw;
            }
            m_ows.push_back (Ow);

        } else {

            m_pOutDfa->SetTransition (State, Iw, DstState);
        }

    } // of for (int iw_idx = 0; ...

    // set up output weights
    if (0 < m_ows.size ()) {

        int * pBegin = m_ows.begin ();
        int Size = m_ows.size ();

        if (false == FAIsSortUniqed (pBegin, Size)) {
            Size = FASortUniq (pBegin, pBegin + Size);
            m_ows.resize (Size);
        }

        if (m_pState2Ows) {

            DebugLogAssert (NULL == m_pState2Ow);
            m_pState2Ows->SetOws (State, m_ows.begin (), m_ows.size ());

        } else if (m_pState2Ow) {

            DebugLogAssert (NULL == m_pState2Ows);
            DebugLogAssert (1 == Size);
            m_pState2Ow->SetOw (State, m_ows [0]);
        }

        // if state has a reaction mark it final
        m_finals.push_back (State);
    }
}


const int FARSDfa2MooreDfa::GetNewMaxIw () const
{
    DebugLogAssert (m_pInDfa);

    // get alphabet
    const int * pIws;
    const int IwsCount = m_pInDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwsCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwsCount));

    // get old MaxIw
    const int OldMaxIw = m_pInDfa->GetMaxIw ();
    DebugLogAssert (OldMaxIw == pIws [IwsCount - 1]);

    // see whether Ows reduce the range of Input-Weights from the right
    if (m_MaxOw >= OldMaxIw && m_BaseOw < OldMaxIw) {

        // find biggest Iw wich is less than m_BaseOw
        for (int i = IwsCount - 1; i >= 0; --i) {

            const int Iw = pIws [i];
            if (Iw < m_BaseOw)
                return Iw;
        }
    }

    return OldMaxIw;
}


void FARSDfa2MooreDfa::Process ()
{
    DebugLogAssert (m_pInDfa);
    DebugLogAssert (m_pOutDfa && (m_pState2Ows || m_pState2Ow));

    const int MaxState = m_pInDfa->GetMaxState ();
    /// const int MaxIw = m_pInDfa->GetMaxIw ();
    const int MaxIw = GetNewMaxIw ();

    m_pOutDfa->SetMaxState (MaxState);
    m_pOutDfa->SetMaxIw (MaxIw);
    m_pOutDfa->Create ();

    // get alphabet
    m_AlphabetSize = m_pInDfa->GetIWs (&m_pAlphabet);
    DebugLogAssert (0 < m_AlphabetSize && m_pAlphabet);

    // setup initial state
    const int InitialState = m_pInDfa->GetInitial ();
    m_pOutDfa->SetInitial (InitialState);

    // copy finals
    const int * pFinals;
    const int FinalsCount = m_pInDfa->GetFinals (&pFinals);
    m_finals.resize (FinalsCount);

    for (int i = 0; i < FinalsCount; ++i) {

        DebugLogAssert (pFinals);
        m_finals [i] = pFinals [i];
    }

    // setup transitions
    for (int State = 0; State <= MaxState; ++State) {
        ProcessState (State);
    } // of for (int State = 0; ...

    // setup finals
    const int NewSize = FASortUniq (m_finals.begin (), m_finals.end ());
    m_finals.resize (NewSize);
    m_pOutDfa->SetFinals (m_finals.begin (), NewSize);

    // make it ready to work
    m_pOutDfa->Prepare ();
}

}
