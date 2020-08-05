/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMapPack.h"
#include "FAEncodeUtils.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMultiMapPack::FAMultiMapPack (FAAllocatorA * pAlloc) :
    m_pMMap (NULL),
    m_vals2dump (pAlloc),
    m_pDump (NULL),
    m_Offset (0)
{
    m_keys.SetAllocator (pAlloc);
    m_keys.Create ();

    m_offsets.SetAllocator (pAlloc);
    m_offsets.Create ();

    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}


void FAMultiMapPack::Clear ()
{
    m_keys.resize (0);
    m_vals2dump.Clear ();
}


void FAMultiMapPack::SetMultiMap (const FAMultiMapA * pMMap)
{
    m_pMMap = pMMap;
}


void FAMultiMapPack::SetSizeOfValue (const int SizeOfValue)
{
    m_vals2dump.SetSizeOfValue (SizeOfValue);
}


const int FAMultiMapPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


void FAMultiMapPack::Prepare ()
{
    DebugLogAssert (m_pMMap);

    int Key = -1;
    const int * pValues;
    int Size = m_pMMap->Prev (&Key, &pValues);

    while (-1 != Size) {

        // store non-empty entries only
        if (0 < Size) {
            // add key
            m_keys.push_back (Key);
            // add values
            m_vals2dump.Add (pValues, Size);
        }

        // get next pair
        Size = m_pMMap->Prev (&Key, &pValues);
    }
    m_vals2dump.Process ();

    // sort the keys
#ifndef NDEBUG
    const int NewSize =
#endif
        FASortUniq (m_keys.begin (), m_keys.end ());
    DebugLogAssert ((unsigned int) NewSize == m_keys.size ());

    const int KeyCount = m_keys.size ();
    for (int i = 0; i < KeyCount; ++i) {

        // get the Key
        Key = m_keys [i];

        // get values
        const int Count = m_pMMap->Get (Key, &pValues);
        DebugLogAssert (0 < Count && pValues);
    
        // get offset
        const int Offset = m_vals2dump.GetOffset (pValues, Count);

        m_offsets.push_back (Offset);
    }
}


const unsigned int FAMultiMapPack::CalcOffsetSize () const
{
    int curr_size;
    int max_size = 1;

    const int OffsetCount = m_offsets.size ();

    for (int i = 0; i < OffsetCount; ++i) {

        const unsigned int Offset = 1 + m_offsets [i];

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


const unsigned int FAMultiMapPack::CalcSize () const
{
    // calc max Offset + 1 size
    const unsigned int SizeOfOffset = CalcOffsetSize ();
    DebugLogAssert (sizeof (char) <= SizeOfOffset && sizeof (int) >= SizeOfOffset);

    // get MaxKey
    const int MaxKey = m_keys [m_keys.size () - 1];

    unsigned int DumpSize = \
        sizeof (int) + sizeof (int) + \
        (SizeOfOffset * (MaxKey + 1));

    // make it sizeof (int) aligned
    const int MisAligned = DumpSize % sizeof (int);
    if (0 != MisAligned) {
        DumpSize -= MisAligned;
        DumpSize += sizeof (int);
    }

    // add values dump
    const unsigned char * pValsDump;
    const int ValsDumpSize = m_vals2dump.GetDump (&pValsDump);
    DebugLogAssert (0 < ValsDumpSize && pValsDump);

    DumpSize += ValsDumpSize;

    return DumpSize;
}


void FAMultiMapPack::
    EncodeOffset (const int Key, const int Offset, const int SizeOfOffset)
{
    DebugLogAssert (m_pDump);
    DebugLogAssert (0 <= Key && 0 <= Offset);

    // calc where to put encoded offset
    unsigned int DumpOffset = m_Offset + (Key * SizeOfOffset);

    FAEncode_1_2_3_4 (m_pDump, DumpOffset, Offset, SizeOfOffset);
}


void FAMultiMapPack::BuildDump ()
{
    // calc the resulting dump size
    const unsigned int DumpSize = CalcSize ();

    m_dump.resize (DumpSize);
    m_pDump = m_dump.begin ();
    m_Offset = 0;

    // remove non-initialized data, due to alignment
    memset (m_pDump, 0, DumpSize);

    // get MaxKey
    const int MaxKey = m_keys [m_keys.size () - 1];
    // size of max (Offset + 1)
    const unsigned int SizeOfOffset = CalcOffsetSize ();
    DebugLogAssert (sizeof (char) <= SizeOfOffset && sizeof (int) >= SizeOfOffset);

    /// <Header>
    *(unsigned int *)(m_pDump + m_Offset) = MaxKey;
    m_Offset += sizeof (int);
    *(unsigned int *)(m_pDump + m_Offset) = SizeOfOffset;
    m_Offset += sizeof (int);

    /// <Body>
    // Store key -> offset mapping
    // 0 means: "no offset associated"
    memset (m_pDump + m_Offset, 0, SizeOfOffset * (MaxKey + 1));

    const int KeyCount = m_keys.size ();
    DebugLogAssert ((unsigned int) KeyCount == m_offsets.size ());

    for (int i = 0; i < KeyCount; ++i) {

        const int Key = m_keys [i];
        const int Offset = m_offsets [i];

        EncodeOffset (Key, 1 + Offset, SizeOfOffset);
    }

    m_Offset += (SizeOfOffset * (MaxKey + 1));

    // skip mis-aligned bytes
    const int MisAligned = m_Offset % sizeof (int);
    if (0 != MisAligned) {
        m_Offset -= MisAligned;
        m_Offset += sizeof (int);
    }

    // copy dump of values
    const unsigned char * pValsDump;
    const int ValsDumpSize = m_vals2dump.GetDump (&pValsDump);
    DebugLogAssert (0 < ValsDumpSize && pValsDump);

    memcpy (m_pDump + m_Offset, pValsDump, ValsDumpSize);
}


void FAMultiMapPack::Process ()
{
    DebugLogAssert (m_pMMap);

    Prepare ();
    BuildDump ();
    Clear ();
}

}

