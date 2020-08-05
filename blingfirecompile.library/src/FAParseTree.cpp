/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAParseTree.h"

namespace BlingFire
{


FAParseTree::FAParseTree (FAAllocatorA * pAlloc)
{
    m_i2n.SetAllocator (pAlloc);
    m_i2n.Create ();

    m_p2i.SetAllocator (pAlloc);
    m_p2i.Create ();

    m_p2l.SetAllocator (pAlloc);
    m_p2l.Create ();
}


FAParseTree::~FAParseTree ()
{}


void FAParseTree::Init (const int Count)
{
    DebugLogAssert (0 <= Count);

    m_i2n.resize (Count);
    m_p2i.resize (Count);
    m_p2l.resize (Count);

    if (0 < Count) {

        const int Count_1 = Count - 1;

        // i here has sense of both position and node idx
        for (int i = 0; i < Count_1; ++i) {

            // get node data
            FANodeData & NodeData = m_i2n [i];

            // setup node data
            NodeData.m_Label = i;
            NodeData.m_Next = i + 1;
            NodeData.m_Child = -1;

            // setup pos -> node idx map
            m_p2i [i] = i;
            // setup pos -> label
            m_p2l [i] = i;
        }

        // get node data
        FANodeData & NodeData = m_i2n [Count_1];

        // setup node data
        NodeData.m_Label = Count_1;
        NodeData.m_Next = -1;
        NodeData.m_Child = -1;

        // setup pos -> node idx map
        m_p2i [Count_1] = Count_1;
        // setup pos -> label
        m_p2l [Count_1] = Count_1;

    } // of if (0 < Count) ...
}


void FAParseTree::AddNode (const int Label, const int FromPos, const int ToPos)
{
    // allocate a new node
    const int NewNode = m_i2n.size ();
    m_i2n.resize (NewNode + 1);

    // get from-node and to-node
    const int ToNode = m_p2i [ToPos];
    const int FromNode = m_p2i [FromPos];

    // this is needed only if inclusion of consituents is allowed at single
    // stage of parsing (higher level constituents should be added first)
    m_p2i [FromPos] = NewNode;

    // get nodes' data
    FANodeData & NewNodeData = m_i2n [NewNode];
    FANodeData & FromNodeData = m_i2n [FromNode];
    FANodeData & ToNodeData = m_i2n [ToNode];

    // copy the content of a FromNode into a NewNode
    NewNodeData = FromNodeData;

    // setup a new label for an old FromNode
    FromNodeData.m_Label = Label;
    // setup a new next-node link for an old FromNode
    FromNodeData.m_Next = ToNodeData.m_Next;
    // setup a new child-node link for an old FromNode
    FromNodeData.m_Child = NewNode;

    // setup -1 as a Next node for ToNode
    if (FromPos != ToPos)
        ToNodeData.m_Next = -1;
    else
        NewNodeData.m_Next = -1;
}


void FAParseTree::Update ()
{
    DebugLogAssert (0 < m_i2n.size ());

    m_p2i.resize (0);
    m_p2l.resize (0);

    int NodeIdx = 0;

    while (-1 != NodeIdx) {

        const FANodeData & NodeData = m_i2n [NodeIdx];

        m_p2i.push_back (NodeIdx);
        m_p2l.push_back (NodeData.m_Label);

        NodeIdx = NodeData.m_Next;
    }
}


const int FAParseTree::GetUpperNodes (const int ** ppNodes) const
{
    DebugLogAssert (ppNodes);
    DebugLogAssert (m_p2i.size () == m_p2l.size ());

    *ppNodes = m_p2i.begin ();
    const int Count = m_p2i.size ();

    return Count;
}


const int FAParseTree::GetUpperLabels (const int ** ppLabels) const
{
    DebugLogAssert (ppLabels);
    DebugLogAssert (m_p2i.size () == m_p2l.size ());

    *ppLabels = m_p2l.begin ();
    const int Count = m_p2l.size ();

    return Count;
}


const int FAParseTree::GetNext (const int Node) const
{
    const FANodeData & NodeData = m_i2n [Node];
    return NodeData.m_Next;
}


const int FAParseTree::GetChild (const int Node) const
{
    const FANodeData & NodeData = m_i2n [Node];
    return NodeData.m_Child;
}


const int FAParseTree::GetLabel (const int Node) const
{
    const FANodeData & NodeData = m_i2n [Node];
    return NodeData.m_Label;
}

}

