/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAChainsPack_triv.h"
#include "FAAllocatorA.h"
#include "FAEncodeUtils.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FAChainsPack_triv::FAChainsPack_triv (FAAllocatorA * pAlloc) :
    m_MaxIdx (-1),
    m_pDump (NULL),
    m_LastOffset (0),
    m_SizeOfValue (sizeof (char)),
    m_MaxCount (0),
    m_ValuesCount (0)
{
    m_set2freq.SetAllocator (pAlloc);

    m_idx_by_freq.SetAllocator (pAlloc);
    m_idx_by_freq.Create ();

    m_tmp_arr.SetAllocator (pAlloc);
    m_tmp_arr.Create ();

    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}


void FAChainsPack_triv::Clear ()
{
    m_set2freq.Clear ();
    m_idx2offset.Clear ();

    m_dump.resize (0);

    m_MaxIdx = -1;
    m_LastOffset = 0;
    m_SizeOfValue = sizeof (char);
    m_MaxCount = 0;
    m_ValuesCount = 0;
}


void FAChainsPack_triv::SetSizeOfValue (const int SizeOfValue)
{
    DebugLogAssert (sizeof (char) == SizeOfValue || \
            sizeof (short) == SizeOfValue || \
            sizeof (int) == SizeOfValue);

    m_SizeOfValue = SizeOfValue;
}


void FAChainsPack_triv::Add (const int * pValues, const int Count)
{
    DebugLogAssert (0 < Count && pValues);

    // update Values size information if it is not already maximum possible
    if (sizeof (int) != m_SizeOfValue) {

        int curr_size;

        for (int i = 0; i < Count; ++i) {

            const int Value = pValues [i];

            if (*(const char *)&(pValues [i]) == Value) {
                curr_size = sizeof (char);
            } else if (*(const short *)&(pValues [i]) == Value) {
                curr_size = sizeof (short);
            } else {
                DebugLogAssert (*(const int *)&(pValues [i]) == Value);
                curr_size = sizeof (int);
            }

            if (m_SizeOfValue < curr_size)
                m_SizeOfValue = curr_size;
        }

        // take Count size into account
        if (*(const char *)&(Count) == Count) {
            curr_size = sizeof (char);
        } else if (*(const short *)&(Count) == Count) {
            curr_size = sizeof (short);
        } else {
            DebugLogAssert (*(const int *)&(Count) == Count);
            curr_size = sizeof (int);
        }

        if (m_SizeOfValue < curr_size)
            m_SizeOfValue = curr_size;
    }

    int Idx;

    // see whether such a set already exists
    const int * pFreq = m_set2freq.Get (pValues, Count);

    if (NULL != pFreq) {

        // update a frequence information for an existing set
        Idx = m_set2freq.Add (pValues, Count, 1 + *pFreq);
        DebugLogAssert (Idx <= m_MaxIdx);

    } else {

        // update MaxCount information
        if (m_MaxCount < Count)
            m_MaxCount = Count;
        // update values count information
        m_ValuesCount += (1 + Count);
        // add a new set, setup frequency to be 1
        Idx = m_set2freq.Add (pValues, Count, 1);
        DebugLogAssert (m_MaxIdx + 1 == Idx);
        // update max idx
        m_MaxIdx = Idx;
    }
}


const int FAChainsPack_triv::
    GetOffset (const int * pValues, const int Count) const
{
    const int Idx = m_set2freq.GetIdx (pValues, Count);

    if (-1 == Idx)
        return -1;

    const int * pOffset = m_idx2offset.Get (Idx);

    if (NULL != pOffset)
        return *pOffset;
    else
        return -1;
}


const int FAChainsPack_triv::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


void FAChainsPack_triv::Prepare ()
{
    DebugLogAssert (0 < m_ValuesCount);
    DebugLogAssert (sizeof (char) == m_SizeOfValue ||  \
            sizeof (short) == m_SizeOfValue || \
            sizeof (int) == m_SizeOfValue);

    // allocate enough space to fit header and all sets into the m_dump
    unsigned int SizeOfDump = \
        (2 * sizeof (int)) + (m_SizeOfValue * m_ValuesCount);

    // make it sizeof (int) aligned
    const int MisAligned = SizeOfDump % sizeof (int);
    if (0 != MisAligned) {
        SizeOfDump -= MisAligned;
        SizeOfDump += sizeof (int);
    }

    m_dump.resize (SizeOfDump);
    m_pDump = m_dump.begin ();
    DebugLogAssert (m_pDump);

    // remove non-initialized data, due to alignment
    memset (m_pDump, 0, SizeOfDump);

    // calculate transposition for indices to have more commonly used
    // sets earlier in the dump, e.g. having smaller offsets
    m_idx_by_freq.resize (m_MaxIdx + 1);
    m_tmp_arr.resize (m_MaxIdx + 1);

    for (int i = 0; i <= m_MaxIdx; ++i) {

        const int Freq = m_set2freq.GetValue (i);

        m_idx_by_freq [i] = i;
        m_tmp_arr [i] = Freq;
    }

    int * pBegin = m_idx_by_freq.begin ();
    DebugLogAssert (pBegin);
    int * pEnd = m_idx_by_freq.end ();
    DebugLogAssert (pEnd);
    const int * pFreqs = m_tmp_arr.begin ();
    DebugLogAssert (pFreqs);

    std::sort (pBegin, pEnd, FAIdxCmp_b2s (pFreqs));
}


void FAChainsPack_triv::Process ()
{
    int i;

    Prepare ();

    // encode m_SizeOfValue
    *(int *)(m_pDump + m_LastOffset) = m_SizeOfValue;
    m_LastOffset += sizeof (int);

    // encode m_MaxCount
    *(int *)(m_pDump + m_LastOffset) = m_MaxCount;
    m_LastOffset += sizeof (int);

    // traverse sets from more to less frequent
    const int SetCount = m_MaxIdx + 1;

    for (i = 0; i < SetCount; ++i) {

        // get next index
        const int Idx = m_idx_by_freq [i];

        // add m_idx2offset entries before m_LastOffset has been changed
        m_idx2offset.Set (Idx, m_LastOffset);

        // get array of values
        const int * pValues;
        const int Count = m_set2freq.GetChain (Idx, &pValues);
        DebugLogAssert (0 < Count && pValues);

        // do actual encoding
        EncodeValues (pValues, Count);
    }
}


void FAChainsPack_triv::EncodeValue (const int Value)
{
    FAEncode_C_S_I (m_pDump, m_LastOffset, Value, m_SizeOfValue);
}


void FAChainsPack_triv::EncodeValues (const int * pValues, const int Count)
{
    DebugLogAssert (0 < Count && pValues);

    EncodeValue (Count);

    for (int i = 0; i < Count; ++i) {

        const int Value = pValues [i];
        EncodeValue (Value);
    }
}

}
