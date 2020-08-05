/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMealyDfa_ro.h"
#include "FARSDfaCA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMealyDfa_ro::FAMealyDfa_ro (FAAllocatorA * pAlloc) :
    m_pRsDfa (NULL),
    m_TmpState (-1)
{
    m_State2Sets.SetAllocator (pAlloc);
    m_State2Sets.Create ();

    m_Sets.SetAllocator (pAlloc);
    m_Sets.SetCopyChains (true);

    m_tmp_iws.SetAllocator (pAlloc);
    m_tmp_iws.Create ();

    m_tmp_ows.SetAllocator (pAlloc);
    m_tmp_ows.Create ();
}


FAMealyDfa_ro::~FAMealyDfa_ro ()
{}


void FAMealyDfa_ro::Clear ()
{
    m_TmpState = -1;

    m_tmp_iws.Clear ();
    m_tmp_iws.Create ();
    m_tmp_ows.Clear ();
    m_tmp_ows.Create ();
    m_State2Sets.Clear ();
    m_State2Sets.Create ();
    m_Sets.Clear ();
}


void FAMealyDfa_ro::SetRsDfa (const FARSDfaCA * pRsDfa)
{
    m_pRsDfa = pRsDfa;
}


inline void FAMealyDfa_ro::AddIwsOws ()
{
    DebugLogAssert (m_tmp_iws.size () == m_tmp_ows.size ());
    DebugLogAssert (0 == m_State2Sets.size () % 2);

    const int * pIws = m_tmp_iws.begin ();
    const int * pOws = m_tmp_ows.begin ();
    const int Count = m_tmp_ows.size ();
    DebugLogAssert (FAIsSortUniqed (pIws, Count));

    const int Idx1 = m_Sets.Add (pIws, Count, 0);
    const int Idx2 = m_Sets.Add (pOws, Count, 0);

    const int OldSize = m_State2Sets.size ();
    const int I = m_TmpState << 1;

    if (OldSize <= I) {
        m_State2Sets.resize (I + 2, OldSize);
        const int GapSize = int (I - OldSize);
        if (0 < GapSize) {
            int * pOut = m_State2Sets.begin ();
            memset (pOut + OldSize, -1, GapSize * sizeof (int));
        }
    }

    m_State2Sets [I] = Idx1;
    m_State2Sets [I + 1] = Idx2;
}


void FAMealyDfa_ro::SetOw (const int Src, const int Iw, const int Ow)
{
    DebugLogAssert (m_tmp_iws.size () == m_tmp_ows.size ());
    DebugLogAssert (0 == m_State2Sets.size () % 2);
    DebugLogAssert (0 <= Src);

    if (Src != m_TmpState) {

        if (-1 != m_TmpState) {
            AddIwsOws ();
        }

        m_TmpState = Src;
        m_tmp_iws.resize (0);
        m_tmp_ows.resize (0);
    }

    m_tmp_iws.push_back (Iw);
    m_tmp_ows.push_back (Ow);
}


void FAMealyDfa_ro::Prepare ()
{
    if (-1 != m_TmpState) {
        AddIwsOws ();
    }

    m_TmpState = -1;
    m_tmp_iws.Clear ();
    m_tmp_iws.Create ();
    m_tmp_ows.Clear ();
    m_tmp_ows.Create ();
}


const int FAMealyDfa_ro::
    GetDestIwOw (const int State, const int Ow1, int * pIw, int * pOw2) const
{
    DebugLogAssert (0 <= State && pIw && pOw2);

    const int I = State << 1;

    if ((unsigned int) I < m_State2Sets.size ()) {

        const int Idx1 = m_State2Sets [I];
        const int Idx2 = m_State2Sets [I + 1];

        if (-1 == Idx1) {
            DebugLogAssert (-1 == Idx2);
            return -1;
        }

        // get Ows
        const int * pOws;
        const int Count = m_Sets.GetChain(Idx2, &pOws);
        DebugLogAssert (0 < Count);

        // find equal or less Ow idx
        const int OwIdx = FAFindEqualOrLess_log (pOws, Count, Ow1);

        // get Ow
        if (0 <= OwIdx && OwIdx < Count) {
            *pOw2 = pOws [OwIdx];
        } else {
            *pOw2 = -1;
            return -1;
        }

        const int * pIws;
#ifndef NDEBUG
        const int Count2 = 
#endif
            m_Sets.GetChain(Idx1, &pIws);
        DebugLogAssert (Count2 == Count && FAIsSortUniqed (pIws, Count2));

        // get Iw
        const int Iw = pIws [OwIdx];
        *pIw = Iw;

        if (m_pRsDfa) {
            return m_pRsDfa->GetDest (State, Iw);
        }
    }

    return -1;
}


const int FAMealyDfa_ro::GetOw (const int State, const int Iw) const
{
    DebugLogAssert (0 <= State);

    const int I = State << 1;

    if ((unsigned int) I < m_State2Sets.size ()) {

        const int Idx1 = m_State2Sets [I];
        const int Idx2 = m_State2Sets [I + 1];

        if (-1 == Idx1) {
            DebugLogAssert (-1 == Idx2);
            return -1;
        }

        const int * pIws;
        const int Count = m_Sets.GetChain(Idx1, &pIws);
        DebugLogAssert (0 < Count && FAIsSortUniqed (pIws, Count));

        const int OwIdx = FAFind_log (pIws, Count, Iw);

        if (-1 != OwIdx) {

            DebugLogAssert (OwIdx < Count);

            const int * pOws;
#ifndef NDEBUG
            const int Count2 = 
#endif
                m_Sets.GetChain (Idx2, &pOws);
            DebugLogAssert (Count2 == Count && pOws);

            return pOws [OwIdx];
        }
    }

    return -1;
}


const int FAMealyDfa_ro::
    GetDestOw (const int State, const int Iw, int * pOw) const
{
    DebugLogAssert (pOw);

    *pOw = FAMealyDfa_ro::GetOw (State, Iw);

    if (m_pRsDfa) {
        return m_pRsDfa->GetDest (State, Iw);
    }

    return -1;
}

}
