/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAEpsilonGraph_mealy.h"
#include "FARSNfaA.h"
#include "FAMealyNfaA.h"

namespace BlingFire
{


FAEpsilonGraph_mealy::FAEpsilonGraph_mealy (FAAllocatorA * pAlloc) :
    FAEpsilonGraph (pAlloc),
    m_pInMealy (NULL),
    m_pOutMealy (NULL)
{
    m_Ows2Id.SetAllocator (pAlloc);
    m_Ows2Id.SetCopyChains (true);

    m_chain.SetAllocator (pAlloc);
    m_chain.Create ();
}


void FAEpsilonGraph_mealy::SetInMealy (const FAMealyNfaA * pInMealy)
{
    m_pInMealy = pInMealy;
}


void FAEpsilonGraph_mealy::SetOutMealy (FAMealyNfaA * pOutMealy)
{
    m_pOutMealy = pOutMealy;
}


const FAChain2NumA * FAEpsilonGraph_mealy::GetOwsMap () const
{
    return & m_Ows2Id;
}


void FAEpsilonGraph_mealy::Clear ()
{
    m_Ows2Id.Clear ();
}


void FAEpsilonGraph_mealy::
    UpdateOws (
        const int Src, 
        const int Iw, 
        const int Dst, 
        const int Ow, 
        const int OwsId
    )
{
    DebugLogAssert (m_pOutMealy);
    DebugLogAssert (-1 != OwsId || -1 != Ow);

    m_chain.resize (0);

    // add Ow in the beginning of new Ow chain, if any
    if (-1 != Ow) {
        m_chain.push_back (Ow);
    }

    /// get the chain of Ows corresponding to OwsId
    if (-1 != OwsId) {

        const int * pOws;
        const int Count = m_Ows2Id.GetChain (OwsId, &pOws); 
        DebugLogAssert (0 < Count && pOws);

        const int OldSize = m_chain.size ();
        m_chain.resize (OldSize + Count);

        memcpy (m_chain.begin () + OldSize, pOws, sizeof (int) * Count);
    }

    /// see whether such a chain already exist
    int NewOwsId = m_Ows2Id.GetIdx (m_chain.begin (), m_chain.size ());

    if (-1 == NewOwsId) {
        NewOwsId = m_Ows2Id.Add (m_chain.begin (), m_chain.size (), 0);
        DebugLogAssert (-1 != NewOwsId);
    }

    m_pOutMealy->SetOw (Src, Iw, Dst, NewOwsId);
}


void FAEpsilonGraph_mealy::
    SetDstNodes (const int Node, const int * pDstNodes, const int Size)
{
    DebugLogAssert (m_pInNfa && m_pOutNfa);
    DebugLogAssert (m_pInMealy && m_pOutMealy);

    // get destinations by epsilon from Node
    const int * pT;
    const int TSize = m_pInNfa->GetDest (Node, m_EpsilonIw, &pT);
    DebugLogAssert (0 < TSize && pT);

    // update Ows
    for (int i = 0; i < TSize; ++i) {

        DebugLogAssert (pT);
        const int T = pT [i];

        // get Ow associated with <Node, E, T>, -1 if no Ow
        const int Ow = m_pInMealy->GetOw (Node, m_EpsilonIw, T);

        const int * pIws;
        const int IwCount = m_pOutNfa->GetIWs (T, &pIws);
        DebugLogAssert (0 < IwCount && pIws);

        for (int j = 0; j < IwCount; ++j) {

            const int Iw = pIws [j];

            const int * pDst;
            const int DstCount = m_pOutNfa->GetDest (T, Iw, &pDst);
            DebugLogAssert (0 < DstCount && pDst);

            for (int k = 0; k < DstCount; ++k) {

                const int D = pDst [k];
                const int OwsId = m_pOutMealy->GetOw (T, Iw, D);

                if (-1 != Ow || -1 != OwsId) {
                    // adds < Node, Iw, D > --> [Ow, RightOws] relation
                    UpdateOws (Node, Iw, D, Ow, OwsId);
                }
            } // for (int k = 0; ...
        } // of for (int j = 0; ... 
    } // of for (int i = 0; ... 

    // update m_pOutNfa
    FAEpsilonGraph::SetDstNodes (Node, pDstNodes, Size);
}

}
