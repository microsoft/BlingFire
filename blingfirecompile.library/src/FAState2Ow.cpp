/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAState2Ow.h"

namespace BlingFire
{


FAState2Ow::FAState2Ow (FAAllocatorA * /*pAlloc*/)
{}

FAState2Ow::~FAState2Ow ()
{}

const int FAState2Ow::GetOw (const int State) const
{
    const int * pOw = m_state2ow.Get (State);

    if (NULL == pOw)
        return -1;

    return *pOw;
}

void FAState2Ow::SetOw (const int State, const int Ow)
{
    if (-1 != Ow) {

        /// add mapping
        m_state2ow.Set (State, Ow);

    } else {

        /// remove mapping
        m_state2ow.Remove (State);
    }
}

void FAState2Ow::Clear ()
{
    m_state2ow.Clear ();
}

}
