/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASplitSets.h"

namespace BlingFire
{


FASplitSets::FASplitSets (FAAllocatorA * pAlloc) :
    m_size (0)
{
    m_info2c.SetAllocator (pAlloc);
    m_info2c.SetEncoder (&m_enc);

    m_e2v.SetAllocator (pAlloc);

    m_e2c.SetAllocator (pAlloc);
    m_e2c.Create ();
}


void FASplitSets::Prepare (const int Count)
{
    FASplitSets::Clear ();

    DebugLogAssert (0 < Count);
    m_e2c.resize (Count);
    memset (m_e2c.begin (), 0, sizeof (int) * Count);
}


inline const int FASplitSets::
    Info2Class (FAChain2Num_hash * pM, const int * pInfo, const int InfoSize)
{
    DebugLogAssert (pM);
    DebugLogAssert (pInfo && 0 < InfoSize);

    int Idx = pM->GetIdx (pInfo, InfoSize);

    if (-1 == Idx) {
        Idx = pM->Add (pInfo, InfoSize, 0);
        DebugLogAssert (-1 != Idx);
    }

    return Idx;
}


inline void FASplitSets::Classify ()
{
    const int Count = m_e2c.size ();

    for (int i = 0; i < Count; ++i) {

        // get old class
        const int C1 = m_e2c [i];

        // add it to the information vector
        m_e2v.Add (i, C1);

        // get info vector
        const int * pInfs;
        const int Infs = m_e2v.Get (i, &pInfs);

        // get new class
        const int C2 = Info2Class (&m_info2c, pInfs, Infs);

        // store new class
        m_e2c [i] = C2;
    }

    m_e2v.Clear ();
    m_info2c.Clear ();
}


void FASplitSets::AddInfo (const int * pI, const int Count)
{
    DebugLogAssert (0 < Count && pI);
    DebugLogAssert ((unsigned int) Count == m_e2c.size ());

    for (int i = 0; i < Count; ++i) {

        const int I = pI [i];
        m_e2v.Add (i, I);
    }

    m_size++;

    if (MaxAtOnce == m_size) {

        Classify ();
        m_size = 0;
    }
}


void FASplitSets::Clear ()
{
    m_size = 0;
    m_e2v.Clear ();
    m_info2c.Clear ();
    m_e2c.Clear ();
    m_e2c.Create ();
}


void FASplitSets::Process ()
{
    Classify ();
    m_size = 0;
}


const int FASplitSets::GetClasses (const int ** ppC)
{
    DebugLogAssert (ppC);

    *ppC = m_e2c.begin ();
    const int Size = m_e2c.size ();
    return Size;
}

}
