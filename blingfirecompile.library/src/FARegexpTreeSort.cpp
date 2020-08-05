/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTreeSort.h"
#include "FAAllocatorA.h"

#include <algorithm>

namespace BlingFire
{


FARegexpTreeSort::_TNodeCmp::_TNodeCmp (const int * pNode2Min,
                                        const int * pNode2Max,
                                        const FARegexpTree2Hash * pRe2Hash,
                                        const bool Reverse) :
    m_pNode2Min (pNode2Min),
    m_pNode2Max (pNode2Max),
    m_pRe2Hash (pRe2Hash),
    m_Reverse (Reverse)
{}


const bool FARegexpTreeSort::_TNodeCmp::operator () (const int NodeId1,
                                                     const int NodeId2) const
{
    DebugLogAssert (m_pNode2Min && m_pNode2Max);
    DebugLogAssert (0 <= NodeId1 && 0 <= NodeId2);

    const int Node1Min = m_pNode2Min [NodeId1];
    const int Node2Min = m_pNode2Min [NodeId2];
    DebugLogAssert (-1 != Node1Min && -1 != Node2Min);

    bool Res = false;

    if (Node1Min > Node2Min) {

        Res = true;

    } else if (Node1Min == Node2Min) {

        const int Node1Max = m_pNode2Max [NodeId1];
        const int Node2Max = m_pNode2Max [NodeId2];
        DebugLogAssert (-1 != Node1Max && -1 != Node2Max);

        if (Node1Max > Node2Max) {

            Res = true;

        } else if (Node1Max == Node2Max) {

            const int Key1 = m_pRe2Hash->GetKey (NodeId1);
            const int Key2 = m_pRe2Hash->GetKey (NodeId2);

            Res = Key1 < Key2;
        }
    }

    if (m_Reverse)
        return !Res;
    else
        return Res;
}


FARegexpTreeSort::FARegexpTreeSort (FAAllocatorA * pAlloc) :
    m_pTree (NULL),
    m_sort (pAlloc),
    m_re2hash (pAlloc),
    m_Reverse (false)
{
    m_node2min.SetAllocator (pAlloc);
    m_node2min.Create ();

    m_node2max.SetAllocator (pAlloc);
    m_node2max.Create ();

    m_disj_nodes.SetAllocator (pAlloc);
    m_disj_nodes.Create ();

    m_child_nodes.SetAllocator (pAlloc);
    m_child_nodes.Create ();

    m_seen.SetAllocator (pAlloc);
    m_seen.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();
}


void FARegexpTreeSort::SetRegexp (const char * pRegexp, const int Length)
{
    m_re2hash.SetRegexp (pRegexp, Length);
}


void FARegexpTreeSort::SetRegexpTree (FARegexpTree * pTree)
{
    m_pTree = pTree;
    m_re2hash.SetRegexpTree (pTree);
}


void FARegexpTreeSort::SetReverse (const bool Reverse)
{
    m_Reverse = Reverse;
}


void FARegexpTreeSort::TopoSort ()
{
    DebugLogAssert (m_pTree);

    // make topological sorting of the nodes
    FARegexpTreeTopoGraph tree (m_pTree);
    m_sort.SetGraph (&tree);
    m_sort.Process ();
}


void FARegexpTreeSort::CalcMinMax ()
{
    DebugLogAssert (m_pTree);

    // go in reverse topological order (bottom-up)
    const int * pOrder;
    const int NodeCount = m_sort.GetTopoOrder (&pOrder);

    for (int i = NodeCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int NodeId = pOrder [i];

        if (m_pTree->IsDeleted (NodeId))
            continue;

        const int NodeType = m_pTree->GetType (NodeId);

        if (
            FARegexpTree::TYPE_SYMBOL == NodeType ||
            FARegexpTree::TYPE_ANY == NodeType ||
            FARegexpTree::TYPE_L_ANCHOR == NodeType ||
            FARegexpTree::TYPE_R_ANCHOR == NodeType
        ) {
            m_node2min [NodeId] = 1;
            m_node2max [NodeId] = 1;

        } else if (FARegexpTree::TYPE_CONCAT == NodeType) {

            const int LeftId = m_pTree->GetLeft (NodeId);
            const int RightId = m_pTree->GetRight (NodeId);

            const int LeftMin = m_node2min [LeftId];
            const int RightMin = m_node2min [RightId];
            DebugLogAssert (-1 != LeftMin && -1 != RightMin);

            m_node2min [NodeId] = LeftMin + RightMin;

            const int LeftMax = m_node2max [LeftId];
            const int RightMax = m_node2max [RightId];
            DebugLogAssert (-1 != LeftMax && -1 != RightMax);

            m_node2max [NodeId] = LeftMax + RightMax;

        } else if (FARegexpTree::TYPE_DISJUNCTION == NodeType) {

            const int LeftId = m_pTree->GetLeft (NodeId);
            const int RightId = m_pTree->GetRight (NodeId);

            const int LeftMin = m_node2min [LeftId];
            const int RightMin = m_node2min [RightId];
            DebugLogAssert (-1 != LeftMin && -1 != RightMin);

            if (LeftMin < RightMin)
                m_node2min [NodeId] = LeftMin;
            else
                m_node2min [NodeId] = RightMin;

            const int LeftMax = m_node2max [LeftId];
            const int RightMax = m_node2max [RightId];
            DebugLogAssert (-1 != LeftMax && -1 != RightMax);

            if (LeftMax > RightMax)
                m_node2max [NodeId] = LeftMax;
            else
                m_node2max [NodeId] = RightMax;

        } else {

            const int LeftId = m_pTree->GetLeft (NodeId);

            const int LeftMin = m_node2min [LeftId];
            DebugLogAssert (-1 != LeftMin);
            m_node2min [NodeId] = LeftMin;

            const int LeftMax = m_node2max [LeftId];
            DebugLogAssert (-1 != LeftMax);
            m_node2max [NodeId] = LeftMax;
        }
    } // of for (int i = NodeCount - 1; ...
}


const int FARegexpTreeSort::FindTopMostParent (const int NodeId) const
{
    DebugLogAssert (m_pTree);

    const int ConstrTrBr = m_pTree->GetTrBr (NodeId);

    int TopMostParent = NodeId;
    int Parent = m_pTree->GetParent (TopMostParent);

    while (true) {

        if (-1 == Parent)
            break;
        if (FARegexpTree::TYPE_DISJUNCTION != m_pTree->GetType (Parent))
            break;
        if (ConstrTrBr != m_pTree->GetTrBr (Parent))
            break;

        TopMostParent = Parent;
        Parent = m_pTree->GetParent (TopMostParent);
    }

    return TopMostParent;
}


void FARegexpTreeSort::GetAdjNodes (const int NodeId)
{
    DebugLogAssert (m_pTree);

    m_stack.resize (0);
    m_disj_nodes.resize (0);
    m_child_nodes.resize (0);

    int CurrNode = FindTopMostParent (NodeId);

    m_disj_nodes.push_back (CurrNode);
    m_seen.set_bit (CurrNode, true);
    m_stack.push_back (CurrNode);

    const int ConstrTrBr = m_pTree->GetTrBr (CurrNode);

    while (0 < m_stack.size ()) {

        CurrNode = m_stack [m_stack.size () - 1];
        m_stack.pop_back ();

        const int LeftNode = m_pTree->GetLeft (CurrNode);
        const int RightNode = m_pTree->GetRight (CurrNode);

        if (-1 != LeftNode) {

            const int LeftType = m_pTree->GetType (LeftNode);
            const int LeftTrBr = m_pTree->GetTrBr (LeftNode);

            if (FARegexpTree::TYPE_DISJUNCTION == LeftType &&
                (LeftTrBr == ConstrTrBr || -1 == LeftTrBr)) {

                // take this node as a part of the root
                m_disj_nodes.push_back (LeftNode);
                m_seen.set_bit (LeftNode, true);
                m_stack.push_back (LeftNode);

            } else {
                // take this node as a child of the root
                m_child_nodes.push_back (LeftNode);
            }
        }
        if (-1 != RightNode) {

            const int RightType = m_pTree->GetType (RightNode);
            const int RightTrBr = m_pTree->GetTrBr (RightNode);

            if (FARegexpTree::TYPE_DISJUNCTION == RightType &&
                (RightTrBr == ConstrTrBr || -1 == RightTrBr)) {

                // take this node as a part of the root
                m_disj_nodes.push_back (RightNode);
                m_seen.set_bit (RightNode, true);
                m_stack.push_back (RightNode);

            } else {
                // take this node as a child of the root
                m_child_nodes.push_back (RightNode);
            }
        }
    } // of while (0 < m_stack.size ()) ...
}


void FARegexpTreeSort::ModifyTree ()
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (0 < m_disj_nodes.size ());
    DebugLogAssert (m_disj_nodes.size () + 1 == m_child_nodes.size ());

    const int DisjNodeCount = m_disj_nodes.size ();

    for (int i = 0; i < DisjNodeCount - 1; ++i) {

        const int DisjNode = m_disj_nodes [i];

        if (0 != i) {
            const int ParentNode = m_disj_nodes [i - 1];
            m_pTree->SetParent (DisjNode, ParentNode);
            m_pTree->SetTrBr (DisjNode, -1);
            m_pTree->SetTrBrOffset (DisjNode, -1);
        }

        const int NextNode = m_disj_nodes [i + 1];
        m_pTree->SetLeft (DisjNode, NextNode);
        const int ChildNode = m_child_nodes [i];
        m_pTree->SetRight (DisjNode, ChildNode);
        m_pTree->SetParent (ChildNode, DisjNode);
    }

    const int LastDisjNode = m_disj_nodes [DisjNodeCount - 1];

    if (0 != DisjNodeCount - 1) {

        const int ParentNode = m_disj_nodes [DisjNodeCount - 2];
        m_pTree->SetParent (LastDisjNode, ParentNode);
        m_pTree->SetTrBr (LastDisjNode, -1);
        m_pTree->SetTrBrOffset (LastDisjNode, -1);
    }

    const int ChildNode1 = m_child_nodes [DisjNodeCount - 1];
    m_pTree->SetRight (LastDisjNode, ChildNode1);
    m_pTree->SetParent (ChildNode1, LastDisjNode);

    const int ChildNode2 = m_child_nodes [DisjNodeCount];
    m_pTree->SetLeft (LastDisjNode, ChildNode2);
    m_pTree->SetParent (ChildNode2, LastDisjNode);
}


void FARegexpTreeSort::Reorder ()
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (m_node2min.size () == m_node2max.size ());

    const int * pNode2Min = m_node2min.begin ();
    const int * pNode2Max = m_node2max.begin ();
    DebugLogAssert (m_node2max.size () == m_node2min.size ());

    // go in reverse topological order (bottom-up)
    const int * pOrder;
    const int NodeCount = m_sort.GetTopoOrder (&pOrder);
    DebugLogAssert ((unsigned int) NodeCount == m_node2max.size ());

    for (int i = NodeCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int NodeId = pOrder [i];
        const int NodeType = m_pTree->GetType (NodeId);

        if (FARegexpTree::TYPE_DISJUNCTION != NodeType || 
            m_seen.get_bit (NodeId)) {
            continue;
        }

        GetAdjNodes (NodeId);

        int * pBegin = m_child_nodes.begin ();
        int * pEnd = m_child_nodes.end ();
        std::sort (pBegin, pEnd, _TNodeCmp (pNode2Min, pNode2Max, &m_re2hash, m_Reverse));

        ModifyTree ();

    } // of for (int i = 0; ...
}


void FARegexpTreeSort::Process ()
{
    DebugLogAssert (m_pTree);

    Prepare ();
    m_re2hash.Process ();
    TopoSort ();
    CalcMinMax ();
    Reorder ();
}


void FARegexpTreeSort::Prepare ()
{
    DebugLogAssert (m_pTree);

    const int MaxNodeId = m_pTree->GetMaxNodeId ();
    m_node2min.resize (MaxNodeId + 1);
    m_node2max.resize (MaxNodeId + 1);

    for (int i = 0; i <= MaxNodeId; ++i) {

        m_node2min [i] = -1;
        m_node2max [i] = -1;
    }

    m_seen.resize (MaxNodeId + 1);
    m_seen.set_bits (0, MaxNodeId, false);
}


void FARegexpTreeSort::Clear ()
{
    m_node2min.resize (0);
    m_node2max.resize (0);
    m_seen.resize (0);
    m_disj_nodes.resize (0);
    m_child_nodes.resize (0);
}

}
