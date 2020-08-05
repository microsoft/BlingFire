/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAArrayPack.h"
#include "FAEncodeUtils.h"
#include "FAException.h"

namespace BlingFire
{


FAArrayPack::FAArrayPack (FAAllocatorA * pAlloc) :
    m_pArr (NULL),
    m_Size (0),
    m_M (1),
    m_SizeOfValue (0),
    m_ForceFlat (false)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();

    m_Data.SetAllocator (pAlloc);
    m_Data.SetCopyChains (true);

    m_tmp_chain.SetAllocator (pAlloc);
    m_tmp_chain.Create ();

    m_Index.SetAllocator (pAlloc);
    m_Index.Create ();
}


void FAArrayPack::Clear ()
{
    m_M = 1;
    m_SizeOfValue = 0;

    m_dump.resize (0);
    m_Data.Clear ();
    m_Index.resize (0);
}


void FAArrayPack::SetArray (const int * pArr, const int Size)
{
    m_pArr = pArr;
    m_Size = Size;
}


void FAArrayPack::SetForceFlat (const bool ForceFlat)
{
    m_ForceFlat = ForceFlat;
}


const int FAArrayPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


inline const int FAArrayPack::GetSizeOf (const int Val)
{
    int curr_size;

    if (0xff000000 & Val)
        curr_size = 4;
    else if (0x00ff0000 & Val)
        curr_size = 3;
    else if (0x0000ff00 & Val)
        curr_size = 2;
    else
        curr_size = 1;

    return curr_size;
}


inline const int FAArrayPack::CalcSizeOfVal () const
{
    int max_size = 1;

    for (int i = 0; i < m_Size; ++i) {

        const int Val = m_pArr [i];

        const int curr_size = GetSizeOf (Val);

        if (max_size < curr_size)
            max_size = curr_size;

        if (4 == max_size)
            break;
    }

    return max_size; 
}


inline const int FAArrayPack::AddChain (const int FromIdx, const int Count)
{
    FAAssert (0 < Count, FAMsg::InvalidParameters);
    FAAssert (0 <= FromIdx && m_Size > FromIdx, FAMsg::InvalidParameters);

    m_tmp_chain.resize (0);

    for (int i = 0; i < Count; ++i) {

        const int Idx = FromIdx + i;

        if (Idx < m_Size) {

            const int Val = m_pArr [Idx];
            m_tmp_chain.push_back (Val);

        } else {

            m_tmp_chain.push_back (0);
        }
    }

    const int ChainIdx = \
        m_Data.Add (m_tmp_chain.begin (), m_tmp_chain.size (), 0);
    DebugLogAssert ((unsigned int) Count == m_tmp_chain.size ());

    return ChainIdx;
}


inline void FAArrayPack::CalcBestM ()
{
    const int FlatSize = m_SizeOfValue * m_Size;

    m_M = -1;
    int BestSize = -1;

    for (int M = 2; M <= 8; ++M) {

        // add all chains of length M
        for (int Idx = 0; Idx < m_Size; Idx += M) {
            AddChain (Idx, M);
        }

        // get the unique chain count
        const int ChainCount = m_Data.GetChainCount ();

        // free the container
        m_Data.Clear ();

        // calculate data-dependent size
        const int CurrSize = \
            // Index
            (int (m_Size / M) * GetSizeOf (ChainCount)) + \
            // Data
            (ChainCount * M * m_SizeOfValue);

        // update m_M/BestSize, if needed
        if (BestSize > CurrSize || -1 == m_M) {
            m_M = M;
            BestSize = CurrSize;
        }
    }

    DebugLogAssert (-1 != BestSize && -1 != m_M);

    if (MinDiffSize > FlatSize - BestSize) {
        m_M = 1;
    }
}


void FAArrayPack::BuildFlat ()
{
    DebugLogAssert (1 == m_M);

    const int DumpSize = (sizeof (int) * 4) + (m_SizeOfValue * m_Size);
    m_dump.resize (DumpSize);

    unsigned char * pDump = m_dump.begin ();
    DebugLogAssert (pDump);

    *((unsigned int*)pDump) = m_M;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = 0;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = m_SizeOfValue;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = m_Size;
    pDump += sizeof (int);

    int Offset = 0;

    // copy Data
    for (int i = 0; i < m_Size; ++i) {

        const int Val = m_pArr [i];
        FAEncode_1_2_3_4 (pDump, Offset, Val, m_SizeOfValue);
        DebugLogAssert (0 < Offset && (unsigned int) Offset < m_dump.size ());
    }
}


void FAArrayPack::BuildPacked ()
{
    DebugLogAssert (1 < m_M && m_M <= 8);

    // add all chains
    for (int Idx = 0; Idx < m_Size; Idx += m_M) {

        const int ChainIdx = AddChain (Idx, m_M);
        DebugLogAssert (0 <= ChainIdx);
        m_Index.push_back (ChainIdx);
    }

    // get the unique chains count
    const int ChainCount = m_Data.GetChainCount ();
    const int SizeOfIdx = GetSizeOf (ChainCount);
    DebugLogAssert (0 < SizeOfIdx && 4 >= SizeOfIdx);

    // calc the dump size, Head + Index + Data
    const int DumpSize = \
        (sizeof (int) * 4) + \
        (m_Index.size () * SizeOfIdx) + \
        (ChainCount * m_M * m_SizeOfValue);

    m_dump.resize (DumpSize);
    unsigned char * pDump = m_dump.begin ();
    DebugLogAssert (pDump);

    // copy header
    *((unsigned int*)pDump) = m_M;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = SizeOfIdx;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = m_SizeOfValue;
    pDump += sizeof (int);
    *((unsigned int*)pDump) = m_Size;
    pDump += sizeof (int);

    int i;
    int Offset = 0;

    // copy Index entries
    const int iMax = m_Index.size ();
    DebugLogAssert (iMax == int ((m_Size + m_M - 1) / m_M));

    for (i = 0; i < iMax; ++i) {

        const int Idx = m_Index [i];
        FAEncode_1_2_3_4 (pDump, Offset, Idx, SizeOfIdx);
        DebugLogAssert (0 < Offset && (unsigned int) Offset < m_dump.size ());
    }

    // copy Data
    for (i = 0; i < ChainCount; ++i) {

        const int * pChain;
#ifndef NDEBUG
        const int Size = 
#endif
            m_Data.GetChain (i, &pChain);
        DebugLogAssert (m_M == Size && pChain);

        for (int j = 0; j < m_M; ++j) {

            const int Val = pChain [j];
            FAEncode_1_2_3_4 (pDump, Offset, Val, m_SizeOfValue);
            DebugLogAssert (0 < Offset && (unsigned int) Offset < m_dump.size ());
        }
    }
}


void FAArrayPack::Process ()
{
    DebugLogAssert (m_pArr && m_Size);

    Clear ();

    m_SizeOfValue = CalcSizeOfVal ();

    if (!m_ForceFlat) {
        CalcBestM ();
    } else {
        m_M = 1;
    }

    if (1 == m_M) {
        BuildFlat ();
    } else {
        DebugLogAssert (1 < m_M && 8 >= m_M);
        BuildPacked ();
    }
}

}
