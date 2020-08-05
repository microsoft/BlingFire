/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAIsDotNfa.h"
#include "FARSNfaA.h"
#include "FALimits.h"
#include "FAUtils.h"

namespace BlingFire
{


FAIsDotNfa::FAIsDotNfa (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_IwBase (0),
    m_IwMax (FALimits::MaxIw),
    m_pAlphabet (NULL),
    m_AlphabetSize (0),
    m_ExpAlphabet (false)
{
    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();
}


void FAIsDotNfa::SetNfa (const FARSNfaA * pNfa)
{
    m_pNfa = pNfa;
}


void FAIsDotNfa::SetIwBase (const int IwBase)
{
    m_IwBase = IwBase;
}


void FAIsDotNfa::SetIwMax (const int IwMax)
{
    m_IwMax = IwMax;
}


void FAIsDotNfa::SetExpIws (const int * pExpIws, const int ExpCount)
{
    if (-1 != ExpCount) {

        DebugLogAssert (0 == ExpCount || FAIsSortUniqed (pExpIws, ExpCount));

        m_AlphabetSize = ExpCount;
        m_pAlphabet = pExpIws;
        m_ExpAlphabet = true;

    } else {

        m_ExpAlphabet = false;
    }
}


void FAIsDotNfa::Prepare ()
{
    DebugLogAssert (m_pNfa);

    if (false == m_ExpAlphabet) {

        // update m_IwMax
        if (FALimits::MaxIw == m_IwMax) {
            m_IwMax = m_pNfa->GetMaxIw ();
        }

        // build the alphabet
        m_AlphabetSize = m_IwMax - m_IwBase + 1;
        m_alphabet.resize (m_AlphabetSize);
        m_pAlphabet = m_alphabet.begin ();

        int * pAlphabet = m_alphabet.begin ();
        DebugLogAssert (pAlphabet);

        int i = 0;

        for (int Iw = m_IwBase; Iw <= m_IwMax; ++Iw) {
            pAlphabet [i++] = Iw;
        }

    } else {

        if (0 < m_AlphabetSize) {

            m_IwBase = m_pAlphabet [0];
            m_IwMax =  m_pAlphabet [m_AlphabetSize - 1];

        } else {

            m_IwBase = -1;
            m_IwMax = -1;
        }
    }
}


const bool FAIsDotNfa::CmpAlphabet (const int State)
{
    DebugLogAssert (m_pNfa);

    m_tmp.resize (0);

    const int * pIws;
    const int Count = m_pNfa->GetIWs (State, &pIws);

    for (int i = 0; i < Count; ++i) {

        const int Iw = pIws [i];

        if (Iw >= m_IwBase && Iw <= m_IwMax) {
            m_tmp.push_back (Iw);
        }
    }

    const int * pTmp = m_tmp.begin ();
    const int TmpSize = m_tmp.size ();

    if (0 < TmpSize) {

        if (TmpSize != m_AlphabetSize || \
            0 != memcmp (pTmp, m_pAlphabet, m_AlphabetSize * sizeof (int)))
            return false;
    }

    return true;
}


const bool FAIsDotNfa::Process ()
{
    DebugLogAssert (m_pNfa);

    Prepare ();

    const int MaxState = m_pNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        // compare state's alphabet with m_pAlphabet
        if (!CmpAlphabet (State)) {
            return false;
        }

        const int * pIws = m_tmp.begin ();
        const int Count = m_tmp.size ();

        if (0 < Count) {

            DebugLogAssert (pIws);

            const int * pDsts;
            const int DstCount = m_pNfa->GetDest (State, *pIws, &pDsts);

            for (int i = 1; i < Count; ++i) {

                const int * pCurrDsts;
                const int CurrDstCount = \
                    m_pNfa->GetDest (State, pIws [i], &pCurrDsts);

                // compare destination sets
                if (DstCount != CurrDstCount || \
                    0 != memcmp (pCurrDsts, pDsts, DstCount * sizeof (int))) {
                    return false;
                }

            } // for (int i = 1; i < Count; ...
        } // if (0 < Count) ... 
    } // for (int State = 0; State <= MaxState; ...

    return true;
}

}
