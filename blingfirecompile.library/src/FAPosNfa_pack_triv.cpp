/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAPosNfa_pack_triv.h"
#include "FAEncodeUtils.h"
#include "FAUtils_cl.h"
#include "FAException.h"

namespace BlingFire
{


FAPosNfa_pack_triv::FAPosNfa_pack_triv () :
    m_pAutImage (NULL),
    m_InitialCount (0),
    m_pInitials (NULL)
{}


void FAPosNfa_pack_triv::SetImage (const unsigned char * pAutImage)
{
    m_pAutImage = pAutImage;

    if (NULL != m_pAutImage) {

        unsigned int Offset = 0;

        // get an offset to dest sets packed
        const unsigned int DestOffset = \
            *(const unsigned int*)(m_pAutImage + Offset);
        FAAssert (0 != DestOffset, FAMsg::InitializationError);

        // skip it and TrBr related information
        Offset += (3 * sizeof (int));

        // initialize dest sets dump interpreter
        m_dest_sets.SetImage (m_pAutImage + DestOffset);

        // get an offset to the state2offset map dump
        const unsigned int State2OffsetOffset = \
            *(const unsigned int*)(m_pAutImage + Offset);
        Offset += sizeof (int);
        FAAssert (0 != State2OffsetOffset, FAMsg::InitializationError);

        // initialize state2offset map
        m_state2offset.SetImage (m_pAutImage + State2OffsetOffset);

        // get initials count
        m_InitialCount = \
            *(const unsigned int *)(m_pAutImage + Offset);
        Offset += sizeof (int);
        FAAssert (0 < m_InitialCount, FAMsg::InitializationError);

        // get pointer to the initial states
        m_pInitials = (const int *) (m_pAutImage + Offset);
        Offset += (sizeof (int) * m_InitialCount);

        // check Iw2Iw map size
        const unsigned int Iw2IwMapSize = \
            *(const unsigned int *)(m_pAutImage + Offset);
        Offset += sizeof (int);
        FAAssert (0 != Iw2IwMapSize, FAMsg::InitializationError);

        // initialize Iw2Iw map
        m_iw2iw.SetImage (m_pAutImage + Offset);
    }
}


const int FAPosNfa_pack_triv::GetInitials (const int ** ppStates) const
{
    DebugLogAssert (ppStates);

    *ppStates = m_pInitials;
    return m_InitialCount;
}


const bool FAPosNfa_pack_triv::IsFinal (const int State) const
{
    DebugLogAssert (m_pAutImage);

    // get State's pointer
    const unsigned int Offset = m_state2offset.GetOffset (State);
    // get Info
    const unsigned char Info = m_pAutImage [Offset];
    // see whether the final or not
    return 0 != (3 & Info);
}


const int FAPosNfa_pack_triv::GetDest (
            const int State,
            const int Iw,
            int * pDstStates,
            const int MaxCount
        ) const
{
    DebugLogAssert (m_pAutImage);

    const int NewIw = m_iw2iw.GetNewIw (Iw);

    if (-1 == NewIw)
        return -1;

    // get State's pointer
    const unsigned int StateOffset = m_state2offset.GetOffset (State);
    const unsigned char * pCurrPtr = m_pAutImage + StateOffset;

    // get info
    const unsigned char info = *pCurrPtr;
    pCurrPtr++;

    // get Iw size
    const int IwSize = ((info & 0xC) >> 2) + 1;
    DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
            sizeof (int) >= (unsigned int) IwSize);

    // get output transitions count
    unsigned int DstCount = info >> 6;
    int Idx;

    if (sizeof (char) == IwSize) {

        // check whether NewIw is out of bounds for this state
        if (0xFFFFFF00 & NewIw) {
            return -1;
        }
        if (0 == DstCount) {
            DstCount = *pCurrPtr;
            pCurrPtr += sizeof (char);
        } else {
            // as DstCount + 1 was actually encoded
            DstCount--;
        }
        // find outgoing transition index, if any
        const unsigned char * pIws = (const unsigned char *) pCurrPtr;
        Idx = FAFind_log (pIws, DstCount, (unsigned char) NewIw);
        // skip Iws array
        pCurrPtr += (DstCount * sizeof (char));

    } else if (sizeof (short) == IwSize) {

        // check whether NewIw is out of bounds for this state
        if (0xFFFF0000 & NewIw) {
            return -1;
        }
        if (0 == DstCount) {
            DstCount = *(const unsigned short *)pCurrPtr;
            pCurrPtr += sizeof (short);
        } else {
            // as DstCount + 1 was actually encoded
            DstCount--;
        }
        // find outgoing transition index, if any
        const unsigned short * pIws = (const unsigned short *) pCurrPtr;
        Idx = FAFind_log (pIws, DstCount, (unsigned short) NewIw);
        // skip Iws array
        pCurrPtr += (DstCount * sizeof (short));

    } else {
        DebugLogAssert (sizeof (int) == IwSize);

        if (0 == DstCount) {
            DstCount = *(const unsigned int *)pCurrPtr;
            pCurrPtr += sizeof (int);
        } else {
            // as DstCount + 1 was actually encoded
            DstCount--;
        }
        // find outgoing transition index, if any
        const unsigned int * pIws = (const unsigned int *) pCurrPtr;
        Idx = FAFind_log (pIws, DstCount, (unsigned int) NewIw);
        // skip Iws array
        pCurrPtr += (DstCount * sizeof (int));
    }

    // transition with NewIw does not exist
    if (-1 == Idx) {
        return -1;
    }

    // get DstSize
    const int DstSize = ((info & 0x30) >> 4) + 1;
    DebugLogAssert (sizeof (char) <= (unsigned int) DstSize && \
            sizeof (int) >= (unsigned int) DstSize);

    // get destination
    unsigned int Dst;

    FADecode_1_2_3_4_idx (pCurrPtr, Idx, Dst, DstSize);

    // see whether Dst is a single destination state or an offset to a set
    if (0 == (Dst & 1)) {

        Dst = Dst >> 1;
        Dst = Dst ^ State;

        if (NULL != pDstStates) {
            DebugLogAssert (0 < MaxCount);
            *pDstStates = Dst;
        }

        return 1;

    } else {
        // get dest set offset
        const unsigned int DestOffset = Dst >> 1;
        // decode dest set
        const int DestSetSize = 
            m_dest_sets.UnPack (DestOffset, pDstStates, MaxCount);

        return DestSetSize;
    }
}

}
