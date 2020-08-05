/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FABrResult.h"

namespace BlingFire
{


FABrResult::FABrResult (FAAllocatorA * pAlloc) :
    m_Base (0)
{
    m_res.SetAllocator (pAlloc);
}


FABrResult::~FABrResult ()
{}


void FABrResult::SetBase (const int Base)
{
    m_Base = Base;
}


void FABrResult::AddRes (const int BrId, const int From, const int To)
{
    m_res.Add (BrId, From + m_Base);
    m_res.Add (BrId, To + m_Base);
}


const int FABrResult::GetRes (const int BrId, const int ** ppFromTo) const
{
    return m_res.Get (BrId, ppFromTo);
}


const int FABrResult::GetFrom (const int BrId) const
{
    const int * pFromTo;
    const int ResCount = m_res.Get (BrId, &pFromTo);
    DebugLogAssert (-1 == ResCount || 0 == ResCount % 2);

    if (0 < ResCount) {
        return *pFromTo;
    } else {
        return -1;
    }
}


const int FABrResult::GetTo (const int BrId) const
{
    const int * pFromTo;
    const int ResCount = m_res.Get (BrId, &pFromTo);
    DebugLogAssert (-1 == ResCount || 0 == ResCount % 2);

    if (0 < ResCount) {
        return *(pFromTo + 1);
    } else {
        return -1;
    }
}


const int FABrResult::GetNextRes (int * pBrId, const int ** ppFromTo) const
{
    return m_res.Prev (pBrId, ppFromTo);
}


void FABrResult::Clear ()
{
    m_res.Clear ();
}

}
