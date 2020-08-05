/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTree.h"
#include "FAAllocatorA.h"

namespace BlingFire
{


FARegexpTree::FARegexpTree (FAAllocatorA* pAlloc) :
    m_root (-1)
{
    m_data.SetAllocator (pAlloc);
    m_data.Create (100 * node_size);
    m_deleted.Create (pAlloc);

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();
}


int * FARegexpTree::GetNodeData (const int NodeId)
{
    DebugLogAssert (0 <= NodeId);
    DebugLogAssert ((unsigned int)(NodeId * node_size) < m_data.size ());

    int * pData = m_data.begin () + (NodeId * node_size);
    return pData;
}


const int * FARegexpTree::GetNodeData (const int NodeId) const
{
    const int * pData = m_data.begin () + (NodeId * node_size);
    return pData;
}


const int FARegexpTree::GetRoot () const
{
    return m_root;
}


void FARegexpTree::SetRoot (const int NodeId)
{
    m_root = NodeId;
}


void FARegexpTree::SetParent (const int NodeId, const int ParentNodeId)
{
    int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    pNode [node_parent] = ParentNodeId;
}


void FARegexpTree::SetLeft (const int NodeId, const int LeftNodeId)
{
    int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    pNode [node_left] = LeftNodeId;
}


void FARegexpTree::SetRight (const int NodeId, const int RightNodeId)
{
    int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    pNode [node_right] = RightNodeId;
}


const int FARegexpTree::GetTrBr (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_trbr];
}


void FARegexpTree::SetTrBr (const int NodeId, const int TrBr)
{
    int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    pNode [node_trbr] = TrBr;
}


const int FARegexpTree::GetTrBrOffset (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_trbr_offset];
}


void FARegexpTree::SetTrBrOffset (const int NodeId, const int Offset)
{
    int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    pNode [node_trbr_offset] = Offset;
}


const int FARegexpTree::GetType (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_type];
}


const int FARegexpTree::GetParent (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_parent];
}


const int FARegexpTree::GetLeft (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_left];
}

const int FARegexpTree::GetRight (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_right];
}


const int FARegexpTree::GetOffset (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_offset];
}


const int FARegexpTree::GetLength (const int NodeId) const
{
    const int * pNode = GetNodeData (NodeId);
    DebugLogAssert (pNode);
    return pNode [node_length];
}


const bool FARegexpTree::IsDeleted (const int NodeId) const
{
    DebugLogAssert (0 <= NodeId);

    if ((unsigned int)(NodeId * node_size) >= m_data.size ()) {

        return true;

    } else {

        const int NodeType = GetType (NodeId);
        return -1 == NodeType;
    }
}


void FARegexpTree::DeleteNode (const int NodeId)
{
    if (IsDeleted (NodeId))
        return;

    m_deleted.push (NodeId);

    int * pData = GetNodeData (NodeId);
    DebugLogAssert (pData);

    for (int i = 0; i < node_size; ++i) {
        pData [i] = -1;
    }
}


void FARegexpTree::DeleteTree (const int NodeId)
{
    m_stack.resize (0);
    m_stack.push_back (NodeId);

    while (0 < m_stack.size ()) {

        const int CurrNode = m_stack [m_stack.size () - 1];
        m_stack.pop_back ();

        const int LeftNode = GetLeft (CurrNode);
        if (-1 != LeftNode)
            m_stack.push_back (LeftNode);

        const int RightNode = GetRight (CurrNode);
        if (-1 != RightNode)
            m_stack.push_back (RightNode);

        DeleteNode (CurrNode);
    }
}


void FARegexpTree::CopyData (const int ToNodeId, const int FromNodeId)
{
    const int * pFromData = GetNodeData (FromNodeId);
    DebugLogAssert (pFromData);

    int * pToData = GetNodeData (ToNodeId);
    DebugLogAssert (pToData);

    pToData [node_type] = pFromData [node_type];
    pToData [node_offset] = pFromData [node_offset];
    pToData [node_length] = pFromData [node_length];
    pToData [node_trbr] = pFromData [node_trbr];
    pToData [node_trbr_offset] = pFromData [node_trbr_offset];
}


const int FARegexpTree::AddNode (const int NodeType, 
                                 const int Offset,
                                 const int Length)
{
    const int Count = m_data.size ();

    int NodeId;
    if (m_deleted.empty ()) {

        NodeId = Count/node_size;
        m_data.resize (Count + node_size);

        // get the pointer to the node memory
        int * pNode = GetNodeData (NodeId);
        DebugLogAssert (pNode);

        // make full initialization
        pNode [node_type] = NodeType;
        pNode [node_offset] = Offset;
        pNode [node_length] = Length;
        pNode [node_parent] = -1;
        pNode [node_left] = -1;
        pNode [node_right] = -1;
        pNode [node_trbr] = -1;
        pNode [node_trbr_offset] = -1;

    } else {

        NodeId = *m_deleted.top ();
        m_deleted.pop ();

        // get the pointer to the node memory
        int * pNode = GetNodeData (NodeId);
        DebugLogAssert (pNode);

        // make necessary initialization
        pNode [node_type] = NodeType;
        pNode [node_offset] = Offset;
        pNode [node_length] = Length;
    }

    return NodeId;
}


const int FARegexpTree::GetMaxNodeId () const
{
    const int Count = m_data.size ();

    if (0 != Count) {
        return (Count - 1)/node_size;
    }

    return -1;
}


void FARegexpTree::Clear ()
{
    m_data.Clear ();
    m_data.Create (100 * node_size);
    m_deleted.clear ();
    m_root = -1;
    m_stack.resize (0);
}

}

