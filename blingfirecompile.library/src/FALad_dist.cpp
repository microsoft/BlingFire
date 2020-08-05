/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"

#ifndef BLING_FIRE_NOAP
#include "FAConfig.h"
#include "FALad_dist.h"
#include "FALadLDB.h"
#include "FAUtf8Utils.h"
#include "FAUtils_cl.h"

namespace BlingFire
{


FALad_dist::FALad_dist () :
    m_MaxTag (0),
    m_UnkTag (0),
    m_Order (0),
    m_MinOrder (0),
    m_MaxCount (0),
    m_MinMatchRatio (0),
    m_MinWordMatchRatio (0),
    m_TokenLen (0),
    m_pMemMgr (NULL),
    m_WordCount (0),
    m_pScores (NULL),
    m_pCounts (NULL),
    m_pCharMap (NULL),
    m_pC2SMap (NULL),
    m_pS2LMap (NULL),
    m_MinScriptTag (0),
    m_MaxScriptTag (0),
    m_pScript2Count (NULL),
    m_fUseNgrams (false),
    m_fUseWords (false)
{
    m_Token [0] = 0x20;

    for (int i = 0; i < MaxNgramOrder; ++i) {
        m_NgramCount [i] = 0;
    }
}


FALad_dist::~FALad_dist ()
{
    FALad_dist::Clear ();
}


void FALad_dist::Clear ()
{
    if (m_pScores) {
        delete [] m_pScores;
        m_pScores = NULL;
    }
    if (m_pCounts) {
        delete [] m_pCounts;
        m_pCounts = NULL;
    }
    if (m_pScript2Count) {
        delete [] m_pScript2Count;
        m_pScript2Count = NULL;
    }

    m_fUseNgrams = false;
    m_fUseWords = false;
}


void FALad_dist::Initialize (CMemLfpManager * pMemMgr, const FALadLDB * pLDB)
{
    FALad_dist::Clear ();

    m_pMemMgr = pMemMgr;

    const FALadConfKeeper * pLadConf = pLDB->GetLadConf ();

    if (NULL == pLadConf) {
        return;
    }

    m_Order = pLadConf->GetOrder ();
    m_MinOrder = pLadConf->GetMinOrder ();
    m_MaxCount = pLadConf->GetMaxCount ();
    m_MaxTag = pLadConf->GetMaxTag ();
    m_UnkTag = pLadConf->GetUnkTag ();
    m_MinMatchRatio = pLadConf->GetMinMatchRatio ();
    m_MinWordMatchRatio = pLadConf->GetMinWordMatchRatio ();
    m_pCharMap = pLadConf->GetCharMap ();

    LogAssert (m_Order >= m_MinOrder && 
        1 <= m_MinOrder && MaxNgramOrder >= m_Order);

    DebugLogAssert (!m_pScores);
    m_pScores = NEW float [m_MaxTag + 1];
    LogAssert (m_pScores);
    DebugLogAssert (!m_pCounts);
    m_pCounts = NEW int [m_MaxTag + 1];
    LogAssert (m_pCounts);

    // language/script maps initialization
    m_pC2SMap = pLadConf->GetC2SMap ();
    m_pS2LMap = pLadConf->GetS2LMap ();
    LogAssert (m_pC2SMap && m_pS2LMap);

    m_MinScriptTag = pLadConf->GetMinScriptTag ();
    m_MaxScriptTag = pLadConf->GetMaxScriptTag ();
    LogAssert (m_MinScriptTag <= m_MaxScriptTag);
    LogAssert (m_MinScriptTag > m_MaxTag);

    const int ScriptCount = m_MaxScriptTag - m_MinScriptTag + 1;

    DebugLogAssert (!m_pScript2Count);
    m_pScript2Count = NEW int [ScriptCount];
    LogAssert (m_pScript2Count);

    // see if the n-gram scorer should be used
    const FAWgConfKeeper * pN2TPConf = pLDB->GetN2TPConf ();

    if (NULL != pN2TPConf) {
        LogAssert (pN2TPConf->GetRsDfa () && pN2TPConf->GetState2Ows ());
        LogAssert (m_Order <= MaxNgramOrder && m_Order >= m_MinOrder);
        m_NgrScorer.Initialize (pN2TPConf, NULL);
        m_fUseNgrams = true;
    }

    // see if the word scorer should be used
    const FAWgConfKeeper * pW2TPConf = pLDB->GetW2TPConf ();

    if (NULL != pW2TPConf) {
        LogAssert (pW2TPConf->GetRsDfa () && pW2TPConf->GetState2Ows ());
        m_WordScorer.Initialize (pW2TPConf, NULL);
        m_fUseWords = true;
    }

    for (int i = 0; i < MaxNgramOrder; ++i) {
        m_Str2Count [i].Reset (m_pMemMgr);
    }
    m_Word2Count.Reset (m_pMemMgr);
}


void FALad_dist::ScoreText (const char * pText, size_t TextSize)
{
    const char * pBegin = pText;
    const char * pEnd = pText + TextSize;

    int C;
    int CharCount = 0;

    // the first letter of each token is a space
    int * const pTokenBuff = m_Token + 1;

    // fill in the first ngram
    while (pBegin < pEnd && CharCount++ <= m_MaxCount) {

        // get next UTF-32 character
        pBegin = ::FAUtf8ToInt (pBegin, pEnd, &C);
        LogAssert (pBegin);

        // normalize a character, only 1 to 1 mapping is allowed
        if (m_pCharMap) {
            int Co;
            if (1 == m_pCharMap->Get (C, &Co, 1)) {
                C = Co;
            }
        } // if (m_pCharMap) ...

        // update script counts
        if (DefJunkSymbol != C) {
            int S;
            if (1 == m_pC2SMap->Get (C, &S, 1)) {
                DebugLogAssert (m_MinScriptTag <= S && m_MaxScriptTag >= S);
                m_pScript2Count [S - m_MinScriptTag]++;
            }
        }

        // see if we found a breaking symbol, or the buffer is full
        if (0x20 >= C || MaxTokenLen == m_TokenLen) {
            // process if it not empty
            if (0 < m_TokenLen) {
                // score the token (m_Token, m_TokenLen)
                ScoreNextToken ();
                // forget it
                m_TokenLen = 0;
            }
        }
        // append next letter to the token
        if (0x20 < C) {
            // add character in the buffer, if the buffer allows
            if (m_TokenLen < MaxTokenLen) {
                pTokenBuff [m_TokenLen] = C;
            }
            m_TokenLen++;
        }

    } // while (pBegin < pEnd) ...

    // process the last token, if any
    if (0 < m_TokenLen) {
        // score the token (m_Token, m_TokenLen)
        ScoreNextToken ();
    }
}


inline const bool FALad_dist::HasJunk (const int * pIn, const int Size) const
{
    for (int i = 0; i < Size; ++i) {
        if (DefJunkSymbol == pIn [i]) {
            return true;
        }
    }
    return false;
}


inline const bool FALad_dist::HasUpper (const int * pIn, const int Size) const
{
    for (int i = 0; i < Size; ++i) {
        if (::FAUtf32IsUpper (pIn [i])) {
            return true;
        }
    }
    return false;
}


inline void FALad_dist::Reset ()
{
    if (m_fUseWords) {
        m_WordCount = 0;
    }
    if (m_fUseNgrams) {
        for (int i = m_MinOrder; i <= m_Order; ++i) {
            m_NgramCount [i - 1] = 0;
        }
    }
    for (int Tag = 0; Tag <= m_MaxTag; ++Tag) {
        m_pScores [Tag] = 0;
        m_pCounts [Tag] = 0;
    }
    if (m_fUseWords || m_fUseNgrams) {
        const int ScriptCount = m_MaxScriptTag - m_MinScriptTag + 1;
        for (int Script = 0; Script < ScriptCount; ++Script) {
            m_pScript2Count [Script] = 0;
        }
    }
}


void FALad_dist::ScoreNextToken ()
{
    DebugLogAssert (m_TokenLen <= MaxTokenLen);

    if (HasJunk (m_Token + 1, m_TokenLen)) {
        return;
    }
    if (HasUpper (m_Token + 1, m_TokenLen)) {
        return;
    }

    m_Token [m_TokenLen + 1] = 0x20;

    // score the word
    if (m_fUseWords) {

        // update the word count
        m_WordCount++;

        // update word's frequency
        int * pFreq = (int*) m_Word2Count.Lookup 
            ((const char *)(& m_Token [1]), m_TokenLen * sizeof (int));

        if (NULL == pFreq) {
            int One = 1;
            int * pNewFreq = (int*) m_Word2Count.AddCopyOfBytes 
                ((const char *)(& m_Token [1]), m_TokenLen * sizeof (int), &One, sizeof(int));
            LogAssert (pNewFreq);
        } else {
            *pFreq = *pFreq + 1;
        }
    }

    // score word's ngrams
    if (m_fUseNgrams) {

        for (int i = 0; i < m_TokenLen; ++i) {

            for (int Order = m_Order; Order >= m_MinOrder; --Order) {

                // see if the ngram can not be extracted
                if (i + Order > m_TokenLen + 2) {
                    break;
                }

                // update the ngram count
                m_NgramCount [Order - 1]++;

                // get the scoring map
                CMemLfpStringMap  * pMap = & m_Str2Count [Order - 1];

                // update n-gram's frequency
                int * pFreq = (int*) pMap->Lookup ((const char *)(& m_Token [i]), Order * sizeof (int));

                if (NULL == pFreq) {
                    int One = 1;
                    int * pNewFreq = (int*) pMap->AddCopyOfBytes 
                        ((const char *)(& m_Token [i]), Order * sizeof (int), &One, sizeof(int));
                    LogAssert (pNewFreq);
                } else {
                    *pFreq = *pFreq + 1;
                }
            } // of for (int Order =  ...
        } // of for (int i = 0; ...
    } // of if (m_fUseNgrams) ...
}


const int FALad_dist::
    GetBestScore (
            CMemLfpStringMap * pMap, 
            FAWordGuesser_prob_t < int > * pScorer, 
            const int TotalCount, 
            const int MinCount,
            const int * pLangs,
            const int LangCount
        )
{
    DebugLogAssert (pMap && pScorer);
    DebugLogAssert (pLangs && 0 < LangCount);
    DebugLogAssert (FAIsSortUniqed (pLangs, LangCount));

    const int * pTags;
    const float * pScores;

    // clear the scores
    int Tag;
    for (Tag = 0; Tag <= m_MaxTag; ++Tag) {
        m_pScores [Tag] = 0;
        m_pCounts [Tag] = 0;
    }

    CMemLfpStringMapIter I (pMap);

    int * pFreq = (int*) I.MoveToStart(pMap);
    const int * pChain = (const int *) I.GetStringKey ();
    int ChainSize = (int) (I.GetStringKeyLength() / sizeof (int));
    DebugLogAssert (!pFreq || 0 < ChainSize);

    while (pFreq) {

        const int Freq = *pFreq;

        // look up probabilities
        const int Count = pScorer->Process (pChain, ChainSize, &pTags, &pScores);

        for (int j = 0; j < Count; ++j) {

            const int Tag = pTags [j];
            const float Score = exp (pScores [j]);

            float Dist = Score - (float (Freq) / TotalCount);
            if (0 > Dist) {
                Dist = - Dist;
            }

            m_pScores [Tag] += Dist;
            m_pCounts [Tag] += Freq;
        }

        // get to the next ngram
        pFreq = (int*) I.Next ();
        pChain = (const int *) I.GetStringKey ();
        ChainSize = (int) (I.GetStringKeyLength() / sizeof (int));
        DebugLogAssert (!pFreq || 0 < ChainSize);
    }

    /// free the memory
    pMap->Reset (m_pMemMgr);

    // add penalties for not-found tags, and normalize scores
    for (Tag = 0; Tag <= m_MaxTag; ++Tag) {
        // get the amount of times we had a score for the tag
        const int Count = m_pCounts [Tag];
        if (0 == Count) {
            continue;
        }
        // get the amount of times we did not have a score for the tag
        const int NotFoundCount = TotalCount - Count;
        // add a score for the unfound cases
        if (0 < NotFoundCount) {
            m_pScores [Tag] += (float (NotFoundCount) / TotalCount);
        }
        // normalize the score
        m_pScores [Tag] = m_pScores [Tag] / TotalCount;
    }

    int BestTag = -1;
    float BestScore = 0;
    int BestCount = 0;

    // find the best solution
    for (Tag = 0; Tag <= m_MaxTag; ++Tag) {

        const int Count = m_pCounts [Tag];

        if (0 == Count) {
            continue;
        }

        if (-1 == FAFind_log (pLangs, LangCount, Tag)) {
            continue;
        }

        const float Score = m_pScores [Tag];

        if (Score < BestScore || -1 == BestTag) {

            BestTag = Tag;
            BestScore = Score;
            BestCount = Count;

        } else if (Score == BestScore && Count > BestCount) {

            BestTag = Tag;
            BestCount = Count;
        }
    }

    /// invalidate all results if the best tag has not been scored 
    /// by enough n-grams 
    if (-1 != BestTag && MinCount > m_pCounts [BestTag]) {
        BestTag = -1;
    }

    return BestTag;
}


const int FALad_dist::GetBestScript () const
{
    int BestScript = -1;
    int BestCount = 0;

    for (int Script = m_MinScriptTag; Script <= m_MaxScriptTag; ++Script) {

        const int Count = m_pScript2Count [Script - m_MinScriptTag];

        if (0 == Count) {
            continue;
        }
        if (BestCount < Count || -1 == BestScript) {
            BestScript = Script;
            BestCount = Count;
        }
    }

    return BestScript;
}


const int FALad_dist::Process (const char * pText, size_t TextSize)
{
    LogAssert (m_fUseWords || m_fUseNgrams);

    // reset total counts and scores
    Reset ();

    // feed the text
    ScoreText (pText, TextSize);

    // get the major script of the page
    const int Script = GetBestScript ();
    if (-1 == Script) {
        return m_UnkTag;
    }

    // get a set of languages of the major script
    const int * pLangs;
    const int LangCount = m_pS2LMap->Get (Script, &pLangs);
    if (0 >= LangCount) {
        return m_UnkTag;
    } else if (1 == LangCount) {
        return *pLangs;
    }

    // find best scores

    if (m_fUseWords) {

        const int MinCount = (m_MinWordMatchRatio * m_WordCount) / 100;
        const int BestTag = GetBestScore 
            (&m_Word2Count, &m_WordScorer, m_WordCount, MinCount, pLangs, LangCount);
        if (-1 != BestTag) {
            return BestTag;
        }
    }
    if (m_fUseNgrams) {

        for (int i = m_Order; i >= m_MinOrder; --i) {

            const int TotalCount = m_NgramCount [i - 1];
            const int MinCount = (m_MinMatchRatio * TotalCount) / 100;
            const int BestTag = GetBestScore 
                (&m_Str2Count [i - 1], &m_NgrScorer, TotalCount, MinCount, pLangs, LangCount);
            if (-1 != BestTag) {
                return BestTag;
            }
        }
    }

    return m_UnkTag;
}


const int FALad_dist::GetScores (const float ** ppScores, const int ** ppCounts) const
{
    *ppScores = m_pScores;
    *ppCounts = m_pCounts;

    return m_MaxTag + 1;
}

}

#endif
