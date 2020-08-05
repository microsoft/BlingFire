/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaDelta_ro.h"
#include "FAException.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FANfaDelta_ro::FANfaDelta_ro (FAAllocatorA * pAlloc)
{
    m_State2FC.SetAllocator (pAlloc);
    m_State2FC.Create ();

    m_Iws.SetAllocator (pAlloc);
    m_Iws.Create ();

    m_Dsts.SetAllocator (pAlloc);
    m_Dsts.Create ();

    m_Sets.SetAllocator (pAlloc);
    m_Sets.SetCopyChains (true);
}


void FANfaDelta_ro::Clear ()
{
    m_State2FC.Clear ();
    m_State2FC.Create ();

    m_Iws.Clear ();
    m_Iws.Create ();

    m_Dsts.Clear ();
    m_Dsts.Create ();

    m_Sets.Clear ();
}


void FANfaDelta_ro::
    AddTransition (
        const int State,
        const int Iw,
        const int * pDsts,
        const int Count
    )
{
    DebugLogAssert (0 <= State && 0 <= Iw && pDsts && 0 <= Count);
    DebugLogAssert (true == FAIsSortUniqed (pDsts, Count));
    DebugLogAssert (m_Dsts.size () == m_Iws.size ());
    DebugLogAssert (0 == m_State2FC.size () % 2);

    int SetId = -1;
    if (1 == Count) {
        DebugLogAssert (0 <= *pDsts || FAFsmConst::NFA_DEAD_STATE == *pDsts);
        SetId = *pDsts;
    } else if (1 < Count) {
        SetId = - (2 + m_Sets.Add (pDsts, Count, 0));
    }

    const int FCCount = m_State2FC.size ();
    const int FCIdx = State << 1;

    if (FCCount <= FCIdx) {

        m_State2FC.resize (FCIdx + 2);
        int * pOut = m_State2FC.begin () + FCCount;

        const int GapSize = int (FCIdx - FCCount);

        // fill in the gap, if any
        if (0 < GapSize) {
            memset (pOut, 0, GapSize * sizeof (int));
            pOut += GapSize;
        }
        // store From index
        *pOut++ = m_Iws.size ();
        // store initial count
        *pOut = 1;

    } else {

        DebugLogAssert (FCIdx + 1 < FCCount);
        // increment the count
        m_State2FC [FCIdx + 1]++;
    }

    // add input weight and SetId of destination states
    m_Iws.push_back (Iw);
    m_Dsts.push_back (SetId);
}


const int FANfaDelta_ro::GetIWs (const int State, const int ** ppIws) const
{
    DebugLogAssert (ppIws && 0 <= State);
    DebugLogAssert (m_Dsts.size () == m_Iws.size ());
    DebugLogAssert (0 == m_State2FC.size () % 2);

    const int FCIdx = State << 1;
    if ((unsigned int) FCIdx >= m_State2FC.size ()) {
        return 0;
    }

    const int From = m_State2FC [FCIdx];
    const int Count = m_State2FC [FCIdx + 1];

    DebugLogAssert (0 <= From && 0 <= Count && \
        m_Dsts.size () >= (unsigned int)(From + Count));

    *ppIws = m_Iws.begin () + From;

    return Count;
}


const int FANfaDelta_ro::
    GetDest (const int State, const int Iw, const int ** ppDsts) const
{
    if (FAFsmConst::NFA_DEAD_STATE == State) {
        return -1;
    }

    DebugLogAssert (ppDsts && 0 <= State);
    DebugLogAssert (m_Dsts.size () == m_Iws.size ());
    DebugLogAssert (0 == m_State2FC.size () % 2);

    const int FCIdx = State << 1;
    if ((unsigned int) FCIdx >= m_State2FC.size ()) {
        return -1;
    }

    const int From = m_State2FC [FCIdx];
    const int Count = m_State2FC [FCIdx + 1];

    if (0 == Count) {
        // the gap in state space
        return -1;
    }

    DebugLogAssert (0 <= From && 0 < Count && \
        m_Dsts.size () >= (unsigned int)(From + Count));

    const int * pIws = m_Iws.begin () + From;
    const int * pDsts = m_Dsts.begin () + From;
    DebugLogAssert (m_Dsts.begin () && m_Iws.begin ());

    const int i = FAFind_log (pIws, Count, Iw);
    DebugLogAssert ((0 <= i && i < Count) || (-1 == i));
    if (-1 == i) {
        return -1;
    }

    const int SetId = pDsts [i];

    if (0 <= SetId) {

        *ppDsts = pDsts + i;  // store SetId's address
        return 1;

    } else if (-1 > SetId) {

        const int ChainIdx = -(SetId + 2);
        const int ChainSize = m_Sets.GetChain (ChainIdx, ppDsts);
        return ChainSize;

    } else {
        DebugLogAssert (-1 == SetId);
        return 0;
    }
}

}
