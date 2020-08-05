/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWord2WordSuff.h"
#include "FARSDfaCA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAWord2WordSuff::FAWord2WordSuff (FAAllocatorA * pAlloc) :
    m_pIws (NULL),
    m_IwsCount (0),
    m_pDfa (NULL),
    m_Delim (0),
    m_PrefFound (false)
{
    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();

    m_split.SetAllocator (pAlloc);
    m_split.Create ();

    m_delim_pos.SetAllocator (pAlloc);
    m_delim_pos.Create ();

    m_left.SetAllocator (pAlloc);
    m_left.Create ();

    m_right.SetAllocator (pAlloc);
    m_right.Create ();
}


FAWord2WordSuff:: ~FAWord2WordSuff ()
{}


void FAWord2WordSuff::SetRsDfa (const FARSDfaCA * pDfa)
{
    m_pDfa = pDfa;
}


void FAWord2WordSuff::SetDelim (const int Delim)
{
    m_Delim = Delim;
}


void FAWord2WordSuff::Process ()
{
    DebugLogAssert (m_pDfa);

    m_IwsCount = m_pDfa->GetIWs (NULL, 0);
    DebugLogAssert (0 < m_IwsCount);

    m_iws.resize (m_IwsCount);
    m_pDfa->GetIWs (m_iws.begin (), m_IwsCount);
    m_pIws = m_iws.begin ();
    DebugLogAssert (m_pIws);

    DebugLogAssert (-1 == FAFind_log < int > (m_pIws, m_IwsCount, m_Delim));

    const int InitialState = m_pDfa->GetInitial ();

    Split_rec (InitialState);
}


void FAWord2WordSuff::PutSplits ()
{
    const int SplitSize = m_split.size ();

    if (0 < SplitSize) {
        const int * pSplit = m_split.begin ();

        m_delim_pos.resize (0);

        for (int j = 0; j < SplitSize; ++j) {
            if (m_Delim == pSplit [j]) {
                m_delim_pos.push_back (j);
            }
        }

        const int DelimCount = m_delim_pos.size ();

        if (2 > DelimCount)
            return;

        for (int DelimIdx = 0; DelimIdx < DelimCount - 1; ++DelimIdx) {

            const int j = m_delim_pos [DelimIdx];

            for (int SizeIdx = DelimIdx + 1; SizeIdx < DelimCount; ++SizeIdx) {

                const int CurrSplitSize = m_delim_pos [SizeIdx];

                m_left.resize (0);
                m_right.resize (0);

                for (int k = 0; k < j; ++k) {
                    const int Sym = pSplit [k];
                    if (m_Delim != Sym)
                        m_left.push_back (Sym);
                }
                for (int k = j + 1; k < CurrSplitSize; ++k) {
                    const int Sym = pSplit [k];
                    if (m_Delim != Sym)
                        m_right.push_back (Sym);
                }

                const int LeftSize = m_left.size ();
                const int RightSize = m_right.size ();

                if (0 < LeftSize && 0 < RightSize) {
                    const int * pLeft = m_left.begin ();
                    const int * pRight = m_right.begin ();
                    PutSplit (pLeft, LeftSize , pRight, RightSize);
                }
            }
        }

    } // if (0 < SplitSize) ...
}


void FAWord2WordSuff::Split_rec (const int State)
{
    DebugLogAssert (m_pDfa);

    if (m_pDfa->IsFinal (State)) {
        m_split.push_back (m_Delim);
    }

    bool PathFound = false;

    for (int i = 0; i < m_IwsCount; ++i) {

        const int Iw = m_pIws [i];
        const int DstState = m_pDfa->GetDest (State, Iw);

        if (-1 != DstState) {

            PathFound = true;
            m_split.push_back (Iw);

            Split_rec (DstState);

            if (m_Delim == m_split [m_split.size () - 1]) {
                m_split.pop_back ();
            }
            m_split.pop_back ();
        }
    }

    if (!PathFound) {
        PutSplits ();
    }
}


void FAWord2WordSuff::PutSplit (
            const int * /*pLeft*/, 
            const int /*LeftSize*/,
            const int * /*pRight*/, 
            const int /*RightSize*/
    )
{
    // assumed to be overloaded
}

}
