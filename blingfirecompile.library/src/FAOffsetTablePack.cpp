/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAOffsetTablePack.h"
#include "FAEncodeUtils.h"

namespace BlingFire
{


FAOffsetTablePack::FAOffsetTablePack (FAAllocatorA * pAlloc) :
    m_pOffsets (NULL),
    m_OffsetCount (0),
    m_SizeOfBase (0),
    m_SkipValue (0),
    m_pDump (NULL),
    m_Offset (0)
{
    m_base.SetAllocator (pAlloc);
    m_base.Create ();

    m_delta.SetAllocator (pAlloc);
    m_delta.Create ();

    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}


void FAOffsetTablePack::Clear ()
{
    m_SizeOfBase  = 0;
    m_SkipValue = 0;

    m_base.resize (0);
    m_delta.resize (0);
    m_dump.resize (0);
}


void FAOffsetTablePack::
    SetOffsets (const unsigned int * pOffsets, const int Count)
{
    m_pOffsets = pOffsets;
    m_OffsetCount = Count;
}


const int FAOffsetTablePack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


const int FAOffsetTablePack::CalcSizeOfBase () const
{
    DebugLogAssert (0 < m_OffsetCount && m_pOffsets);

    int curr_size;
    int max_size = 1;

    for (int i = 0; i < m_OffsetCount; ++i) {

        const unsigned int Offset = m_pOffsets [i];

        if (0xff000000 & Offset)
            curr_size = 4;
        else if (0x00ff0000 & Offset)
            curr_size = 3;
        else if (0x0000ff00 & Offset)
            curr_size = 2;
        else
            curr_size = 1;

        if (max_size < curr_size)
            max_size = curr_size;

        if (4 == max_size)
            break;
    }

    return max_size;
}


const int FAOffsetTablePack::CalcSkipValue () const
{
    DebugLogAssert (0 < m_OffsetCount && m_pOffsets);

    unsigned int ShiftValue;
    unsigned int MaxDelta;
    unsigned int Base = 0;

    // go from higher to lower
    for (ShiftValue = 7; ShiftValue >= 1; --ShiftValue) {

        const unsigned int Mask = (1 << ShiftValue) - 1;

        MaxDelta = 0;

        for (int i = 0; i < m_OffsetCount; ++i) {

            const int Offset = m_pOffsets [i];

            if (0 == (Mask & i)) {

                Base = Offset;

            } else {

                const unsigned int Delta = Offset - Base;
                DebugLogAssert (0 < Delta);

                if (MaxDelta < Delta)
                    MaxDelta = Delta;
            }
        } // of for (int i = 0; ...

        // see whether MaxDelta fits the byte
        DebugLogAssert (0 < MaxDelta);

        if (0xFFu >= MaxDelta) {
            return ShiftValue;
        }
    }

    return 0;
}


void FAOffsetTablePack::Process ()
{
    DebugLogAssert (0 < m_OffsetCount && m_pOffsets);

    Clear ();

    m_SizeOfBase = CalcSizeOfBase ();

    if (1 < m_SizeOfBase) {

        m_SkipValue = CalcSkipValue ();

        if (0 < m_SkipValue) {

            StoreCompressed ();

        } else {

            StoreUncompressed ();
        }

    } else {

        StoreUncompressed ();
    }
}


void FAOffsetTablePack::EncodeOffset (const int Offset)
{
    FAEncode_1_2_3_4 (m_pDump, m_Offset, Offset, m_SizeOfBase);
}


void FAOffsetTablePack::StoreUncompressed ()
{
    DebugLogAssert (0 < m_OffsetCount && m_pOffsets);
    DebugLogAssert (sizeof (char) <= (unsigned int) m_SizeOfBase && \
            sizeof (int) >=  (unsigned int) m_SizeOfBase);

    // adjust dump storage
    int DumpSize = \
        (sizeof (int) + sizeof (int) + sizeof (int)) + \
        (m_SizeOfBase * m_OffsetCount);

    // make it sizeof (int) aligned
    const int MisAligned = DumpSize % sizeof (int);
    if (0 != MisAligned) {
        DumpSize -= MisAligned;
        DumpSize += sizeof (int);
    }

    m_dump.resize (DumpSize);

    m_Offset = 0;
    m_pDump = m_dump.begin ();

    // remove non-initialized data, due to alignment
    memset (m_pDump, 0, DumpSize);

    // skip value 0 - stands for uncompressed
    *(unsigned int *)(m_pDump + m_Offset) = 0;
    m_Offset += sizeof (int);
    // size of offset
    *(unsigned int *)(m_pDump + m_Offset) = m_SizeOfBase;
    m_Offset += sizeof (int);
    // offset count
    *(unsigned int *)(m_pDump + m_Offset) = m_OffsetCount;
    m_Offset += sizeof (int);

    // encode offsets
    for (int i = 0; i < m_OffsetCount; ++i) {
        const int Offset = m_pOffsets [i];
        EncodeOffset (Offset);
    }
}


void FAOffsetTablePack::BuildArrays ()
{
    // the amount to skip
    DebugLogAssert (0 < m_SkipValue);
    const unsigned int Mask = (1 << m_SkipValue) - 1;

    m_base.resize (0);
    m_delta.resize (0);

    unsigned int Base = 0;
    int i;
    for (i = 0; i < m_OffsetCount; ++i) {

        const int Offset = m_pOffsets [i];

        if (0 == (Mask & i)) {

            DebugLogAssert (0 <= Offset);
            Base = Offset;

            m_base.push_back (Base);
            m_delta.push_back (0);

        } else {

            const unsigned int Delta = Offset - Base;
            DebugLogAssert (0 < Delta && 0xFFu >= Delta);

            m_delta.push_back ((unsigned char) Delta);
        }
    } // of for (int i = 0; ...
}


void FAOffsetTablePack::StoreCompressed ()
{
    DebugLogAssert (0 < m_OffsetCount && m_pOffsets);
    DebugLogAssert (sizeof (char) <= (unsigned int) m_SizeOfBase && \
            sizeof (int) >= (unsigned int) m_SizeOfBase);

    // build m_base and m_delta arrays
    BuildArrays ();

    const int BaseCount = m_base.size ();
    const int DeltaCount = m_OffsetCount;
    DebugLogAssert ((unsigned int) m_OffsetCount == m_delta.size ());

    // adjust dump storage
    int DumpSize = \
        (sizeof (int) + sizeof (int) + sizeof (int)) + \
        (m_SizeOfBase * BaseCount) + \
        (DeltaCount * sizeof (char));

    // make it sizeof (int) aligned
    const int MisAligned = DumpSize % sizeof (int);
    if (0 != MisAligned) {
        DumpSize -= MisAligned;
        DumpSize += sizeof (int);
    }

    m_dump.resize (DumpSize);
    m_Offset = 0;
    m_pDump = m_dump.begin ();

    // remove non-initialized data, due to alignment
    memset (m_pDump, 0, DumpSize);

    /// encode the header
    // skip value
    *(unsigned int *)(m_pDump + m_Offset) = m_SkipValue;
    m_Offset += sizeof (int);
    // size of offset
    *(unsigned int *)(m_pDump + m_Offset) = m_SizeOfBase;
    m_Offset += sizeof (int);
    // offset count
    *(unsigned int *)(m_pDump + m_Offset) = m_OffsetCount;
    m_Offset += sizeof (int);

    // store deltas
    DebugLogAssert (m_delta.size () == (unsigned int) m_OffsetCount);
    memcpy (m_pDump + m_Offset, m_delta.begin (), m_OffsetCount);
    m_Offset += m_OffsetCount;

    // encode bases
    for (int i = 0; i < BaseCount; ++i) {
        const unsigned int Base = m_base [i];
        EncodeOffset (Base);    
    }
}

}
