/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTree2Hash.h"
#include "FAAllocatorA.h"
#include "FARegexpTree.h"

namespace BlingFire
{


FARegexpTree2Hash::FARegexpTree2Hash (FAAllocatorA * pAlloc) :
    m_pRegexp (NULL),
    m_Length (-1),
    m_pTree (NULL),
    m_sort (pAlloc)
{
    m_node2key.SetAllocator (pAlloc);
    m_node2key.Create ();
}


void FARegexpTree2Hash::SetRegexp (const char * pRegexp, const int Length)
{
    m_pRegexp = pRegexp;
    m_Length = Length;
}


void FARegexpTree2Hash::SetRegexpTree (FARegexpTree * pTree)
{
    m_pTree = pTree;
}


const int FARegexpTree2Hash::GetKey (const int NodeId) const
{
    DebugLogAssert (0 <= NodeId && (unsigned int) NodeId < m_node2key.size ());
    return m_node2key [NodeId];
}


const int FARegexpTree2Hash::Symbol2Key (const int Offset, 
                                         const int Length) const
{
    DebugLogAssert (m_pRegexp);

    // specially added symbol
    if (-1 == Offset)
        return 0;

    DebugLogAssert (0 <= Offset && Offset < m_Length);
    DebugLogAssert (0 <= Length && Offset + Length <= m_Length);

    unsigned long Key = 0;

    for (int i = 0; i < Length; ++i) {

        const char Symbol = m_pRegexp [Offset + i];
        Key = Key * 33 + Symbol;
    }
    return Key;
}


const int FARegexpTree2Hash::Node2Key (const int NodeId) const
{
    DebugLogAssert (m_pTree);

    const int NodeType = m_pTree->GetType (NodeId);
    const int LeftNode = m_pTree->GetLeft (NodeId);
    const int RightNode = m_pTree->GetRight (NodeId);
    const int TrBr = m_pTree->GetTrBr (NodeId);

    unsigned long Key = NodeType;

    if (FARegexpTree::TYPE_SYMBOL == NodeType) {

        const int Offset = m_pTree->GetOffset (NodeId);
        const int Length = m_pTree->GetLength (NodeId);

        Key = Symbol2Key (Offset, Length);
    }
    if (-1 != LeftNode) {

        Key = m_node2key [LeftNode] * 33 + Key;
    }
    if (-1 != RightNode) {

        Key = m_node2key [RightNode] * 33 + Key;
    }

    Key = 0x00FFFFFF & Key;

    if (-1 != TrBr) {

        const int TrBrKey = (
              33*(TrBr & 0x000000FF) + 
              33*((TrBr & 0x0000FF00) >> 8) +
              33*((TrBr & 0x00FF0000) >> 16) +
              33*((TrBr & 0xFF000000) >> 24)) & 0xFF;

        Key |= (TrBrKey << 24);
    }

    return Key;
}


void FARegexpTree2Hash::Update (const int NodeId)
{
    DebugLogAssert (m_pTree);

    int CurrNode = NodeId;

    while (-1 != CurrNode) {

        const int Key = Node2Key (CurrNode);
        m_node2key [CurrNode] = Key;
        CurrNode = m_pTree->GetParent (CurrNode);
    }
}


void FARegexpTree2Hash::Process ()
{
    DebugLogAssert (m_pTree);

    m_node2key.resize (1 + m_pTree->GetMaxNodeId ());
    memset ((void*) m_node2key.begin (), 0, sizeof (int) * m_node2key.size ());

    // do topological sort
    FARegexpTreeTopoGraph tree (m_pTree);
    m_sort.SetGraph (&tree);
    m_sort.Process ();

    // go in reverse topological order (bottom-up)
    const int * pOrder;
    const int NodeCount = m_sort.GetTopoOrder (&pOrder);

    for (int i = NodeCount - 1; i >= 0; --i) {

        DebugLogAssert (pOrder);
        const int NodeId = pOrder [i];
        const int Key = Node2Key (NodeId);
        m_node2key [NodeId] = Key;
    }
}

}


