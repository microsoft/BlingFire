/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWRETokenParser.h"
#include "FAAllocatorA.h"
#include "FAWREToken.h"
#include "FAUtils.h"
#include "FAException.h"


namespace BlingFire
{

FAWRETokenParser::FAWRETokenParser (FAAllocatorA * pAlloc) :
    m_pTokenStr (NULL),
    m_Len (-1),
    m_pToken (NULL)
{
    m_char2info.SetAllocator (pAlloc);
    m_char2info.Create ();
    m_char2info.resize (MaxCharSymbol + 1);

    for (int i = 0; i <= MaxCharSymbol; ++i) {
        m_char2info [i] = 0;
    }

    m_char2info [CharWordListBegin] |= WordListBegin;
    m_char2info [CharRegexpListBegin] |= RegexpListBegin;
    m_char2info [CharDictListBegin] |= DictListBegin;
    m_char2info [CharNegative] |= Negative;
    m_char2info [CharListDelimier] |= ListDelimiter;
    m_char2info [CharDictDelimier] |= DictDelimiter;
    m_char2info [CharDisjunction] |= Disjunction;
}


void FAWRETokenParser::SetTokenStr (const char * pTokenStr, const int Len)
{
    m_pTokenStr = pTokenStr;
    m_Len = Len;
}


void FAWRETokenParser::SetToken (FAWREToken * pToken)
{
    m_pToken = pToken;
}


inline const bool FAWRETokenParser::
    IsEscaped (const char * pBegin, const int Pos)
{
    DebugLogAssert (pBegin && 0 <= Pos);
    return FAIsEscaped (Pos, pBegin, Pos + 1);
}


inline const char * FAWRETokenParser::
    FindListEnd (const char * pBegin, const char * pEnd) const
{
    if (NULL == pBegin || pEnd <= pBegin) {
        FASyntaxError (m_pTokenStr, m_Len, -1, "fatal error");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }

    const char * pStart = pBegin;
    bool withing_list = false;

    while (pBegin < pEnd) {

        const unsigned char C = *pBegin;

        if (0 == C)
            break;

        if (((WordListBegin | RegexpListBegin) & m_char2info [C]) && \
            (false == IsEscaped (pStart, int (pBegin - pStart)))) {
            withing_list = !withing_list;
        }

        if ((ListDelimiter & m_char2info [C]) && false == withing_list)
            break;

        pBegin++;
    }

    return pBegin;
}


inline void FAWRETokenParser::
    ParseWordList (const char * pBegin, const char * pEnd)
{
    if (NULL == pBegin || pEnd <= pBegin) {
        FASyntaxError (m_pTokenStr, m_Len, -1, "fatal error");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }

    const char * pStart = pBegin;
    unsigned char C = *pBegin;

    /// see whether the list is negative, 
    /// the value of negative must be kept all over the word-list
    int TokenType = FAWREToken::TT_POSITIVE_WORD;

    if (Negative & m_char2info [C]) {
        TokenType = FAWREToken::TT_NEGATIVE_WORD;
        pBegin++;
    }

    bool quotation = false;
    const char * pWordBegin = NULL;
    const char * pWordEnd = NULL;

    while (pBegin < pEnd) {

        C = *pBegin;

        /// set up word list to be disjunctive
        if((Disjunction & m_char2info [C]) && \
           (false == IsEscaped (pStart, int (pBegin - pStart)))) {

            m_pToken->SetWordsDisj (true);
        }
        /// see whether we have a quotation mark here
        if((WordListBegin & m_char2info [C]) && \
           (false == IsEscaped (pStart, int (pBegin - pStart)))) {

            quotation = !quotation;

            if (quotation) {

                /// skip quotation mark
                pWordBegin = pBegin + 1;

            } else {

                pWordEnd = pBegin;

                /// add word token
                const int Offset = int (pWordBegin - m_pTokenStr);
                const int Length = int (pWordEnd - pWordBegin);
                m_pToken->AddWordToken (TokenType, Offset, Length);
            }
        }

        pBegin++;
    } // of while (pBegin < pEnd)
}


inline void FAWRETokenParser::
    ParseRegexpList (const char * pBegin, const char * pEnd)
{
    if (NULL == pBegin || pEnd <= pBegin) {
        FASyntaxError (m_pTokenStr, m_Len, -1, "fatal error");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }

    int TokenType = FAWREToken::TT_POSITIVE_REGEXP;
    bool quotation = false;
    const char * pRegexpBegin = NULL;
    const char * pRegexpEnd = NULL;
    const char * pStart = pBegin;

    while (pBegin < pEnd) {

        const unsigned char C = *pBegin;

        /// see whether negative indicator specified
        if (false == quotation && Negative & m_char2info [C]) {

            TokenType = FAWREToken::TT_NEGATIVE_REGEXP;
            pBegin++;
            continue;
        }

        /// set up regexp list to be disjunctive
        if((Disjunction & m_char2info [C]) && \
           (false == IsEscaped (pStart, int (pBegin - pStart)))) {

            m_pToken->SetRegexpsDisj (true);
            pBegin++;
            continue;
        }

        /// see whether we have a quotation mark here
        if((RegexpListBegin & m_char2info [C]) && \
            (false == IsEscaped (pStart, int (pBegin - pStart)))) {

            quotation = !quotation;

            if (quotation) {

                /// skip quotation mark
                pRegexpBegin = pBegin + 1;

            } else {

                pRegexpEnd = pBegin;

                /// add regexp token
                const int Offset = int (pRegexpBegin - m_pTokenStr);
                const int Length = int (pRegexpEnd - pRegexpBegin);
                m_pToken->AddRegexpToken (TokenType, Offset, Length);

                /// return token type into the initial value
                TokenType = FAWREToken::TT_POSITIVE_REGEXP;
            }

            pBegin++;
            continue;
        }

        pBegin++;

    } // of while (pBegin < pEnd)
}


inline void FAWRETokenParser::
    ParseDictList (const char * pBegin, const char * pEnd)
{
    if (NULL == pBegin || pEnd <= pBegin) {
        FASyntaxError (m_pTokenStr, m_Len, -1, "fatal error");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }

    int TokenType = FAWREToken::TT_POSITIVE_DICT;
    bool IsInside = false;
    const char * pDictBegin = NULL;
    const char * pDictEnd = NULL;

    while (pBegin < pEnd) {

        const unsigned char C = *pBegin;

        /// see whether the dictionary name has not started already
        if (!IsInside && DictListBegin & m_char2info [C]) {
            IsInside = true;
            pBegin++;
            pDictBegin = pBegin;
            continue;
        }
        /// see whether negative indicator is specified
        if (IsInside && Negative & m_char2info [C]) {
            TokenType = FAWREToken::TT_NEGATIVE_DICT;
            pBegin++;
            pDictBegin = pBegin;
            continue;
        }
        /// see whether the dictionary name has finished
        if(IsInside && \
           ((DictDelimiter & m_char2info [C]) || \
            (Disjunction & m_char2info [C]))) {

            /// set up dict list to be disjunctive
            if(Disjunction & m_char2info [C]) {
                m_pToken->SetDictsDisj (true);
            }

            IsInside = false;
            pDictEnd = pBegin;

            /// add dict token
            const int Offset = int (pDictBegin - m_pTokenStr);
            const int Length = int (pDictEnd - pDictBegin);
            m_pToken->AddDictToken (TokenType, Offset, Length);

            /// return token type into the initial value
            TokenType = FAWREToken::TT_POSITIVE_DICT;

            /// skip disjunction symbol
            if (Disjunction & m_char2info [C]) {
                pBegin++;    
            }
            continue;
        }
        pBegin++;

    } // of while (pBegin < pEnd)

    if (IsInside) {

        pDictEnd = pBegin;

        /// add dict token
        const int Offset = int (pDictBegin - m_pTokenStr);
        const int Length = int (pDictEnd - pDictBegin);
        m_pToken->AddDictToken (TokenType, Offset, Length);
    }
}


inline void FAWRETokenParser::
    ParseTagList (const char * pBegin, const char * pEnd)
{
    if (NULL == pBegin || pEnd <= pBegin) {
        FASyntaxError (m_pTokenStr, m_Len, -1, "fatal error");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }

    const char * const pStart = pBegin;
    int TokenType = FAWREToken::TT_POSITIVE_TAG;
    bool IsInside = false;
    const char * pTagBegin = NULL;
    const char * pTagEnd = NULL;

    while (pBegin < pEnd) {

        const unsigned char C = *pBegin;

        /// see whether Disjunction or Negative starts the new TAG
        if (!IsInside && (Disjunction | Negative) & m_char2info [C]) {

            if (Negative & m_char2info [C]) {
                TokenType = FAWREToken::TT_NEGATIVE_TAG;
            }
            pBegin++;
            pTagBegin = pBegin;
            IsInside = true;
            continue;
        }
        /// see whether the TAG is first
        if (pStart == pBegin && !IsInside) {

            pTagBegin = pBegin;
            IsInside = true;
            pBegin++;
            continue;
        }
        /// see whether Disjunction or Negative ends prev TAG
        if(IsInside && (Disjunction | Negative) & m_char2info [C]) {

            /// set up dict list to be disjunctive
            if(Disjunction & m_char2info [C]) {
                m_pToken->SetTagsDisj (true);
            }

            /// add tag token
            pTagEnd = pBegin;
            const int Offset = int (pTagBegin - m_pTokenStr);
            const int Length = int (pTagEnd - pTagBegin);
            m_pToken->AddTagToken (TokenType, Offset, Length);

            /// return token type into the initial value
            TokenType = FAWREToken::TT_POSITIVE_TAG;
            IsInside = false;
            continue;
        }
        pBegin++;

    } // of while (pBegin < pEnd)

    /// add last TAG
    if (IsInside) {
        pTagEnd = pBegin;
        const int Offset = int (pTagBegin - m_pTokenStr);
        const int Length = int (pTagEnd - pTagBegin);
        m_pToken->AddTagToken (TokenType, Offset, Length);
    }
}


void FAWRETokenParser::Process  ()
{
    DebugLogAssert (m_pTokenStr && 0 < m_Len);
    DebugLogAssert (m_pToken);

    /// set up the token string
    m_pToken->SetStr (m_pTokenStr);

    const char * pBegin = m_pTokenStr;
    const char * pEnd = m_pTokenStr + m_Len;

    bool is_word_list = false;
    bool is_regexp_list = false;
    bool is_dict_list = false;
    bool is_tag_list = false;

    // make token parsing
    while (pBegin < pEnd) {

        DebugLogAssert (pBegin);
        /// the C should be the begining of some list
        unsigned char C = *pBegin;

        /// see whether the list is negative
        if (Negative & m_char2info [C]) {
            /// syntax error: unexpected symbol at pBegin
            if (pBegin + 1 >= pEnd) {
                FASyntaxError (m_pTokenStr, m_Len, -1, "unexpected end of token");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }
            C = *(pBegin + 1);
        }
        /// skip trailing 0-bytes
        if (0 == C && \
            (is_word_list || is_regexp_list || is_dict_list || is_tag_list)) {
            pBegin++;
            continue;
        }
        /// we have found a word list at pBegin
        if (!is_word_list && WordListBegin & m_char2info [C]) {

            // calc the end of the list
            const char * const pWordListEnd = FindListEnd (pBegin, pEnd);
            // parse the list
            ParseWordList (pBegin, pWordListEnd);
            // skip the list delimiter if any
            pBegin = pWordListEnd + 1;
            // assumes there is only one word list in the token
            is_word_list = true;
            continue;
        }
        /// we have found a regexp list at pBegin
        if (!is_regexp_list && RegexpListBegin & m_char2info [C]) {

            // calc the end of the list
            const char * const pRegexpListEnd = FindListEnd (pBegin, pEnd);
            // parse the list
            ParseRegexpList (pBegin, pRegexpListEnd);
            // skip the list delimiter if any
            pBegin = pRegexpListEnd + 1;
            // assumes there is only one regexp list in the token
            is_regexp_list = true;
            continue;
        }
        /// we have found a dict list at pBegin
        if (!is_dict_list && DictListBegin & m_char2info [C]) {

            // calc the end of the list
            const char * const pDictListEnd = FindListEnd (pBegin, pEnd);
            // parse the list
            ParseDictList (pBegin, pDictListEnd);
            // skip the list delimiter if any
            pBegin = pDictListEnd + 1;
            // assumes there is only one regexp list in the token
            is_dict_list = true;
            continue;
        }
        /// tag-related fields
        if (!is_tag_list) {

            // calc the end of the list
            const char * const pTagListEnd = FindListEnd (pBegin, pEnd);
            // parse the list
            ParseTagList (pBegin, pTagListEnd);
            // skip the list delimiter if any
            pBegin = pTagListEnd + 1;
            // assumes there is only one tag list in the token
            is_tag_list = true;
            continue;
        }

        /// syntax error: unexpected symbol at pBegin
        FASyntaxError (m_pTokenStr, m_Len, int (pBegin - m_pTokenStr), 
                         "unexpected symbol");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }
}

}
