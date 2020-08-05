/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpSimplify.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FARegexpSimplify::FARegexpSimplify (FAAllocatorA * pAlloc) :
    m_LeftBr (FARegexpTree::TYPE_LBR, -1, 0),
    m_RightBr (FARegexpTree::TYPE_RBR, -1, 0),
    m_TerminalSymbol (FARegexpTree::TYPE_SYMBOL, -1, 0),
    m_pRegexp (NULL),
    m_Length (-1),
    m_tree (pAlloc),
    m_pRe2Tokens (&m_re2tokens_digit),
    m_regexp_sort (pAlloc),
    m_simpl (pAlloc),
    m_tree2re (pAlloc),
    m_pSimpleRe (NULL)
{
    m_tokens.SetAllocator (pAlloc);
    m_tokens.Create ();
    m_tokens.push_back (m_LeftBr);

    m_re2tokens_digit.SetTokens (&m_tokens);
    m_re2tokens_char.SetTokens (&m_tokens);
    m_re2tokens_wre.SetTokens (&m_tokens);

    m_tokens2tree.SetTree (&m_tree);
    m_regexp_sort.SetRegexpTree (&m_tree);
    m_simpl.SetRegexpTree (&m_tree);
    m_tree2re.SetRegexpTree (&m_tree);
}


void FARegexpSimplify::SetRegexp (const char * pRegexp, const int Length)
{
    m_pRegexp = pRegexp;
    m_Length = Length;

    m_re2tokens_digit.SetRegexp (pRegexp, Length);
    m_re2tokens_char.SetRegexp (pRegexp, Length);
    m_re2tokens_wre.SetRegexp (pRegexp, Length);

    m_tokens2tree.SetRegexp (pRegexp, Length);
    m_simpl.SetRegexp (pRegexp, Length);
    m_regexp_sort.SetRegexp (pRegexp, Length);
    m_tree2re.SetRegexp (pRegexp, Length);
}


void FARegexpSimplify::SetLabelType (const int LabelType)
{
    m_pRe2Tokens = &m_re2tokens_digit;

    if (FAFsmConst::LABEL_CHAR == LabelType) {

        m_pRe2Tokens = &m_re2tokens_char;

    } else if (FAFsmConst::LABEL_WRE == LabelType) {

        m_pRe2Tokens = &m_re2tokens_wre;
    }
}


void FARegexpSimplify::SetUseUtf8 (const bool UseUtf8)
{
    m_re2tokens_char.SetUseUtf8 (UseUtf8);
}


void FARegexpSimplify::SetReverse (const bool Reverse)
{
    m_regexp_sort.SetReverse (Reverse);
}


void FARegexpSimplify::Clear ()
{
    m_tokens.resize (1);
    m_tree.Clear ();
}


void FARegexpSimplify::PutError ()
{
    DebugLogAssert (true == m_tokens2tree.GetStatus ());

    const int TokenIdx = m_tokens2tree.GetTokenIdx ();

    int ErrorOffset = -1;
    if ((unsigned int) TokenIdx < m_tokens.size ())
        ErrorOffset = m_tokens [TokenIdx].GetOffset ();

    FASyntaxError (m_pRegexp, m_Length, ErrorOffset, "Syntax error in regular expression");
}


void FARegexpSimplify::ReverseTree ()
{
    const int RootId = m_tree.GetRoot ();
    DebugLogAssert (-1 != RootId);

    const int MaxNodeId = m_tree.GetMaxNodeId ();

    for (int i = 0; i <= MaxNodeId; ++i) {

        if (m_tree.IsDeleted (i))
            continue;

        const int LeftNodeId = m_tree.GetLeft (i);
        const int RightNodeId = m_tree.GetRight (i);

        if (-1 != LeftNodeId && -1 != RightNodeId && RootId != i) {

            m_tree.SetLeft (i, RightNodeId);
            m_tree.SetRight (i, LeftNodeId);
        }
    }
}


void FARegexpSimplify::Process ()
{
    DebugLogAssert (m_pRe2Tokens);

    Clear ();

    // make tokenization
    m_pRe2Tokens->Process ();

    m_tokens.push_back (m_RightBr);
    m_tokens.push_back (m_TerminalSymbol);

    // build the tree
    m_tokens2tree.SetTokens (m_tokens.begin (), m_tokens.size ());
    m_tokens2tree.Process ();
    
    // check for syntax errors
    if (true == m_tokens2tree.GetStatus ())
        PutError ();

    // simplify disjunctions
    m_simpl.Process ();
    m_simpl.Process ();
    m_simpl.Process ();

    // sort disjunctions
    m_regexp_sort.Process ();

    // simplify again
    m_simpl.Process ();
    m_simpl.Process ();
    m_simpl.Process ();

    // reverse the tree
    ReverseTree ();
    // sort disjunctions
    m_regexp_sort.Process ();

    // simplify again
    m_simpl.Process ();
    m_simpl.Process ();
    m_simpl.Process ();

    // reverse the tree back
    ReverseTree ();
    // sort disjunctions
    m_regexp_sort.Process ();

    // build output string
    const int RootId = m_tree.GetRoot ();
    DebugLogAssert (-1 != RootId);
    const int LeftNodeId = m_tree.GetLeft (RootId);
    DebugLogAssert (-1 != LeftNodeId);

    m_pSimpleRe = m_tree2re.Process (LeftNodeId);
}


const char * FARegexpSimplify::GetRegexp () const
{
    DebugLogAssert (m_pSimpleRe);
    return m_pSimpleRe;
}


const FARegexpTree * FARegexpSimplify::GetRegexpTree () const
{
    return & m_tree;
}

}

