/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FADictSplit.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAException.h"

#include <algorithm>
#include <iomanip>

namespace BlingFire
{


FADictSplit::FADictSplit (FAAllocatorA * pAlloc) :
    m_pChainBuffer (NULL),
    m_ChainSize (0),
    m_KeySize (0),
    m_pPrevKey (NULL),
    m_PrevKeySize (0),
    m_Base (16),
    m_NumSize (4),
    m_pOs (NULL),
    m_Freq (0),
    m_Mode (FAFsmConst::DM_TAGS),
    m_NoK2I (false),
    m_InfoBase (0)
{
    m_Set.SetAllocator (pAlloc);
    m_Set.Create ();

    m_Tags.SetAllocator (pAlloc);
    m_Tags.Create ();

    m_Probs.SetAllocator (pAlloc);
    m_Probs.Create ();

    m_KeyNum2SetId.SetAllocator (pAlloc);
    m_KeyNum2SetId.Create ();

    m_Sets.SetAllocator (pAlloc);
    m_SetId2Set.SetAllocator (pAlloc);

    m_PrevKey.SetAllocator (pAlloc);
    m_PrevKey.Create (MaxChainSize);
    m_PrevKey.resize (MaxChainSize);

    m_pPrevKey = m_PrevKey.begin ();
}


inline void FADictSplit::
    PrintKey (const int * pKey, const int KeySize, const int SetId)
{
    DebugLogAssert (0 < KeySize && pKey);

    // see whether no Key stream output is needed
    if (!m_pOs) {
        return;
    }

    // print out the Key
    for (int i = 0; i < KeySize; ++i) {

        if (0 != i) {
            (*m_pOs) << ' ';
        }

        const int Value = pKey [i];

        if (10 == m_Base) {
            (*m_pOs) << std::setw (m_NumSize) << std::setfill ('0') \
                << Value;
        } else {
            DebugLogAssert (16 == m_Base);
            (*m_pOs) << std::setw (m_NumSize) << std::setfill ('0') \
                << std::hex << Value;
        }
    }

    // print out SetId if neded
    if (m_NoK2I) {

        if (10 == m_Base) {
            (*m_pOs) << ' ' << std::setw (m_NumSize) << std::setfill ('0') \
                << (SetId + m_InfoBase);
        } else {
            DebugLogAssert (16 == m_Base);
            (*m_pOs) << ' ' << std::setw (m_NumSize) << std::setfill ('0') \
                << std::hex << (SetId + m_InfoBase);
        }
    }

    (*m_pOs) << '\n';
}


inline void FADictSplit::ProcessEntry_raw ()
{
    DebugLogAssert (0 < m_ChainSize && m_pChainBuffer);

    /// find first 0-delimiter
    for (m_KeySize = 0; m_KeySize < m_ChainSize; ++m_KeySize) {
        if (0 == m_pChainBuffer [m_KeySize]) {
            break;
        }
    }
    FAAssert (0 != m_KeySize && m_ChainSize != m_KeySize, \
        FAMsg::IOError);

    const int * pRawInfo = m_pChainBuffer + m_KeySize + 1;
    const int RawInfoSize = m_ChainSize - m_KeySize - 1;

    // add I2Info entry
    const int Idx = m_Sets.Add (pRawInfo, RawInfoSize, 0);
    DebugLogAssert (0 <= Idx);

    // add K2I entry
    m_KeyNum2SetId.push_back (Idx);

    // prints out current key chain
    PrintKey (m_pChainBuffer, m_KeySize, Idx);
}


inline void FADictSplit::ProcessEntry_tags ()
{
    // make a set
    const int SetSize = FASortUniq (m_Set.begin (), m_Set.end ());

    // create KeyNum --> SetId --> Set relation
    const int Idx = m_Sets.Add (m_Set.begin (), SetSize, 0);
    DebugLogAssert (0 <= Idx);

    m_KeyNum2SetId.push_back (Idx);

    // prints out prev key chain
    PrintKey (m_pPrevKey, m_PrevKeySize, Idx);

    // clean the set
    m_Set.resize (0);

    // change prev key
    if (0 < m_KeySize) {
        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
    }
    m_PrevKeySize = m_KeySize;
}


inline void FADictSplit::UpdateKeys_tags ()
{
    DebugLogAssert (0 < m_ChainSize && m_pChainBuffer);

    int i;

    // find the key size
    m_KeySize = 0;

    for (i = 0; i < m_ChainSize; ++i) {
        if (0 == m_pChainBuffer [i]) {
            m_KeySize = i;
            break;
        }
    }
    FAAssert (0 != m_KeySize, FAMsg::IOError);

    // see whether that is the first input chain and PrevKey is not defined
    if (0 == m_PrevKeySize) {

        DebugLogAssert (0 == m_Set.size ());
        DebugLogAssert (0 == m_KeyNum2SetId.size ());

        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
        m_PrevKeySize = m_KeySize;

    // see whether prev key and key differes
    } else if (m_KeySize != m_PrevKeySize || \
        0 != memcmp (m_pChainBuffer, m_pPrevKey, sizeof (int) * m_KeySize)) {

        ProcessEntry_tags ();
    }

    // add information into the set
    ++i;
    for (;i < m_ChainSize; ++i) {
        const int T = m_pChainBuffer [i];
        m_Set.push_back (T);
    }
}


inline void FADictSplit::ProcessEntry_hyph ()
{
    int i;

    // remove duplicates
    const int SetSize = FASortUniq (m_Set.begin (), m_Set.end ());
    FAAssert (0 < SetSize, FAMsg::IOError);

    // build the INFO: [F, Ow1, Ow2, ..., OwN], N - pattern length
    const int MaxPos = (m_Set [SetSize - 1]) >> 20;
    FAAssert (0 <= MaxPos, FAMsg::IOError);

    const int InfoSize = MaxPos + 2;
    m_Tags.resize (InfoSize);

    m_Tags [0] = m_Freq;

    for (i = 1; i < InfoSize; ++i) {
        m_Tags [i] = FAFsmConst::HYPH_DONT_CARE;
    }

    for (i = 0; i < SetSize; ++i) {

        const int T = m_Set [i];

        const int Pos = T >> 20;
        DebugLogAssert (0 <= Pos && Pos <= MaxPos);

        const int H = T & 0xFFFFF;
        DebugLogAssert (0 <= H);

        if (FAFsmConst::HYPH_DONT_CARE != H) {
            m_Tags [1 + Pos] = H;
        }
    }

    // create KeyNum --> SetId --> Set relation
    const int Idx = m_Sets.Add (m_Tags.begin (), m_Tags.size (), 0);
    DebugLogAssert (0 <= Idx);

    m_KeyNum2SetId.push_back (Idx);

    // prints out prev key chain
    PrintKey (m_pPrevKey, m_PrevKeySize, Idx);

    // clean the set
    m_Set.resize (0);
    m_Tags.resize (0);

    // change prev key
    if (0 < m_KeySize) {
        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
    }
    m_PrevKeySize = m_KeySize;
}


inline void FADictSplit::UpdateKeys_hyph ()
{
    DebugLogAssert (0 < m_ChainSize && m_pChainBuffer);

    int i;

    // find the key size
    m_KeySize = 0;

    for (i = 0; i < m_ChainSize; ++i) {
        if (0 == m_pChainBuffer [i]) {
            m_KeySize = i;
            break;
        }
    }
    FAAssert (0 != m_KeySize, FAMsg::IOError);

    // see whether that is the first input chain and PrevKey is not defined
    if (0 == m_PrevKeySize) {

        DebugLogAssert (0 == m_Set.size ());
        DebugLogAssert (0 == m_KeyNum2SetId.size ());

        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
        m_PrevKeySize = m_KeySize;

    // see whether prev key and key differes
    } else if (m_KeySize != m_PrevKeySize || \
        0 != memcmp (m_pChainBuffer, m_pPrevKey, sizeof (int) * m_KeySize)) {

        ProcessEntry_hyph ();
    }

    // add information into the set
    ++i;
    FAAssert (i < m_ChainSize, FAMsg::IOError);

    m_Freq = m_pChainBuffer [i];
    FAAssert (0 < m_Freq, FAMsg::IOError);
    ++i;

    int Pos = 0;
    for (;i < m_ChainSize; ++i) {

        const int H = m_pChainBuffer [i];
        // to big hyphenation point id
        FAAssert (0 == (H & 0xFFF00000), FAMsg::IOError);
        const int T = (Pos++ << 20) | H;
        m_Set.push_back (T);
    }
}


inline void FADictSplit::ProcessEntry_tag_prob ()
{
    DebugLogAssert (m_Tags.size () == m_Probs.size () && 0 < m_Probs.size ());

    /// Build a common array of tags and probabilities, s.t.
    /// [T1, T2, ..., Tm, P1, P2, ... Pm], where Ti < Tj if i < j.

    const int Count = m_Probs.size ();
    const int SetSize = 2 * Count;

    m_Set.resize (SetSize);
    int * pSet = m_Set.begin ();

    int i;
    for (i = 0; i < Count; ++i) {
        pSet [i] = i;
    }

    // sort indices, if needed
    if (false == FAIsSorted (m_Tags.begin (), Count)) {
        std::sort (pSet, pSet + Count, FAIdxCmp_s2b (m_Tags.begin ()));
    }

    for (i = 0; i < Count; ++i) {

        const int Idx = pSet [i];
        const int Tag = m_Tags [Idx];
        const int Prob = m_Probs [Idx];

        pSet [i] = Tag;
        pSet [i + Count] = Prob;
    }

    // tag duplicates are not allowed
    FAAssert (FAIsSortUniqed (pSet, Count), FAMsg::IOError);

    // create KeyNum --> SetId --> Set relation
    const int Idx = m_Sets.Add (pSet, SetSize, 0);
    DebugLogAssert (0 <= Idx);

    m_KeyNum2SetId.push_back (Idx);

    // prints out prev key chain
    PrintKey (m_pPrevKey, m_PrevKeySize, Idx);

    // clean the set
    m_Set.resize (0);
    m_Tags.resize (0);
    m_Probs.resize (0);

    // change prev key
    if (0 < m_KeySize) {
        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
    }
    m_PrevKeySize = m_KeySize;
}


inline void FADictSplit::UpdateKeys_tag_prob ()
{
    DebugLogAssert (0 < m_ChainSize && m_pChainBuffer);

    int i;

    // find the key size
    m_KeySize = 0;

    for (i = 0; i < m_ChainSize; ++i) {
        if (0 == m_pChainBuffer [i]) {
            m_KeySize = i;
            break;
        }
    }
    FAAssert (m_KeySize + 3 == m_ChainSize, FAMsg::IOError);

    // see whether that is the first input chain and PrevKey is not defined
    if (0 == m_PrevKeySize) {

        DebugLogAssert (0 == m_Tags.size () && 0 == m_Probs.size ());
        DebugLogAssert (0 == m_KeyNum2SetId.size ());

        memcpy (m_pPrevKey, m_pChainBuffer, sizeof (int) * m_KeySize);
        m_PrevKeySize = m_KeySize;

    // see whether prev key and key differes
    } else if (m_KeySize != m_PrevKeySize || \
        0 != memcmp (m_pChainBuffer, m_pPrevKey, sizeof (int) * m_KeySize)) {

        ProcessEntry_tag_prob ();
    }

    // get a < Tag, Prob > pair
    const int Tag = m_pChainBuffer [m_KeySize + 1];
    m_Tags.push_back (Tag);

    const int Prob = m_pChainBuffer [m_KeySize + 2];
    m_Probs.push_back (Prob);
}



inline void FADictSplit::RevSetMap ()
{
    const int Count = m_Sets.GetChainCount ();

    for (int Idx = 0; Idx < Count; ++Idx) {

        const int * pSet;
        const int Size = m_Sets.GetChain (Idx, &pSet);
        DebugLogAssert (0 < Size && pSet);

        m_SetId2Set.Set (Idx, pSet, Size);
    }
}


void FADictSplit::SetKeyOs (std::ostream * pOs)
{
    m_pOs = pOs;
}


void FADictSplit::SetBase (const int Base)
{
    m_Base = Base;
}


void FADictSplit::SetNumSize (const int NumSize)
{
    m_NumSize = NumSize;
}


void FADictSplit::SetMode (const int Mode)
{
    m_Mode = Mode;
}


void FADictSplit::SetNoK2I (const bool NoK2I)
{
    m_NoK2I = NoK2I;
}


void FADictSplit::SetInfoIdBase (const int InfoBase)
{
    m_InfoBase = InfoBase;
}


void FADictSplit::AddChain (const int * pChain, const int Count)
{
    DebugLogAssert (pChain);

    // the input chain is empty or too big
    FAAssert (0 < Count && MaxChainSize >= Count, FAMsg::IOError);

    m_pChainBuffer  = pChain;
    m_ChainSize = Count;

    if (FAFsmConst::DM_TAG_PROB == m_Mode) {

        UpdateKeys_tag_prob ();

    } else if (FAFsmConst::DM_TAGS == m_Mode) {

        UpdateKeys_tags ();

    } else if (FAFsmConst::DM_HYPH == m_Mode) {

        UpdateKeys_hyph ();

    } else if (FAFsmConst::DM_RAW == m_Mode) {

        ProcessEntry_raw ();
    }
}


void FADictSplit::Process ()
{
    m_KeySize = 0;

    if (FAFsmConst::DM_TAG_PROB == m_Mode) {

        ProcessEntry_tag_prob ();

    } else if (FAFsmConst::DM_TAGS == m_Mode) {

        ProcessEntry_tags ();

    } else if (FAFsmConst::DM_HYPH == m_Mode) {

        ProcessEntry_hyph ();
    }

    RevSetMap ();
}


const int FADictSplit::GetK2I (const int ** ppK2I) const
{
    DebugLogAssert (ppK2I);

    if (false == m_NoK2I) {

        *ppK2I = m_KeyNum2SetId.begin ();
        return m_KeyNum2SetId.size ();

    } else {

        *ppK2I = NULL;
        return -1;
    }
}


const FAMultiMapA * FADictSplit::GetI2Info () const
{
    return & m_SetId2Set;
}


void FADictSplit::Clear ()
{
    m_pChainBuffer  = NULL;
    m_ChainSize = 0;
    m_KeySize = 0;
    m_PrevKeySize = 0;
    m_Set.resize (0);
    m_Tags.resize (0);
    m_Probs.resize (0);
    m_KeyNum2SetId.resize (0);
    m_Sets.Clear ();
    m_SetId2Set.Clear ();
    m_Mode = FAFsmConst::DM_TAGS;
    m_NoK2I  = false;
    m_InfoBase = 0;
}

}

