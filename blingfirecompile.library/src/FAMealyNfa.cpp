/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMealyNfa.h"

namespace BlingFire
{


FAMealyNfa::FAMealyNfa (FAAllocatorA * pAlloc)
{
    m_arc2ow.SetAllocator (pAlloc);
    m_arc2ow.SetCopyChains (true);
    m_arc2ow.SetEncoder (&m_enc);
}


FAMealyNfa::~FAMealyNfa ()
{}


const int FAMealyNfa::
    GetOw (const int Src, const int Iw, const int Dst) const
{
    int T [3];
    T [0] = Src; T [1] = Iw; T [2] = Dst;

    const int * pOw = m_arc2ow.Get (& (T [0]), 3);

    if (NULL == pOw) {
        return -1;
    } else {
        return *pOw;
    }
}


void FAMealyNfa::
    SetOw (const int Src, const int Iw, const int Dst, const int Ow)
{
    int T [3];
    T [0] = Src; T [1] = Iw; T [2] = Dst;

    if (-1 != Ow) {

        m_arc2ow.Add (& (T [0]), 3, Ow);

    } else {

        m_arc2ow.Remove (& (T [0]), 3);
    }
}


void FAMealyNfa::Clear ()
{
    m_arc2ow.Clear ();
}


void FAMealyNfa::Prepare ()
{}

}
