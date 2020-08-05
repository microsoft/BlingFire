/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWREToken.h"
#include "FAAllocatorA.h"

namespace BlingFire
{


FAWREToken::FAWREToken (FAAllocatorA * pAlloc) :
    m_disj_words (false),
    m_disj_regexps (false),
    m_disj_dicts (false),
    m_disj_tags (false),
    m_pStr (NULL)
{
    m_word_list.SetAllocator (pAlloc);
    m_word_list.Create ();

    m_regexp_list.SetAllocator (pAlloc);
    m_regexp_list.Create ();

    m_dict_list.SetAllocator (pAlloc);
    m_dict_list.Create ();

    m_tag_list.SetAllocator (pAlloc);
    m_tag_list.Create ();
}


void FAWREToken::SetStr (const char * pStr)
{
    m_pStr = pStr;
}


const char * FAWREToken::GetStr () const
{
    return m_pStr;
}


const int FAWREToken::GetWordCount () const
{
    return m_word_list.size ();
}


const int FAWREToken::GetRegexpCount () const
{
    return m_regexp_list.size ();
}


const int FAWREToken::GetDictCount () const
{
    return m_dict_list.size ();
}


const int FAWREToken::GetTagCount () const
{
    return m_tag_list.size ();
}


const FAToken * FAWREToken::GetWordToken (const int Idx) const
{
    return & (m_word_list [Idx]);
}


const FAToken * FAWREToken::GetRegexpToken (const int Idx) const
{
    return & (m_regexp_list [Idx]);
}


const FAToken * FAWREToken::GetDictToken (const int Idx) const
{
    return & (m_dict_list [Idx]);
}


const FAToken * FAWREToken::GetTagToken (const int Idx) const
{
    return & (m_tag_list [Idx]);
}


void FAWREToken::AddWordToken (const int Type, const int Offset, const int Length)
{
    m_word_list.push_back (FAToken (Type, Offset, Length));
}


void FAWREToken::AddRegexpToken (const int Type, const int Offset, const int Length)
{
    m_regexp_list.push_back (FAToken (Type, Offset, Length));
}


void FAWREToken::AddDictToken (const int Type, const int Offset, const int Length)
{
    m_dict_list.push_back (FAToken (Type, Offset, Length));
}


void FAWREToken::AddTagToken (const int Type, const int Offset, const int Length)
{
    m_tag_list.push_back (FAToken (Type, Offset, Length));
}


const bool FAWREToken::GetWordsDisj () const
{
    return m_disj_words;
}


const bool FAWREToken::GetRegexpsDisj () const
{
    return m_disj_regexps;
}


const bool FAWREToken::GetDictsDisj () const
{
    return m_disj_dicts;
}


const bool FAWREToken::GetTagsDisj () const
{
    return m_disj_tags;
}


void FAWREToken::SetWordsDisj (const bool WordsDisj)
{
    m_disj_words = WordsDisj;
}


void FAWREToken::SetRegexpsDisj (const bool RegexpsDisj)
{
    m_disj_regexps = RegexpsDisj;
}


void FAWREToken::SetDictsDisj (const bool DictsDisj)
{
    m_disj_dicts = DictsDisj;
}


void FAWREToken::SetTagsDisj (const bool TagsDisj)
{
    m_disj_tags = TagsDisj;
}


void FAWREToken::Clear ()
{
    m_word_list.resize (0);
    m_regexp_list.resize (0);
    m_dict_list.resize (0);
    m_tag_list.resize (0);

    m_pStr = NULL;

    m_disj_words = false;
    m_disj_regexps = false;
    m_disj_dicts = false;
    m_disj_tags = false;
}

}
