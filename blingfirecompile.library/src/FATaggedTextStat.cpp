/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATaggedTextStat.h"
#include "FAFsmConst.h"
#include "FATaggedTextCA.h"
#include "FALimits.h"
#include "FAException.h"
#include "FAUtf32Utils.h"

namespace BlingFire
{


FATaggedTextStat::FATaggedTextStat (FAAllocatorA * pAlloc) :
    m_StatMask (FAFsmConst::STAT_TYPE_NONE),
    m_IgnoreCase (false),
    m_BosTag (0),
    m_EosTag (0),
    m_pBosWord (NULL),
    m_BosWordLen (0),
    m_pEosWord (NULL),
    m_EosWordLen (0)
{
    m_c2f_w.SetAllocator (pAlloc);
    m_c2f_w.SetCopyChains (true);

    m_c2f_ww.SetAllocator (pAlloc);
    m_c2f_ww.SetCopyChains (true);

    m_c2f_www.SetAllocator (pAlloc);
    m_c2f_www.SetCopyChains (true);

    m_c2f_wt.SetAllocator (pAlloc);
    m_c2f_wt.SetCopyChains (true);

    m_c2f_wtt.SetAllocator (pAlloc);
    m_c2f_wtt.SetCopyChains (true);

    m_c2f_twt.SetAllocator (pAlloc);
    m_c2f_twt.SetCopyChains (true);

    m_c2f_wtwt.SetAllocator (pAlloc);
    m_c2f_wtwt.SetCopyChains (true);

    m_c2f_t.SetAllocator (pAlloc);
    m_c2f_t.SetCopyChains (true);

    m_c2f_tt.SetAllocator (pAlloc);
    m_c2f_tt.SetCopyChains (true);

    m_c2f_ttt.SetAllocator (pAlloc);
    m_c2f_ttt.SetCopyChains (true);

    m_c2f_tttt.SetAllocator (pAlloc);
    m_c2f_tttt.SetCopyChains (true);

    m_c2f_w_t.SetAllocator (pAlloc);
    m_c2f_w_t.SetCopyChains (true);

    m_c2f_tw.SetAllocator (pAlloc);
    m_c2f_tw.SetCopyChains (true);
}


void FATaggedTextStat::SetStatMask (const int StatMask)
{
    m_StatMask = StatMask;
}


void FATaggedTextStat::SetIgnoreCase (const bool IgnoreCase)
{
    m_IgnoreCase = IgnoreCase;
}


void FATaggedTextStat::SetBosTag (const int BosTag)
{
    m_BosTag = BosTag;
}

void FATaggedTextStat::SetEosTag (const int EosTag)
{
    m_EosTag = EosTag;
}


void FATaggedTextStat::
    SetBosWord (const int * pBosWord, const unsigned int BosWordLen)
{
    DebugLogAssert (FALimits::MaxWordLen >= BosWordLen);

    m_pBosWord = pBosWord;
    m_BosWordLen = BosWordLen;
}


void FATaggedTextStat::
    SetEosWord (const int * pEosWord, const unsigned int EosWordLen)
{
    DebugLogAssert (FALimits::MaxWordLen >= EosWordLen);

    m_pEosWord = pEosWord;
    m_EosWordLen = EosWordLen;
}


void FATaggedTextStat::Clear ()
{
    m_c2f_w.Clear ();
    m_c2f_ww.Clear ();
    m_c2f_www.Clear ();
    m_c2f_wt.Clear ();
    m_c2f_wtt.Clear ();
    m_c2f_twt.Clear ();
    m_c2f_wtwt.Clear ();
    m_c2f_t.Clear ();
    m_c2f_tt.Clear ();
    m_c2f_ttt.Clear ();
    m_c2f_tttt.Clear ();
    m_c2f_w_t.Clear ();
    m_c2f_tw.Clear ();
}


void FATaggedTextStat::
    PutChain (FAChain2NumA * pMap, const int * pChain, const int Size)
{
    DebugLogAssert (pMap && pChain && 0 < Size);

    const int * pFreq = pMap->Get (pChain, Size);

    if (NULL == pFreq) {

        pMap->Add (pChain, Size, 1);

    } else {

        const int NewFreq = *pFreq + 1;
        FAAssert (0 < NewFreq, FAMsg::LimitIsExceeded); // int overflow

        pMap->Add (pChain, Size, NewFreq);
    }
}


void FATaggedTextStat::UpdateW (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        const int * pWord;
        const int Length = pT->GetWord (i, &pWord);

        DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
        __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
        DebugLogAssert (pWord);
        __analysis_assume (pWord);

        memcpy (Chain, pWord, sizeof (int) * Length);

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length);
        }

        // add chain
        PutChain (&m_c2f_w, Chain, Length);
    }
}


void FATaggedTextStat::UpdateWW (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [2 * FALimits::MaxWordLen + 1];

    const int * pWord1;
    int Length1;
    const int * pWord2;
    int Length2;

    const int Count = pT->GetWordCount ();

    for (int i = -1; i < Count; ++i) {

        if (0 <= i) {

            Length1 = pT->GetWord (i, &pWord1);

            DebugLogAssert (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);
            __analysis_assume (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);

            memcpy (Chain, pWord1, sizeof (int) * Length1);

        } else {

            Length1 = m_BosWordLen;

            if (0 < m_BosWordLen) {

                DebugLogAssert (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pBosWord, sizeof (int) * m_BosWordLen);
            }
        }

        Chain [Length1] = 0;

        if (Count > i + 1) {

            Length2 = pT->GetWord (i + 1, &pWord2);

            DebugLogAssert (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);
            __analysis_assume (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);

            memcpy (Chain + Length1 + 1, pWord2, sizeof (int) * Length2);

        } else {

            Length2 = m_EosWordLen;

            if (0 < m_EosWordLen) {

                DebugLogAssert (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pEosWord, sizeof (int) * m_EosWordLen);
            }
        }

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length1 + Length2 + 1);
        }

        // add chain
        PutChain (&m_c2f_ww, Chain, Length1 + Length2 + 1);
    }
}


void FATaggedTextStat::UpdateWWW (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [3 * FALimits::MaxWordLen + 2];

    const int * pWord1;
    int Length1;
    const int * pWord2;
    int Length2;
    const int * pWord3;
    int Length3;

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        // w_{i-1}
        if (0 <= i - 1) {
            Length1 = pT->GetWord (i - 1, &pWord1);

            DebugLogAssert (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);
            __analysis_assume (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);

            memcpy (Chain, pWord1, sizeof (int) * Length1);

        } else {

            Length1 = m_BosWordLen;

            if (0 < m_BosWordLen) {

                DebugLogAssert (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pBosWord, sizeof (int) * m_BosWordLen);
            }
        }
        Chain [Length1] = 0;

        // w_i
        Length2 = pT->GetWord (i, &pWord2);

        DebugLogAssert (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);
        __analysis_assume (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);

        memcpy (Chain + Length1 + 1, pWord2, sizeof (int) * Length2);

        Chain [Length1 + Length2 + 1] = 0;

        // w_{i+1}
        if (Count > i + 1) {

            Length3 = pT->GetWord (i + 1, &pWord3);

            DebugLogAssert (pWord3 && 0 < Length3 && Length3 < FALimits::MaxWordLen);
            __analysis_assume (pWord3 && 0 < Length3 && Length3 < FALimits::MaxWordLen);

            memcpy (Chain + Length1 + Length2 + 2, pWord3, \
                sizeof (int) * Length3);
        } else {

            Length3 = m_EosWordLen;

            if (0 < m_EosWordLen) {

                DebugLogAssert (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pEosWord, sizeof (int) * m_EosWordLen);
            }
        }

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length1 + Length2 + Length3 + 2);
        }

        // add chain
        PutChain (&m_c2f_www, Chain, Length1 + Length2 + Length3 + 2);
    }
}


void FATaggedTextStat::UpdateWT (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen + 1];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        const int * pWord;
        const int Length = pT->GetWord (i, &pWord);

        DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
        __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
        DebugLogAssert (pWord);
        __analysis_assume (pWord);

        memcpy (Chain, pWord, sizeof (int) * Length);

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length);
        }

        const int Tag = pT->GetTag (i);
        DebugLogAssert (0 < Tag);
        Chain [Length] = Tag;

        // add chain
        PutChain (&m_c2f_wt, Chain, Length + 1);
    }
}


void FATaggedTextStat::UpdateW_T (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen + 1];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        int Length;

        if (-1 != i - 1) {
            // get the word
            const int * pWord;
            Length = pT->GetWord (i - 1, &pWord);

            DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
            __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
            DebugLogAssert (pWord);
            __analysis_assume (pWord);

            // copy the word
            memcpy (Chain, pWord, sizeof (int) * Length);

            // normalize the case, if needed
            if (m_IgnoreCase) {
                FAUtf32StrLower (Chain, Length);
            }
        } else {
            Length = m_BosWordLen;

            if (0 < m_BosWordLen) {

                DebugLogAssert (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pBosWord, sizeof (int) * m_BosWordLen);
            }
        }

        const int Tag = pT->GetTag (i);
        DebugLogAssert (0 < Tag);
        Chain [Length] = Tag;

        // add chain
        PutChain (&m_c2f_w_t, Chain, Length + 1);
    }
}


void FATaggedTextStat::UpdateTW (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen + 1];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        int Length;

        if (Count != i + 1) {
            // get the word
            const int * pWord;
            Length = pT->GetWord (i + 1, &pWord);

            DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
            __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
            DebugLogAssert (pWord);
            __analysis_assume (pWord);

            // copy the word
            memcpy (Chain, pWord, sizeof (int) * Length);

            // normalize the case, if needed
            if (m_IgnoreCase) {
                FAUtf32StrLower (Chain, Length);
            }
        } else {

            Length = m_EosWordLen;

            if (0 < m_EosWordLen) {

                DebugLogAssert (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pEosWord, sizeof (int) * m_EosWordLen);
            }
        }

        const int Tag = pT->GetTag (i);
        DebugLogAssert (0 < Tag);
        Chain [Length] = Tag;

        // add chain
        PutChain (&m_c2f_tw, Chain, Length + 1);
    }
}


void FATaggedTextStat::UpdateWTT (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen + 2];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        const int * pWord;
        const int Length = pT->GetWord (i, &pWord);

        DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
        __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
        DebugLogAssert (pWord);
        __analysis_assume (pWord);

        memcpy (Chain, pWord, sizeof (int) * Length);

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length);
        }

        // copy tag
        Chain [Length] = pT->GetTag (i);
        DebugLogAssert (0 < Chain [Length]);

        // copy next tag
        Chain [Length + 1] = m_EosTag;
        if (Count > i + 1) {
            Chain [Length + 1] = pT->GetTag (i + 1);
            DebugLogAssert (0 < Chain [Length + 1]);
        }

        // add chain
        PutChain (&m_c2f_wtt, Chain, Length + 2);
    }
}


void FATaggedTextStat::UpdateTWT (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [FALimits::MaxWordLen + 2];

    const int Count = pT->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        const int * pWord;
        const int Length = pT->GetWord (i, &pWord);

        DebugLogAssert (0 < Length && Length < FALimits::MaxWordLen);
        __analysis_assume (0 < Length && Length < FALimits::MaxWordLen);
        DebugLogAssert (pWord);
        __analysis_assume (pWord);

        memcpy (Chain, pWord, sizeof (int) * Length);

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length);
        }

        // copy tag
        Chain [Length] = pT->GetTag (i);
        DebugLogAssert (0 < Chain [Length]);

        // copy prev tag
        Chain [Length + 1] = m_BosTag;
        if (0 < i - 1) {
            Chain [Length + 1] = pT->GetTag (i - 1);
            DebugLogAssert (0 < Chain [Length + 1]);
        }

        // add chain
        PutChain (&m_c2f_twt, Chain, Length + 2);
    }
}


void FATaggedTextStat::UpdateWTWT (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    int Chain [2 * FALimits::MaxWordLen + 3];

    const int * pWord1;
    int Tag1, Length1;
    const int * pWord2;
    int Tag2, Length2;

    const int Count = pT->GetWordCount ();

    for (int i = -1; i < Count; ++i) {

        if (0 <= i) {

            Length1 = pT->GetWord (i, &pWord1);
            Tag1 = pT->GetTag (i);

            DebugLogAssert (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);
            __analysis_assume (pWord1 && 0 < Length1 && Length1 < FALimits::MaxWordLen);

            memcpy (Chain, pWord1, sizeof (int) * Length1);

        } else {

            Length1 = m_BosWordLen;

            if (0 < m_BosWordLen) {

                DebugLogAssert (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pBosWord && m_BosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pBosWord, sizeof (int) * m_BosWordLen);
            }
            Tag1 = m_BosTag;
        }

        Chain [Length1] = 0;

        if (Count > i + 1) {

            Length2 = pT->GetWord (i + 1, &pWord2);
            Tag2 = pT->GetTag (i + 1);

            DebugLogAssert (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);
            __analysis_assume (pWord2 && 0 < Length2 && Length2 < FALimits::MaxWordLen);

            memcpy (Chain + Length1 + 1, pWord2, sizeof (int) * Length2);

        } else {

            Length2 = m_EosWordLen;

            if (0 < m_EosWordLen) {

                DebugLogAssert (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);
                __analysis_assume (m_pEosWord && m_EosWordLen < FALimits::MaxWordLen);

                memcpy (Chain, m_pEosWord, sizeof (int) * m_EosWordLen);
            }
            Tag2 = m_EosTag;
        }

        // normalize case, if needed
        if (m_IgnoreCase) {
            FAUtf32StrLower (Chain, Length1 + Length2 + 1);
        }

        Chain [Length1 + Length2 + 1] = Tag1;
        Chain [Length1 + Length2 + 2] = Tag2;

        // add chain
        PutChain (&m_c2f_wtwt, Chain, Length1 + Length2 + 3);
    }
}


void FATaggedTextStat::UpdateT (const FATaggedTextCA * pT)
{
    DebugLogAssert (pT);

    const int Count = pT->GetWordCount ();

    if (0 == Count) {
        return;
    }

    for (int i = 0; i < Count; ++i) {

        int Tag = pT->GetTag (i);
        DebugLogAssert (0 <= Tag);

        // add chain
        PutChain (&m_c2f_t, &Tag, 1);
    }
}


// For UpdateTT+, uses the sliding tag-ngram window across the tagged sentence,
// bracketing with n-1 BOS and EOS tags.  For example, for trigrams:
//       <BOS>  <BOS>  tag1
//       <BOS>  tag1   tag2
//       tag1   tag2   tag3
//       tag2   tag3   <EOS>
//       tag3   <EOS>  <EOS>
void FATaggedTextStat::UpdateTT (const FATaggedTextCA * pT)
{
    UpdateTChains(&m_c2f_tt, pT, 2);
}


void FATaggedTextStat::UpdateTTT (const FATaggedTextCA * pT)
{
    UpdateTChains(&m_c2f_ttt, pT, 3);
}


void FATaggedTextStat::UpdateTTTT (const FATaggedTextCA * pT)
{
    UpdateTChains(&m_c2f_tttt, pT, 4);
}


void FATaggedTextStat::
    UpdateTChains (FAChain2NumA * pMap, const FATaggedTextCA * pT, int cGram)
{
    DebugLogAssert (pT);

    DebugLogAssert (cGram >= 2 && cGram <= FALimits::MaxGram);
    __analysis_assume (cGram >= 2 && cGram <= FALimits::MaxGram);

    int Chain [FALimits::MaxGram];

    const int Count = pT->GetWordCount ();
    if (0 == Count) {
        return;
    }

    // Make sure we don't forget "-1".  n-1 is the history of the ngram.
    const int cHist = cGram - 1;

    // Initialize the array's BOS bracketing.
    for (int hh = 0; hh < cHist; ++hh) {
        Chain [hh] = m_BosTag;
    }

    // Process each tag ngram.
    for (int ii = 0; ii < Count + cHist; ++ii) {

        // Append the next tag, or EOS bracketing if we have finished all tags.
        Chain[cHist] = m_EosTag;
        if (ii < Count) {
            Chain [cHist] = pT->GetTag (ii);
            DebugLogAssert (0 < Chain [cHist]);
        }

        // add chain
        PutChain (pMap, Chain, cGram);

        // Shift the chain for the next tag.
        for (int hh = 0; hh < cHist; ++hh) {
            Chain[hh] = Chain[hh + 1];
        }
    }
}


void FATaggedTextStat::UpdateStat (const FATaggedTextCA * pT)
{
    if (m_StatMask & FAFsmConst::STAT_TYPE_W) {
        UpdateW (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_WT) {
        UpdateWT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_WTT) {
        UpdateWTT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_TWT) {
        UpdateTWT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_WTWT) {
        UpdateWTWT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_T) {
        UpdateT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_TT) {
        UpdateTT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_TTT) {
        UpdateTTT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_TTTT) {
        UpdateTTTT (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_WW) {
        UpdateWW (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_WWW) {
        UpdateWWW (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_W_T) {
        UpdateW_T (pT);
    }
    if (m_StatMask & FAFsmConst::STAT_TYPE_TW) {
        UpdateTW (pT);
    }
}


const FAChain2NumA * FATaggedTextStat::GetStat (const int StatName) const
{
    if (FAFsmConst::STAT_TYPE_W == StatName) {
        return & m_c2f_w;
    }
    if (FAFsmConst::STAT_TYPE_WT == StatName) {
        return & m_c2f_wt;
    }
    if (FAFsmConst::STAT_TYPE_WTT == StatName) {
        return & m_c2f_wtt;
    }
    if (FAFsmConst::STAT_TYPE_TWT == StatName) {
        return & m_c2f_twt;
    }
    if (FAFsmConst::STAT_TYPE_WTWT == StatName) {
        return & m_c2f_wtwt;
    }
    if (FAFsmConst::STAT_TYPE_T == StatName) {
        return & m_c2f_t;
    }
    if (FAFsmConst::STAT_TYPE_TT == StatName) {
        return & m_c2f_tt;
    }
    if (FAFsmConst::STAT_TYPE_TTT == StatName) {
        return & m_c2f_ttt;
    }
    if (FAFsmConst::STAT_TYPE_TTTT == StatName) {
        return & m_c2f_tttt;
    }
    if (FAFsmConst::STAT_TYPE_WW == StatName) {
        return & m_c2f_ww;
    }
    if (FAFsmConst::STAT_TYPE_WWW == StatName) {
        return & m_c2f_www;
    }
    if (FAFsmConst::STAT_TYPE_W_T == StatName) {
        return & m_c2f_w_t;
    }
    if (FAFsmConst::STAT_TYPE_TW == StatName) {
        return & m_c2f_tw;
    }

    return NULL;
}

}
