/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexp2Nfa.h"
#include "FAFsmConst.h"
#include "FANfaCreator_base.h"

namespace BlingFire
{


FARegexp2Nfa::FARegexp2Nfa (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_TrBr2Iw (false),
    m_tree (pAlloc),
    m_left_br (FARegexpTree::TYPE_LBR, -1, 0),
    m_right_br (FARegexpTree::TYPE_RBR, -1, 0),
    m_terminal_symbol (FARegexpTree::TYPE_SYMBOL, -1, 0),
    m_pRe2Tokens (&m_re2tokens_digit),
    m_tree2funcs (pAlloc),
    m_pos_eqs (pAlloc),
    m_pPos2Class (NULL),
    m_MaxPos (-1),
    m_trbr2iw (pAlloc)
{
    m_tokens.SetAllocator (pAlloc);
    m_tokens.Create (1000);
    m_tokens.push_back (m_left_br);

    m_re2tokens_digit.SetTokens (&m_tokens);
    m_re2tokens_char.SetTokens (&m_tokens);
    m_re2tokens_wre.SetTokens (&m_tokens);

    m_tokens2tree.SetTree (&m_tree);
    m_tree2funcs.SetRegexpTree (&m_tree);

    m_pos_eqs.SetRegexpTree (&m_tree);
    m_pPos2Class = m_pos_eqs.GetPos2Class ();

    m_trbr2iw.SetTokens (&m_tokens);
}


void FARegexp2Nfa::SetRegexp (const char * pRegexp, const int Length)
{
    m_re2tokens_digit.SetRegexp (pRegexp, Length);
    m_re2tokens_char.SetRegexp (pRegexp, Length);
    m_re2tokens_wre.SetRegexp (pRegexp, Length);

    m_tokens2tree.SetRegexp (pRegexp, Length);
    m_trbr2iw.SetRegexp (pRegexp, Length);
}


void FARegexp2Nfa::SetLabelType (const int LabelType)
{
    m_pRe2Tokens = &m_re2tokens_digit;

    if (FAFsmConst::LABEL_CHAR == LabelType) {

        m_pRe2Tokens = &m_re2tokens_char;

    } else if (FAFsmConst::LABEL_WRE == LabelType) {

        m_pRe2Tokens = &m_re2tokens_wre;
    }
}


void FARegexp2Nfa::SetUseUtf8 (const bool UseUtf8)
{
    m_re2tokens_char.SetUseUtf8 (UseUtf8);
}


void FARegexp2Nfa::SetNfa (FANfaCreator_base * pNfa)
{
    m_pNfa = pNfa;
}


void FARegexp2Nfa::SetKeepPos (const bool KeepPos)
{
    if (!KeepPos) {

        m_pPos2Class = m_pos_eqs.GetPos2Class ();
        DebugLogAssert (m_pPos2Class);

    } else {

        m_pPos2Class = NULL;
    }
}


void FARegexp2Nfa::SetTrBr2Iw (const bool TrBr2Iw)
{
    m_TrBr2Iw = TrBr2Iw;
}


const FARegexpTree * FARegexp2Nfa::GetRegexpTree () const
{
    return & m_tree;
}


const FARegexpTree2Funcs * FARegexp2Nfa::GetRegexpFuncs () const
{
    return & m_tree2funcs;
}


const FAMapA * FARegexp2Nfa::GetPos2Class () const
{
    return m_pPos2Class;
}


void FARegexp2Nfa::Process ()
{
    DebugLogAssert (m_pNfa);
    DebugLogAssert (m_pRe2Tokens);

    // delete previously stored data
    Clear ();

    // make tokenization
    m_pRe2Tokens->Process ();
    // add two more special tokens
    m_tokens.push_back (m_right_br);
    m_tokens.push_back (m_terminal_symbol);

    // see whether it is needed to convert trbr into symbols
    if (m_TrBr2Iw) {
        m_trbr2iw.Process ();
    }

    // make parsing
    m_tokens2tree.SetTokens (m_tokens.begin (), m_tokens.size ());
    m_tokens2tree.Process ();

    // see the parsing status
    if (true == m_tokens2tree.GetStatus ()) {

        // put syntactic error
        PutError ();

    } else {

        // calc functions
        m_tree2funcs.Process ();

        m_MaxPos = m_tree2funcs.GetMaxPos ();

        // calc position equivalence classes, if needed
        if (m_pPos2Class) {
            m_pos_eqs.Process ();
        }

        // put automaton
        PutInitials ();
        PutFinals ();
        PutTransitions ();
    }
}


void FARegexp2Nfa::PutError ()
{
    DebugLogAssert (m_pNfa);
    DebugLogAssert (true == m_tokens2tree.GetStatus ());

    const int TokenIdx = m_tokens2tree.GetTokenIdx ();

    int ErrorOffset = -1;

    if ((unsigned int) TokenIdx < m_tokens.size ()) {

        ErrorOffset = m_tokens [TokenIdx].GetOffset ();
    }

    m_pNfa->SetError (ErrorOffset);
}


const int FARegexp2Nfa::Pos2Class (const int Pos) const
{
    if (NULL == m_pPos2Class) {

        return Pos;

    } else {

        const int * pClass = m_pPos2Class->Get (Pos);
        DebugLogAssert (pClass);
        return *pClass;
    }
}


void FARegexp2Nfa::PutInitials ()
{
    const int RootId = m_tree.GetRoot ();

    // tree may be empty
    if (-1 != RootId) {

        const int * pSet;
        const int Size = m_tree2funcs.GetFirstPos (RootId, &pSet);

        for (int i = 0; i < Size; ++i) {

            const int Pos = pSet [i];
            m_pNfa->SetInitial (Pos2Class (Pos));
        }
    }
}


void FARegexp2Nfa::PutFinals ()
{
    const int RootId = m_tree.GetRoot ();

    // tree may be empty
    if (-1 != RootId) {

        const int * pSet;
        const int Size = m_tree2funcs.GetLastPos (RootId, &pSet);

        for (int i = 0; i < Size; ++i) {

            const int Pos = pSet [i];
            m_pNfa->SetFinal (Pos2Class (Pos));
        }
    }
}


void FARegexp2Nfa::PutTransitions ()
{
    // make iteration thru all positions
    for (int from_pos = 0; from_pos < m_MaxPos; ++from_pos) {

        const int * pFollowSet;
        const int Size = m_tree2funcs.GetFollowPos (from_pos, &pFollowSet);

        // make iteration thru the corresponding following positions
        for (int to_pos_idx = 0; to_pos_idx < Size; ++to_pos_idx) {

            DebugLogAssert (pFollowSet);

            const int ToPos = pFollowSet[to_pos_idx];

            // get NodeId
            const int NodeId = m_tree2funcs.GetNodeId (from_pos);

            // get the type, offset and length
            const int Type =  m_tree.GetType (NodeId);
            const int Offset = m_tree.GetOffset (NodeId);
            const int Length = m_tree.GetLength (NodeId);

            switch (Type) {

                case FARegexpTree::TYPE_SYMBOL:
                    {
                        // NfaCreator should know how to treat transition labels
                        m_pNfa->SetTransition (
                                Pos2Class (from_pos), 
                                Pos2Class (ToPos), 
                                Offset, 
                                Length
                            );
                        break;
                    }

                case FARegexpTree::TYPE_ANY:
                case FARegexpTree::TYPE_L_ANCHOR:
                case FARegexpTree::TYPE_R_ANCHOR:
                    {
                        // NfaCreator should know how to treat transition labels
                        m_pNfa->SetSpecTransition (
                                Pos2Class (from_pos),
                                Pos2Class (ToPos), 
                                Type
                            );
                        break;
                    }

                default:
                    {
                        // wrong node type
                        DebugLogAssert (0);
                    }
            };
        }
    }
}


void FARegexp2Nfa::Clear ()
{
    m_tokens.resize (1);
    m_tree.Clear ();
}

}
