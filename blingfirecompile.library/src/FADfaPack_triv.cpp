/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADfaPack_triv.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAState2OwsA.h"
#include "FAMealyDfaA.h"
#include "FAEncodeUtils.h"
#include "FAException.h"
#include "FAUtils.h"
#include "FAFsmConst.h"

#include <algorithm>

namespace BlingFire
{


FADfaPack_triv::FADfaPack_triv (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pState2Ow (NULL),
    m_pState2Ows (NULL),
    m_pSigma (NULL),
    m_RemapIws (false),
    m_UseIwIA (false),
    m_UseRanges (false),
    m_pOutBuff (NULL),
    m_LastOffset (0),
    m_ows2dump (pAlloc),
    m_iws2dump (pAlloc),
    m_iws2eqs (pAlloc),
    m_DstSize (3),
    m_DstMask (0x00ffffff)
{
    m_state2offset.SetAllocator (pAlloc);
    m_state2offset.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();

    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();

    m_dsts.SetAllocator (pAlloc);
    m_dsts.Create ();

    m_tmp_arr.SetAllocator (pAlloc);
    m_tmp_arr.Create ();

    m_iw2iw.SetAllocator (pAlloc);
    m_iw2iw.Create ();

    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();

    m_eq2iw.SetAllocator (pAlloc);
    m_eq2iw.Create ();

    m_eqs.SetAllocator (pAlloc);
    m_eqs.Create ();

    m_mealy_ows.SetAllocator (pAlloc);
    m_mealy_ows.Create ();

    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();
}


void FADfaPack_triv::SetDfa (const FARSDfaA * pDfa)
{
    m_pDfa = pDfa;
}


void FADfaPack_triv::SetState2Ow (const FAState2OwA * pState2Ow)
{
    m_pState2Ow = pState2Ow;
}


void FADfaPack_triv::SetState2Ows (const FAState2OwsA * pState2Ows)
{
    m_pState2Ows = pState2Ows;
}


void FADfaPack_triv::SetSigma (const FAMealyDfaA * pSigma)
{
    m_pSigma = pSigma;
}


void FADfaPack_triv::SetRemapIws (const bool RemapIws)
{
    m_RemapIws = RemapIws;
}


void FADfaPack_triv::SetUseIwIA (const bool UseIwIA)
{
    m_UseIwIA = UseIwIA;
}


void FADfaPack_triv::SetUseRanges (const bool UseRanges)
{
    m_UseRanges = UseRanges;
}


void FADfaPack_triv::SetDstSize (const int DstSize)
{
    FAAssert (1 <= DstSize && 4 >= DstSize, \
        FAMsg::InvalidParameters);

    m_DstSize = DstSize;
    m_DstMask = 0;

    for (int i = 0; i < m_DstSize; ++i) {
        m_DstMask |= 0xff << (i * 8);
    }
}


const int FADfaPack_triv::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


inline const int FADfaPack_triv::
    GetIWs_eq (const int ** ppIws) const
{
    DebugLogAssert (m_pDfa);

    if (0 == m_eq2iw.size ()) {

        const int Size = m_pDfa->GetIWs (ppIws);
        return Size;

    } else {

        DebugLogAssert (ppIws);
        *ppIws = m_eqs.begin ();
        const int Size = m_eqs.size ();
        return Size;
    }
}


inline const int FADfaPack_triv::
    GetDest_eq (const int State, const int Iw) const
{
    DebugLogAssert (m_pDfa);

    if (0 == m_eq2iw.size ()) {

        const int Dst = m_pDfa->GetDest (State, Iw);
        return Dst;

    } else {

        DebugLogAssert (0 <= Iw && (unsigned int) Iw < m_eq2iw.size ());
        const int OrigIw = m_eq2iw [Iw];

        const int Dst = m_pDfa->GetDest (State, OrigIw);
        return Dst;
    }
}


inline const int FADfaPack_triv::
    GetMealyOws (const int State, const int ** ppOws)
{
    DebugLogAssert (ppOws);

    m_mealy_ows.resize (0);

    bool IsEmpty = true;

    const int * pIws;
    const int Iws = GetIWs_eq (&pIws);

    for (int i = 0; i < Iws; ++i) {

        const int Iw = pIws [i];
        const int Dst = GetDest_eq (State, Iw);

        if (-1 != Dst) {

            const int Ow = m_pSigma->GetOw (State, Iw);
            m_mealy_ows.push_back (Ow);

            if (-1 != Ow) {
                IsEmpty = false;
            }
        }
    }

    if (IsEmpty) {
        return -1;
    }

    *ppOws = m_mealy_ows.begin ();
    return m_mealy_ows.size ();
}


void FADfaPack_triv::BuildIwsDsts (const int State)
{
    DebugLogAssert (m_pDfa);

    const int * pIws;
    const int Size = GetIWs_eq (&pIws);
    DebugLogAssert (0 < Size && pIws);

    m_iws.resize (0);
    m_dsts.resize (0);

    if (m_RemapIws) {

        for (int iw_idx = 0; iw_idx < Size; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = GetDest_eq (State, Iw);

            if (-1 != DstState) {
                const int NewIw = m_iw2iw [Iw];
                m_iws.push_back (NewIw);
                m_dsts.push_back (DstState);
            }
        } // of for (int iw_idx = 0; ...

        const int IwsCount = m_iws.size ();
        m_tmp_arr.resize (IwsCount);

        int i;

        for (i = 0; i < IwsCount; ++i) {
            m_tmp_arr [i] = i;
        }

        // calc transposition for destination states
        const int * pTmpIws = m_iws.begin ();
        DebugLogAssert (pTmpIws);
        std::sort (m_tmp_arr.begin (), m_tmp_arr.end (), FAIdxCmp_s2b (pTmpIws));
        // sort iws in-place
        std::sort (m_iws.begin (), m_iws.end ());

        // make transposition via intermediate array
        for (i = 0; i < IwsCount; ++i) {
            const int Idx = m_tmp_arr [i];
            m_tmp_arr [i] = m_dsts [Idx];
        }
        for (i = 0; i < IwsCount; ++i) {
            m_dsts [i] = m_tmp_arr [i];
        }

    } else {

        for (int iw_idx = 0; iw_idx < Size; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = GetDest_eq (State, Iw);

            if (-1 != DstState) {
                m_iws.push_back (Iw);
                m_dsts.push_back (DstState);
            }
        } // of for (int iw_idx = 0; ...

    } // of if (m_RemapIws) ...
}


const unsigned int FADfaPack_triv::GetIwSize () const
{
    DebugLogAssert (m_iws.size () == m_dsts.size ());

    const int DstCount = m_iws.size ();
    DebugLogAssert (FAIsSortUniqed (m_iws.begin (), DstCount));

    if (0 < DstCount) {

        const unsigned int MaxIw = (unsigned int) m_iws [DstCount - 1];

        if (0 != (0xffff0000 & MaxIw))
            return sizeof (int);
        else if (0 != (0x0000ff00 & MaxIw))
            return sizeof (short);
        else
            return sizeof (char);
    }

    return sizeof (char);
}


const unsigned int FADfaPack_triv::
    GetTrsSize_iwia (const int IwSize) const
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert (0 < DstCount);
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());
    DebugLogAssert (FAIsSortUniqed (m_iws.begin (), DstCount));

    const int IwBase = m_iws [0];
    const int IwMax = m_iws [DstCount - 1];
    const int ArrSize = IwMax - IwBase + 1;
    DebugLogAssert (DstCount <= ArrSize);

    int Size = 0;

    // size += sizeof (IwBase)
    Size += IwSize;
    // size += sizeof (IwMax)
    Size += IwSize;
    // size += sizeof (DstArrayWithGaps)
    Size += (m_DstSize * ArrSize);

    return Size;
}


const unsigned int FADfaPack_triv::
    GetTrsSize_para (const int IwSize) const
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert (0 < DstCount);
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    int Size = 0;

    // size += sizeof (count)
    Size += IwSize;
    // size += sizeof (IwArray) + sizeof (DstArray)
    Size += ((IwSize * DstCount) + (m_DstSize * DstCount));

    return Size;
}


const unsigned int FADfaPack_triv::
    GetTrsSize_impl (const int IwSize) const
{
    DebugLogAssert (m_dsts.size () == m_iws.size ());
    DebugLogAssert (1 == m_dsts.size ());

    return IwSize;
}


const unsigned int FADfaPack_triv::
    GetTrsSize_range (const int IwSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert (0 < DstCount);
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());
    DebugLogAssert (FAIsSortUniqed (m_iws.begin (), DstCount));

    int PrevIw = m_iws [0];
    int PrevDst = m_dsts [0];
    int Ranges = 1;

    for (int i = 1; i < DstCount; ++i) {

        const int Iw = m_iws [i];
        const int Dst = m_dsts [i];

        if (!(Iw == PrevIw + 1 && Dst == PrevDst)) {
            Ranges++;
        }

        PrevIw = Iw;
        PrevDst = Dst;
    }

    const int Size = IwSize + (IwSize * Ranges * 2) + (m_DstSize * Ranges);
    return Size;
}


const int FADfaPack_triv::GetTrType (const int State, const int IwSize)
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (0 < m_dsts.size ());

    // check whether TRS_IMPL can be used 
    if (1 == m_dsts.size () && State + 1 == m_dsts [0]) {
        return FAFsmConst::TRS_IMPL;
    }

    // always prefer IWIA for the initial state
    if (m_UseIwIA && State == m_pDfa->GetInitial ()) {
        return FAFsmConst::TRS_IWIA;
    }

    const int ParaTrsSize = GetTrsSize_para (IwSize);
    DebugLogAssert (0 < ParaTrsSize && ParaTrsSize < INT_MAX);

    int IwiaTrsSize = INT_MAX;
    if (m_UseIwIA) {
        IwiaTrsSize = GetTrsSize_iwia (IwSize);
        DebugLogAssert (0 < IwiaTrsSize && IwiaTrsSize < INT_MAX);
    }

    int RangeTrsSize = INT_MAX;
    if (m_UseRanges) {
        RangeTrsSize = GetTrsSize_range (IwSize);
        DebugLogAssert (0 < RangeTrsSize && RangeTrsSize < INT_MAX);
    }

    if (ParaTrsSize < IwiaTrsSize && ParaTrsSize <= RangeTrsSize) {
        return FAFsmConst::TRS_PARA;
    } else if (RangeTrsSize < IwiaTrsSize) {
        return FAFsmConst::TRS_RANGE;
    } else {
        return FAFsmConst::TRS_IWIA;
    }
}


const unsigned int FADfaPack_triv::GetStateSize (const int State)
{
    // get iws and dsts
    BuildIwsDsts (State);

    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    const int IwSize = GetIwSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
        sizeof (int) >= (unsigned int) IwSize);

    int Size = sizeof (char); // for info byte

    if (0 < DstCount) {

        const int TrsType = GetTrType (State, IwSize);

        if (FAFsmConst::TRS_IWIA == TrsType) {
            Size += GetTrsSize_iwia (IwSize);
        } else if (FAFsmConst::TRS_IMPL == TrsType) {
            Size += GetTrsSize_impl (IwSize);
        } else if (FAFsmConst::TRS_RANGE == TrsType) {
            Size += GetTrsSize_range (IwSize);
        } else {
            DebugLogAssert (FAFsmConst::TRS_PARA == TrsType);
            Size += GetTrsSize_para (IwSize);
        }
    }

    const int OwSize = CalcOwSize (State);
    Size += OwSize;

    return Size;
}


const unsigned int FADfaPack_triv::GetSize ()
{
    DebugLogAssert (m_pDfa);

    unsigned int Size = 0;

    // two offsets: to a long address table and to a reaction sets
    Size += (2 * sizeof (int));

    // get size for the alphabet representation
    const int AlphabetSize = m_alphabet.size ();
    // size of alphabet plus alphabet itself
    Size += (sizeof (int) * (1 + AlphabetSize));

    if (m_RemapIws) {
        // get the Iw2Iw map dump size
        const unsigned char * pIw2IwDump;
        const int Iw2IwDumpSize = m_iws2dump.GetDump (&pIw2IwDump);
        DebugLogAssert (0 < Iw2IwDumpSize && pIw2IwDump);
        DebugLogAssert (0 == (Iw2IwDumpSize % sizeof (int)));

        Size += (sizeof (int) + Iw2IwDumpSize);
    }

    // calc size for states representation
    const int MaxState = m_pDfa->GetMaxState ();
    for (int State = 0; State <= MaxState; ++State) {

        const unsigned int StateSize = GetStateSize (State);
        Size += StateSize;
    }

    // make size aligned by sizeof (int) if it is not
    const int MisAligned = Size % sizeof (int);
    if (0 != MisAligned) {
        Size -= MisAligned;
        Size += sizeof (int);
    }

    return Size;
}


void FADfaPack_triv::EncodeIw (const int Iw, const int IwSize)
{
    DebugLogAssert (m_pOutBuff);

    FAEncode_UC_US_UI(m_pOutBuff, m_LastOffset, Iw, IwSize);
}


const int FADfaPack_triv::DecodeIw (const int Offset, const int IwSize) const
{
    DebugLogAssert (m_pOutBuff);

    unsigned int Iw;
    FADecode_UC_US_UI(m_pOutBuff, Offset, Iw, IwSize);
    return Iw;
}


void FADfaPack_triv::EncodeDst (const int Dst)
{
    if (FAFsmConst::DFA_DEAD_STATE != Dst) {
        // destination state cannot be encoded
        FAAssert ((unsigned) Dst == (m_DstMask & Dst) && \
			(unsigned) Dst != m_DstMask, FAMsg::InvalidParameters);
        FAEncode_1_2_3_4 (m_pOutBuff, m_LastOffset, Dst, m_DstSize);
    } else {
        const int NewDeadState = m_DstMask;
        FAEncode_1_2_3_4 (m_pOutBuff, m_LastOffset, NewDeadState, m_DstSize);
    }
}


const int FADfaPack_triv::DecodeDst (const int Offset) const
{
    DebugLogAssert (m_pOutBuff);

    unsigned int Dst;
    FADecode_1_2_3_4 (m_pOutBuff, Offset, Dst, m_DstSize);

    if (m_DstMask == Dst) {
        return FAFsmConst::DFA_DEAD_STATE;
    } else {
        return Dst;
    }
}


const unsigned int FADfaPack_triv::CalcOwSize (const int State)
{
    int Ow;

    // get Ow or return 0 if does not exist
    if (m_pState2Ow) {

        Ow = m_pState2Ow->GetOw (State);

        if (-1 == Ow) {
            return 0;
        }

    } else if (m_pState2Ows) {

        const int * pOws;
        const int OwsCount = m_pState2Ows->GetOws (State, &pOws);

        if (0 < OwsCount) {
            const int Offset = m_ows2dump.GetOffset (pOws, OwsCount);
            DebugLogAssert (-1 != Offset);
            Ow = Offset;
        } else {
            return 0;
        }

    } else if (m_pSigma) {

        const int * pOws;
        const int OwsCount = GetMealyOws (State, &pOws);

        if (0 < OwsCount) {
            const int Offset = m_ows2dump.GetOffset (pOws, OwsCount);
            DebugLogAssert (-1 != Offset);
            Ow = Offset;
        } else {
            return 0;
        }

    } else {

        return 0;
    }

    // calc the size
    if (*(const char*)&Ow == Ow) {
        return sizeof (char);
    } else if (*(const short*)&Ow == Ow) {
        return sizeof (short);
    } else {
        return sizeof (int);
    }
}


void FADfaPack_triv::EncodeOw (const int State)
{
    DebugLogAssert (m_pOutBuff);

    int Ow;

    // get Ow or return if does not exist
    if (m_pState2Ow) {

        Ow = m_pState2Ow->GetOw (State);
        if (-1 == Ow) {
            return;
        }

    } else if (m_pState2Ows) {

        const int * pOws;
        const int OwsCount = m_pState2Ows->GetOws (State, &pOws);

        if (0 < OwsCount) {
            const int Offset = m_ows2dump.GetOffset (pOws, OwsCount);
            DebugLogAssert (-1 != Offset);
            Ow = Offset;
        } else {
            return;
        }

    } else if (m_pSigma) {

        const int * pOws;
        const int OwsCount = GetMealyOws (State, &pOws);

        if (0 < OwsCount) {
            const int Offset = m_ows2dump.GetOffset (pOws, OwsCount);
            DebugLogAssert (-1 != Offset);
            Ow = Offset;
        } else {
            return;
        }

    } else {

        return;
    }

    const int OwSize = CalcOwSize (State);
    DebugLogAssert (sizeof (char) <= (unsigned int) OwSize && \
            sizeof (int) >= (unsigned int) OwSize);

    // encode Ow
    FAEncode_C_S_I(m_pOutBuff, m_LastOffset, Ow, OwSize);
}


// assumes m_iws, m_dsts contain outgoing transitions sorted by Iw
void FADfaPack_triv::EncodeTrs_impl (const int IwSize)
{
    DebugLogAssert (1 == m_iws.size () && 1 == m_dsts.size ());

#ifndef NDEBUG
    const int OffsetBefore = m_LastOffset;
#endif

    const int Iw = m_iws [0];
    EncodeIw (Iw, IwSize);

    // make sure that calculated size and the actual size are the same
    DebugLogAssert (m_LastOffset - OffsetBefore == GetTrsSize_impl (IwSize));
}


void FADfaPack_triv::EncodeTrs_para (const int IwSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

#ifndef NDEBUG
    const int OffsetBefore = m_LastOffset;
#endif

    // encode (DstCount - 1) next
    EncodeIw (DstCount - 1, IwSize);

    // encode the array of input weights
    int i;
    for (i = 0; i < DstCount; ++i) {
        const int Iw = m_iws [i];
        EncodeIw (Iw, IwSize);
    }

    // encode the array of destination states
    for (i = 0; i < DstCount; ++i) {
        const int Dst = m_dsts [i];
        EncodeDst (Dst);
    }

    // make sure that calculated size and the actual size are the same
    DebugLogAssert (m_LastOffset - OffsetBefore == GetTrsSize_para (IwSize));
}


void FADfaPack_triv::EncodeTrs_iwia (const int IwSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert (0 < DstCount);
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());
    DebugLogAssert (FAIsSortUniqed (m_iws.begin (), DstCount));

#ifndef NDEBUG
    const int OffsetBefore = m_LastOffset;
#endif

    const int * pIws = m_iws.begin ();
    DebugLogAssert (pIws);

    // encode base
    const int IwBase = pIws [0];
    EncodeIw (IwBase, IwSize);

    // encode max
    const int IwMax = pIws [DstCount - 1];
    EncodeIw (IwMax, IwSize);

    // encode array of Dsts, 0 if there is no corresponding Dst
    for (int Iw = IwBase; Iw <= IwMax; ++Iw) {

        const int Idx = FAFind_log (pIws, DstCount, Iw);

        if (-1 != Idx) {
            const int Dst = m_dsts [Idx];
            EncodeDst (Dst);
        } else {
            EncodeDst (0);
        }
    } // of for (int Iw = IwBase; ...

    // make sure that calculated size and the actual size are the same
    DebugLogAssert (m_LastOffset - OffsetBefore == GetTrsSize_iwia (IwSize));
}


void FADfaPack_triv::EncodeTrs_range (const int IwSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert (0 < DstCount);
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());
    DebugLogAssert (FAIsSortUniqed (m_iws.begin (), DstCount));

#ifndef NDEBUG
    const int OffsetBefore = m_LastOffset;
#endif

    // will keep triplets <FromIw, ToIw, Dst>
    m_tmp_arr.resize (0);

    int PrevIw = m_iws [0];
    int PrevDst = m_dsts [0];

    m_tmp_arr.push_back (PrevIw);
    m_tmp_arr.push_back (PrevIw);
    m_tmp_arr.push_back (PrevDst);

    int i;
    for (i = 1; i < DstCount; ++i) {

        const int Iw = m_iws [i];
        const int Dst = m_dsts [i];

        if (Iw == PrevIw + 1 && Dst == PrevDst) {
            m_tmp_arr [m_tmp_arr.size () - 2] = Iw; // update the upper bound
        } else  {
            m_tmp_arr.push_back (Iw);
            m_tmp_arr.push_back (Iw);
            m_tmp_arr.push_back (Dst);
        }

        PrevIw = Iw;
        PrevDst = Dst;
    }

    const int ArrSize = m_tmp_arr.size ();
    DebugLogAssert (0 == ArrSize % 3);

    // encode ranges count: (Ranges - 1)
    const int Ranges = ArrSize / 3;
    EncodeIw (Ranges - 1, IwSize);

    // encode FromIws
    for (i = 0; i < ArrSize; i += 3) {
        const int IwFrom = m_tmp_arr [i];
        EncodeIw (IwFrom, IwSize);
        DebugLogAssert (0 <= IwFrom && IwFrom <= m_tmp_arr [i + 1]);
    }
    // encode ToIws
    for (i = 0; i < ArrSize; i += 3) {
        const int IwTo = m_tmp_arr [i + 1];
        EncodeIw (IwTo, IwSize);
        DebugLogAssert (0 <= m_tmp_arr [i] && m_tmp_arr [i] <= IwTo);
    }
    // encode corresponding Dsts
    for (i = 0; i < ArrSize; i += 3) {
        const int Dst = m_tmp_arr [i + 2];
        EncodeDst (Dst);
    }

    // ensure it is temporary
    m_tmp_arr.resize (0);

    // make sure that calculated size and the actual size are the same
    DebugLogAssert (m_LastOffset - OffsetBefore == GetTrsSize_range (IwSize));
}


void FADfaPack_triv::EncodeInfo (const int State, const int IwSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    unsigned char info = 0;

    if (0 < DstCount) {
        // setup transition representation type
        const int TrsType = GetTrType (State, IwSize);
        info |= TrsType;
        // setup the IwSize
        info |= ((IwSize - 1) << 3);
    }

    const unsigned int OwsSize = CalcOwSize (State);
    unsigned char OwsSizeCode = 0;

    if (sizeof (char) == OwsSize) {
        OwsSizeCode = 1;
    } else if (sizeof (short) == OwsSize) {
        OwsSizeCode = 2;
    } else if (sizeof (int) == OwsSize) {
        OwsSizeCode = 3;
    }
    // put OwsSizeCode into 5th and 6th bits of info
    info |= (OwsSizeCode << 5);

    // setup finality bit, if needed
    if (true == m_finals.get_bit (State)) {
        info |= 0x80;
    }

    // store the info byte
    m_pOutBuff [m_LastOffset] = info;
    m_LastOffset++;
}


void FADfaPack_triv::EncodeState (const int State)
{
    DebugLogAssert (m_pDfa);

    // get iws and dsts
    BuildIwsDsts (State);

    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    m_state2offset [State] = m_LastOffset;

    const int IwSize = GetIwSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && 
            sizeof (int) >= (unsigned int) IwSize);

    // encode info byte
    EncodeInfo (State, IwSize);

    if (0 < DstCount) {

        const int TrsType = GetTrType (State, IwSize);

        if (FAFsmConst::TRS_IWIA == TrsType) {
            EncodeTrs_iwia (IwSize);
        } else if (FAFsmConst::TRS_IMPL == TrsType) {
            EncodeTrs_impl (IwSize);
        } else if (FAFsmConst::TRS_RANGE == TrsType) {
            EncodeTrs_range (IwSize);
        } else {
            DebugLogAssert (FAFsmConst::TRS_PARA == TrsType);
            EncodeTrs_para (IwSize);
        }

    } // of if (0 < DstCount) ...

    // encode Ow, if there is any
    EncodeOw (State);
}


void FADfaPack_triv::
    State2Offset_iwia (int Offset, const int State, const int IwSize)
{
    BuildIwsDsts (State);

    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    const int IwBase = DecodeIw (Offset, IwSize);
    Offset += IwSize;

    const int IwMax = DecodeIw (Offset, IwSize);
    Offset += IwSize;

    const int * pIws = m_iws.begin ();

    for (int Iw = IwBase; Iw <= IwMax; ++Iw) {

        const int Idx = FAFind_log (pIws, DstCount, Iw);

        // remap existing transitions only, the rest was set to 0
        // (0 offsets will represent non-existing transitions)
        if (-1 != Idx) {

            // calc offset where Dst is encoded
            unsigned int DstOffset = ((Iw - IwBase) * m_DstSize) + Offset;
            // take Dst from m_dsts by index, faster than DecodeDst (DstOffset)
            int Dst = m_dsts [Idx];
            // check whether the same is encoded here
            DebugLogAssert (Dst == DecodeDst (DstOffset));

            // don't re-code the dead state
            if (FAFsmConst::DFA_DEAD_STATE == Dst) {
                continue;
            }

            // remap into offset
            Dst = m_state2offset [Dst];
            // as 0-offset stands for non-existing transition
            DebugLogAssert (0 != Dst);
            DebugLogAssert (0 == ((~m_DstMask) & Dst));
            FAEncode_1_2_3_4 (m_pOutBuff, DstOffset, Dst, m_DstSize);
        }
    } // of for (int Iw = IwBase; ...
}


void FADfaPack_triv::
    State2Offset_para (int Offset, const int /*State*/, const int IwSize)
{
    int DstCount;

    // decode DstCount
    DstCount = 1 + DecodeIw (Offset, IwSize);
    Offset += IwSize;

    // skip iws
    Offset += (DstCount * IwSize);

    // change dst states into offsets
    for (int i = 0; i < DstCount; ++i) {

        // get destination state number
        int Dst = DecodeDst (Offset);

        // don't re-code the dead state
        if (FAFsmConst::DFA_DEAD_STATE == Dst) {
            Offset += m_DstSize;
        } else {
            // get destination offset
            Dst = m_state2offset [Dst];
            DebugLogAssert (0 == ((~m_DstMask) & Dst));
            FAEncode_1_2_3_4 (m_pOutBuff, Offset, Dst, m_DstSize);
        }
    } // of for (int i = 0; ...
}


void FADfaPack_triv::
    State2Offset_range (int Offset, const int /*State*/, const int IwSize)
{
    int RangeCount;

    // decode RangeCount
    RangeCount = 1 + DecodeIw (Offset, IwSize);
    Offset += IwSize;

    // skip iws
    Offset += (RangeCount * IwSize * 2);

    // change dst states into offsets
    for (int i = 0; i < RangeCount; ++i) {

        // get destination state number
        int Dst = DecodeDst (Offset);

        // don't re-code the dead state
        if (FAFsmConst::DFA_DEAD_STATE == Dst) {
            Offset += m_DstSize;
        } else {
            // get destination offset
            Dst = m_state2offset [Dst];
            DebugLogAssert (0 == ((~m_DstMask) & Dst));
            FAEncode_1_2_3_4 (m_pOutBuff, Offset, Dst, m_DstSize);
        }
    } // of for (int i = 0; ...
}


void FADfaPack_triv::State2Offset (const int State)
{
    DebugLogAssert (m_pOutBuff);

    unsigned int Offset = m_state2offset [State];
    const unsigned char info = m_pOutBuff [Offset];
    Offset++;

    const int IwSize = ((info & 0x18) >> 3) + 1;
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && 
            sizeof (int) >= (unsigned int) IwSize);

    const char TrType = info & 0x07;

    if (FAFsmConst::TRS_PARA == TrType) {
        State2Offset_para (Offset, State, IwSize);
    } else if (FAFsmConst::TRS_IWIA == TrType) {
        State2Offset_iwia (Offset, State, IwSize);
    } else if (FAFsmConst::TRS_RANGE == TrType) {
        State2Offset_range (Offset, State, IwSize);
    }
}


void FADfaPack_triv::BuildIwMap ()
{
    DebugLogAssert (m_pDfa);

    const int * pIws;
    const int IwsCount = GetIWs_eq (&pIws);
    DebugLogAssert (0 < IwsCount && pIws && 0 <= *pIws);
    DebugLogAssert (true == FAIsSortUniqed (pIws, IwsCount));

    const int MaxIw = pIws [IwsCount - 1];
    // m_tmp_arr will map each Iw into its frequency of usage
    m_tmp_arr.resize (MaxIw + 1);
    // m_iw2iw maps old Iws into new consequent Iws
    m_iw2iw.resize (MaxIw + 1);

    for (int iw = 0; iw <= MaxIw; ++iw) {
        m_tmp_arr [iw] = 0;
        m_iw2iw [iw] = -1;
    }

    const int MaxState = m_pDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = GetDest_eq (State, Iw);

            if (-1 != DstState) {
                m_tmp_arr [Iw]++;
            }
        }
    } // of for (int State = 0; ...

    int i;

    m_iws.resize (IwsCount);

    for (i = 0; i < IwsCount; ++i) {

        const int Iw = pIws [i];
        m_iws [i] = Iw;
    }

    const int * pTmpArr = m_tmp_arr.begin ();
    DebugLogAssert (pTmpArr);
    std::sort (m_iws.begin (), m_iws.end (), FAIdxCmp_b2s (pTmpArr));

    for (i = 0; i < IwsCount; ++i) {
        const int OldIw = m_iws [i];
        m_iw2iw [OldIw] = i;
    }
}


void FADfaPack_triv::BuildIw2IwDump ()
{
    DebugLogAssert (m_pDfa);
    DebugLogAssert (m_RemapIws && 0 < m_iw2iw.size ());

    int NewIw;

    // get the original alphabet
    const int * pIws;
    const int IwsCount = m_pDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwsCount && pIws && 0 <= *pIws);
    DebugLogAssert (true == FAIsSortUniqed (pIws, IwsCount));

    // m_tmp_arr will keep parallel to pIws array of new Iws
    m_tmp_arr.resize (IwsCount);

    for (int i = 0; i < IwsCount; ++i) {

        const int OrigIw = pIws [i];

        if (0 != m_eq2iw.size ()) {

            const int * pEq = m_iw2eq.Get (OrigIw);
            DebugLogAssert (pEq);
            const int Eq = *pEq;
            DebugLogAssert (0 <= Eq && (unsigned int) Eq < m_iw2iw.size ());
            NewIw = m_iw2iw [Eq];

        } else {

            NewIw = m_iw2iw [OrigIw];
        }

        DebugLogAssert (-1 != NewIw);
        m_tmp_arr [i] = NewIw;
    }

    // build Iw2Iw map dump
    m_iws2dump.SetIws (pIws, m_tmp_arr.begin (), IwsCount);
    m_iws2dump.Process ();
}


void FADfaPack_triv::BuildEqs ()
{
    DebugLogAssert (m_pDfa && m_RemapIws);

    const int * pIws;
    const int IwCount = m_pDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwCount));

    const int MaxIw = pIws [IwCount - 1];

    m_iws2eqs.SetIwMax (MaxIw);
    m_iws2eqs.SetRsDfa (m_pDfa);
    m_iws2eqs.SetIw2NewIw (&m_iw2eq);
    m_iws2eqs.Process ();

    const int EqMax = m_iws2eqs.GetMaxNewIw ();
    DebugLogAssert (0 <= EqMax && EqMax < IwCount);

    // see whether it make sense to renum Iws into Eqs
    if (EqMax + 1 < IwCount) {

        m_eq2iw.resize (EqMax + 1);
        m_eqs.resize (EqMax + 1);

        for (int eq = 0; eq <= EqMax; ++eq) {
            m_eqs [eq] = eq;
        }

        int Iw = -1;
        const int * pEq = m_iw2eq.Prev (&Iw);

        while (NULL != pEq) {

            const int Eq = *pEq;
            DebugLogAssert (0 <= Eq && Eq <= EqMax);
            DebugLogAssert (Iw <= MaxIw);

            m_eq2iw [Eq] = Iw;
            pEq = m_iw2eq.Prev (&Iw);
        }
    } // of if (EqMax + 1 < IwCount) ...
}


void FADfaPack_triv::PackAlphabet ()
{
    DebugLogAssert (m_pDfa);

    m_alphabet.resize (0);

    const int * pIws;
    const int IwsCount = m_pDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwsCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwsCount));

    int PrevIw = -2;

    for (int i = 0; i < IwsCount; ++i) {

        const int Iw = pIws [i];
        DebugLogAssert (0 <= Iw && Iw <= FALimits::MaxIw);

        if (PrevIw + 1 < Iw) {
            // add a new range
            m_alphabet.push_back (Iw);
            m_alphabet.push_back (Iw);
        } else {
            // update the upper bound
            DebugLogAssert (PrevIw + 1 == Iw);
            DebugLogAssert (0 < m_alphabet.size ());
            m_alphabet [m_alphabet.size () - 1] = Iw;
        }

        PrevIw = Iw;
    }
}


void FADfaPack_triv::Prepare ()
{
    DebugLogAssert (m_pDfa);

    m_dump.Clear ();
    m_LastOffset = 0;
    m_ows2dump.Clear ();
    m_iw2eq.Clear ();
    m_eq2iw.resize (0);

    PackAlphabet ();

    const int MaxState = m_pDfa->GetMaxState ();

    m_finals.resize (MaxState + 1);    
    m_finals.set_bits (0, MaxState, false);

    m_state2offset.resize (MaxState + 1);

    int i;
    for (i = 0; i <= MaxState; ++i) {
        m_state2offset [i] = 0;
    }

    const int * pFinals;
    const int FinalsCount = m_pDfa->GetFinals (&pFinals);
    DebugLogAssert (0 < FinalsCount && pFinals);

    // get final states
    for (i = 0; i < FinalsCount; ++i) {
        const int FinalState = pFinals [i];
        m_finals.set_bit (FinalState, true);
    }

    // build Old2New Iw map, if needed
    if (m_RemapIws) {
        BuildEqs ();
        BuildIwMap ();
        BuildIw2IwDump ();
    }

    // pack Ow sets, if specified
    if (m_pState2Ows || m_pSigma) {
        DebugLogAssert (NULL == m_pState2Ow);
        PackOws ();
    }

    // calc automaton's size and allocate memory
    const int AutSize = GetSize ();
    m_dump.Create (AutSize);
    m_dump.resize (AutSize);

    // get the dump pointer
    m_pOutBuff = m_dump.begin ();

    // remove non-initialized data, due to alignment
    memset (m_pOutBuff, 0, AutSize);
}


void FADfaPack_triv::PackOws ()
{
    DebugLogAssert (m_pDfa && (m_pState2Ows || m_pSigma));

    const int * pOws = NULL;
    int OwsCount = 0;

    const int MaxState = m_pDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        if (m_pState2Ows) {

            OwsCount = m_pState2Ows->GetOws (State, &pOws);

        } else if (m_pSigma) {

            OwsCount = GetMealyOws (State, &pOws);
        }

        if (0 < OwsCount) {
            m_ows2dump.Add (pOws, OwsCount);
        }
    } 

    m_ows2dump.Process ();
}


void FADfaPack_triv::StoreIws ()
{
    int i;

    // get packed alphabet
    const int * pAlphabet = m_alphabet.begin ();
    const int AlphabetCount = m_alphabet.size ();
    DebugLogAssert (0 < AlphabetCount && pAlphabet);

    // store the original alphabet size and Iw2Iw map presence indicator
    if (m_RemapIws) {
        // setup the highest bit in AlphabetCount if Iws remapped
        *(unsigned int *)(m_pOutBuff + m_LastOffset) = 
            0x80000000 | (unsigned int) AlphabetCount;
        m_LastOffset += sizeof (int);

    } else {

        *(int *)(m_pOutBuff + m_LastOffset) = AlphabetCount;
        m_LastOffset += sizeof (int);
    }

    // store the alphabet
    for (i = 0; i < AlphabetCount; ++i) {
        const int Iw = pAlphabet [i];
        *((int *)(m_pOutBuff + m_LastOffset)) = Iw;
        m_LastOffset += sizeof (int);
    }

    // store Iw2Iw map dump, if needed
    if (m_RemapIws) {
        // get dump
        const unsigned char * pIw2IwDump;
        const int Iw2IwDumpSize = m_iws2dump.GetDump (&pIw2IwDump);
        DebugLogAssert (0 < Iw2IwDumpSize && pIw2IwDump);
        DebugLogAssert (0 == (Iw2IwDumpSize % sizeof (int)));
        // store Iw2Iw size
        *((unsigned int *)(m_pOutBuff + m_LastOffset)) = Iw2IwDumpSize;
        m_LastOffset += sizeof (int);
        // store map dump
        memcpy (m_pOutBuff + m_LastOffset, pIw2IwDump, Iw2IwDumpSize);
        m_LastOffset += Iw2IwDumpSize;
    }
}


void FADfaPack_triv::StoreStates ()
{
    const int MaxState = m_pDfa->GetMaxState ();

    // store states into buffer
    int State;
    for (State = 0; State <= MaxState; ++State) {
        EncodeState (State);
    }

    const int MaxOffset = m_state2offset [MaxState];
    if (m_DstMask < (unsigned int) MaxOffset) {
        // a transition function is too big, use other packing algorithm
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    // change indices to offsets
    for (State = 0; State <= MaxState; ++State) {
        State2Offset (State);
    }
}


void FADfaPack_triv::StoreOws ()
{
    if (m_pState2Ows || m_pSigma) {

        // get dump
        const unsigned char * pOwsDump;
        const int OwsDumpSize = m_ows2dump.GetDump (&pOwsDump);
        DebugLogAssert (0 < OwsDumpSize && pOwsDump);
        DebugLogAssert (0 == (OwsDumpSize % sizeof (int)));

        // resize the output dump
        const int TableOffset = m_dump.size ();
        m_dump.resize (TableOffset + OwsDumpSize);
        m_pOutBuff = m_dump.begin ();
        DebugLogAssert (m_pOutBuff);

        // store encoded ows offset
        *(int *)(m_pOutBuff + sizeof (int)) = TableOffset;

        // copy Ows data
        memcpy (m_pOutBuff + TableOffset, pOwsDump, OwsDumpSize);
    }
}


void FADfaPack_triv::Process ()
{
    DebugLogAssert (m_pDfa);

    // see whether current algorithm can be used with this automaton
    if (m_DstMask <= (unsigned int) m_pDfa->GetMaxState ()) {
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    // 1. return all structures into the initial state
    // 2. calc automaton size
    // 3. allocate memory
    Prepare ();
    DebugLogAssert (m_pOutBuff && 0 == m_LastOffset);

    // <Header>
    // store DstSize
    *(int *)(m_pOutBuff + m_LastOffset) = m_DstSize;
    m_LastOffset += sizeof (int);
    // offset of Ows sets, 0 if there are no FAState2OwsA specified
    *(int *)(m_pOutBuff + m_LastOffset) = 0;
    m_LastOffset += sizeof (int);

    // <Body>
    StoreIws ();
    StoreStates ();
    StoreOws ();
}

}
