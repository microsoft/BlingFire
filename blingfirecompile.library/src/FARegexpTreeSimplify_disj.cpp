/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTreeSimplify_disj.h"
#include "FARegexpTree.h"

namespace BlingFire
{


FARegexpTreeSimplify_disj::FARegexpTreeSimplify_disj (FAAllocatorA * pAlloc):
    m_pTree (NULL),
    m_sort (pAlloc),
    m_node2key (pAlloc),
    m_Node1Str (pAlloc),
    m_Node2Str (pAlloc)
{}


void FARegexpTreeSimplify_disj::
    SetRegexp (const char * pRegexp, const int Length)
{
    m_pTmpRegexp = pRegexp;
    m_node2key.SetRegexp (pRegexp, Length);
    m_Node1Str.SetRegexp (pRegexp, Length);
    m_Node2Str.SetRegexp (pRegexp, Length);
}


void FARegexpTreeSimplify_disj::
    SetRegexpTree (FARegexpTree * pTree)
{
    m_pTree = pTree;
    m_node2key.SetRegexpTree (pTree);
    m_Node1Str.SetRegexpTree (pTree);
    m_Node2Str.SetRegexpTree (pTree);
}


void FARegexpTreeSimplify_disj::TopoOrder ()
{
    // do topological sort
    FARegexpTreeTopoGraph tree (m_pTree);
    m_sort.SetGraph (&tree);
    m_sort.Process ();
}


void FARegexpTreeSimplify_disj::Simplify_11 (const int NodeId, 
                                             const int TrBr,
                                             const int TrBrOffset)
{
    DebugLogAssert (m_pTree);

    const int LeftNode = m_pTree->GetLeft (NodeId);
    DebugLogAssert (-1 != LeftNode);
    const int RightNode = m_pTree->GetRight (NodeId);
    DebugLogAssert (-1 != RightNode);

    // copy data and child links
    m_pTree->CopyData (NodeId, LeftNode);

    // setup TrBr
    m_pTree->SetTrBr (NodeId, TrBr);
    m_pTree->SetTrBrOffset (NodeId, TrBrOffset);

    const int LeftLeft = m_pTree->GetLeft (LeftNode);
    const int LeftRight = m_pTree->GetRight (LeftNode);

    // set up child links
    m_pTree->SetLeft (NodeId, LeftLeft);
    if (-1 != LeftLeft)
        m_pTree->SetParent (LeftLeft, NodeId);
    m_pTree->SetRight (NodeId, LeftRight);
    if (-1 != LeftRight)
        m_pTree->SetParent (LeftRight, NodeId);
    // remove right sub-tree
    m_pTree->DeleteTree (RightNode);
    // remove left node
    m_pTree->DeleteNode (LeftNode);

    m_node2key.Update (NodeId);
}


void FARegexpTreeSimplify_disj::Simplify_12 (const int NodeId,
                                             const int TrBr,
                                             const int TrBrOffset)
{
    DebugLogAssert (m_pTree);

    const int LeftNode = m_pTree->GetLeft (NodeId);
    DebugLogAssert (-1 != LeftNode);
    const int RightNode = m_pTree->GetRight (NodeId);
    DebugLogAssert (-1 != RightNode);

    const int LeftLeftNode = m_pTree->GetLeft (LeftNode);
    DebugLogAssert (-1 != LeftLeftNode);
    const int LeftRightNode = m_pTree->GetRight (LeftNode);
    DebugLogAssert (-1 != LeftRightNode);

    // copy child links from Left to Parent
    m_pTree->SetLeft (NodeId, LeftLeftNode);
    if (-1 != LeftLeftNode)
        m_pTree->SetParent (LeftLeftNode, NodeId);
    m_pTree->SetRight (NodeId, LeftRightNode);
    if (-1 != LeftRightNode)
        m_pTree->SetParent (LeftRightNode, NodeId);
    // remove right sub-tree
    m_pTree->DeleteTree (RightNode);
    // remove left node
    m_pTree->DeleteNode (LeftNode);

    // setup TrBr
    m_pTree->SetTrBr (NodeId, TrBr);
    m_pTree->SetTrBrOffset (NodeId, TrBrOffset);

    m_node2key.Update (NodeId);
}


void FARegexpTreeSimplify_disj::UniqDisj ()
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
        const int TrBr = m_pTree->GetTrBr (NodeId);
        const int TrBrOffset = m_pTree->GetTrBrOffset (NodeId);

        if (FARegexpTree::TYPE_DISJUNCTION == NodeType) {

            const int LeftNode = m_pTree->GetLeft (NodeId);
            DebugLogAssert (-1 != LeftNode);
            const int RightNode = m_pTree->GetRight (NodeId);
            DebugLogAssert (-1 != RightNode);

            const int LeftTrBr = m_pTree->GetTrBr (LeftNode);

            if (TrBr != LeftTrBr && -1 != LeftTrBr)
                continue;

            const int LeftKey = m_node2key.GetKey (LeftNode);
            const int RightKey = m_node2key.GetKey (RightNode);

            if (LeftKey == RightKey) {

                const char * pStrLeft = m_Node1Str.Process (LeftNode);
                const char * pStrRight = m_Node2Str.Process (RightNode);

                if (0 == strcmp (pStrLeft, pStrRight))
                    Simplify_11 (NodeId, TrBr, TrBrOffset);


            } else {

                const int LeftType = m_pTree->GetType (LeftNode);

                if (FARegexpTree::TYPE_DISJUNCTION == LeftType) {

                    const int LeftRightNode = m_pTree->GetRight (LeftNode);
                    DebugLogAssert (-1 != LeftRightNode);

                    const int LeftRightKey = m_node2key.GetKey (LeftRightNode);

                    if (RightKey != LeftRightKey)
                        continue;

                    const char * pStrLeftRight = m_Node1Str.Process (LeftRightNode);
                    const char * pStrRight = m_Node2Str.Process (RightNode);

                    if (0 == strcmp (pStrLeftRight, pStrRight))
                        Simplify_12 (NodeId, TrBr, TrBrOffset);
                }
            } // if (LeftKey == RightKey)

        } // of if (FARegexpTree::TYPE_DISJUNCTION == NodeType) ...

    } // of for (int i = NodeCount - 1; ...
}


void FARegexpTreeSimplify_disj::Simplify_21 (
        const int NodeId, 
        const int TrBr,
        const int TrBrOffset,
        const int LeftPref,
        const int RightPref
    )
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (-1 != NodeId && -1 != LeftPref && -1 != RightPref);

    const int Parent = m_pTree->GetParent (NodeId);
    const int ConcLeft = m_pTree->GetLeft (NodeId);
    const int ConcRight = m_pTree->GetRight (NodeId);
    DebugLogAssert (-1 != ConcLeft && -1 != ConcRight);

    const int LeftPrefParent = m_pTree->GetParent (LeftPref);
    const int RightPrefParent = m_pTree->GetParent (RightPref);
    DebugLogAssert (-1 != LeftPrefParent && -1 != RightPrefParent);

    const int LeftPrefSibling = m_pTree->GetRight (LeftPrefParent);
    const int RightPrefSibling = m_pTree->GetRight (RightPrefParent);
    DebugLogAssert (-1 != LeftPrefSibling && -1 != RightPrefSibling);

    // delete right prefix
    m_pTree->DeleteTree (RightPref);

    // move LeftPrefSibling up
    m_pTree->CopyData (LeftPrefParent, LeftPrefSibling);
    const int LeftSibLeft = m_pTree->GetLeft (LeftPrefSibling);
    m_pTree->SetLeft (LeftPrefParent, LeftSibLeft);
    if (-1 != LeftSibLeft)
        m_pTree->SetParent (LeftSibLeft, LeftPrefParent);
    const int LeftSibRight = m_pTree->GetRight (LeftPrefSibling);
    m_pTree->SetRight (LeftPrefParent, LeftSibRight);
    if (-1 != LeftSibRight)
        m_pTree->SetParent (LeftSibRight, LeftPrefParent);

    // delete old LeftPrefSibling
    m_pTree->DeleteNode (LeftPrefSibling);

    // move RightPrefSibling up
    m_pTree->CopyData (RightPrefParent, RightPrefSibling);
    const int RightSibLeft = m_pTree->GetLeft (RightPrefSibling);
    m_pTree->SetLeft (RightPrefParent, RightSibLeft);
    if (-1 != RightSibLeft)
        m_pTree->SetParent (RightSibLeft, RightPrefParent);
    const int RightSibRight = m_pTree->GetRight (RightPrefSibling);
    m_pTree->SetRight (RightPrefParent, RightSibRight);
    if (-1 != RightSibRight)
        m_pTree->SetParent (RightSibRight, RightPrefParent);

    // delete old RightPrefSibling
    m_pTree->DeleteNode (RightPrefSibling);

    // create new concatenation node
    const int NewConc = m_pTree->AddNode (FARegexpTree::TYPE_CONCAT, -1, 0);
    DebugLogAssert (-1 != NewConc);

    m_pTree->SetLeft (NewConc, LeftPref);
    m_pTree->SetParent (LeftPref, NewConc);

    m_pTree->SetRight (NewConc, NodeId);
    m_pTree->SetParent (NodeId, NewConc);

    // setup TrBr
    m_pTree->SetTrBr (NewConc, TrBr);
    m_pTree->SetTrBrOffset (NewConc, TrBrOffset);
    if (ConcLeft != LeftPrefParent) {
        m_pTree->SetTrBr (ConcLeft, -1);
        m_pTree->SetTrBrOffset (ConcLeft, -1);
    }
    if (ConcRight != RightPrefParent) {
        m_pTree->SetTrBr (ConcRight, -1);
        m_pTree->SetTrBrOffset (ConcRight, -1);
    }
    m_pTree->SetTrBr (NodeId, -1);
    m_pTree->SetTrBrOffset (NodeId, -1);

    m_pTree->SetParent (NewConc, Parent);
    if (-1 != Parent) {
        if (NodeId == m_pTree->GetLeft (Parent))
            m_pTree->SetLeft (Parent, NewConc);
        else
            m_pTree->SetRight (Parent, NewConc);
    }
}


void FARegexpTreeSimplify_disj::Simplify_22 (
        const int NodeId, 
        const int DisjTrBr,
        const int DisjTrBrOffset,
        const int ConcTrBr,
        const int ConcTrBrOffset,
        const int LeftPref,
        const int RightPref
    )
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (-1 != NodeId && -1 != LeftPref && -1 != RightPref);

    const int Parent = m_pTree->GetParent (NodeId);

    const int ConcRight = m_pTree->GetRight (NodeId);
    const int LeftNode = m_pTree->GetLeft (NodeId);
    const int ConcLeft = m_pTree->GetRight (LeftNode);
    DebugLogAssert (-1 != ConcLeft && -1 != ConcRight);

    const int LeftPrefParent = m_pTree->GetParent (LeftPref);
    const int RightPrefParent = m_pTree->GetParent (RightPref);
    DebugLogAssert (-1 != LeftPrefParent && -1 != RightPrefParent);

    const int LeftPrefSibling = m_pTree->GetRight (LeftPrefParent);
    const int RightPrefSibling = m_pTree->GetRight (RightPrefParent);
    DebugLogAssert (-1 != LeftPrefSibling && -1 != RightPrefSibling);

    // delete right prefix
    m_pTree->DeleteTree (RightPref);

    // move LeftPrefSibling up
    m_pTree->CopyData (LeftPrefParent, LeftPrefSibling);
    const int LeftSibLeft = m_pTree->GetLeft (LeftPrefSibling);
    m_pTree->SetLeft (LeftPrefParent, LeftSibLeft);
    if (-1 != LeftSibLeft)
        m_pTree->SetParent (LeftSibLeft, LeftPrefParent);
    const int LeftSibRight = m_pTree->GetRight (LeftPrefSibling);
    m_pTree->SetRight (LeftPrefParent, LeftSibRight);
    if (-1 != LeftSibRight)
        m_pTree->SetParent (LeftSibRight, LeftPrefParent);

    // delete old LeftPrefSibling
    m_pTree->DeleteNode (LeftPrefSibling);

    // move RightPrefSibling up
    m_pTree->CopyData (RightPrefParent, RightPrefSibling);
    const int RightSibLeft = m_pTree->GetLeft (RightPrefSibling);
    m_pTree->SetLeft (RightPrefParent, RightSibLeft);
    if (-1 != RightSibLeft)
        m_pTree->SetParent (RightSibLeft, RightPrefParent);
    const int RightSibRight = m_pTree->GetRight (RightPrefSibling);
    m_pTree->SetRight (RightPrefParent, RightSibRight);
    if (-1 != RightSibRight)
        m_pTree->SetParent (RightSibRight, RightPrefParent);

    // delete old RightPrefSibling
    m_pTree->DeleteNode (RightPrefSibling);

    // create new disjunction and concatenation nodes
    const int NewDisj = m_pTree->AddNode (FARegexpTree::TYPE_DISJUNCTION, -1, 0);
    DebugLogAssert (-1 != NewDisj);
    const int NewConc = m_pTree->AddNode (FARegexpTree::TYPE_CONCAT, -1, 0);
    DebugLogAssert (-1 != NewConc);

    const int LeftLeftNode = m_pTree->GetLeft (LeftNode);
    DebugLogAssert (-1 != LeftLeftNode);

    // setup TrBr(s)
    m_pTree->SetTrBr (NewDisj, DisjTrBr);
    m_pTree->SetTrBrOffset (NewDisj, DisjTrBrOffset);
    if (DisjTrBr != ConcTrBr) {
        m_pTree->SetTrBr (NewConc, ConcTrBr);
        m_pTree->SetTrBrOffset (NewConc, ConcTrBrOffset);
    }
    if (ConcLeft != LeftPrefParent) {
        m_pTree->SetTrBr (ConcLeft, -1);
        m_pTree->SetTrBrOffset (ConcLeft, -1);
    }
    if (ConcRight != RightPrefParent) {
        m_pTree->SetTrBr (ConcRight, -1);
        m_pTree->SetTrBrOffset (ConcRight, -1);
    }
    m_pTree->SetTrBr (NodeId, -1);
    m_pTree->SetTrBrOffset (NodeId, -1);

    // adjust their links
    m_pTree->SetLeft (NewDisj, LeftLeftNode);
    m_pTree->SetParent (LeftLeftNode, NewDisj);
    m_pTree->SetRight (NewDisj, NewConc);
    m_pTree->SetParent (NewConc, NewDisj);

    m_pTree->SetLeft (NewConc, LeftPref);
    m_pTree->SetParent (LeftPref, NewConc);
    m_pTree->SetRight (NewConc, NodeId);
    m_pTree->SetParent (NodeId, NewConc);

    m_pTree->SetLeft (NodeId, ConcLeft);
    m_pTree->SetParent (ConcLeft, NodeId);

    m_pTree->SetParent (NewDisj, Parent);
    if (-1 != Parent) {
        if (NodeId == m_pTree->GetLeft (Parent))
            m_pTree->SetLeft (Parent, NewDisj);
        else
            m_pTree->SetRight (Parent, NewDisj);
    }

    // delete left disjunction node
    m_pTree->DeleteNode (LeftNode);
}


const bool FARegexpTreeSimplify_disj::FindCommonPref (
        const int LeftConc,
        const int RightConc,
        int * pLeftPref, 
        int * pRightPref
    )
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (pLeftPref && pRightPref);
    DebugLogAssert (LeftConc != RightConc);

    const int LeftTrBr = m_pTree->GetTrBr (LeftConc);
    const int RightTrBr = m_pTree->GetTrBr (RightConc);

    if (LeftTrBr != RightTrBr)
        return false;

    *pLeftPref = LeftConc;
    DebugLogAssert (-1 != *pLeftPref);
    // go down left as far as possible
    int LeftChild = m_pTree->GetLeft (*pLeftPref);
    while (-1 != LeftChild) {
        *pLeftPref = LeftChild;
        if (FARegexpTree::TYPE_CONCAT != m_pTree->GetType (LeftChild))
            break;
        const int TrBr = m_pTree->GetTrBr (LeftChild);
        if (-1 != TrBr && LeftTrBr != TrBr)
            break;
        LeftChild = m_pTree->GetLeft (LeftChild);
    }

    *pRightPref = RightConc;
    DebugLogAssert (-1 != *pRightPref);
    // go down left as far as possible
    LeftChild = m_pTree->GetLeft (*pRightPref);
    while (-1 != LeftChild) {
        *pRightPref = LeftChild;
        if (FARegexpTree::TYPE_CONCAT != m_pTree->GetType (LeftChild))
            break;
        const int TrBr = m_pTree->GetTrBr (LeftChild);
        if (-1 != TrBr && RightTrBr != TrBr)
            break;
        LeftChild = m_pTree->GetLeft (LeftChild);
    }

    // check whether there is any prefix
    int LeftKey = m_node2key.GetKey (*pLeftPref);
    int RightKey = m_node2key.GetKey (*pRightPref);

    if (LeftKey != RightKey)
        return false;

    const char * pStrLeft = m_Node1Str.Process (*pLeftPref);
    const char * pStrRight = m_Node2Str.Process (*pRightPref);

    if (0 != strcmp (pStrLeft, pStrRight))
        return false;

    // find maximum prefix
    while (true) {

        const int LeftPrefParent = m_pTree->GetParent (*pLeftPref);
        DebugLogAssert (-1 != LeftPrefParent);
        const int RightPrefParent = m_pTree->GetParent (*pRightPref);
        DebugLogAssert (-1 != RightPrefParent);

        if (LeftConc == LeftPrefParent || RightConc == RightPrefParent)
            break;

        LeftKey = m_node2key.GetKey (LeftPrefParent);
        RightKey = m_node2key.GetKey (RightPrefParent);

        if (LeftKey != RightKey)
            break;

        pStrLeft = m_Node1Str.Process (LeftPrefParent);
        pStrRight = m_Node2Str.Process (RightPrefParent);

        if (0 != strcmp (pStrLeft, pStrRight))
            break;

        *pLeftPref = LeftPrefParent;
        *pRightPref = RightPrefParent;
    }

    return true;
}


void FARegexpTreeSimplify_disj::LeftFact ()
{
    DebugLogAssert (m_pTree);

    int LeftPref, RightPref;

    // go in reverse topological order (bottom-up)
    const int * pOrder;
    const int NodeCount = m_sort.GetTopoOrder (&pOrder);

    for (int i = NodeCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int NodeId = pOrder [i];

        if (m_pTree->IsDeleted (NodeId))
            continue;

        const int NodeType = m_pTree->GetType (NodeId);
        const int TrBr = m_pTree->GetTrBr (NodeId);
        const int TrBrOffset = m_pTree->GetTrBrOffset (NodeId);

        if (FARegexpTree::TYPE_DISJUNCTION == NodeType) {

            const int LeftNode = m_pTree->GetLeft (NodeId);
            DebugLogAssert (-1 != LeftNode);
            const int RightNode = m_pTree->GetRight (NodeId);
            DebugLogAssert (-1 != RightNode);

            const int LeftType = m_pTree->GetType (LeftNode);
            const int LeftTrBr = m_pTree->GetTrBr (LeftNode);
            const int RightType = m_pTree->GetType (RightNode);
            const int RightTrBr = m_pTree->GetTrBr (RightNode);

            if (FARegexpTree::TYPE_CONCAT == LeftType &&
                FARegexpTree::TYPE_CONCAT == RightType) {

                if (FindCommonPref (LeftNode, RightNode, &LeftPref, &RightPref)) {

                    if (-1 == TrBr && LeftTrBr == RightTrBr) {

                        const int LeftTrBrOffset = m_pTree->GetTrBrOffset (LeftNode);
                        Simplify_21 (NodeId, LeftTrBr, LeftTrBrOffset, LeftPref, RightPref);

                    } else if (-1 == LeftTrBr && -1 == RightTrBr) {

                        Simplify_21 (NodeId, TrBr, TrBrOffset, LeftPref, RightPref);
                    }
                }

            } else if (FARegexpTree::TYPE_DISJUNCTION == LeftType &&
                       FARegexpTree::TYPE_CONCAT == RightType &&
                       (-1 == LeftTrBr || TrBr == LeftTrBr)) {

                const int LeftRight = m_pTree->GetRight (LeftNode);
                DebugLogAssert (-1 != LeftRight);

                const int LeftRightType = m_pTree->GetType (LeftRight);

                if (FARegexpTree::TYPE_CONCAT == LeftRightType) {

                    if (FindCommonPref (LeftRight, RightNode, &LeftPref, &RightPref)) {

                        // this is guaranteed by FindCommonPref
                        DebugLogAssert (m_pTree->GetTrBr (LeftRight) == RightTrBr);

                        const int RightTrBrOffset = m_pTree->GetTrBrOffset (RightNode);

                        Simplify_22 (
                            NodeId, 
                            TrBr, TrBrOffset, 
                            RightTrBr, RightTrBrOffset, 
                            LeftPref, RightPref
                        );
                    }
                }
            }
        } // of if (FARegexpTree::TYPE_DISJUNCTION ...
    } // of for (int i = NodeCount - 1; ...
}


void FARegexpTreeSimplify_disj::Process ()
{
    m_node2key.Process ();

    TopoOrder ();
    UniqDisj ();
    // as some of the nodes do not exists (TODO: test without this)
    TopoOrder ();
    LeftFact ();

}

}
