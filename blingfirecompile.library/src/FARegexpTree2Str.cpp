/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpTree2Str.h"
#include "FARegexpTree.h"
#include "FAUtils.h"

namespace BlingFire
{


FARegexpTree2Str::FARegexpTree2Str (FAAllocatorA * pAlloc) :
    m_pRegexp (NULL),
    m_Length (-1),
    m_pTree (NULL)
{
    m_RegexpText.SetAllocator (pAlloc);
    m_RegexpText.Create ();
}


void FARegexpTree2Str::SetRegexp (const char * pRegexp, const int Length)
{
    m_pRegexp = pRegexp;
    m_Length = Length;
}


void FARegexpTree2Str::SetRegexpTree (const FARegexpTree * pTree)
{
    m_pTree = pTree;
}


void FARegexpTree2Str::PutNode (const int NodeId)
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (m_pRegexp && 0 < m_Length);
    DebugLogAssert (0 <= NodeId && m_pTree->GetMaxNodeId () >= NodeId);

    const int NodeType = m_pTree->GetType (NodeId);

    if (FARegexpTree::TYPE_SYMBOL == NodeType) {

        const int Offset = m_pTree->GetOffset (NodeId);
        const int Length = m_pTree->GetLength (NodeId);
        DebugLogAssert (0 <= Offset && Offset < m_Length);
        DebugLogAssert (0 <= Length && Offset + Length <= m_Length);

        if (FAIsEscaped (Offset, m_pRegexp, m_Length)) {
            m_RegexpText.push_back ('\\');
        }

        for (int i = 0; i < Length; ++i) {
            const char Symbol = m_pRegexp [Offset + i];
            m_RegexpText.push_back (Symbol);
        }

    } else if (FARegexpTree::TYPE_CONCAT == NodeType) {
        ;
    } else if (FARegexpTree::TYPE_DISJUNCTION == NodeType) {

        m_RegexpText.push_back ('|');

    } else if (FARegexpTree::TYPE_ANY == NodeType) {

        m_RegexpText.push_back ('.');

    } else if (FARegexpTree::TYPE_ITERATION == NodeType) {

        m_RegexpText.push_back ('*');

    } else if (FARegexpTree::TYPE_NON_EMPTY_ITERATION == NodeType) {

        m_RegexpText.push_back ('+');

    } else if (FARegexpTree::TYPE_OPTIONAL == NodeType) {

        m_RegexpText.push_back ('?');

    } else if (FARegexpTree::TYPE_L_ANCHOR == NodeType) {

        m_RegexpText.push_back ('^');

    } else if (FARegexpTree::TYPE_R_ANCHOR == NodeType) {

        m_RegexpText.push_back ('$');

    } else {

        DebugLogAssert (0);
    }
}


const bool FARegexpTree2Str::NeedBrackets (const int NodeId) const
{
    DebugLogAssert (m_pTree);

    bool NeedBrs = true;
    const int TrBr = m_pTree->GetTrBr (NodeId);

    if (-1 == TrBr) {

        const int ParentId = m_pTree->GetParent (NodeId);

        if (-1 != ParentId) {

            const int NodeType = m_pTree->GetType (NodeId);
            const int ParentType = m_pTree->GetType (ParentId);

            if (NodeType == ParentType)
                NeedBrs = false;
        }
    }

    return NeedBrs;
}


void FARegexpTree2Str::PutLeftBracket (const int NodeId)
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (m_pRegexp);

    const int TrBr = m_pTree->GetTrBr (NodeId);

    if (-1 != TrBr) {

        m_RegexpText.push_back ('<');

        int Offset = 1 + m_pTree->GetTrBrOffset (NodeId);
        DebugLogAssert (0 < Offset && Offset < m_Length);

        while (Offset < m_Length && 
               isdigit ((unsigned char) m_pRegexp [Offset])) {

            m_RegexpText.push_back (m_pRegexp [Offset]);
            Offset++;
        }

        m_RegexpText.push_back (' ');

    } else {

        m_RegexpText.push_back ('(');
    }
}


void FARegexpTree2Str::PutRightBracket (const int NodeId)
{
    DebugLogAssert (m_pTree);
    const int TrBr = m_pTree->GetTrBr (NodeId);

    if (-1 != TrBr)
        m_RegexpText.push_back ('>');
    else
        m_RegexpText.push_back (')');
}



void FARegexpTree2Str::Process_rec (const int NodeId)
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (0 <= NodeId && m_pTree->GetMaxNodeId () >= NodeId);

    const bool NeedBrs = NeedBrackets (NodeId);

    if (NeedBrs) {
        PutLeftBracket (NodeId);
    }

    const int LeftNode = m_pTree->GetLeft (NodeId);

    if (-1 != LeftNode) {
        Process_rec (LeftNode);
    }

    PutNode (NodeId);

    const int RightNode = m_pTree->GetRight (NodeId);

    if (-1 != RightNode) {
        Process_rec (RightNode);
    }

    if (NeedBrs) {
        PutRightBracket (NodeId);
    }
}


const char * FARegexpTree2Str::Process (const int NodeId)
{
    DebugLogAssert (m_pTree);
    DebugLogAssert (m_pRegexp && 0 < m_Length);
    DebugLogAssert (0 <= NodeId && m_pTree->GetMaxNodeId () >= NodeId);

    m_RegexpText.resize (0);

    Process_rec (NodeId);

    m_RegexpText.push_back (0);

    return m_RegexpText.begin ();
}

}
