/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMergeMwe.h"
#include "FARSDfaCA.h"
#include "FATaggedTextA.h"
#include "FALimits.h"
#include "FAUtf32Utils.h"

namespace BlingFire
{


FAMergeMwe::FAMergeMwe () :
    m_pDfa (NULL),
    m_MweDelim (int(' ')),
    m_IgnoreCase (false)
{}


void FAMergeMwe::SetMweDelim (const int MweDelim)
{
    m_MweDelim = MweDelim;
}


void FAMergeMwe::SetRsDfa (const FARSDfaCA * pDfa)
{
    m_pDfa = pDfa;
}


void FAMergeMwe::SetIgnoreCase (const bool IgnoreCase)
{
    m_IgnoreCase = IgnoreCase;
}


inline const int FAMergeMwe::
    GetTokenCount (const int Pos, const FATaggedTextCA * pIn) const
{
    DebugLogAssert (pIn && m_pDfa);

    const int WordCount = pIn->GetWordCount ();

    int State = m_pDfa->GetInitial ();
    DebugLogAssert (-1 != State);

    int BestJ = Pos;

    for (int j = Pos; j < WordCount; ++j) {

        const int * pWord;
        const int WordLen = pIn->GetWord (j, &pWord);

        for (int k = 0; k < WordLen && -1 != State; ++k) {

            int Symbol = pWord [k];

            if (m_IgnoreCase) {
                Symbol = FAUtf32ToLower (Symbol) ;
            }

            State = m_pDfa->GetDest (State, Symbol);
        }
        if (-1 == State) {
            break;
        }
        if (m_pDfa->IsFinal (State)) {
            BestJ = j;
        }

        State = m_pDfa->GetDest (State, m_MweDelim);

    } // for (j = i; ...

    const int TokenCount = BestJ + 1 - Pos;
    DebugLogAssert (1 <= TokenCount);

    return TokenCount;
}


void FAMergeMwe::
    Process (FATaggedTextA * pOut, const FATaggedTextCA * pIn) const
{
    DebugLogAssert (pOut && pIn);

    int j;

    // MWE have the same length limit as an ordinary word
    const int MaxMweLen = FALimits::MaxWordLen + 1;
    int MweBuff [MaxMweLen];

    pOut->Clear ();

    const int WordCount = pIn->GetWordCount ();

    for (int i = 0; i < WordCount; ++i) {

        const int MweTokenCount = GetTokenCount (i, pIn);

        if (1 < MweTokenCount) {

            int MweLen = 0;

            for (j = 0; j < MweTokenCount; ++j) {

                const int * pWord;
                const int WordLen = pIn->GetWord (j + i, &pWord);

                if (MweLen + WordLen + 1 <= MaxMweLen) {
                    // copy token text
                    memcpy (MweBuff + MweLen, pWord, sizeof (int) * WordLen);
                    // copy a delimiter
                    MweBuff [MweLen + WordLen] = m_MweDelim;
                    // increase MWE length
                    MweLen += WordLen + 1;
                } else {
                    // MWE is too long ...
                    break;
                }
            } // of for (j = 0; ...

            if (j == MweTokenCount) {

                DebugLogAssert (0 < MweLen);

                const int Offset = pIn->GetOffset (i);
                const int Tag = pIn->GetTag (i + MweTokenCount - 1);

                // MweLen - 1, ignores trailing delimiter
                pOut->AddWord (MweBuff, MweLen - 1, Tag, Offset);

                // skip middle tokens
                i += (MweTokenCount - 1);
                continue;
            }
        } // if (1 < MweTokenCount) ...

        // copy a single word
        const int * pWord;
        const int WordLen = pIn->GetWord (i, &pWord);
        const int Tag =  pIn->GetTag (i);
        const int Offset = pIn->GetOffset (i);

        pOut->AddWord (pWord, WordLen, Tag, Offset);

    } // of for (int i = 0; ...
}

}
