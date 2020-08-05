/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTree2Funcs.h"
#include "FAAllocatorA.h"
#include "FARegexpTree.h"
#include "FAMapA.h"
#include "FAMultiMapA.h"

#include <algorithm>

namespace BlingFire
{


FARegexpTree2Funcs::FARegexpTree2Funcs (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc)
{
    m_Nullable.SetAllocator (pAlloc);
    m_Nullable.Create ();

    m_FirstPos.SetAllocator (pAlloc);
    m_FirstPos.Create ();

    m_LastPos.SetAllocator (pAlloc);
    m_LastPos.Create ();

    m_FollowPos.SetAllocator (pAlloc);
    m_FollowPos.Create ();

    m_Node2Pos.SetAllocator (pAlloc);
    m_Node2Pos.Create ();

    m_Pos2Node.SetAllocator (pAlloc);
    m_Pos2Node.Create ();

    m_sorted_nodes.SetAllocator (pAlloc);
    m_sorted_nodes.Create ();

    m_tmp_array.SetAllocator (pAlloc);
    m_tmp_array.Create ();
}


FARegexpTree2Funcs::~FARegexpTree2Funcs ()
{
    m_sorted_nodes.Clear ();
    m_tmp_array.Clear ();
    m_Node2Pos.Clear ();
    m_Pos2Node.Clear ();
    m_Nullable.Clear ();

    int i;
    int iMax = m_FirstPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_FirstPos [i];

        /// delete set, if any
        if (NULL != pSet) {
            pSet->Clear ();
            FAFree (m_pAlloc, pSet);
        }
    }
    m_FirstPos.Clear ();

    iMax = m_LastPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_LastPos [i];

        /// delete set, if any
        if (NULL != pSet) {
            pSet->Clear ();
            FAFree (m_pAlloc, pSet);
        }
    }
    m_LastPos.Clear ();

    iMax = m_FollowPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_FollowPos [i];

        /// delete set, if any
        if (NULL != pSet) {
            pSet->Clear ();
            FAFree (m_pAlloc, pSet);
        }
    }
    m_FollowPos.Clear ();
}


void FARegexpTree2Funcs::Clear ()
{
    m_sorted_nodes.resize (0);
    m_Node2Pos.resize (0);
    m_Pos2Node.resize (0);
    m_tmp_array.resize (0);
    m_Nullable.resize (0);

    int i;
    int iMax = m_FirstPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_FirstPos [i];

        /// clear set, if any
        if (NULL != pSet)
            pSet->resize (0);
    }

    iMax = m_LastPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_LastPos [i];

        /// clear set, if any
        if (NULL != pSet)
            pSet->resize (0);
    }

    iMax = m_FollowPos.size ();
    for (i = 0; i < iMax; ++i) {

        /// get Set pointer
        _TSet * pSet = m_FollowPos [i];

        /// clear set, if any
        if (NULL != pSet)
            pSet->resize (0);
    }
}


void FARegexpTree2Funcs::Init (const int NodeCount)
{
    m_Nullable.resize (NodeCount);
    m_Node2Pos.resize (NodeCount);
    m_Pos2Node.resize (NodeCount);
}


const int FARegexpTree2Funcs::GetMaxPos () const
{
    return m_max_pos - 1;
}


const int FARegexpTree2Funcs::GetNodeId (const int Pos) const
{
    return m_Pos2Node [Pos];
}


const bool FARegexpTree2Funcs::GetNullable (const int NodeId) const
{
    return m_Nullable [NodeId];
}


const int FARegexpTree2Funcs::GetFirstPos (const int NodeId,
                                           const int ** ppSet) const
{
    DebugLogAssert (ppSet);

    _TSet * pSet = m_FirstPos [NodeId];
    DebugLogAssert (pSet);

    *ppSet = pSet->begin ();
    const int Size = pSet->size ();

    return Size;
}


const int FARegexpTree2Funcs::GetLastPos (const int NodeId, 
                                          const int ** ppSet) const
{
    DebugLogAssert (ppSet);

    _TSet * pSet = m_LastPos [NodeId];
    DebugLogAssert (pSet);

    *ppSet = pSet->begin ();
    const int Size = pSet->size ();

    return Size;
}


const int FARegexpTree2Funcs::GetFollowPos (const int Pos, 
                                            const int ** ppSet) const
{
    DebugLogAssert (ppSet);

    _TSet * pSet = m_FollowPos [Pos];

    if (NULL != pSet) {

        *ppSet = pSet->begin ();
        const int Size = pSet->size ();

        return Size;
    }

    *ppSet = NULL;
    return 0;
}


void FARegexpTree2Funcs::SetSet (_TNode2Set * pArray,
                                 const int Idx, 
                                 const _TSet * pSet)
{
    _TSet * pDstSet = GetSet (pArray, Idx);

    const int Size = pSet->size ();
    pDstSet->resize (Size);
    memcpy (pDstSet->begin (), pSet->begin (), Size * sizeof (int));
}


FARegexpTree2Funcs::_TSet *
FARegexpTree2Funcs::GetSet (_TNode2Set * pArray, const int Idx)
{
    DebugLogAssert (pArray);

    const int Size = pArray->size ();

    // check whether we have to add Sets pointers between Size and Idx
    if (Idx >= Size) {

        pArray->resize (Idx + 1);

        for (int i = Size; i <= Idx; ++i) {
            (*pArray) [i] = NULL;
        }
    }

    // get the pointer
    _TSet * pSet = (*pArray) [Idx];

    // see whether the set has not been allocated yet
    if (NULL == pSet) {

        /// allocate memory for the set
        pSet = (_TSet *) FAAlloc (m_pAlloc, sizeof (_TSet));
        /// set up allocator and create an empty set
        pSet->SetAllocator (m_pAlloc);
        pSet->Create ();
        // initialize the pointer
        (*pArray) [Idx] = pSet;
    }

    return pSet;
}


void FARegexpTree2Funcs::SetRegexpTree (const FARegexpTree * pRegexpTree)
{
    m_pRegexpTree = pRegexpTree;
}


void FARegexpTree2Funcs::SortNodes ()
{
    int NodeId = m_pRegexpTree->GetRoot ();

    if (-1 != NodeId)
        m_tmp_array.push_back (NodeId);

    while (!m_tmp_array.empty ()) {

        // get node
        NodeId = m_tmp_array [m_tmp_array.size () - 1];
        m_tmp_array.pop_back ();

        // put it into sorted array
        m_sorted_nodes.push_back (NodeId);

        // traverse children
        const int LeftId = m_pRegexpTree->GetLeft (NodeId);
        if (-1 != LeftId)
            m_tmp_array.push_back (LeftId);

        const int RightId = m_pRegexpTree->GetRight (NodeId);
        if (-1 != RightId)
            m_tmp_array.push_back (RightId);
    }
}


void FARegexpTree2Funcs::CalcNode2Pos ()
{
    DebugLogAssert (m_pRegexpTree);

    m_max_pos = 0;

    const int MaxNodeId = m_pRegexpTree->GetMaxNodeId ();

    for (int i = 0; i <= MaxNodeId; ++i) {

        const int LeftId = m_pRegexpTree->GetLeft (i);
        const int RightId = m_pRegexpTree->GetRight (i);

        if (-1 == LeftId && -1 == RightId) {

            m_Node2Pos [i] = m_max_pos;
            m_Pos2Node [m_max_pos] = i;
            m_max_pos++;

        } else {

            m_Node2Pos [i] = -1;
        }
    }
}


void FARegexpTree2Funcs::Process ()
{
    DebugLogAssert (m_pRegexpTree);

    // delete previously stored data
    Clear ();

    // get tree node count
    const int NodeCount = m_pRegexpTree->GetMaxNodeId () + 1;

    // prepare: m_Node2Pos and m_Nullable arrays
    Init (NodeCount);

    // build node -> pos map
    CalcNode2Pos ();

    // sort nodes in reverse topological order
    SortNodes ();

    int i;
    const int LastNode = NodeCount - 1;

    for (i = LastNode; 0 <= i; --i) {

        const int NodeId = m_sorted_nodes [i];
        DebugLogAssert (-1 != NodeId);

        CalcNullable (NodeId);
        CalcFirst (NodeId);
        CalcLast (NodeId);
    }

    for (i = LastNode; 0 <= i; --i) {

        const int NodeId = m_sorted_nodes [i];
        DebugLogAssert (-1 != NodeId);

        CalcFollow (NodeId);
    }
}


void FARegexpTree2Funcs::CalcNullable (const int NodeId)
{
    const int NodeType = m_pRegexpTree->GetType (NodeId);
    const int LeftId = m_pRegexpTree->GetLeft (NodeId);
    const int RightId = m_pRegexpTree->GetRight (NodeId);

    switch (NodeType) {

        case FARegexpTree::TYPE_OPTIONAL: // newly added
        case FARegexpTree::TYPE_ITERATION:
        case FARegexpTree::TYPE_EPSILON:
            {
                m_Nullable [NodeId] = true;
                break;
            }

        case FARegexpTree::TYPE_NON_EMPTY_ITERATION:
            {
                DebugLogAssert (-1 != LeftId);
                m_Nullable [NodeId] = m_Nullable [LeftId];
                break;
            }

        case FARegexpTree::TYPE_L_ANCHOR:
        case FARegexpTree::TYPE_R_ANCHOR:
        case FARegexpTree::TYPE_SYMBOL:
        case FARegexpTree::TYPE_ANY:
            {
                m_Nullable [NodeId] = false;
                break;
            }

        case FARegexpTree::TYPE_CONCAT:
            {
                DebugLogAssert (-1 != LeftId);
                DebugLogAssert (-1 != RightId);
                m_Nullable [NodeId] = m_Nullable [LeftId] && m_Nullable [RightId];
                break;
            }

        case FARegexpTree::TYPE_DISJUNCTION:
            {
                DebugLogAssert (-1 != LeftId);
                DebugLogAssert (-1 != RightId);
                m_Nullable [NodeId] = m_Nullable [LeftId] || m_Nullable [RightId];
                break;
            }

        default:
            /// unknown node type
            DebugLogAssert (false);
    };
}


void FARegexpTree2Funcs::MergeSets (_TSet * pDst,
                                    const _TSet * pSrc1,
                                    const _TSet * pSrc2)
{
    DebugLogAssert (pSrc1);
    DebugLogAssert (pSrc2);
    DebugLogAssert (pDst);

    const int * const pSrc1Ptr = pSrc1->begin ();
    DebugLogAssert (pSrc1Ptr);
    const int Size1 = pSrc1->size ();

    const int * const pSrc2Ptr = pSrc2->begin ();
    DebugLogAssert (pSrc2Ptr);
    const int Size2 = pSrc2->size ();

    int i;
    pDst->resize (0);

    // check whether we have work to do
    if (0 != Size1 && 0 != Size2) {

        for (i = 0; i<Size1; ++i) {
            pDst->push_back (pSrc1Ptr [i]);
        }
        for (i = 0; i<Size2; ++i) {
            pDst->push_back (pSrc2Ptr [i]);
        }

        int * pBegin = pDst->begin ();
        int * pEnd = pDst->end ();

        std::sort (pBegin, pEnd);
        const int NewSize = int (std::unique (pBegin, pEnd) - pBegin);
        pDst->resize (NewSize);

    } else {

        // trivial case

        if (0 != Size1) {
            for (i = 0; i<Size1; ++i) {
                pDst->push_back (pSrc1Ptr [i]);
            }
        }
        if (0 != Size2) {
            for (i = 0; i<Size2; ++i) {
                pDst->push_back (pSrc2Ptr [i]);
            }
        }
    }
}


void FARegexpTree2Funcs::AppendSet (_TSet * pDst, const _TSet * pSet)
{
    const int Size = pSet->size ();

    if (0 < Size) {

        const int OldDstSize = pDst->size ();

        // enlarge destination set
        pDst->resize (OldDstSize + Size);

        // copy new elements
        memcpy (pDst->begin () + OldDstSize, pSet->begin (), Size * sizeof (int));

        // sort_uniq
        int * pBegin = pDst->begin ();
        int * pEnd = pDst->end ();
        std::sort (pBegin, pEnd);
        const int NewSize = int (std::unique (pBegin, pEnd) - pBegin);
        pDst->resize (NewSize);
    }
}


void FARegexpTree2Funcs::CalcFirst (const int NodeId)
{
    const int NodeType = m_pRegexpTree->GetType (NodeId);
    const int LeftId = m_pRegexpTree->GetLeft (NodeId);
    const int RightId = m_pRegexpTree->GetRight (NodeId);

    switch (NodeType) {

        case FARegexpTree::TYPE_EPSILON:
            {
                GetSet (&m_FirstPos, NodeId);
                break;
            }

        case FARegexpTree::TYPE_OPTIONAL: // newly added
        case FARegexpTree::TYPE_NON_EMPTY_ITERATION:
        case FARegexpTree::TYPE_ITERATION:
            {
                DebugLogAssert (-1 != LeftId);
                const _TSet * pSet = GetSet (&m_FirstPos, LeftId);
                SetSet (&m_FirstPos, NodeId, pSet);
                break;
            }

        case FARegexpTree::TYPE_L_ANCHOR:
        case FARegexpTree::TYPE_R_ANCHOR:
        case FARegexpTree::TYPE_SYMBOL:
        case FARegexpTree::TYPE_ANY:
            {
                _TSet * pSet = GetSet (&m_FirstPos, NodeId);
                const int Pos = m_Node2Pos [NodeId];
                pSet->push_back (Pos);
                break;
            }

        case FARegexpTree::TYPE_CONCAT:
            {
                const _TSet * pSet1 = GetSet (&m_FirstPos, LeftId);

                if (true == m_Nullable [LeftId]) {

                    const _TSet * pSet2 = GetSet (&m_FirstPos, RightId);
                    _TSet * pDstSet = GetSet (&m_FirstPos, NodeId);
                    MergeSets (pDstSet, pSet1, pSet2);

                } else {

                    SetSet (&m_FirstPos, NodeId, pSet1);
                }
                break;
            }

        case FARegexpTree::TYPE_DISJUNCTION:
            {
                const _TSet * pSet1 = GetSet (&m_FirstPos, LeftId);
                const _TSet * pSet2 = GetSet (&m_FirstPos, RightId);
                _TSet * pDstSet = GetSet (&m_FirstPos, NodeId);
                MergeSets (pDstSet, pSet1, pSet2);
                break;
            }

        default:
            /// unknown node type
            DebugLogAssert (false);
    };
}


void FARegexpTree2Funcs::CalcLast (const int NodeId)
{
    const int NodeType = m_pRegexpTree->GetType (NodeId);
    const int LeftId = m_pRegexpTree->GetLeft (NodeId);
    const int RightId = m_pRegexpTree->GetRight (NodeId);

    switch (NodeType) {

        case FARegexpTree::TYPE_EPSILON:
            {
                GetSet (&m_LastPos, NodeId);
                break;
            }

        case FARegexpTree::TYPE_OPTIONAL: // newly added
        case FARegexpTree::TYPE_NON_EMPTY_ITERATION:
        case FARegexpTree::TYPE_ITERATION:
            {
                DebugLogAssert (-1 != LeftId);
                const _TSet * pSet = GetSet (&m_LastPos, LeftId);
                SetSet (&m_LastPos, NodeId, pSet);
                break;
            }

        case FARegexpTree::TYPE_L_ANCHOR:
        case FARegexpTree::TYPE_R_ANCHOR:
        case FARegexpTree::TYPE_SYMBOL:
        case FARegexpTree::TYPE_ANY:
            {
                _TSet * pSet = GetSet (&m_LastPos, NodeId);
                const int Pos = m_Node2Pos [NodeId];
                pSet->push_back (Pos);
                break;
            }

        case FARegexpTree::TYPE_CONCAT:
            {
                const _TSet * pSet2 = GetSet (&m_LastPos, RightId);

                if (true == m_Nullable [RightId]) {

                    const _TSet * pSet1 = GetSet (&m_LastPos, LeftId);
                    _TSet * pDstSet = GetSet (&m_LastPos, NodeId);
                    MergeSets (pDstSet, pSet1, pSet2);

                } else {

                    SetSet (&m_LastPos, NodeId, pSet2);
                }
                break;
            }

        case FARegexpTree::TYPE_DISJUNCTION:
            {
                const _TSet * pSet1 = GetSet (&m_LastPos, LeftId);
                const _TSet * pSet2 = GetSet (&m_LastPos, RightId);
                _TSet * pDstSet = GetSet (&m_LastPos, NodeId);
                MergeSets (pDstSet, pSet1, pSet2);
                break;
            }

        default:
            /// unknown node type
            DebugLogAssert (false);
    };
}


void FARegexpTree2Funcs::CalcFollow (const int NodeId)
{
    const int NodeType = m_pRegexpTree->GetType (NodeId);
    const int LeftId = m_pRegexpTree->GetLeft (NodeId);
    const int RightId = m_pRegexpTree->GetRight (NodeId);

    switch (NodeType) {

        case FARegexpTree::TYPE_CONCAT:
            {
                const _TSet * pRightFirstSet = GetSet (&m_FirstPos, RightId);
                DebugLogAssert (pRightFirstSet);

                const _TSet * pSet = GetSet (&m_LastPos, LeftId);
                DebugLogAssert (pSet);

                const int Size = pSet->size ();
                const int * pSetPtr = pSet->begin ();
                DebugLogAssert (pSetPtr);

                for (int i = 0; i < Size; ++i) {

                    const int LastPos = pSetPtr [i];
                    // get follow pos set
                    _TSet * pFollowSet = GetSet (&m_FollowPos, LastPos);
                    // append right first set to it
                    AppendSet (pFollowSet, pRightFirstSet);
                }
                break;
            }

        case FARegexpTree::TYPE_ITERATION:
        case FARegexpTree::TYPE_NON_EMPTY_ITERATION:
            {
                const _TSet * pLeftFirstSet = GetSet (&m_FirstPos, LeftId);
                DebugLogAssert (pLeftFirstSet);

                const _TSet * pLeftLastSet = GetSet (&m_LastPos, LeftId);
                DebugLogAssert (pLeftFirstSet);

                const int Size = pLeftLastSet->size ();
                const int * pSetPtr = pLeftLastSet->begin ();
                DebugLogAssert (pSetPtr);

                for (int i = 0; i < Size; ++i) {

                    const int LastPos = pSetPtr [i];

                    // get follow pos set
                    _TSet * pFollowSet = GetSet (&m_FollowPos, LastPos);
                    // append left first set to it
                    AppendSet (pFollowSet, pLeftFirstSet);
                }
                break;
            }
    };
}

}

