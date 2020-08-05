/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAPosNfaPack_triv.h"
#include "FARSNfaA.h"
#include "FAMultiMapA.h"
#include "FAEncodeUtils.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FAPosNfaPack_triv::FAPosNfaPack_triv (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_pPos2BrBegin (NULL),
    m_pPos2BrEnd (NULL),
    m_pOutBuff (NULL),
    m_LastOffset (0),
    m_dsts2dump (pAlloc),
    m_offsets2dump (pAlloc),
    m_trbrs2dump (pAlloc),
    m_iws2dump (pAlloc),
    m_SizeOfTrBrOffset (0)
{
    m_state2offset.SetAllocator (pAlloc);
    m_state2offset.Create ();

    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();

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
}


void FAPosNfaPack_triv::SetNfa (const FARSNfaA * pNfa)
{
    m_pNfa = pNfa;
}


void FAPosNfaPack_triv::SetPos2BrBegin (const FAMultiMapA * pPos2BrBegin)
{
    m_pPos2BrBegin = pPos2BrBegin;
}


void FAPosNfaPack_triv::SetPos2BrEnd (const FAMultiMapA * pPos2BrEnd)
{
    m_pPos2BrEnd = pPos2BrEnd;
}


const int FAPosNfaPack_triv::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


const int FAPosNfaPack_triv::GetDest (const int State, const int Iw)
{
    DebugLogAssert (m_pNfa);

    const int * pDsts;
    const int DstCount = m_pNfa->GetDest (State, Iw, &pDsts);

    if (0 < DstCount) {

        DebugLogAssert (pDsts);

        unsigned int Dst;

        if (1 == DstCount) {

            // ( SrcState XOR DstState ) SHL 1
            Dst = *(const unsigned int*)pDsts;
            Dst = Dst ^ State;
            Dst = Dst << 1;

        } else {

            // ( DstSetOffset SHL 1 ) | 1
            const int Offset = m_dsts2dump.GetOffset (pDsts, DstCount);
            DebugLogAssert (-1 != Offset);
            Dst = Offset;
            Dst = (Dst << 1) | 1;
        }

        DebugLogAssert (0 == (0x80000000 & Dst));
        return Dst;
    }

    return -1;
}


void FAPosNfaPack_triv::BuildIwsDsts (const int State)
{
    DebugLogAssert (m_pNfa);

    const int * pIws;
    const int Size = m_pNfa->GetIWs (State, &pIws);

    m_iws.resize (0);
    m_dsts.resize (0);

    if (0 < Size) {

        for (int iw_idx = 0; iw_idx < Size; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = GetDest (State, Iw);

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
    }
}


// relys on data from the prev BuildIwsDsts call
const unsigned int FAPosNfaPack_triv::CalcIwSize () const
{
    DebugLogAssert (m_iws.size () == m_dsts.size ());

    int curr_size;
    int max_size = sizeof (char);

    const int DstCount = m_iws.size ();

    for (int i = 0; i < DstCount; ++i) {

        DebugLogAssert (-1 != m_dsts [i]);

        const int Iw = m_iws [i];

        if (0xffff0000 & Iw)
            curr_size = sizeof (int);
        else if (0x0000ff00 & Iw)
            curr_size = sizeof (short);
        else
            curr_size = sizeof (char);

        if (max_size < curr_size)
            max_size = curr_size;

        if (sizeof (int) == max_size)
            break;
    }

    return max_size;
}


inline const int FAPosNfaPack_triv::SizeOfValue (const int Value)
{
    if (0xff000000 & Value)
        return 4;
    else if (0x00ff0000 & Value)
        return 3;
    else if (0x0000ff00 & Value)
        return 2;
    else
        return 1;
}


// relys on data from the prev BuildIwsDsts call
const unsigned int FAPosNfaPack_triv::CalcDstSize () const
{
    DebugLogAssert (m_iws.size () == m_dsts.size ());

    int curr_size;
    int max_size = 1;

    const int DstCount = m_iws.size ();

    for (int i = 0; i < DstCount; ++i) {

        DebugLogAssert (-1 != m_iws [i]);
        const int Dst = m_dsts [i];

        curr_size = SizeOfValue (Dst);

        if (max_size < curr_size)
            max_size = curr_size;

        if (4 == max_size)
            break;
    }

    return max_size;
}


const unsigned int FAPosNfaPack_triv::GetStateSize (const int State)
{
    // get iws and dsts
    BuildIwsDsts (State);

    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    const int IwSize = CalcIwSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
            sizeof (int) >= (unsigned int) IwSize);

    const int DstSize = CalcDstSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) DstSize && \
            sizeof (int) >= (unsigned int) DstSize);

    // <info>
    int Size = sizeof (char);

    // see whether (DstCount + 1) does not fit highest 3 bits of <info>
    if (4 <= DstCount + 1) {
        Size += IwSize;
    }

    // size += sizeof (IwArray) + sizeof (DstArray)
    Size += ((IwSize * DstCount) + (DstSize * DstCount));

    // see whether TrBr-maps have entries at this state
    if (NULL != m_pPos2BrBegin && NULL != m_pPos2BrEnd) {

        const int * pTrBrs;
        int Count;
        // beginning map
        Count = m_pPos2BrBegin->Get (State, &pTrBrs);
        if (0 < Count) {
            Size += m_SizeOfTrBrOffset;
        }
        // ending map
        Count = m_pPos2BrEnd->Get (State, &pTrBrs);
        if (0 < Count) {
            Size += m_SizeOfTrBrOffset;
        }
    }

    return Size;
}


const unsigned int FAPosNfaPack_triv::GetSize ()
{
    DebugLogAssert (m_pNfa);

    const unsigned char * pDump;
    int DumpSize;

    unsigned int Size = 0;

    // 1. offset to the packed destination sets
    // 2. offset to the encoded trbr arrays
    // 3. global size of TrBr offset
    // 4. offset to the state2offset table
    Size += (4 * sizeof (int));

    // see how much will initials take
    const int * pInitials;
    const int InitialCount = m_pNfa->GetInitials (&pInitials);
    DebugLogAssert (0 < InitialCount && pInitials);
    // initial states as array of ints
    Size += ((InitialCount + 1) * sizeof (int));

    // Iw2Iw map size and map itself
    DumpSize = m_iws2dump.GetDump (&pDump);
    DebugLogAssert (0 < DumpSize && 0 == (DumpSize % sizeof (int)));
    Size += (sizeof (int) + DumpSize);

    // calc size for states representation
    const int MaxState = m_pNfa->GetMaxState ();
    for (int State = 0; State <= MaxState; ++State) {
        const unsigned int StateSize = GetStateSize (State);
        Size += StateSize;
    }

    // make it sizeof (int) aligned
    const int MisAligned = Size % sizeof (int);
    if (0 != MisAligned) {
        Size -= MisAligned;
        Size += sizeof (int);
    }

    // plus packed destination sets dump size
    DumpSize = m_dsts2dump.GetDump (&pDump);
    DebugLogAssert (0 <= DumpSize && 0 == (DumpSize % sizeof (int)));
    Size += DumpSize;

    // plus triangular brackets information dump size
    DumpSize = m_trbrs2dump.GetDump (&pDump);
    DebugLogAssert (0 <= DumpSize && 0 == (DumpSize % sizeof (int)));
    Size += DumpSize;

    return Size;
}


void FAPosNfaPack_triv::EncodeIw (const int Iw, const int IwSize)
{
    FAEncode_UC_US_UI (m_pOutBuff, m_LastOffset, Iw, IwSize);
}


void FAPosNfaPack_triv::EncodeDst (const int Dst, const int DstSize)
{
    FAEncode_1_2_3_4 (m_pOutBuff, m_LastOffset, Dst, DstSize);
}


// assumes m_iws, m_dsts contain outgoing transitions sorted by Iw
void FAPosNfaPack_triv::EncodeTrs (const int IwSize, const int DstSize)
{
    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    // encode DstCount + 1, if does not fit into <info>'s two highest bits
    if (4 <= DstCount + 1) {
        EncodeIw (DstCount, IwSize);
    }

    // encode array of input weights
    int i;
    for (i = 0; i < DstCount; ++i) {
        const int Iw = m_iws [i];
        EncodeIw (Iw, IwSize);
    }

    // encode array of destination states
    for (i = 0; i < DstCount; ++i) {
        const int Dst = m_dsts [i];
        EncodeDst (Dst, DstSize);
    }
}


// assumes m_iws, m_dsts contain outgoing transitions sorted by Iw
void FAPosNfaPack_triv::
    EncodeInfo (const int State, const int IwSize, const int DstSize)
{
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
            sizeof (int) >= (unsigned int) IwSize);
    DebugLogAssert (sizeof (char) <= (unsigned int) DstSize && \
            sizeof (int) >= (unsigned int) DstSize);

    const int DstCount = m_dsts.size ();
    DebugLogAssert ((unsigned int) DstCount == m_iws.size ());

    const int * pTrBr;
    int TrBrCount;

    unsigned char info = 0;

    // see whether State begins some TrBr-s
    if (m_pPos2BrBegin) {
        // get TrBrs
        TrBrCount = m_pPos2BrBegin->Get (State, &pTrBr);
        // set 0-th bit 
        if (0 < TrBrCount) {
            info |= 1;
        }
    }
    // see whether State ends some TrBr-s
    if (m_pPos2BrBegin) {
        // get TrBrs
        TrBrCount = m_pPos2BrEnd->Get (State, &pTrBr);
        // set 0-th bit 
        if (0 < TrBrCount) {
            info |= 2;
        }
    }
    // put (IwSize - 1) into 2nd and 3rd bits of <info>
    info |= ((IwSize - 1) << 2);
    // put (DstSize - 1) into 4th and 5th bits of <info>
    info |= ((DstSize - 1) << 4);
    // see whether DstCount + 1 fits 6th and 7th bits of <info>
    if (4 > DstCount + 1) {
        info |= ((DstCount + 1) << 6);
    }

    // store <info>
    m_pOutBuff [m_LastOffset] = info;
    m_LastOffset++;
}


void FAPosNfaPack_triv::EncodeTrBr (const int State)
{
    if (NULL != m_pPos2BrBegin && NULL != m_pPos2BrEnd) {

        const int * pTrBrs;
        int Count;
        // beginning map
        Count = m_pPos2BrBegin->Get (State, &pTrBrs);
        if (0 < Count) {
            const int Offset = m_trbrs2dump.GetOffset (pTrBrs, Count);
            EncodeDst (Offset, m_SizeOfTrBrOffset);
        }
        // ending map
        Count = m_pPos2BrEnd->Get (State, &pTrBrs);
        if (0 < Count) {
            const int Offset = m_trbrs2dump.GetOffset (pTrBrs, Count);
            EncodeDst (Offset, m_SizeOfTrBrOffset);
        }
    }
}


void FAPosNfaPack_triv::EncodeState (const int State)
{
    // get iws and dsts
    BuildIwsDsts (State);
    DebugLogAssert (m_dsts.size () == m_iws.size ());

    m_state2offset [State] = m_LastOffset;

    const int IwSize = CalcIwSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
            sizeof (int) >= (unsigned int) IwSize);

    const int DstSize = CalcDstSize ();
    DebugLogAssert (sizeof (char) <= (unsigned int) DstSize && \
            sizeof (int) >= (unsigned int) DstSize);

    // encode info byte
    EncodeInfo (State, IwSize, DstSize);

    // encode transitions
    EncodeTrs (IwSize, DstSize);

    // encode trbr offsets, if any
    EncodeTrBr (State);
}


void FAPosNfaPack_triv::CalcIwMap ()
{
    DebugLogAssert (m_pNfa);

    const int * pIws = m_alphabet.begin ();
    const int IwsCount = m_alphabet.size ();
    DebugLogAssert (0 < IwsCount && pIws);
    DebugLogAssert (true == FAIsSortUniqed (pIws, IwsCount));

    const int MaxIw = pIws [IwsCount - 1];
    DebugLogAssert (m_pNfa->GetMaxIw () == MaxIw);

    m_tmp_arr.resize (MaxIw + 1);
    m_iw2iw.resize (MaxIw + 1);

    for (int iw = 0; iw <= MaxIw; ++iw) {
        m_tmp_arr [iw] = 0;
        m_iw2iw [iw] = -1;
    }

    const int MaxState = m_pNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int * pDsts;
            const int DstCount = m_pNfa->GetDest (State, Iw, &pDsts);

            if (0 < DstCount) {
                m_tmp_arr [Iw]++;
            }
        }
    } // of for (int State = 0; ...

    m_iws.resize (IwsCount);

    int i;
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

    // m_tmp_arr will keep parallel to pIws array of new Iws
    m_tmp_arr.resize (IwsCount);

    for (i = 0; i < IwsCount; ++i) {

        const int OldIw = pIws [i];
        const int NewIw = m_iw2iw [OldIw];
        DebugLogAssert (-1 != NewIw);
        m_tmp_arr [i] = NewIw;
    }

    // build Iw2Iw map dump
    m_iws2dump.SetIws (pIws, m_tmp_arr.begin (), IwsCount);
    m_iws2dump.Process ();
}


void FAPosNfaPack_triv::CalcAlphabet ()
{
    DebugLogAssert (m_pNfa);

    m_alphabet.resize (0);

    const int MaxState = m_pNfa->GetMaxState ();

    for (int i = 0; i <= MaxState; ++i) {

        m_state2offset [i] = 0;

        /// TODO: calculate alphabet via bit-vector
        const int * pIws;
        const int IwsCount = m_pNfa->GetIWs (i, &pIws);

        if (0 < IwsCount) {

            DebugLogAssert (pIws);

            const int OldSize = m_alphabet.size ();
            m_alphabet.resize (OldSize + IwsCount);

            int * pBegin = m_alphabet.begin ();
            int * pEnd = m_alphabet.end ();

            memcpy (pBegin + OldSize, pIws, IwsCount * sizeof (int));
            const int NewSize = FASortUniq (pBegin, pEnd);
            m_alphabet.resize (NewSize);
        }
    }
}


void FAPosNfaPack_triv::CalcDestDump ()
{
    DebugLogAssert (m_pNfa);

    m_dsts2dump.Clear ();

    const int MaxState = m_pNfa->GetMaxState ();   

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwsCount = m_pNfa->GetIWs (State, &pIws);

        for (int j = 0; j < IwsCount; ++j) {

            DebugLogAssert (pIws);
            const int Iw = pIws [j];

            const int * pDsts;
            const int Count = m_pNfa->GetDest (State, Iw, &pDsts);

            // two and more destination states only
            if (1 < Count) {
                m_dsts2dump.Add (pDsts, Count);
            }
        }
    }

    m_dsts2dump.Process ();
}


void FAPosNfaPack_triv::CalcTrBrDump ()
{
    DebugLogAssert (m_pNfa);

    m_trbrs2dump.Clear ();
    m_SizeOfTrBrOffset = 0;

    if (NULL != m_pPos2BrBegin && NULL != m_pPos2BrEnd) {

        int Count;
        int State;
        const int * pTrBrs;

        const int MaxState = m_pNfa->GetMaxState ();

        for (State = 0; State <= MaxState; ++State) {

            // get the array from the beginning trbr map
            Count = m_pPos2BrBegin->Get (State, &pTrBrs);
            // add it if not empty
            if (0 < Count) {
                m_trbrs2dump.Add (pTrBrs, Count);
            }
            // get the array from the ending trbr map
            Count = m_pPos2BrEnd->Get (State, &pTrBrs);
            // add it if not empty
            if (0 < Count) {
                m_trbrs2dump.Add (pTrBrs, Count);
            }
        }

        // pack arrays as ints
        m_trbrs2dump.SetSizeOfValue (sizeof (int));
        m_trbrs2dump.Process ();

        // calc max size of TrBrOffset
        const unsigned char * pDump;
        const int MaxOffset = m_trbrs2dump.GetDump (&pDump) - 1;
        DebugLogAssert (0 < MaxOffset);

        m_SizeOfTrBrOffset = SizeOfValue (MaxOffset);
    }
}


void FAPosNfaPack_triv::Prepare ()
{
    DebugLogAssert (m_pNfa);

    m_dump.Clear ();
    m_LastOffset = 0;

    const int MaxState = m_pNfa->GetMaxState ();
    m_state2offset.resize (MaxState + 1);

    CalcAlphabet ();
    CalcDestDump ();
    CalcTrBrDump ();
    CalcIwMap ();

    // calc automaton's size and allocate memory
    const int AutSize = GetSize ();
    m_dump.Create (AutSize);
    m_dump.resize (AutSize);

    m_pOutBuff = m_dump.begin ();

    // remove non-initialized data, due to alignment
    memset (m_pOutBuff, 0, AutSize);
}


void FAPosNfaPack_triv::StoreInitials ()
{
    DebugLogAssert (m_pNfa);

    const int * pInitials;
    const int InitialCount = m_pNfa->GetInitials (&pInitials);
    DebugLogAssert (0 < InitialCount && pInitials);

    *(int *)(m_pOutBuff + m_LastOffset) = InitialCount;
    m_LastOffset += sizeof (int);

    for (int i = 0; i < InitialCount; ++i) {

        const int InitialState = pInitials [i];

        *(int *)(m_pOutBuff + m_LastOffset) = InitialState;
        m_LastOffset += sizeof (int);
    }
}


void FAPosNfaPack_triv::StoreIwMap ()
{
    // get Iw2Iw map dump
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


void FAPosNfaPack_triv::StoreDestSets ()
{
    // get destination sets dump
    const unsigned char * pDestSetsDump;
    const int DumpSize = m_dsts2dump.GetDump (&pDestSetsDump);

    if (0 < DumpSize) {

        DebugLogAssert (pDestSetsDump);

        // store the offset to this table in the automaton's header
        DebugLogAssert (m_pOutBuff == m_dump.begin ());
        *(int *)(m_pOutBuff) = m_LastOffset;

        // copy destination sets dump into the output dump
        memcpy (m_pOutBuff + m_LastOffset, pDestSetsDump, DumpSize);
        m_LastOffset += DumpSize;
    }
}


void FAPosNfaPack_triv::StoreTrBrs ()
{
    // get trbr dump
    const unsigned char * pTrBrsDump;
    const int DumpSize = m_trbrs2dump.GetDump (&pTrBrsDump);

    if (0 < DumpSize) {

        DebugLogAssert (pTrBrsDump);

        // store the offset to this table in the automaton's header
        DebugLogAssert (m_pOutBuff == m_dump.begin ());
        *(int *)(m_pOutBuff + sizeof (int)) = m_LastOffset;
        // store size of trbr offset
        *(int *)(m_pOutBuff + (2 * sizeof (int))) = m_SizeOfTrBrOffset;

        // copy trbr dump into the output dump
        memcpy (m_pOutBuff + m_LastOffset, pTrBrsDump, DumpSize);
        m_LastOffset += DumpSize;
    }
}


void FAPosNfaPack_triv::StoreOffsetTable ()
{
    // build offset table dump
    const unsigned int * pOffsets = m_state2offset.begin ();
    const int OffsetsCount = m_state2offset.size ();

    m_offsets2dump.SetOffsets (pOffsets, OffsetsCount);
    m_offsets2dump.Process ();

    // get offset table dump
    const unsigned char * pOffsetsDump = NULL;
    const int OffsetsDumpSize = m_offsets2dump.GetDump (&pOffsetsDump);
    DebugLogAssert (0 < OffsetsDumpSize && pOffsetsDump);

    // store the offset to this table into automaton's header
    DebugLogAssert (m_pOutBuff == m_dump.begin ());
    DebugLogAssert (m_LastOffset == m_dump.size ());
    *(int *)(m_pOutBuff + (3 * sizeof (int))) = m_LastOffset;

    // make output dump bigger as offset table size was not known before
    const int NewDumpSize = OffsetsDumpSize + m_dump.size ();
    m_dump.resize (NewDumpSize);
    m_pOutBuff = m_dump.begin ();

    // copy offset table dump into the beginning
    memcpy (m_pOutBuff + m_LastOffset, pOffsetsDump, OffsetsDumpSize);

    m_LastOffset = NewDumpSize;
}


void FAPosNfaPack_triv::Process ()
{
    DebugLogAssert (m_pNfa);

    Prepare ();

    DebugLogAssert (m_pOutBuff && 0 == m_LastOffset);

    /// <Header>
    // store the offset to the encoded destination sets, 0 if empty
    *(int *)(m_pOutBuff + m_LastOffset) = 0;
    m_LastOffset += sizeof (int);
    // store the offset to the encoded trbr arrays, 0 if empty
    *(int *)(m_pOutBuff + m_LastOffset) = 0;
    m_LastOffset += sizeof (int);
    // store the global size of TrBr offset, 0 if empty
    *(int *)(m_pOutBuff + m_LastOffset) = 0;
    m_LastOffset += sizeof (int);
    // store the offset to the encoded offset table, 0 for a while
    *(int *)(m_pOutBuff + m_LastOffset) = 0;
    m_LastOffset += sizeof (int);

    /// <Body>
    // store initial states as array of ints
    StoreInitials ();
    // store Iw2Iw map
    StoreIwMap ();
    // store states
    const int MaxState = m_pNfa->GetMaxState ();
    for (int State = 0; State <= MaxState; ++State) {
        EncodeState (State);
    }
    // skip mis-aligned empty bytes
    const int MisAligned = m_LastOffset % sizeof (int);
    if (0 != MisAligned) {
        m_LastOffset -= MisAligned;
        m_LastOffset += sizeof (int);
    }
    // store destination sets
    StoreDestSets ();
    // store trbr dump
    StoreTrBrs ();
    // add State2Offset map
    StoreOffsetTable ();
}

}
