/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAIwOwSuffArr2Patterns.h"
#include "FAUtils.h"
#include "FAException.h"

namespace BlingFire
{


FAIwOwSuffArr2Patterns::FAIwOwSuffArr2Patterns (FAAllocatorA * pAlloc) :
    m_MinPatLen (DefMinPatLen),
    m_MinPatPrec (100),
    m_iwow_trie (pAlloc),
    m_iw_trie (pAlloc)
{
    m_iwow_state2freq.SetAllocator (pAlloc);
    m_iwow_state2freq.Create ();

    m_iw_state2freq.SetAllocator (pAlloc);
    m_iw_state2freq.Create ();

    m_state2state.SetAllocator (pAlloc);
    m_state2state.Create ();

    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();

    m_tmp_arr.SetAllocator (pAlloc);
    m_tmp_arr.Create ();

    m_pat.SetAllocator (pAlloc);
    m_pat.Create ();

    FAIwOwSuffArr2Patterns::Clear ();
}


FAIwOwSuffArr2Patterns::~FAIwOwSuffArr2Patterns ()
{}


void FAIwOwSuffArr2Patterns::SetMinPatLen (const int MinPatLen)
{
    if (0 >= MinPatLen || FALimits::MaxWordLen < MinPatLen) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    m_MinPatLen = MinPatLen;
}


void FAIwOwSuffArr2Patterns::SetMinPatPrec (const float MinPatPrec)
{
    if (0 >= MinPatPrec || 100 < MinPatPrec) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    m_MinPatPrec = MinPatPrec;
}


void FAIwOwSuffArr2Patterns::Clear ()
{
    m_iwow_trie.Clear ();
    m_iwow_trie.SetMaxState (1);
    m_iwow_trie.SetMaxIw (1);
    m_iwow_trie.Create ();
    m_iwow_trie.SetInitial (0);

    m_iw_trie.Clear ();
    m_iw_trie.SetMaxState (1);
    m_iw_trie.SetMaxIw (1);
    m_iw_trie.Create ();
    m_iw_trie.SetInitial (0);

    m_iwow_state2freq.Clear ();
    m_iwow_state2freq.Create (2);
    m_iwow_state2freq.resize (2);
    m_iwow_state2freq [0] = 0;
    m_iwow_state2freq [1] = 0;

    m_iw_state2freq.Clear ();
    m_iw_state2freq.Create (2);
    m_iw_state2freq.resize (2);
    m_iw_state2freq [0] = 0;
    m_iw_state2freq [1] = 0;

    m_state2state.Clear ();
    m_state2state.Create (2);
    m_state2state.resize (2);
    m_state2state [0] = -1;
    m_state2state [1] = -1;

    m_iws.resize (0);
    m_ows.resize (0);
    m_tmp_arr.resize (0);
    m_pat.resize (0);
}


inline const bool FAIwOwSuffArr2Patterns::
    HasPrefChanged (const int * pChain, const int Size) const
{
    DebugLogAssert (0 < m_MinPatLen);

    const int MinPatLen2 = m_MinPatLen << 1;

    const int PrevSize = m_tmp_arr.size ();
    const int * pPrev = m_tmp_arr.begin ();

    if (0 == PrevSize) {
        return false;
    }

    for (int i = 0; i < MinPatLen2 && i < Size; i += 2) {
        if (pChain [i] != pPrev [i]) {
            return true;
        }
    }

    return false;
}


void FAIwOwSuffArr2Patterns::
    AddTail_iwow (
            const int State, 
            const int * pTail, 
            const int Size, 
            const int Freq
        )
{
    DebugLogAssert (0 < Size && pTail);
    DebugLogAssert (0 <= State);
    DebugLogAssert (0 < Freq);

    int MaxIw = m_iwow_trie.GetMaxIw ();
    const int MaxState = m_iwow_trie.GetMaxState ();
    DebugLogAssert (m_iwow_state2freq.size () == (unsigned int) 1 + MaxState);

    m_iwow_trie.AddStateCount (Size);
    m_iwow_state2freq.resize (MaxState + Size + 1, MaxState);

    int Src = State;
    int Dst = MaxState + 1;

    for (int i = 0; i < Size; ++i) {

        const int C = pTail [i];

        if (MaxIw < C) {
            m_iwow_trie.AddIwCount (C - MaxIw);
            MaxIw = C;
        }

        m_iwow_trie.SetTransition (Src, C, Dst);
        m_iwow_state2freq [Dst] = Freq;

        Src = Dst;
        Dst++;
    }
}

/// Note: State1, Dst1, Src1 are states of IwOw-trie and
///       State2, Dst2, Src2 are *corresponding* states of the Iw-trie

void FAIwOwSuffArr2Patterns::
    AddTail_iw (
            const int State1, 
            const int State2, 
            const int * pTail, 
            const int Size,
            const int Freq
    )
{
    DebugLogAssert (0 < Size && pTail && 0 == (Size % 2));
    DebugLogAssert (0 <= State1 && 0 <= State2);
    DebugLogAssert (0 < Freq);

    const int Size_2 = Size >> 1;

    int MaxIw = m_iw_trie.GetMaxIw ();
    const int MaxState = m_iw_trie.GetMaxState ();
    DebugLogAssert (m_state2state.size () == (unsigned int) 1 + MaxState);
    DebugLogAssert (m_iw_state2freq.size () == (unsigned int) 1 + MaxState);

    m_iw_trie.AddStateCount (Size_2);

    const int NewSize = MaxState + Size_2 + 1;
    m_state2state.resize (NewSize, MaxState);
    m_iw_state2freq.resize (NewSize, MaxState);

    int Src2 = State2;
    int Dst2 = MaxState + 1;
    int Src1 = State1;

    for (int i = 0; i < Size; i += 2) {

        const int Iw = pTail [i];
        const int Ow = pTail [i+1];

        if (MaxIw < Iw) {
            m_iw_trie.AddIwCount (Iw - MaxIw);
            MaxIw = Iw;
        }

        m_iw_trie.SetTransition (Src2, Iw, Dst2);

        const int TmpDst1 = m_iwow_trie.GetDest (Src1, Iw);
        const int Dst1 = m_iwow_trie.GetDest (TmpDst1, Ow);
        DebugLogAssert (0 < Dst1 && 0 < TmpDst1); // as IwOw-chain is already there

        m_state2state [Dst2] = Dst1;
        m_iw_state2freq [Dst2] = Freq;

        Src1 = Dst1;
        Src2 = Dst2;
        Dst2++;
    }
}

/// Note: State1, Dst1, Src1 are states of IwOw-trie and
///       State2, Dst2, Src2 are *corresponding* states of the Iw-trie

void FAIwOwSuffArr2Patterns::
    AddChain_int (const int * pChain, const int Size, const int Freq)
{
    int i;

    /// add the chain into m_iwow_trie
    int State = m_iwow_trie.GetInitial ();

    for (i = 0; i < Size; ++i) {

        const int C = pChain [i];
        const int Dst = m_iwow_trie.GetDest (State, C);

        if (0 < Dst) {
            DebugLogAssert (m_iwow_state2freq.size () > (unsigned int) Dst);
            m_iwow_state2freq [Dst] += Freq;
            State = Dst;
        } else {
            AddTail_iwow (State, pChain + i, Size - i, Freq);
            break;
        }
    }

    /// add the chain into m_iw_trie
    int State1 = m_iwow_trie.GetInitial ();
    int State2 = m_iw_trie.GetInitial ();

    for (i = 0; i < Size; i += 2) {

        const int Iw = pChain [i];
        const int Ow = pChain [i+1];

        const int TmpDst1 = m_iwow_trie.GetDest (State1, Iw);
        const int Dst1 = m_iwow_trie.GetDest (TmpDst1, Ow);
        DebugLogAssert (0 < Dst1 && 0 < TmpDst1); // as chains is already there

        const int Dst2 = m_iw_trie.GetDest (State2, Iw);

        if (0 < Dst2) {

            // update frequency
            DebugLogAssert (m_iw_state2freq.size () > (unsigned int) Dst2);
            m_iw_state2freq [Dst2] += Freq;

            // update best IwOw state candidate
            DebugLogAssert (m_state2state.size () > (unsigned int) Dst2);
            const int BestDst1 = m_state2state [Dst2];
            DebugLogAssert (-1 != BestDst1);

            const int BestFreq = m_iwow_state2freq [BestDst1];
            const int Dst1Freq = m_iwow_state2freq [Dst1];

            if (BestFreq < Dst1Freq) {
                m_state2state [Dst2] = Dst1;
            }

            // move on
            State1 = Dst1;
            State2 = Dst2;

        } else {

            AddTail_iw (State1, State2, pChain + i, Size - i, Freq);
            break;
        }
    }
}


inline void FAIwOwSuffArr2Patterns::
    UpdateAlphabets (const int * pChain, const int Size)
{
    DebugLogAssert (0 == (Size % 2) && 0 < Size);

    for (int i = 0; i < Size; i += 2) {

        const int Iw = pChain [i];
        const int Ow = pChain [i + 1];

        m_iws.push_back (Iw);
        m_ows.push_back (Ow);
    }

    const int NewIwsCount = FASortUniq (m_iws.begin (), m_iws.end ());
    m_iws.resize (NewIwsCount);

    const int NewOwsCount = FASortUniq (m_ows.begin (), m_ows.end ());
    m_ows.resize (NewOwsCount);
}


void FAIwOwSuffArr2Patterns::
    AddChain (const int * pChain, const int Size, const int Freq)
{
    // check whether Size is ok
    if ((Size / 2) > FALimits::MaxWordLen || 0 != (Size % 2)) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    // check whether we can generate a chunk of patterns
    if (HasPrefChanged (pChain, Size)) {
        BuildPatterns ();
        Clear ();
    }

    // add a new chain
    AddChain_int (pChain, Size, Freq);

    // update the prev chain
    m_tmp_arr.resize (Size);
    memcpy (m_tmp_arr.begin (), pChain, sizeof (int) * Size);

    // update alphabets
    UpdateAlphabets (pChain, Size);
}


inline void FAIwOwSuffArr2Patterns::
    SetIwOw (const int Iw, const int Ow, const int Pos)
{
    DebugLogAssert (0 == (m_pat.size () % 2));
    DebugLogAssert (-1 == Pos || (unsigned int) Pos <= m_pat.size () / 2);

    if (0 <= Pos) {

        const int Size_2 = m_pat.size () / 2;

        if (Pos < Size_2) {

            const int Idx = 2 * Pos;
            m_pat [Idx] = Iw;
            m_pat [Idx + 1] = Ow;

        } else {

            DebugLogAssert (Pos == Size_2);
            m_pat.push_back (Iw);
            m_pat.push_back (Ow);
        }
    }
}


void FAIwOwSuffArr2Patterns::BuildPatterns ()
{
    const int * pIws = m_iws.begin ();
    const int IwsCount = m_iws.size ();

    const int * pOws = m_ows.begin ();
    const int OwsCount = m_ows.size ();

    // use m_tmp_arr as a stack of [InIw, InOw, IwOw-State, Iw-State, Depth]
    m_tmp_arr.resize (0);

    m_tmp_arr.push_back (-1);
    m_tmp_arr.push_back (-1);
    m_tmp_arr.push_back (m_iwow_trie.GetInitial ());
    m_tmp_arr.push_back (m_iw_trie.GetInitial ());
    m_tmp_arr.push_back (0);

    // do depth-first traversal and take patterns of the smallest depth
    while (!m_tmp_arr.empty ()) {

        const int StackSize = m_tmp_arr.size ();
        DebugLogAssert (0 == (StackSize % 5));

        const int InIw = m_tmp_arr [StackSize - 5];
        const int InOw = m_tmp_arr [StackSize - 4];
        const int State = m_tmp_arr [StackSize - 3];
        const int q = m_tmp_arr [StackSize - 2];
        const int Depth = m_tmp_arr [StackSize - 1];
        m_tmp_arr.resize (StackSize - 5);

        // set Iw:Ow pattern symbol at position Depth - 1
        SetIwOw (InIw, InOw, Depth - 1);

        const int NewDepth = Depth + 1;

        // make reverse iterations so patterns will come out almost in
        // the lexicographical order
        for (int i = IwsCount - 1; 0 <= i; --i) {

            const int Iw = pIws [i];
            const int Dst = m_iwow_trie.GetDest (State, Iw);

            if (0 > Dst)
                continue;

            const int p = m_iw_trie.GetDest (q, Iw);
            DebugLogAssert (0 <= p && (unsigned int) p < m_state2state.size ());

            // see whether depth is long enough
            if (NewDepth >= m_MinPatLen) {

                int BestOw = -1;
                const int BestState = m_state2state [p];
                DebugLogAssert (0 <= BestState);

                // calculate the precision of this best state
                const int BestFreq = m_iwow_state2freq [BestState];
                const int AllFreq = m_iw_state2freq [p];
                DebugLogAssert (0 < BestFreq && BestFreq <= AllFreq);

                // do floating operations when necessary
                if (AllFreq == BestFreq || \
                    m_MinPatPrec <= 100 * float (BestFreq)/float (AllFreq)) {

                    for (int j = 0; j < OwsCount; ++j) {
                        const int Ow = pOws [j];
                        if (BestState == m_iwow_trie.GetDest (Dst, Ow)) {
                            DebugLogAssert (-1 == BestOw);
                            BestOw = Ow;
                        }
                    }
                    DebugLogAssert (-1 != BestOw);
                }

                // see whether we can generate a pattern
                if (-1 != BestOw) {
                    // set final Iw:Ow pair
                    SetIwOw (Iw, BestOw, NewDepth - 1);
                    // get the frequency
                    const int Freq = m_iwow_state2freq [BestState];
                    // put next pattern
                    PutPattern (m_pat.begin (), 2 * NewDepth, Freq);
                    continue;
                }
            }
            // go deeper, make reverse Ow-iteration
            for (int j = OwsCount - 1; 0 <= j; --j) {

                const int Ow = pOws [j];
                const int DstDst = m_iwow_trie.GetDest (Dst, Ow);

                if (-1 != DstDst) {
                    m_tmp_arr.push_back (Iw);
                    m_tmp_arr.push_back (Ow);
                    m_tmp_arr.push_back (DstDst);
                    m_tmp_arr.push_back (p);
                    m_tmp_arr.push_back (NewDepth);
                }
            }
        } // of for (int i = 0; i < IwsCount; ...
    } // of while (!m_tmp_arr.empty ()) ...
}


void FAIwOwSuffArr2Patterns::Process ()
{
    // build patterns
    BuildPatterns ();
    Clear ();
}


void FAIwOwSuffArr2Patterns::PutPattern (const int *, const int, const int)
{}

}

