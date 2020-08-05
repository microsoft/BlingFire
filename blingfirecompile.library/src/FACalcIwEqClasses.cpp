/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACalcIwEqClasses.h"
#include "FAAllocatorA.h"
#include "FARSDfaA.h"
#include "FARSNfaA.h"
#include "FAMealyDfaA.h"
#include "FAMealyNfaA.h"
#include "FAMapA.h"
#include "FAUtils.h"

namespace BlingFire
{


FACalcIwEqClasses::FACalcIwEqClasses (FAAllocatorA * pAlloc) :
    m_pInDfa (NULL),
    m_pDfaSigma (NULL),
    m_pInNfa (NULL),
    m_pNfaSigma (NULL),
    m_IwBase (0),
    m_IwMax (0),
    m_NewIwBase (0),
    m_pIw2NewIw (NULL),
    m_MaxNewIw (-1),
    m_split_sets (pAlloc)
{
    m_e2iw.SetAllocator (pAlloc);
    m_e2iw.Create ();

    m_e2info.SetAllocator (pAlloc);
    m_e2info.Create ();

    m_set2id.SetAllocator (pAlloc);
    m_set2id.SetEncoder (&m_enc);

    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();
}


void FACalcIwEqClasses::SetIwBase (const int IwBase)
{
    m_IwBase = IwBase;
}


void FACalcIwEqClasses::SetIwMax (const int IwMax)
{
    m_IwMax = IwMax;
}


void FACalcIwEqClasses::SetNewIwBase (const int NewIwBase)
{
    m_NewIwBase = NewIwBase;
}


void FACalcIwEqClasses::SetRsDfa (const FARSDfaA * pInDfa)
{
    m_pInDfa = pInDfa;
}


void FACalcIwEqClasses::SetDfaSigma (const FAMealyDfaA * pDfaSigma)
{
    m_pDfaSigma = pDfaSigma;
}


void FACalcIwEqClasses::SetRsNfa (const FARSNfaA * pInNfa)
{
    m_pInNfa = pInNfa;
}


void FACalcIwEqClasses::SetNfaSigma (const FAMealyNfaA * pNfaSigma)
{
    m_pNfaSigma = pNfaSigma;
}


void FACalcIwEqClasses::SetIw2NewIw (FAMapA * pIw2NewIw)
{
    m_pIw2NewIw = pIw2NewIw;
}


const int FACalcIwEqClasses::GetMaxNewIw () const
{
    return m_MaxNewIw;
}


void FACalcIwEqClasses::Prepare ()
{
    DebugLogAssert ((m_pInDfa || m_pInNfa) && m_pIw2NewIw);

    m_MaxNewIw = -1;
    m_e2iw.resize (0);

    const int * pIws = NULL;
    int Count = -1;

    if (m_pInDfa) {

        Count = m_pInDfa->GetIWs (&pIws);

    } else {
        DebugLogAssert (m_pInNfa);

        FAGetAlphabet (m_pInNfa, &m_iws);
        pIws = m_iws.begin ();
        Count = m_iws.size ();
    }

    DebugLogAssert (0 < Count && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, Count));

    for (int i = 0; i < Count; ++i) {

        const int Iw = pIws [i];

        if (m_IwBase > Iw || m_IwMax < Iw) {
            m_pIw2NewIw->Set (Iw, Iw);
            continue;
        }

        m_e2iw.push_back (Iw);
    }

    m_split_sets.Prepare (m_e2iw.size ());
}


inline const int FACalcIwEqClasses::
    GetDestId (const int State, const int Iw)
{
    DebugLogAssert (m_pInNfa);

    const int * pDsts;
    const int Dsts = m_pInNfa->GetDest (State, Iw, &pDsts);

    int Id = -1;

    if (0 < Dsts) {
        // it does not add chain if it's already there
        Id = m_set2id.Add (pDsts, Dsts, 0);
    }

    return Id;
}


inline const int FACalcIwEqClasses::
    GetOwsId (const int State, const int Iw)
{
    DebugLogAssert (m_pInNfa && m_pNfaSigma);

    m_ows.resize (0);

    const int * pDsts;
    const int Dsts = m_pInNfa->GetDest (State, Iw, &pDsts);

    int Id = -1;

    if (0 < Dsts) {

        DebugLogAssert (pDsts);

        for (int i = 0; i < Dsts; ++i) {

            const int Dst = pDsts [i];
            const int Ow = m_pNfaSigma->GetOw (State, Iw, Dst);
            m_ows.push_back (Ow);
        }
    }

    const int OwsSize = m_ows.size ();

    if (0 < OwsSize) {

        const int * pOws = m_ows.begin ();
        DebugLogAssert (pOws);

        // it does not add chain if it's already there
        Id = m_set2id.Add (pOws, OwsSize, 0);
    }

    return Id;
}


void FACalcIwEqClasses::Clear ()
{
    m_e2iw.resize (0);
    m_e2info.resize (0);
    m_iws.resize (0);
    m_set2id.Clear ();
}


void FACalcIwEqClasses::Process ()
{
    DebugLogAssert ((m_pInDfa && !m_pInNfa) || (!m_pInDfa && m_pInNfa));
    DebugLogAssert (m_pIw2NewIw);

    Prepare ();

    /// classify

    const int * pIws = m_e2iw.begin ();
    const int Count = m_e2iw.size ();
    DebugLogAssert (0 < Count && pIws);

    m_e2info.resize (Count);
    int * pInfo = m_e2info.begin ();
    DebugLogAssert (pInfo);

    int MaxState;
    
    if (m_pInDfa) {
        MaxState = m_pInDfa->GetMaxState ();
    } else {
        DebugLogAssert (m_pInNfa);
        MaxState = m_pInNfa->GetMaxState ();
    }

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < Count; ++i) {

            const int Iw = pIws [i];

            int Dst;

            if (m_pInDfa) {
                Dst = m_pInDfa->GetDest (State, Iw);
            } else {
                Dst = GetDestId (State, Iw);
            }

            if (-1 == Dst) {
                Dst = MaxState + 1;
            }

            pInfo [i] = Dst;

        } // of for (int i = 0; i < IwsCount; ...

        m_split_sets.AddInfo (pInfo, Count);

        if (m_pDfaSigma || m_pNfaSigma) {

            for (int i = 0; i < Count; ++i) {

                const int Iw = pIws [i];

                int Ow = -1;

                if (m_pDfaSigma) {
                    Ow = m_pDfaSigma->GetOw (State, Iw);
                } else {
                    Ow = GetOwsId (State, Iw);
                }

                pInfo [i] = Ow;

            } // of for (int i = 0; i < Count; ++i) ...

            m_split_sets.AddInfo (pInfo, Count);

        } // of if (m_pDfaSigma || m_pNfaSigma) ...

    } // of for (int State = 0; State <= MaxState; ...

    m_split_sets.Process ();

    /// build the output map

    const int * pI2C;
#ifndef NDEBUG
    const int Count2 = 
#endif
        m_split_sets.GetClasses (&pI2C);
    DebugLogAssert (Count2 == Count && pI2C);

    // create the output map
    for (int i = 0; i < Count; ++i) {

        const int Iw = pIws [i];
        const int C = pI2C [i];
        DebugLogAssert (0 <= C);
        const int NewIw = C + m_NewIwBase;

        m_pIw2NewIw->Set (Iw, NewIw);

        if (m_MaxNewIw < NewIw) {
            m_MaxNewIw = NewIw;
        }
    }

    Clear ();
}

}
