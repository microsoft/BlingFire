/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAArray_p2ca.h"

namespace BlingFire
{


FAArray_p2ca::FAArray_p2ca () :
    m_pA (NULL),
    m_Count (0)
{}

const int FAArray_p2ca::GetAt (const int Idx) const
{
    DebugLogAssert (0 <= Idx && Idx < m_Count);
    return m_pA [Idx];
}

const int FAArray_p2ca::GetCount () const
{
    return m_Count;
}

void FAArray_p2ca::SetArray (const int * pA, const int Count)
{
    m_pA = pA;
    m_Count = Count;
}

}

