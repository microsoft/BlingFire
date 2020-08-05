/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATaggedText.h"

namespace BlingFire
{


FATaggedText::FATaggedText (FAAllocatorA * pAlloc)
{
    m_chars.SetAllocator (pAlloc);
    m_chars.Create ();

    m_num2sl.SetAllocator (pAlloc);
    m_num2sl.Create ();

    m_num2tag.SetAllocator (pAlloc);
    m_num2tag.Create ();

    m_num2offset.SetAllocator (pAlloc);
    m_num2offset.Create ();
}


FATaggedText::~FATaggedText ()
{}


const int FATaggedText::GetWordCount () const
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));

    const int Count = m_num2tag.size ();
    return Count;
}


const int FATaggedText::GetWord (const int Num, const int ** pWord) const
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));
    DebugLogAssert (0 <= Num && m_num2tag.size () > (unsigned int) Num);
    DebugLogAssert (pWord);

    const int * pSL = m_num2sl.begin () + (Num << 1);
    *pWord = m_chars.begin () + *pSL++;
    return *pSL;
}


const int FATaggedText::GetTag (const int Num) const
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));
    DebugLogAssert (0 <= Num && m_num2tag.size () > (unsigned int) Num);

    const int Tag = m_num2tag [Num];
    return Tag;
}


const int FATaggedText::GetOffset (const int Num) const
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));
    DebugLogAssert (0 <= Num && m_num2tag.size () > (unsigned int) Num);

    const int Offset = m_num2offset [Num];
    return Offset;
}


void FATaggedText::AddWord (
        const int * pWord, 
        const int Length, 
        const int Tag
    )
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));

    const int Start = m_chars.size ();
    m_num2sl.push_back (Start);
    m_num2sl.push_back (Length);

    m_chars.resize (Start + Length);
    memcpy (m_chars.begin () + Start, pWord, sizeof (int) * Length);

    m_num2tag.push_back (Tag);
}


void FATaggedText::AddWord (
        const int * pWord, 
        const int Length, 
        const int Tag, 
        const int Offset
    )
{
    DebugLogAssert (m_num2tag.size () == m_num2offset.size ());
    DebugLogAssert (m_num2tag.size () == (m_num2sl.size () / 2));

    FATaggedText::AddWord (pWord, Length, Tag);
    m_num2offset.push_back (Offset);
}


void FATaggedText::Clear ()
{
    m_chars.resize (0);
    m_num2sl.resize (0);
    m_num2tag.resize (0);
    m_num2offset.resize (0);
}


void FATaggedText::SetTags (const int * pTags, const int Count)
{
    LogAssert (0 <= Count && (unsigned) Count == m_num2tag.size ());

    memcpy (m_num2tag.begin (), pTags, Count * sizeof (int));
}

}
