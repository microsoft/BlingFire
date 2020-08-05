/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAState2Ows_amb.h"
#include "FARSNfaA.h"

namespace BlingFire
{


FAState2Ows_amb::FAState2Ows_amb (FAAllocatorA * pAlloc) :
    FAState2Ows (pAlloc),
    m_pRevPosNfa (NULL)
{
    m_is_amb.SetAllocator (pAlloc);
    m_is_amb.Create ();

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();
}

void FAState2Ows_amb::SetRevPosNfa (const FARSNfaA * pRevPosNfa)
{
    m_pRevPosNfa = pRevPosNfa;
}

void FAState2Ows_amb::Prepare ()
{
    if (NULL == m_pRevPosNfa)
        return;

    const int MaxState = m_pRevPosNfa->GetMaxState ();

    m_is_amb.resize (MaxState + 1);
    m_is_amb.set_bits (0, MaxState, false);

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwsCount = m_pRevPosNfa->GetIWs (State, &pIws);

        for (int i = 0; i < IwsCount; ++i) {

            DebugLogAssert (pIws);
            const int Iw = pIws [i];

            const int * pDsts;
            const int DstsCount = m_pRevPosNfa->GetDest (State, Iw, &pDsts);

            // see whether there is an ambiguity
            if (1 < DstsCount) {

                for (int j = 0; j < DstsCount; ++j) {

                    DebugLogAssert (pDsts);
                    const int DstState = pDsts [j];

                    m_is_amb.set_bit (DstState, true);
                }
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...
}

void FAState2Ows_amb::SetOws (const int State, const int * pOws, const int Size)
{
    if (NULL == m_pRevPosNfa) {

        FAState2Ows::SetOws (State, pOws, Size);

    } else if (0 < Size) {

        m_ows.resize (0);

        for (int i = 0; i < Size; ++i) {

            DebugLogAssert (pOws);
            const int Ow = pOws [i];

            if (true == m_is_amb.get_bit (Ow))
                m_ows.push_back (Ow);
        }

        const int NewSize = m_ows.size ();

        if (0 < NewSize)
            FAState2Ows::SetOws (State, m_ows.begin (), NewSize);

    } // of if (0 < Size) ...
}

}
