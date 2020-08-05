/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTree2PosEqs.h"
#include "FARegexpTree.h"
#include "FAUtils.h"

namespace BlingFire
{


FARegexpTree2PosEqs::FARegexpTree2PosEqs (FAAllocatorA * pAlloc) :
    m_pTree (NULL),
    m_sort (pAlloc),
    m_MaxPos (0)
{
    m_Node2Pos.SetAllocator (pAlloc);
    m_Node2Pos.Create ();

    m_eq_pos.SetAllocator (pAlloc);
    m_eq_pos.Create ();

    m_seen.SetAllocator (pAlloc);
    m_seen.Create ();
}


void FARegexpTree2PosEqs::SetRegexpTree (const FARegexpTree * pTree)
{
    m_pTree = pTree;
}


const FAMapA * FARegexpTree2PosEqs::GetPos2Class () const
{
    return & m_Pos2Class;
}


void FARegexpTree2PosEqs::CalcNode2Pos ()
{
    DebugLogAssert (m_pTree);

    m_MaxPos = 0;

    const int MaxNodeId = m_pTree->GetMaxNodeId ();
    m_Node2Pos.resize (MaxNodeId + 1);

    for (int i = 0; i <= MaxNodeId; ++i) {

        DebugLogAssert (false == m_pTree->IsDeleted (i));

        const int LeftId = m_pTree->GetLeft (i);
        const int RightId = m_pTree->GetRight (i);

        if (-1 == LeftId && -1 == RightId) {

            m_Node2Pos [i] = m_MaxPos++;

        } else {

            m_Node2Pos [i] = -1;
        }
    }
}


void FARegexpTree2PosEqs::TopoSort ()
{
    DebugLogAssert (m_pTree);

    FARegexpTreeTopoGraph tree (m_pTree);
    m_sort.SetGraph (&tree);
    m_sort.Process ();
}


void FARegexpTree2PosEqs::FindEqPos (const int NodeId)
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (FARegexpTree::TYPE_DISJUNCTION == m_pTree->GetType (NodeId));

    m_seen.set_bit (NodeId, true);
    m_eq_pos.resize (0);

    const int LeftChildOfNode = m_pTree->GetLeft (NodeId);
    DebugLogAssert (-1 != LeftChildOfNode);

    if (FARegexpTree::TYPE_SYMBOL != m_pTree->GetType (LeftChildOfNode) ||
        -1 != m_pTree->GetTrBr (LeftChildOfNode))
        return;

    const int RightChildOfNode = m_pTree->GetRight (NodeId);
    DebugLogAssert (-1 != RightChildOfNode);

    if (FARegexpTree::TYPE_SYMBOL != m_pTree->GetType (RightChildOfNode) ||
        -1 != m_pTree->GetTrBr (RightChildOfNode))
        return;

    const int LeftPosOfNode = m_Node2Pos [LeftChildOfNode];
    DebugLogAssert (-1 != LeftPosOfNode);
    m_eq_pos.push_back (LeftPosOfNode);

    const int RightPosOfNode = m_Node2Pos [RightChildOfNode];
    DebugLogAssert (-1 != RightPosOfNode);
    m_eq_pos.push_back (RightPosOfNode);

    int Parent = m_pTree->GetParent (NodeId);
    if (-1 != m_pTree->GetTrBr (NodeId))
        return;

    while (true) {

        if (-1 == Parent)
            break;
        if (FARegexpTree::TYPE_DISJUNCTION != m_pTree->GetType (Parent))
            break;

        const int RightChild = m_pTree->GetRight (Parent);

        if (FARegexpTree::TYPE_SYMBOL == m_pTree->GetType (RightChild) && 
            -1 == m_pTree->GetTrBr (RightChild)) {

            const int RightPos = m_Node2Pos [RightChild];
            DebugLogAssert (-1 != RightPos);
            m_eq_pos.push_back (RightPos);

            m_seen.set_bit (Parent, true);
            if (-1 != m_pTree->GetTrBr (Parent))
                break;

            Parent = m_pTree->GetParent (Parent);
            continue;
        }

        const int LeftChild = m_pTree->GetLeft (Parent);

        if (FARegexpTree::TYPE_SYMBOL == m_pTree->GetType (LeftChild) && 
            -1 == m_pTree->GetTrBr (LeftChild)) {

            const int LeftPos = m_Node2Pos [LeftChild];
            DebugLogAssert (-1 != LeftPos);
            m_eq_pos.push_back (LeftPos);

            m_seen.set_bit (Parent, true);
            if (-1 != m_pTree->GetTrBr (Parent))
                break;

            Parent = m_pTree->GetParent (Parent);
            continue;
        }
        break;
    }
}


void FARegexpTree2PosEqs::BuildEqClasses ()
{
    DebugLogAssert (m_pTree);

    int i;

    // build initial classes
    for (i = 0; i < m_MaxPos; ++i) {
        m_Pos2Class.Set (i, i);
    }

    // go in reverse topological order (bottom-up)
    const int * pOrder;
    const int NodeCount = m_sort.GetTopoOrder (&pOrder);

    m_seen.resize (NodeCount);
    m_seen.set_bits (0, NodeCount - 1, false);

    for (i = NodeCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int NodeId = pOrder [i];
        const int NodeType = m_pTree->GetType (NodeId);

        if (FARegexpTree::TYPE_DISJUNCTION != NodeType || 
            m_seen.get_bit (NodeId)) {
            continue;
        }

        FindEqPos (NodeId);

        const int Count = FASortUniq (m_eq_pos.begin (), m_eq_pos.end ());
        m_eq_pos.resize (Count);

        if (1 < Count) {

            const int MinPos = m_eq_pos [0];
            for (int j = 1; j < Count; ++j) {

                const int Pos = m_eq_pos [j];
                m_Pos2Class.Set (Pos, MinPos);
            }
        }

    } // of for (i = NodeCount - 1; ...
}


void FARegexpTree2PosEqs::Process ()
{
    DebugLogAssert (m_pTree);

    m_Pos2Class.Clear ();

    CalcNode2Pos ();
    TopoSort ();
    BuildEqClasses ();
}

}
