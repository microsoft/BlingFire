/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestCmpDfa.h"
#include "FARSDfaCA.h"
#include "FAState2OwCA.h"
#include "FAState2OwsCA.h"
#include "FAUtils.h"
#include "FAMealyDfaCA.h"
#include "FAFsmConst.h"

namespace BlingFire
{


FATestCmpDfa::FATestCmpDfa (FAAllocatorA * pAlloc) :
    m_pDfa1 (NULL),
    m_pDfa2 (NULL),
    m_pState2Ow1 (NULL),
    m_pState2Ow2 (NULL),
    m_pState2Ows1 (NULL),
    m_pState2Ows2 (NULL),
    m_pSigma1 (NULL),
    m_pSigma2 (NULL),
    m_pOws1 (NULL),
    m_MaxOws1Count (0),
    m_pOws2 (NULL),
    m_MaxOws2Count (0)
{
    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_visited1.SetAllocator (pAlloc);
    m_visited1.Create ();

    m_visited2.SetAllocator (pAlloc);
    m_visited2.Create ();

    m_ows1.SetAllocator (pAlloc);
    m_ows1.Create ();

    m_ows2.SetAllocator (pAlloc);
    m_ows2.Create ();

    m_iws1.SetAllocator (pAlloc);
    m_iws1.Create ();

    m_iws2.SetAllocator (pAlloc);
    m_iws2.Create ();
}


void FATestCmpDfa::SetFsm1 (
        const FARSDfaCA * pDfa1,
        const FAState2OwCA * pState2Ow1,
        const FAState2OwsCA * pState2Ows1,
        const FAMealyDfaCA * pSigma1
    )
{
    m_pDfa1 = pDfa1;
    m_pState2Ow1 = pState2Ow1;
    m_pState2Ows1 = pState2Ows1;
    m_pSigma1 = pSigma1;

    if (m_pState2Ows1) {

        const int MaxOws1 = m_pState2Ows1->GetMaxOwsCount ();
        m_ows1.resize (MaxOws1);

        m_pOws1 = m_ows1.begin ();
        m_MaxOws1Count = MaxOws1;
    }
}


void FATestCmpDfa::SetFsm2 (
        const FARSDfaCA * pDfa2,
        const FAState2OwCA * pState2Ow2,
        const FAState2OwsCA * pState2Ows2,
        const FAMealyDfaCA * pSigma2
    )
{
    m_pDfa2 = pDfa2;
    m_pState2Ow2 = pState2Ow2;
    m_pState2Ows2 = pState2Ows2;
    m_pSigma2 = pSigma2;

    if (m_pState2Ows2) {

        const int MaxOws2 = m_pState2Ows2->GetMaxOwsCount ();
        m_ows2.resize (MaxOws2);

        m_pOws2 = m_ows2.begin ();
        m_MaxOws2Count = MaxOws2;
    }
}


void FATestCmpDfa::Clear ()
{
    m_stack.resize (0);
    m_visited1.resize (0);
    m_visited2.resize (0);
}


const bool FATestCmpDfa::
    WasSeen (const FABitArray * pVisited, const int State) const
{
    if (FAFsmConst::DFA_DEAD_STATE == State) {
        return true;
    }

    DebugLogAssert (pVisited);
    DebugLogAssert (0 <= State);

    const int Size = pVisited->size ();

    if (Size > State) {

        const bool Res = pVisited->get_bit (State);
        return Res;

    } else {

        return false;
    }
}


void FATestCmpDfa::SetSeen (FABitArray * pVisited, const int State)
{
    DebugLogAssert (pVisited);
    DebugLogAssert (0 <= State);

    const int Size = pVisited->size ();

    if (Size <= State) {

        pVisited->resize (State + 1);
        pVisited->set_bits (Size, State, false);
    }

    pVisited->set_bit (State, true);
}


const bool FATestCmpDfa::CmpAlphabets ()
{
    DebugLogAssert (m_pDfa1 && m_pDfa2);

    const int Count1 = m_pDfa1->GetIWs (NULL, 0);
    const int Count2 = m_pDfa2->GetIWs (NULL, 0);

    if (Count1 != Count2) {
        // alphabet sizes are different
        DebugLogAssert (0);
        return false;
    }
    if (0 > Count1) {
        // no alphabet
        DebugLogAssert (0);
        return false;
    }

    m_iws1.resize (Count1);
    int * pA1 = m_iws1.begin ();
    m_pDfa1->GetIWs (pA1, Count1);

    m_iws2.resize (Count2);
    int * pA2 = m_iws2.begin ();
    m_pDfa2->GetIWs (pA2, Count2);

    if (0 != memcmp (pA1, pA2, sizeof (int) * Count1)) {
        // alphabets are different
        DebugLogAssert (0);
        return false;
    }

    m_iws1.resize (0);
    m_iws2.resize (0);

    return true;
}


const bool FATestCmpDfa::Process (const int /*MaxState*/, const int MaxIw)
{
    DebugLogAssert (0 < MaxIw);
    DebugLogAssert (m_pDfa1 && m_pDfa2);

    Clear ();

    if (false == CmpAlphabets ()) {
        return false;
    }

    int State1 = m_pDfa1->GetInitial ();
    int State2 = m_pDfa2->GetInitial ();

    m_stack.push_back (State1);
    m_stack.push_back (State2);

    // mark the as "seen"
    SetSeen (&m_visited1, State1);
    SetSeen (&m_visited2, State2);

    int StackSize = 2;
    DebugLogAssert (m_stack.size () == (unsigned int) StackSize);

    while (0 < StackSize) {

        DebugLogAssert (m_stack.size () == (unsigned int) StackSize && \
                0 == (StackSize & 1));

        State2 = m_stack [StackSize - 1];
        State1 = m_stack [StackSize - 2];
        m_stack.resize (StackSize - 2);
        StackSize -= 2;

        /// // check whether they both final
        /// const bool IsFinal1 = m_pDfa1->IsFinal (State1);
        /// const bool IsFinal2 = m_pDfa2->IsFinal (State2);

        /// if (IsFinal1 != IsFinal2) {
        ///     // both should be final or non-final
        ///     DebugLogAssert (0);
        ///     return false;
        /// }

        // check whether they both have same Ows associated
        if (m_pState2Ow1 && m_pState2Ow2) {

            const int Ow1 = m_pState2Ow1->GetOw (State1);
            const int Ow2 = m_pState2Ow2->GetOw (State2);

            if (Ow1 != Ow2) {
                // Ows different
                DebugLogAssert (0);
                return false;
            }
        }
        // check whether they both have same Ow-sets associated
        if (m_pState2Ows1 && m_pState2Ows2) {

            const int Count1 = 
                m_pState2Ows1->GetOws (State1, m_pOws1, m_MaxOws1Count);
            const int Count2 = 
                m_pState2Ows2->GetOws (State2, m_pOws2, m_MaxOws2Count);

            if (Count1 != Count2 || \
                Count1 > m_MaxOws1Count || \
                Count2 > m_MaxOws2Count) {
                // problems with max values or different Ows
                DebugLogAssert (0);
                return false;
            }
            if (-1 != Count1 && \
                0 != memcmp (m_pOws1, m_pOws2, Count1 * sizeof (int))) {
                // different Ows
                DebugLogAssert (0);
                return false;
            }
        }

        // process destination states
        for (int iw = 0; iw <= MaxIw; ++iw) {

            const int Dest1 = m_pDfa1->GetDest (State1, iw);
            const int Dest2 = m_pDfa2->GetDest (State2, iw);

            // transition does not exist
            if (-1 == Dest1 && -1 == Dest2) {
                continue;
            }

            if ((-1 == Dest1 || -1 == Dest2) && \
                Dest2 != Dest1) {
                // missing transition
                DebugLogAssert (0);
                return false;
            }
            if ((FAFsmConst::DFA_DEAD_STATE == Dest1 || \
                 FAFsmConst::DFA_DEAD_STATE == Dest2) && \
                Dest2 != Dest1) {
                // dead-state misrepresentation
                DebugLogAssert (0);
                return false;
            }

            const bool IsSeen1 = WasSeen (&m_visited1, Dest1);
            const bool IsSeen2 = WasSeen (&m_visited2, Dest2);

            if (IsSeen1 != IsSeen2) {
                // transition graphs have different structure
                DebugLogAssert (0);
                return false;
            }

            if (m_pSigma1 && m_pSigma2) {

                int Ow1 = -2;
                const int D1 = m_pSigma1->GetDestOw (State1, iw, &Ow1);

                int Ow2 = -3;
                const int D2 = m_pSigma2->GetDestOw (State2, iw, &Ow2);

                if (Ow1 != Ow2) {
                    // sigma difference in Ows
                    DebugLogAssert (0);
                    return false;
                }
                if (D1 != Dest1 || D2 != Dest2) {
                    // sigma difference in Dsts
                    return false;
                }
            }

            if (!IsSeen1) {

                m_stack.push_back (Dest1);
                m_stack.push_back (Dest2);

                StackSize += 2;

                SetSeen (&m_visited1, Dest1);
                SetSeen (&m_visited2, Dest2);
            }
        } // of for (int iw = 0; ...

    } // of while (0 < StackSize) ...

    return true;
}

}
