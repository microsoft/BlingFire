/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAParsedRegexp2TrBrMaps.h"
#include "FAAllocatorA.h"
#include "FARegexpTree.h"
#include "FARegexpTree2Funcs.h"
#include "FAMultiMapA.h"
#include "FAMapA.h"
#include "FAUtils.h"

#include <algorithm>

namespace BlingFire
{


FAParsedRegexp2TrBrMaps::TTrBrOffsetCmp::TTrBrOffsetCmp (
        const FARegexpTree * pRegexpTree
    ) :
    m_pRegexpTree (pRegexpTree)
{}


const bool FAParsedRegexp2TrBrMaps::TTrBrOffsetCmp::operator() (
        const int NodeId1, 
        const int NodeId2
    ) const
{
    DebugLogAssert (m_pRegexpTree);

    const int Offset1 = m_pRegexpTree->GetTrBrOffset (NodeId1);
    DebugLogAssert (-1 != Offset1);

    const int Offset2 = m_pRegexpTree->GetTrBrOffset (NodeId2);
    DebugLogAssert (-1 != Offset2);

    return Offset1 < Offset2;
}


FAParsedRegexp2TrBrMaps::FAParsedRegexp2TrBrMaps (FAAllocatorA * pAlloc) :
    m_pRegexpTree (NULL),
    m_pRegexpFuncs (NULL),
    m_pPos2Class (NULL),
    m_pStartTrBr (NULL),
    m_pEndTrBr (NULL),
    m_sets (pAlloc),
    m_MaxPos (-1)
{
    m_pos_arr.SetAllocator (pAlloc);
    m_pos_arr.Create ();

    m_node_arr.SetAllocator (pAlloc);
    m_node_arr.Create ();
}


void FAParsedRegexp2TrBrMaps::SetRegexpTree (const FARegexpTree * pRegexpTree)
{
    m_pRegexpTree = pRegexpTree;
}


void FAParsedRegexp2TrBrMaps::
    SetRegexpFuncs (const FARegexpTree2Funcs * pRegexpFuncs)
{
    m_pRegexpFuncs = pRegexpFuncs;
}


void FAParsedRegexp2TrBrMaps::SetPos2Class (const FAMapA * pPos2Class)
{
    m_pPos2Class = pPos2Class;
}


void FAParsedRegexp2TrBrMaps::SetStartMap (FAMultiMapA * pStartTrBr)
{
    m_pStartTrBr = pStartTrBr;
}


void FAParsedRegexp2TrBrMaps::SetEndMap (FAMultiMapA * pEndTrBr)
{
    m_pEndTrBr = pEndTrBr;
}


void FAParsedRegexp2TrBrMaps::Process ()
{
    DebugLogAssert (m_pRegexpTree && m_pRegexpFuncs);

    m_MaxPos = m_pRegexpFuncs->GetMaxPos ();

    Prepare ();
    BuildStartMap ();
    BuildEndMap ();
}


const int FAParsedRegexp2TrBrMaps::Pos2Class (const int Pos) const
{
    DebugLogAssert (0 <= Pos && 0 <= m_MaxPos);

    if (NULL == m_pPos2Class) {

        return Pos;

    } else {

        const int * pPosClass = m_pPos2Class->Get (Pos);
        DebugLogAssert (pPosClass);

        return *pPosClass;
    }
}


void FAParsedRegexp2TrBrMaps::Prepare ()
{
    DebugLogAssert (m_pRegexpFuncs);

    const int PosCount = m_pRegexpFuncs->GetMaxPos () + 1;
    // +1 for extra set container
    m_sets.SetResCount (PosCount + 1);
}


void FAParsedRegexp2TrBrMaps::BuildStartMap ()
{
    DebugLogAssert (m_pRegexpTree && m_pRegexpFuncs);
    DebugLogAssert (m_pStartTrBr);

    int i;

    m_pos_arr.resize (0);

    const int MaxNodeId = m_pRegexpTree->GetMaxNodeId ();

    for (i = 0; i <= MaxNodeId; ++i) {

        const int TrBr = m_pRegexpTree->GetTrBr (i);

        if (-1 != TrBr) {

            DebugLogAssert (-1 != m_pRegexpTree->GetTrBrOffset (i));

            const int * pSet;
            const int Size = m_pRegexpFuncs->GetFirstPos (i, &pSet);
            DebugLogAssert (0 < Size && pSet);
            DebugLogAssert (FAIsSortUniqed (pSet, Size));

            for (int j = 0; j < Size; ++j) {

                const int StartPos = Pos2Class (pSet [j]);

                m_sets.SelfUnion (&i, 1, StartPos);
                m_pos_arr.push_back (StartPos);
            }
        }
    } // of for ...

    const int NewSize = FASortUniq (m_pos_arr.begin (), m_pos_arr.end ());

    for (i = 0; i < NewSize; ++i) {

        const int Pos = m_pos_arr [i];

        const int * pNodeIds;
        const int NodeIdsCount = m_sets.GetRes (&pNodeIds, Pos);
        DebugLogAssert (0 < NodeIdsCount && pNodeIds);

        m_node_arr.resize (NodeIdsCount);

        int * pBegin = m_node_arr.begin ();
        DebugLogAssert (pBegin);

        memcpy (pBegin, pNodeIds, NodeIdsCount * sizeof (int));

        // make TrBr nodes to be sorted by offset
        std::sort (pBegin, m_node_arr.end (), TTrBrOffsetCmp (m_pRegexpTree));

        for (int j = 0; j < NodeIdsCount; ++j) {

            const int NodeId = pBegin [j];

            const int TrBr = m_pRegexpTree->GetTrBr (NodeId);
            DebugLogAssert (-1 != TrBr);

            m_pStartTrBr->Add (Pos, TrBr);
        }

        m_sets.SetRes (NULL, 0, Pos);

    } // of for ...
}


void FAParsedRegexp2TrBrMaps::BuildEndMap ()
{
    DebugLogAssert (m_pRegexpTree && m_pRegexpFuncs);
    DebugLogAssert (m_pEndTrBr);

    int i, j, SetSize;
    const int * pSet;

    const int SpecSetId = 1 + m_pRegexpFuncs->GetMaxPos ();
    const int MaxNodeId = m_pRegexpTree->GetMaxNodeId ();

    m_pos_arr.resize (0);

    for (i = 0; i <= MaxNodeId; ++i) {

        const int TrBr = m_pRegexpTree->GetTrBr (i);

        if (-1 != TrBr) {

            // get last function values
            SetSize = m_pRegexpFuncs->GetLastPos (i, &pSet);
            DebugLogAssert (0 < SetSize && pSet);
            DebugLogAssert (FAIsSortUniqed (pSet, SetSize));

            for (j = 0; j < SetSize; ++j) {

                const int LastPos = Pos2Class (pSet [j]);

                const int * pFollowSet;
                const int FollowSize = 
                    m_pRegexpFuncs->GetFollowPos (LastPos, &pFollowSet);
                DebugLogAssert (0 < FollowSize && pFollowSet);
                DebugLogAssert (FAIsSortUniqed (pFollowSet, FollowSize));

                m_sets.SelfUnion (pFollowSet, FollowSize, SpecSetId);
            }

            SetSize = m_pRegexpFuncs->GetFirstPos (i, &pSet);
            DebugLogAssert (0 < SetSize && pSet);
            DebugLogAssert (FAIsSortUniqed (pSet, SetSize));
            int NegSetMin = pSet [0];

            SetSize = m_pRegexpFuncs->GetLastPos (i, &pSet);
            DebugLogAssert (0 < SetSize && pSet);
            DebugLogAssert (FAIsSortUniqed (pSet, SetSize));
            int NegSetMax = pSet [SetSize - 1];

            SetSize = m_sets.GetRes (&pSet, SpecSetId);
            DebugLogAssert (0 < SetSize && pSet);
            DebugLogAssert (FAIsSortUniqed (pSet, SetSize));

            for (j = 0; j < SetSize; ++j) {

                const int EndPos = Pos2Class (pSet [j]);

                if (NegSetMin > EndPos || NegSetMax < EndPos) {

                    m_sets.SelfUnion (&TrBr, 1, EndPos);
                    m_pos_arr.push_back (EndPos);
                }
            }

            m_sets.SetRes (NULL, 0, SpecSetId);

            const int Parent = m_pRegexpTree->GetParent (i);
            DebugLogAssert (-1 != Parent);
            const int ParentType = m_pRegexpTree->GetType (Parent);

            if (FARegexpTree::TYPE_ITERATION == ParentType ||
                FARegexpTree::TYPE_NON_EMPTY_ITERATION == ParentType) {

                SetSize = m_pRegexpFuncs->GetFirstPos (Parent, &pSet);
                DebugLogAssert (0 < SetSize && pSet);

                for (j = 0; j < SetSize; ++j) {

                    const int FirstPos = Pos2Class (pSet [j]);

                    m_sets.SelfUnion (&TrBr, 1, FirstPos);
                    m_pos_arr.push_back (FirstPos);
                }
            } // if (ParentType ...

        } // of if (-1 != TrBr) ...
    } // of for ...

    const int NewSize = FASortUniq (m_pos_arr.begin (), m_pos_arr.end ());

    for (i = 0; i < NewSize; ++i) {

        const int Pos = m_pos_arr [i];

        SetSize = m_sets.GetRes (&pSet, Pos);
        m_pEndTrBr->Set (Pos, pSet, SetSize);
        m_sets.SetRes (NULL, 0, Pos);
    }
}

}
