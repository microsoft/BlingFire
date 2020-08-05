/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMealyDfa.h"
#include "FARSDfaCA.h"

namespace BlingFire
{


FAMealyDfa::FAMealyDfa (FAAllocatorA * pAlloc) :
    m_pRsDfa (NULL)
{
    m_arc2ow.SetAllocator (pAlloc);
    m_arc2ow.SetCopyChains (true);
    m_arc2ow.SetEncoder (&m_enc);
}


FAMealyDfa::~FAMealyDfa ()
{}


void FAMealyDfa::SetRsDfa (const FARSDfaCA * pRsDfa)
{
    m_pRsDfa = pRsDfa;
}


const int FAMealyDfa::GetOw (const int Src, const int Iw) const
{
    int T [2];
    T [0] = Src;
    T [1] = Iw;

    const int * pOw = m_arc2ow.Get (& (T [0]), 2);

    if (NULL == pOw) {
        return -1;
    } else {
        return *pOw;
    }
}


void FAMealyDfa::SetOw (const int Src, const int Iw, const int Ow)
{
    int T [2];
    T [0] = Src;
    T [1] = Iw;

    if (-1 != Ow) {
        m_arc2ow.Add (& (T [0]), 2, Ow);
    } else {
        m_arc2ow.Remove (& (T [0]), 2);
    }
}


void FAMealyDfa::Clear ()
{
    m_arc2ow.Clear ();
}


void FAMealyDfa::Prepare ()
{}


const int FAMealyDfa::
    GetDestOw (const int State, const int Iw, int * pOw) const
{
    DebugLogAssert (pOw);
    *pOw = FAMealyDfa::GetOw (State, Iw);

    if (m_pRsDfa) {

        const int Dest = m_pRsDfa->GetDest (State, Iw);
        return Dest;

    } else {

        return -1;
    }
}

}
