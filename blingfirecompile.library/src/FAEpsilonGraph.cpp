/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAEpsilonGraph.h"
#include "FAAllocatorA.h"
#include "FARSNfaA.h"
#include "FAFsmConst.h"

#include <iostream>

namespace BlingFire
{


FAEpsilonGraph::FAEpsilonGraph (FAAllocatorA * pAlloc) :
    m_pInNfa (NULL),
    m_pOutNfa (NULL),
    m_EpsilonIw (-1),
    m_set_ops (pAlloc),
    m_DeadState (FAFsmConst::NFA_DEAD_STATE)
{
  m_set_ptrs.SetAllocator (pAlloc);
  m_set_ptrs.Create ();

  m_set_lengths.SetAllocator (pAlloc);
  m_set_lengths.Create ();
}


FAEpsilonGraph::~FAEpsilonGraph ()
{}


void FAEpsilonGraph::SetInNfa (const FARSNfaA * pInNfa)
{
    m_pInNfa = pInNfa;
}

void FAEpsilonGraph::SetOutNfa (FARSNfaA * pOutNfa)
{
    m_pOutNfa = pOutNfa;
}


void FAEpsilonGraph::SetEpsilonIw (const int EpsilonIw)
{
    m_EpsilonIw = EpsilonIw;
}


const int FAEpsilonGraph::GetNodeCount () const
{
    DebugLogAssert (NULL != m_pInNfa);

    return 1 + m_pInNfa->GetMaxState ();
}


const int FAEpsilonGraph::GetDstNodes (
        const int Node, 
        __out_ecount(DstNodesSize) int * pDstNodes, 
        const int DstNodesSize
    ) const
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (0 <= DstNodesSize);

    // get Nfa destination states
    const int * pDstStates;
    int Size = m_pInNfa->GetDest (Node, m_EpsilonIw, &pDstStates);

    // copy dst states if memory is allocated and all elements can be copied
    if (pDstNodes && DstNodesSize >= Size) {

        if (0 < Size) {
            for (int i = 0; i < Size; ++i) {
                pDstNodes [i] = pDstStates [i];
            }
            return Size;

        } else if (0 == Size) {

            pDstNodes [0] = m_DeadState;
            return 1;
        }
    }

    return -1;
}


const int FAEpsilonGraph::GetDstNodes2 (const int Node, const int ** ppDstNodes) const
{
    DebugLogAssert (m_pInNfa);
    DebugLogAssert (ppDstNodes);

    // get Nfa destination states
    const int Size = m_pInNfa->GetDest (Node, m_EpsilonIw, ppDstNodes);

    if (0 == Size) {
        *ppDstNodes = &m_DeadState;
        return 1;
    }

    // return size anywhere
    return Size;
}


void FAEpsilonGraph::SetDstNodes (const int Node, 
                                  const int * pDstNodes, 
                                  const int Size)
{
    DebugLogAssert (0 == m_set_ptrs.size () && 0 == m_set_lengths.size ());
    DebugLogAssert (NULL != m_pInNfa && NULL != m_pOutNfa);
    DebugLogAssert (0 <= Size);

    int i, j;

    // update destination states for the (Node, EpsilonIw) pair
    m_pOutNfa->SetTransition (Node, m_EpsilonIw, pDstNodes, Size);

    // calculate the union of Iws_DstNodes
    m_set_ptrs.resize (Size);
    m_set_lengths.resize (Size);

    for (i = 0; i < Size; ++i) {

        DebugLogAssert (pDstNodes);
        const int DstState = pDstNodes [i];

        const int * pIws;
        const int IwsSize = m_pOutNfa->GetIWs (DstState, &pIws);

        m_set_ptrs [i] = pIws;
        m_set_lengths [i] = IwsSize;
    }

    m_set_ops.UnionN (m_set_ptrs.begin (), m_set_lengths.begin (), Size, 0);

    const int * pIwsUnion;
    const int IwsUnionSize = m_set_ops.GetRes (&pIwsUnion, 0);

    m_set_ptrs.resize (Size + 1);
    m_set_lengths.resize (Size + 1);

    // make iteration thru the union of Iws_DstNodes
    for (j = 0; j < IwsUnionSize; ++j) {

        DebugLogAssert (pIwsUnion);
        const int Iw = pIwsUnion [j];

        // skip epsilon transition
        if (Iw == m_EpsilonIw)
            continue;

        // make iteration thru the destination states
        // and calculate the destination sets union for the given Iw
        for (i = 0; i < Size; ++i) {

            DebugLogAssert (pDstNodes);
            const int DstState = pDstNodes [i];

            const int * pDsts;
            const int DstsSize = m_pOutNfa->GetDest (DstState, Iw, &pDsts);

            m_set_ptrs [i] = pDsts;
            m_set_lengths [i] = DstsSize;
        }

        const int * pDsts;
        const int DstsSize = m_pOutNfa->GetDest (Node, Iw, &pDsts);

        m_set_ptrs [Size] = pDsts;
        m_set_lengths [Size] = DstsSize;

        m_set_ops.UnionN (m_set_ptrs.begin (), m_set_lengths.begin (), Size + 1, 1);

        const int * pDstsUnion;
        const int DstsUnionSize = m_set_ops.GetRes (&pDstsUnion, 1);

        // update Node's Dst_Iw set
        if (0 < DstsUnionSize) {
            m_pOutNfa->SetTransition (Node, Iw, pDstsUnion, DstsUnionSize);
        } else if (0 == DstsUnionSize) {
            m_pOutNfa->SetTransition (Node, Iw, &m_DeadState, 1);
        }

    } // of for (j = 0; ...

    // return class into the proper state
    m_set_ptrs.resize (0);
    m_set_lengths.resize (0);
}

}
