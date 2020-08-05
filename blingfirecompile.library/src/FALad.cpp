/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FALad.h"
#include "FALadLDB.h"
#include "FAUtf8Utils.h"
#include "FAUtils_cl.h"

namespace BlingFire
{


FALad::FALad () :
    m_MaxTag (0),
    m_UnkTag (0),
    m_Order (0),
    m_MinOrder (0),
    m_MaxCount (0),
    m_MinMatchRatio (0),
    m_MinWordMatchRatio (0),
    m_TokenLen (0),
    m_WordCount (0),
    m_BestWordTag (-1),
    m_pCharMap (NULL),
    m_pC2SMap (NULL),
    m_pS2LMap (NULL),
    m_MinScriptTag (0),
    m_MaxScriptTag (0),
    m_pScript2Count (NULL),
    m_pCounts (NULL),
    m_pScores (NULL),
    m_fUseNgrams (false),
    m_fUseWords (false)
{
    m_Token [0] = 0x20;

    for (int i = 0; i < MaxNgramOrder; ++i) {
        m_NgramCount [i] = 0;
        m_BestTag [i] = -1;
    }
}


FALad::~FALad ()
{
    FALad::Clear ();
}


void FALad::Clear ()
{
    if (m_pScript2Count) {
        delete [] m_pScript2Count;
        m_pScript2Count = NULL;
    }

    m_fUseNgrams = false;
    m_fUseWords = false;
}


void FALad::Initialize (const FALadLDB * pLDB)
{
    FALad::Clear ();

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

    m_WordsScores.SetUnkScore (FAFsmConst::MIN_LOG_PROB * 2);
    m_WordsScores.SetMaxTag (m_MaxTag);

    LogAssert (m_Order >= m_MinOrder && 
        1 <= m_MinOrder && MaxNgramOrder >= m_Order);

    for (int i = m_MinOrder; i <= m_Order; ++i) {
        m_NgramScores [i - 1].SetUnkScore (FAFsmConst::MIN_LOG_PROB * 2);
        m_NgramScores [i - 1].SetMaxTag (m_MaxTag);
    }

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
}


inline const int FALad::
    GetBestTag (
        const float * pScores,
        const int * pCounts,
        const int MinCount,
        const int * pLangs,
        const int LangCount
    ) const
{
    DebugLogAssert (pScores && pCounts && 0 <= MinCount);

    int BestTag = -1;
    float BestScore = 0;
    int BestCount = 0;

    for (int Tag = 0; Tag <= m_MaxTag; ++Tag) {

        // get the count
        const int Count = pCounts [Tag];
        // see if the count is no smaller than MinCount
        if (0 == Count || MinCount > Count) {
            continue;
        }

        // see if the language is amoung allowed
        if (-1 == FAFind_log (pLangs, LangCount, Tag)) {
            continue;
        }

        // get the score
        const float Score = pScores [Tag];
        if (Score > BestScore || -1 == BestTag) {

            BestScore = Score;
            BestCount = Count;
            BestTag = Tag;

        } else if (Score == BestScore) {

            if (Count > BestCount) {
                BestCount = Count;
                BestTag = Tag;
            }
        }
    } // of for (int Tag = 0; ...

    if (BestTag == m_UnkTag) {
        return -1;
    }

    return BestTag;
}


void FALad::ScoreText (const char * pText, size_t TextSize)
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
        pBegin = FAUtf8ToInt (pBegin, pEnd, &C);
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


inline const bool FALad::HasJunk (const int * pIn, const int Size) const
{
    for (int i = 0; i < Size; ++i) {
        if (DefJunkSymbol == pIn [i]) {
            return true;
        }
    }
    return false;
}


inline const bool FALad::HasUpper (const int * pIn, const int Size) const
{
    for (int i = 0; i < Size; ++i) {
        if (FAUtf32IsUpper (pIn [i])) {
            return true;
        }
    }
    return false;
}


void FALad::ScoreNextToken ()
{
    DebugLogAssert (m_TokenLen <= MaxTokenLen);

    if (HasJunk (m_Token + 1, m_TokenLen)) {
        return;
    }
    if (HasUpper (m_Token + 1, m_TokenLen)) {
        return;
    }

    m_Token [m_TokenLen + 1] = 0x20;

    const int * pTags;
    const float * pScores;

    // score a word
    if (m_fUseWords) {

        // update the word count
        m_WordCount++;

        // look up the word scores
        const int Count = m_WordScorer.Process (m_Token + 1, m_TokenLen, &pTags, &pScores);
        m_WordsScores.AddScores (pTags, pScores, Count);

    } // of if (m_fUseWords) ...

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

                // look up the ngram score 
                const int Count = m_NgrScorer.Process (m_Token + i, Order, &pTags, &pScores);
                m_NgramScores [Order - 1].AddScores (pTags, pScores, Count);
            }
        }
    } // of if (m_fUseNgrams) ...
}


inline void FALad::ResetScores ()
{
    // clear the counters
    if (m_fUseWords) {
        m_WordsScores.Clear ();
        m_WordCount = 0;
        m_BestWordTag = -1;
    }
    if (m_fUseNgrams) {
        for (int i = m_MinOrder; i <= m_Order; ++i) {
            m_NgramScores [i - 1].Clear ();
            m_NgramCount [i - 1] = 0;
            m_BestTag [i - 1] = -1;
        }
    }
    if (m_fUseWords || m_fUseNgrams) {
        const int ScriptCount = m_MaxScriptTag - m_MinScriptTag + 1;
        for (int Script = 0; Script < ScriptCount; ++Script) {
            m_pScript2Count [Script] = 0;
        }
    }

    m_pCounts = NULL;
    m_pScores = NULL;
}


inline void FALad::FindBestScores (const int * pLangs, const int LangCount)
{
    const int * pCounts;
    const float * pScores;

    // finalize the scores
    if (m_fUseWords) {

        m_WordsScores.Process ();

        /// get an array of scores
#ifndef NDEBUG
        const int Size = 
#endif
            m_WordsScores.GetScores (&pScores, &pCounts);
        DebugLogAssert (Size == m_MaxTag + 1 && pScores && pCounts);

        /// calculate the minimum accepatable count
        const int MinCount = (m_MinWordMatchRatio * m_WordCount) / 100;

        // get the best tag
        const int BestTag = GetBestTag (pScores, pCounts, MinCount, pLangs, LangCount);
        DebugLogAssert (-1 == BestTag || 0 <= BestTag && BestTag <= m_MaxTag);
        DebugLogAssert (m_UnkTag != BestTag);

        m_BestWordTag = BestTag;

    } // of if (m_fUseWords) ...

    if (m_fUseNgrams) {

        for (int i = m_MinOrder; i <= m_Order; ++i) {

            FASummTagScores * pScorer = & m_NgramScores [i - 1];

            pScorer->Process ();

            /// get an array of scores
#ifndef NDEBUG
            const int Size = 
#endif
                pScorer->GetScores (&pScores, &pCounts);
            DebugLogAssert (Size == m_MaxTag + 1 && pScores && pCounts);

            /// calculate the minimum accepatable count
            const int MinCount = (m_MinMatchRatio * m_NgramCount [i - 1]) / 100;

            // get the best tag
            const int BestTag = GetBestTag (pScores, pCounts, MinCount, pLangs, LangCount);
            DebugLogAssert (-1 == BestTag || 0 <= BestTag && BestTag <= m_MaxTag);
            DebugLogAssert (m_UnkTag != BestTag);

            m_BestTag [i - 1] = BestTag;
        }
    } // of if (m_fUseNgrams) ...

}


const int FALad::GetBestScript () const
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


const int FALad::Process (const char * pText, size_t TextSize)
{
    LogAssert (m_fUseWords || m_fUseNgrams);

    // reset scores
    ResetScores ();

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
    FindBestScores (pLangs, LangCount);

    // combine scores together
    if (m_fUseWords) {
        if (-1 != m_BestWordTag) {
            m_WordsScores.GetScores (&m_pScores, &m_pCounts);
            return m_BestWordTag;
        }
    }
    if (m_fUseNgrams) {
        for (int Order = m_Order; Order >= m_MinOrder; --Order) {
            const int BestTag = m_BestTag [Order - 1];
            if (-1 != BestTag) {
                m_NgramScores [Order - 1].GetScores (&m_pScores, &m_pCounts);
                return BestTag;
            }
        }
    }

    return m_UnkTag;
}


const int FALad::GetScores (const float ** ppScores, const int ** ppCounts) const
{
    *ppCounts = m_pCounts;
    *ppScores = m_pScores;
    return m_MaxTag + 1;
}

}
