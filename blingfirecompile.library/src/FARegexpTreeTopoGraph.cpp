/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTreeTopoGraph.h"
#include "FARegexpTree.h"

namespace BlingFire
{


FARegexpTreeTopoGraph::FARegexpTreeTopoGraph (const FARegexpTree * pTree)
{
    m_pTree = pTree;
}


const int FARegexpTreeTopoGraph::GetNodeCount () const
{
    return 1 + m_pTree->GetMaxNodeId ();
}


const int FARegexpTreeTopoGraph::GetDstNodes (
        const int NodeId,
        __out_ecount(MaxDstNodes) int * pDstNodes,
        const int MaxDstNodes
    ) const
{
    DebugLogAssert (m_pTree);

    if (m_pTree->IsDeleted (NodeId))
        return 0;

    int Count = 0;

    const int LeftNode = m_pTree->GetLeft (NodeId);
    const int RightNode = m_pTree->GetRight (NodeId);

    if (-1 != LeftNode) {
        Count++;
        if (NULL != pDstNodes && 0 < MaxDstNodes) {
            *pDstNodes = LeftNode;
            pDstNodes++;
        }
    }
    if (-1 != RightNode) {
        Count++;
        if (NULL != pDstNodes && 1 < MaxDstNodes)
            *pDstNodes = RightNode;
    }

    return Count;
}

}
