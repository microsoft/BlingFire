/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaCreator_base.h"
#include "FARegexpTree.h"
#include "FAAllocatorA.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAException.h"

#include <string>

namespace BlingFire
{


FANfaCreator_base::FANfaCreator_base (FAAllocatorA * pAlloc):
    m_pRegexp (NULL),
    m_ReLength (-1),
    m_tmp_nfa (pAlloc)
{
    m_type2spec.SetAllocator (pAlloc);
    m_type2spec.Create ();
    m_type2spec.resize (FARegexpTree::TYPE_OFFSET + FARegexpTree::TYPE_COUNT + 1);

    m_type2spec [FARegexpTree::TYPE_ANY] = FAFsmConst::IW_ANY;
    m_type2spec [FARegexpTree::TYPE_L_ANCHOR] = FAFsmConst::IW_L_ANCHOR;
    m_type2spec [FARegexpTree::TYPE_R_ANCHOR] = FAFsmConst::IW_R_ANCHOR;

    m_initials.SetAllocator (pAlloc);
    m_initials.Create ();

    m_finals.SetAllocator (pAlloc);
    m_finals.Create ();
}


FANfaCreator_base::~FANfaCreator_base ()
{}


const FARSNfaA * FANfaCreator_base::GetNfa () const
{
    return &m_tmp_nfa;
}


void FANfaCreator_base::SetRegexp (const char * pRegexp, const int ReLength)
{
    m_pRegexp = pRegexp;
    m_ReLength = ReLength;
}


void FANfaCreator_base::SetAnyIw (const int AnyIw)
{
    m_type2spec [FARegexpTree::TYPE_ANY] = AnyIw;
}


void FANfaCreator_base::SetAnchorLIw (const int AnchorLIw)
{
    m_type2spec [FARegexpTree::TYPE_L_ANCHOR] = AnchorLIw;
}


void FANfaCreator_base::SetAnchorRIw (const int AnchorRIw)
{
    m_type2spec [FARegexpTree::TYPE_R_ANCHOR] = AnchorRIw;
}


void FANfaCreator_base::SetTrBr2Iw (const bool TrBr2Iw)
{
    m_TrBr2Iw = TrBr2Iw;
}


void FANfaCreator_base::SetTrBrBaseIw (const int TrBrBaseIw)
{
    m_TrBrBaseIw = TrBrBaseIw;
}


void FANfaCreator_base::Clear ()
{
    m_initials.resize (0);
    m_finals.resize (0);

    m_tmp_nfa.Clear ();
    m_tmp_nfa.SetMaxState (0);
    m_tmp_nfa.SetMaxIw (0);
    m_tmp_nfa.Create ();
}


void FANfaCreator_base::SetSpecTransition (const int FromState,
                                           const int ToState,
                                           const int Type)
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (FARegexpTree::TYPE_ANY == Type ||
            FARegexpTree::TYPE_L_ANCHOR == Type ||
            FARegexpTree::TYPE_R_ANCHOR == Type);

    const int Iw = m_type2spec [Type];

    m_tmp_nfa.SetTransition (FromState, Iw, ToState);
}


void FANfaCreator_base::SetTransition (
            const int FromState,
            const int ToState,
            const int LabelOffset,
            const int LabelLength
        )
{
    if (m_TrBr2Iw && (FAFsmConst::TRBR_LEFT == LabelLength || \
                      FAFsmConst::TRBR_RIGHT == LabelLength)) {

        const int TrBr = LabelOffset;
        DebugLogAssert (0 <= TrBr && 0x7FFFFFFF >= TrBr);

        int Iw;

        if (-1 == LabelLength) {
            Iw = m_TrBrBaseIw + (TrBr << 1);
        } else {
            Iw = m_TrBrBaseIw + ((TrBr << 1) + 1);
        }

        m_tmp_nfa.SetTransition (FromState, Iw, ToState);

    } else if (0 > LabelOffset || 0 > LabelLength) {

        FASyntaxError (m_pRegexp, m_ReLength, -1, \
            "Unexpected token with negative offset or length");
        throw FAException (FAMsg::IOError, __FILE__, __LINE__);
    }
}


void FANfaCreator_base::SetInitial (const int State)
{
    const int InitialCount = m_initials.size ();

    if (0 < InitialCount) {
        if (State == m_initials [InitialCount - 1])
            return;
    }

    m_initials.push_back (State);
}


void FANfaCreator_base::SetFinal (const int State)
{
    const int FinalCount = m_finals.size ();

    if (0 < FinalCount) {

        if (State == m_finals [FinalCount - 1])
            return;
    }

    m_finals.push_back (State);
}


void FANfaCreator_base::Prepare ()
{
    int * pBegin = m_initials.begin ();
    int Size = m_initials.size ();

    if (false == FAIsSortUniqed (pBegin, Size)) {

        Size = FASortUniq (pBegin, pBegin + Size);
        m_initials.resize (Size);
    }

    pBegin = m_finals.begin ();
    Size = m_finals.size ();

    if (false == FAIsSortUniqed (pBegin, Size)) {

        Size = FASortUniq (pBegin, pBegin + Size);
        m_finals.resize (Size);
    }

    m_tmp_nfa.SetInitials (m_initials.begin (), m_initials.size ());
    m_tmp_nfa.SetFinals (m_finals.begin (), m_finals.size ());

    m_tmp_nfa.Prepare ();
}


void FANfaCreator_base::SetError (const int ErrorOffset)
{
    FASyntaxError (m_pRegexp, m_ReLength, ErrorOffset, "unexpected symbol");
    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
}

}
