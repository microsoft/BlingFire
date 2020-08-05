/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADfaTopoGraph.h"
#include "FAAllocatorA.h"
#include "FARSDfaA.h"
#include "FAArray_cont_t.h"
#include "FAUtils.h"

namespace BlingFire
{


FADfaTopoGraph::FADfaTopoGraph (FAAllocatorA * pAlloc) : 
    m_pInDfa (NULL),
    m_pAlloc (pAlloc)
{}


void FADfaTopoGraph::SetDfa (const FARSDfaA * pInDfa)
{
    m_pInDfa = pInDfa;
}


const int FADfaTopoGraph::GetNodeCount () const
{
    DebugLogAssert (m_pInDfa);
    const int MaxState = m_pInDfa->GetMaxState ();
    return MaxState + 1;
}


const int FADfaTopoGraph::GetDstNodes (
        const int Node,
        __out_ecount(DstNodesSize) int * pDstNodes,
        const int DstNodesSize
    ) const
{
    DebugLogAssert (m_pInDfa);

    FAArray_cont_t < int > tmp_dst_nodes;
    tmp_dst_nodes.SetAllocator (m_pAlloc);
    tmp_dst_nodes.Create ();
    tmp_dst_nodes.resize (0);

    const int * pIws;
    const int IwCount = m_pInDfa->GetIWs (&pIws);

    for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

        const int Iw = pIws [iw_idx];
        const int DstState = m_pInDfa->GetDest (Node, Iw);

        if (-1 != DstState) {
            tmp_dst_nodes.push_back (DstState);
        }
    }

    const int Size = 
        FASortUniq (tmp_dst_nodes.begin (), tmp_dst_nodes.end ());
    tmp_dst_nodes.resize (Size);

    if (pDstNodes && DstNodesSize >= Size) {
        for (int i = 0; i < Size; ++i)
            pDstNodes [i] = tmp_dst_nodes [i];
    }

    return Size;
}

}
