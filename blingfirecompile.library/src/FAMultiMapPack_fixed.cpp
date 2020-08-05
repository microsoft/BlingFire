/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */



#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMapPack_fixed.h"
#include "FAMultiMapA.h"
#include "FAEncodeUtils.h"
#include "FAException.h"

namespace BlingFire
{


FAMultiMapPack_fixed::
    FAMultiMapPack_fixed (FAAllocatorA * pAlloc) :
        m_pMMap (NULL),
        m_ValueSize (0),
        m_MinKey (0),
        m_MaxKey (0),
        m_MaxCount (0),
        m_pDump (NULL)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}


void FAMultiMapPack_fixed::SetMultiMap (const FAMultiMapA * pMMap)
{
    m_pMMap = pMMap;
}


void FAMultiMapPack_fixed::SetSizeOfValue (const int SizeOfValue)
{
    FAAssert (0 == SizeOfValue || sizeof (char) == SizeOfValue || \
        sizeof (short) == SizeOfValue || sizeof (int) == SizeOfValue, \
        FAMsg::InvalidParameters);

    m_ValueSize = SizeOfValue;
}


const int FAMultiMapPack_fixed::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_pDump;
    return m_dump.size ();
}


const int FAMultiMapPack_fixed::Prepare ()
{
    DebugLogAssert (m_pMMap);

    int curr_size;

    int SizeOfValue = 0;

    int Key = -1;
    const int * pValues;
    int Size = m_pMMap->Prev (&Key, &pValues);

    m_MinKey = Key;
    m_MaxKey = Key;
    m_MaxCount = Size;

    while (-1 != Size) {

        for (int i = 0; i < Size; ++i) {

            DebugLogAssert (pValues);
            const int Value = pValues [i];

            if (*(const char *)&(pValues [i]) == Value) {
                curr_size = sizeof (char);
            } else if (*(const short *)&(pValues [i]) == Value) {
                curr_size = sizeof (short);
            } else {
                DebugLogAssert (*(const int *)&(pValues [i]) == Value);
                curr_size = sizeof (int);
            }
            if (SizeOfValue < curr_size)
                SizeOfValue = curr_size;
        }
        if (m_MaxCount < Size) {
            m_MaxCount = Size;
        }
        if (m_MinKey > Key) {
            m_MinKey = Key;
        }
        if (m_MaxKey < Key) {
            m_MaxKey = Key;
        }

        // get next pair
        Size = m_pMMap->Prev (&Key, &pValues);
    }
    if (0 < m_MaxCount) {
        int None = m_MaxCount + 1;
        if (*(const char *)&(None) == None) {
            curr_size = sizeof (char);
        } else if (*(const short *)&(None) == None) {
            curr_size = sizeof (short);
        } else {
            DebugLogAssert (*(const int *)&(None) == None);
            curr_size = sizeof (int);
        }
        if (SizeOfValue < curr_size)
            SizeOfValue = curr_size;
    }

    return SizeOfValue;
}


void FAMultiMapPack_fixed::Process ()
{
    FAAssert (m_pMMap, FAMsg::InvalidParameters);

    m_pDump = NULL;
    m_dump.resize (0);

    const int ValSize = Prepare ();

    FAAssert (0 <= m_MinKey && m_MinKey <= m_MaxKey, FAMsg::InvalidParameters);
    FAAssert (0 < m_MaxCount, FAMsg::InvalidParameters);

    if (0 == m_ValueSize) {
        m_ValueSize = ValSize;
    } else {
        FAAssert (m_ValueSize >= ValSize, FAMsg::InvalidParameters);
    }
    DebugLogAssert (sizeof (char) == m_ValueSize || sizeof (short) == m_ValueSize || \
        sizeof (int) == m_ValueSize);

    const int KeyCount = m_MaxKey - m_MinKey + 1;
    DebugLogAssert (0 < KeyCount);

    unsigned int DumpSize = (4 * sizeof (int)) + \
        (KeyCount * m_ValueSize * (m_MaxCount + 1));

    // make it sizeof (int) aligned
    const int MisAligned = DumpSize % sizeof (int);
    if (0 != MisAligned) {
        DumpSize -= MisAligned;
        DumpSize += sizeof (int);
    }

    m_dump.resize (DumpSize);

    unsigned char * pOut = m_dump.begin ();
    DebugLogAssert (pOut);
    memset (pOut, 0, DumpSize);

    // store the header
    *((int*)pOut) = m_ValueSize;
    pOut += sizeof (int);
    *((int*)pOut) = m_MaxCount;
    pOut += sizeof (int);
    *((int*)pOut) = m_MinKey;
    pOut += sizeof (int);
    *((int*)pOut) = m_MaxKey;
    pOut += sizeof (int);

    // store all arrays
    for (int Key = m_MinKey; Key <= m_MaxKey; ++Key) {

        const int * pValues = NULL;
        int Size = m_pMMap->Get (Key, &pValues);

        int Offset = 0;

        // store the size or m_MaxCount + 1 to indicate no mapping
        if (-1 == Size) {
            int None = m_MaxCount + 1;
            FAEncode_C_S_I (pOut, Offset, None, m_ValueSize);
        } else {
            FAEncode_C_S_I (pOut, Offset, Size, m_ValueSize);
        }

        // store all elements, use 0 if not defined
        for (int i = 0; i < m_MaxCount; ++i) {
            const int Value = i < Size ? pValues [i] : 0 ;
            FAEncode_C_S_I (pOut, Offset, Value, m_ValueSize);
        }

        pOut += Offset;
    }

    m_pDump = m_dump.begin ();

    FAAssert (int (DumpSize) >= int (pOut - m_pDump), FAMsg::InternalError);
}

}
