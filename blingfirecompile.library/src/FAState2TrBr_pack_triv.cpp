/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAState2TrBr_pack_triv.h"
#include "FAEncodeUtils.h"
#include "FAException.h"

namespace BlingFire
{


FAState2TrBr_pack_triv::FAState2TrBr_pack_triv (const int MapType) :
    m_MapType (MapType),
    m_pPosNfaImage (NULL),
    m_SizeOfTrBrOffset (0)
{
    DebugLogAssert (MapTypeTrBrBegin == m_MapType || MapTypeTrBrEnd == m_MapType);
}


void FAState2TrBr_pack_triv::
    SetImage (const unsigned char * pPosNfaImage)
{
    m_pPosNfaImage = pPosNfaImage;

    if (NULL != m_pPosNfaImage) {

        unsigned int Offset;

        // get an offset to trbr values
        Offset = *(const unsigned int*)(m_pPosNfaImage + sizeof (int));
        FAAssert (0 != Offset, FAMsg::InitializationError);

        const unsigned char * pTrBrDump = m_pPosNfaImage + Offset;
        m_trbrs.SetImage (pTrBrDump);

        // get size of trbr offset in bytes
        m_SizeOfTrBrOffset = \
            *(const unsigned int*)(m_pPosNfaImage + (2 * sizeof (int)));

        FAAssert (sizeof (char) <= (unsigned int) m_SizeOfTrBrOffset && \
                  sizeof (int) >= (unsigned int) m_SizeOfTrBrOffset, \
                  FAMsg::InitializationError);

        // get an offset to the state2offset map dump
        Offset = *(const unsigned int*)(m_pPosNfaImage + (3 * sizeof (int)));
        FAAssert (0 != Offset, FAMsg::InitializationError);

        const unsigned char * pState2OffsetDump = m_pPosNfaImage + Offset;
        m_state2offset.SetImage (pState2OffsetDump);
    }
}


inline const int FAState2TrBr_pack_triv::GetTrBrOffset (const int State) const
{
    // get State's pointer
    const unsigned int Offset = m_state2offset.GetOffset (State);
    const unsigned char * pCurrPtr = m_pPosNfaImage + Offset;

    // get info
    const unsigned char info = *pCurrPtr;

    // see whether this state begins some triangular brackets
    if (0 != (m_MapType & info)) {

        // skip info
        pCurrPtr++;

        // get Iw size
        const int IwSize = ((info & 0xC) >> 2) + 1;
        DebugLogAssert (sizeof (char) <= (unsigned int) IwSize && \
                sizeof (int) >= (unsigned int) IwSize);

        // get DstSize
        const int DstSize = ((info & 0x30) >> 4) + 1;
        DebugLogAssert (sizeof (char) <= (unsigned int) DstSize && \
                sizeof (int) >= (unsigned int) DstSize);

        // get output transitions count
        unsigned int DstCount = info >> 6;

        // skip Iws array and IwsCount, if any
        if (sizeof (char) == IwSize) {

            if (0 == DstCount) {
                DstCount = *pCurrPtr;
                pCurrPtr += sizeof (char);
            } else {
                // as DstCount + 1 was actually encoded
                DstCount--;
            }
            // skip Iws array
            pCurrPtr += (DstCount * sizeof (char));

        } else if (sizeof (short) == IwSize) {

            if (0 == DstCount) {
                DstCount = *(const unsigned short *)pCurrPtr;
                pCurrPtr += sizeof (short);
            } else {
                // as DstCount + 1 was actually encoded
                DstCount--;
            }
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
            // skip Iws array
            pCurrPtr += (DstCount * sizeof (int));
        }

        // skip Dst array
        DebugLogAssert (1 <= DstSize && 4 >= DstSize);
        pCurrPtr += (DstSize * DstCount);

        // skip TrBrBegin offset, if needed
        if ((MapTypeTrBrEnd == m_MapType) && 0 != (MapTypeTrBrBegin & info)) {
            pCurrPtr += m_SizeOfTrBrOffset;
        }

        // get TrBrOffset
        unsigned int TrBrOffset;

        FADecode_1_2_3_4 (pCurrPtr, 0, TrBrOffset, m_SizeOfTrBrOffset);

        return TrBrOffset;

    } else {

        return -1;
    }
}


const int FAState2TrBr_pack_triv::
    Get (const int State, int * pTrBrs, const int MaxCount) const
{
    const int TrBrOffset = GetTrBrOffset (State);

    if (-1 != TrBrOffset) {

        const int TrBrCount = m_trbrs.UnPack (TrBrOffset, pTrBrs, MaxCount);
        return TrBrCount;

    } else {

        return -1;
    }
}


const int FAState2TrBr_pack_triv::
    Get (const int State, const int ** ppTrBrs) const
{
    const int TrBrOffset = GetTrBrOffset (State);

    if (-1 != TrBrOffset) {

        const int TrBrCount = m_trbrs.UnPack (TrBrOffset, ppTrBrs);
        return TrBrCount;

    } else {

        return -1;
    }
}


const int FAState2TrBr_pack_triv::GetMaxCount () const
{
    const int MaxCount = m_trbrs.GetMaxCount ();
    return MaxCount;
}

}

