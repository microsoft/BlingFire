/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAState2Ows_ar_uniq.h"

namespace BlingFire
{


FAState2Ows_ar_uniq::FAState2Ows_ar_uniq (FAAllocatorA * pAlloc) :
    m_MaxOwsCount (0)
{
    m_state2ows.SetAllocator (pAlloc);
}


FAState2Ows_ar_uniq::~FAState2Ows_ar_uniq ()
{}


const int FAState2Ows_ar_uniq::GetOws (const int State, const int ** ppOws) const
{
    const int Count =  m_state2ows.Get (State, ppOws);
    return Count;
}


const int FAState2Ows_ar_uniq::
    GetOws (const int State, int * pOws, const int MaxCount) const
{
    const int * pOwsArray;
    const int Size = m_state2ows.Get (State, &pOwsArray);

    if (0 < Size && NULL != pOws && MaxCount >= Size) {
        memcpy (pOws, pOwsArray, Size * sizeof (int));
    }

    return Size;
}


const int FAState2Ows_ar_uniq::GetMaxOwsCount () const
{
    return m_MaxOwsCount;
}


void FAState2Ows_ar_uniq::SetOws (const int State, const int * pOws, const int Size)
{
    m_state2ows.Set (State, pOws, Size);

    if (Size > m_MaxOwsCount)
        m_MaxOwsCount = Size;
}


void FAState2Ows_ar_uniq::Clear ()
{
    m_state2ows.Clear ();
    m_MaxOwsCount = 0;
}

}
