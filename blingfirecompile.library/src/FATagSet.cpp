/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATagSet.h"

namespace BlingFire
{


FATagSet::FATagSet (FAAllocatorA * pAlloc) :
    FAStr2Int_hash (pAlloc),
    m_iMaxTag (-1)
{
    m_tag2idx.SetAllocator (pAlloc);
    m_tag2idx.Create ();
}

void FATagSet::ensure (const int Tag)
{
    DebugLogAssert (0 < Tag); // Tag != 0 

    const int OldSize = m_tag2idx.size ();

    if (Tag >= OldSize) {

        m_tag2idx.resize (Tag + 1);

        for (int tag = OldSize; tag <= Tag; ++tag) {
            m_tag2idx [tag] = -1;
        }
    }
}

const int FATagSet::Add (const char * pStr, const int Size, const int Tag)
{
    DebugLogAssert (0 < Tag && pStr && 0 < Size);

    const int Idx = FAStr2Int_hash::Add (pStr, Size, Tag);
    DebugLogAssert (0 <= Idx);

    ensure (Tag);

    // duplicate tags with different values are prohibited
    DebugLogAssert (-1 == m_tag2idx [Tag]);

    m_tag2idx [Tag] = Idx;

    if (m_iMaxTag < Tag) {
        m_iMaxTag = Tag;
    }

    return Idx;
}

const int FATagSet::Tag2Str (const int Tag, const char ** ppStr) const
{
    DebugLogAssert (0 < Tag && ppStr);

    if (m_tag2idx.size () > (unsigned int) Tag) {

        const int Idx = m_tag2idx [Tag];

        if (-1 != Idx) {

            return FAStr2Int_hash::GetStr (Idx, ppStr);
        }
    }

    return -1;
}

const int FATagSet::Str2Tag (const char * pStr, const int Size) const
{
    int Tag;

    if (FAStr2Int_hash::Get (pStr, Size, &Tag))
        return Tag;
    else
        return -1;
}

const int FATagSet::GetMaxTag() const
{
    return m_iMaxTag;
}

}
