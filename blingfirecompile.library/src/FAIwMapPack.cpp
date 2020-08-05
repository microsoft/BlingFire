/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAIwMapPack.h"
#include "FAEncodeUtils.h"
#include "FAUtils.h"

namespace BlingFire
{


FAIwMapPack::FAIwMapPack (FAAllocatorA * pAlloc) :
    m_pOldIws (NULL),
    m_pNewIws (NULL),
    m_Count (0),
    m_NewIwSize (0),
    m_MaxIdx (0),
    m_pDump (NULL),
    m_Offset (0),
    m_MaxGap (DefaultMaxGap)
{
    m_ArrStart.SetAllocator (pAlloc);
    m_ArrStart.Create ();

    m_ArrEnd.SetAllocator (pAlloc);
    m_ArrEnd.Create ();

    m_ArrIdx.SetAllocator (pAlloc);
    m_ArrIdx.Create ();
    
    m_NewIws.SetAllocator (pAlloc);
    m_NewIws.Create ();

    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}


void FAIwMapPack::SetMaxGap (const int MaxGap)
{
    m_MaxGap = MaxGap;
}


void FAIwMapPack::
    SetIws (const int * pOldIws, const int * pNewIws, const int Count)
{
    DebugLogAssert (0 < Count && pOldIws && pNewIws);
    DebugLogAssert (FAIsSortUniqed (pOldIws, Count));

    m_pOldIws = pOldIws;
    m_pNewIws = pNewIws;
    m_Count = Count;
}


const int FAIwMapPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


void FAIwMapPack::Clear ()
{
    m_ArrStart.resize (0);
    m_ArrEnd.resize (0);
    m_ArrIdx.resize (0);
    m_NewIws.resize (0);
    m_dump.resize (0);

    m_MaxIdx = 0;
}


void FAIwMapPack::CalcNewIwSize ()
{
    int MaxNewIW = -1;

    for (int i = 0; i < m_Count; ++i) {

        const int NewIw = m_pNewIws [i];

        if (MaxNewIW < NewIw)
            MaxNewIW = NewIw;
    }

    DebugLogAssert (-1 != MaxNewIW);

    // as Iw + 1 will be encoded
    MaxNewIW++;

    if (0xff000000 & (unsigned int) MaxNewIW)
        m_NewIwSize = 4;
    else if (0x00ff0000 & (unsigned int) MaxNewIW)
        m_NewIwSize = 3;
    else if (0x0000ff00 & (unsigned int) MaxNewIW)
        m_NewIwSize = 2;
    else
        m_NewIwSize = 1;
}


void FAIwMapPack::AddInterval (const int IwFrom, const int IwTo)
{
    DebugLogAssert (IwTo >= IwFrom);

    m_ArrStart.push_back (IwFrom);
    m_ArrEnd.push_back (IwTo);
    m_ArrIdx.push_back (m_MaxIdx);

    // skip Count entries
    const int Count = IwTo - IwFrom + 1;
    m_MaxIdx += Count;
}


void FAIwMapPack::BuildIntervals ()
{
    DebugLogAssert (0 < m_Count && m_pOldIws);

    int PrevIw = m_pOldIws [0];
    DebugLogAssert (0 <= PrevIw);

    int Begin = PrevIw;
    int End = PrevIw;

    for (int i = 1; i < m_Count; ++i) {

        const int OldIw = m_pOldIws [i];
        DebugLogAssert (0 <= OldIw);

        if (PrevIw + m_MaxGap < OldIw) {

            AddInterval (Begin, End);

            Begin = OldIw;
            End = OldIw;

        } else {

            End += (OldIw - PrevIw);
        }

        PrevIw = OldIw;
    }

    AddInterval (Begin, End);
}


const int FAIwMapPack::GetNewIw (const int OldIw) const
{
    const int IwIdx = FAFind_log (m_pOldIws, m_Count, OldIw);

    if (-1 == IwIdx)
        return -1;
    else
        return m_pNewIws [IwIdx];
}


void FAIwMapPack::BuildNewIwsArray ()
{
    m_NewIws.resize (m_MaxIdx);

    const int IntervalCount = m_ArrStart.size ();
    DebugLogAssert (m_ArrEnd.size () == (unsigned int) IntervalCount);
    DebugLogAssert (m_ArrIdx.size () == (unsigned int) IntervalCount);

    for (int i = 0; i < IntervalCount; ++i) {

        const int FromIw = m_ArrStart [i];
        const int ToIw = m_ArrEnd [i];

        int Idx = m_ArrIdx [i];
        DebugLogAssert (0 <= Idx);

        for (int Iw = FromIw; Iw <= ToIw; ++Iw) {

            const int NewIw = GetNewIw (Iw);

            if (-1 != NewIw)
                m_NewIws [Idx] = NewIw + 1;
            else
                m_NewIws [Idx] = 0;

            Idx++;

        } // for (int Iw = FromIw; ...
    } // of for (int i = 0; ... 
}


void FAIwMapPack::EncodeIw (const unsigned int NewIw)
{
    FAEncode_1_2_3_4 (m_pDump, m_Offset, NewIw, m_NewIwSize);
}


void FAIwMapPack::BuildDump ()
{
    const int IntervalCount = m_ArrStart.size ();
    const int NewIwsCount = m_MaxIdx;
    DebugLogAssert (m_NewIws.size () == (unsigned int) NewIwsCount);
    DebugLogAssert (1 <= m_NewIwSize && 4 >= m_NewIwSize);

    unsigned int DumpSize = \
        sizeof (int) + sizeof (int) + \
        (IntervalCount * 3 * sizeof (int)) + \
        (NewIwsCount * m_NewIwSize);

    // make the entire dump to be int aligned
    const int MisAligned = DumpSize % sizeof (int);
    if (0 != MisAligned) {
        DumpSize -= MisAligned;
        DumpSize += sizeof (int);
    }

    m_dump.resize (DumpSize);
    m_pDump = m_dump.begin ();
    m_Offset = 0;

    // remove non-initialized data, due to alignment
    memset (m_pDump, 0, DumpSize);

    /// <Header>
    *(unsigned int *)(m_pDump + m_Offset) = m_NewIwSize;
    m_Offset += sizeof (int);
    *(unsigned int *)(m_pDump + m_Offset) = IntervalCount;
    m_Offset += sizeof (int);

    int i;

    /// <Body>
    // store the array of beginnings of the intervals
    for (i = 0; i < IntervalCount; ++i) {

        const int FromIw = m_ArrStart [i];
        DebugLogAssert (0 <= FromIw);

        *(unsigned int *)(m_pDump + m_Offset) = FromIw;
        m_Offset += sizeof (int);
    }
    // store the array of EndIw, Idx pairs
    for (i = 0; i < IntervalCount; ++i) {

        const int EndIw = m_ArrEnd [i];
        DebugLogAssert (0 <= EndIw);
        const int Idx = m_ArrIdx [i];
        DebugLogAssert (0 <= Idx);
        const int NewIwOffset = Idx * m_NewIwSize;

        *(unsigned int *)(m_pDump + m_Offset) = EndIw;
        m_Offset += sizeof (int);
        *(unsigned int *)(m_pDump + m_Offset) = NewIwOffset;
        m_Offset += sizeof (int);
    }
    // store array of internal Iws
    for (i = 0; i < NewIwsCount; ++i) {

        const int NewIw = m_NewIws [i];
        EncodeIw (NewIw);
    }
}


void FAIwMapPack::Process ()
{
    DebugLogAssert (0 < m_Count && m_pOldIws && m_pNewIws);

    Clear ();

    CalcNewIwSize ();
    BuildIntervals ();
    BuildNewIwsArray ();
    BuildDump ();
}

}
