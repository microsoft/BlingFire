/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACorpusIOTools_utf8.h"
#include "FAFsmConst.h"
#include "FAUtf8Utils.h"
#include "FAPrintUtils.h"
#include "FATaggedTextA.h"
#include "FATagSet.h"
#include "FAParseTreeA.h"
#include "FAException.h"
#include "FALimits.h"
#include "FAArray_cont_t.h"

#include <string>

namespace BlingFire
{


FACorpusIOTools_utf8::FACorpusIOTools_utf8 (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc),
    m_pTagSet (NULL),
    m_NoPosTags (false)
{}


void FACorpusIOTools_utf8::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FACorpusIOTools_utf8::SetNoPosTags (const bool NoPosTags)
{
    m_NoPosTags = NoPosTags;
}


inline static void FAAddWord (
        FATaggedTextA * pS,
        const FATagSet * pTagSet,
        const char * pWord, 
        const int WordLen,
        const char * pTag,
        const int TagLen,
        const int Offset
    )
{
    int Word [FALimits::MaxWordLen];
    int Tag = FAFsmConst::POS_TAG_DEFAULT;

    FAAssert (pTagSet || 0 == TagLen, FAMsg::InvalidParameters);
    FAAssert (0 < WordLen && 0 <= TagLen, FAMsg::IOError);

    // UTF-8 -> UTF-32
    const int SymbolCount = \
        FAStrUtf8ToArray (pWord, WordLen, Word, FALimits::MaxWordLen);
    FAAssert (0 <= SymbolCount && SymbolCount <= FALimits::MaxWordLen, \
        FAMsg::IOError);

    // change MWE separator to space
    for (int i = 0; i < SymbolCount; ++i) {
        if (FAFsmConst::CHAR_MWE_DELIM == Word [i]) {
            Word [i] = FAFsmConst::CHAR_SPACE;
        }
    }

    // see whether input contained POS tag
    if (TagLen) {
        // get tag value
        Tag = pTagSet->Str2Tag (pTag, TagLen);
        // see if Tag is found
        FAAssert (-1 != Tag, std::string( std::string (FAMsg::IOError) + \
         std::string (" Unknown Tag ") + std::string (pTag, TagLen)).c_str ());
    }

    // add word
    pS->AddWord (Word, SymbolCount, Tag, Offset);
}


void FACorpusIOTools_utf8::
    Read (std::istream& is, FATaggedTextA * pS) const
{
    FAAssert (pS, FAMsg::InvalidParameters);

    pS->Clear ();

    std::string buffer;
    if (!std::getline (is, buffer)) {
      return;
    }

    const char * pStr = buffer.c_str ();
    int Len = (const int) buffer.length ();
    if (0 < Len) {
        DebugLogAssert (pStr);
        if (0x0D == (unsigned char) pStr [Len - 1])
            Len--;
    }
    if (0 == Len) {
        return;
    }

    const char * pBegin = pStr;
    const char * pEnd = pStr;
    const char * pDelim = NULL;

    // Len + 1 - to treat the end of string as if it was a CHAR_WORD_DELIM
    for (int i = 0; i < Len + 1; ++i) {

        DebugLogAssert (pEnd);

        char C = FAFsmConst::CHAR_WORD_DELIM;
        if (i < Len) {
            C = *pEnd;
        }

        if (FAFsmConst::CHAR_TAG_DELIM == C && false == m_NoPosTags) {

            pDelim = pStr + i;

        } else if (FAFsmConst::CHAR_WORD_DELIM == C) {

            if (false == m_NoPosTags) {

                const int WordLen = int (pDelim - pBegin);
                const int Offset = int (pBegin - pStr);
                const int TagLen = int (pEnd - pDelim - 1);

                // no word/tag delimiter found, no tag or bad word
                FAAssert (pDelim && 0 <= Offset && 0 < TagLen && 0 < WordLen \
                    && WordLen <= FALimits::MaxWordLen, FAMsg::IOError);

                FAAddWord (pS, m_pTagSet, pBegin, WordLen, pDelim + 1, \
                    TagLen, Offset);
            } else {

                const int WordLen = int (pEnd - pBegin);
                const int Offset = int (pBegin - pStr);

                // bad word
                FAAssert (0 <= Offset && 0 < WordLen && \
                    WordLen <= FALimits::MaxWordLen, FAMsg::IOError);

                FAAddWord (pS, m_pTagSet, pBegin, WordLen, NULL, 0, Offset);
            }

            pBegin = pEnd + 1;
        }

        pEnd++;

    } // of for (int i = 0; ...
}


void FACorpusIOTools_utf8::
    Print (std::ostream& os, const FATaggedTextCA * pS) const
{
    FAAssert (pS, FAMsg::InvalidParameters);

    const int WordCount = pS->GetWordCount ();

    for (int i = 0; i < WordCount; ++i) {

        if (0 != i) {
            os << char (FAFsmConst::CHAR_WORD_DELIM);
        }

        const int * pWord;
        const int WordLen = pS->GetWord (i, &pWord);

        if (false == m_NoPosTags) {
            const int Tag = pS->GetTag (i);
            FAPrintTaggedWord (os, m_pTagSet, pWord, WordLen, Tag);
        } else {
            FAPrintWord (os, pWord, WordLen);
        }

    } // of for (int i = 0; ...

    os << '\n';
}


void FACorpusIOTools_utf8::
    Read (
            std::istream& is, 
            FATaggedTextA * pS, 
            FAParseTreeA * pT
        ) const
{
    FAAssert (m_pAlloc && m_pTagSet, FAMsg::ObjectIsNotReady);
    FAAssert (pS && pT, FAMsg::InvalidParameters);

    pS->Clear ();

    // stack
    FAArray_cont_t < int > S;
    S.SetAllocator (m_pAlloc);
    S.Create ();

    // temporary consituents container
    FAArray_cont_t < int > Cs;
    Cs.SetAllocator (m_pAlloc);
    Cs.Create ();

    std::string buffer;

    if (!std::getline (is, buffer)) {
      return;
    }

    const char * pStr = buffer.c_str ();
    int Len = (const int) buffer.length ();

    if (0 < Len) {
        DebugLogAssert (pStr);
        if (0x0D == (unsigned char) pStr [Len - 1])
            Len--;
    }
    if (0 == Len) {
        return;
    }

    int Pos = 0;

    const char * pBegin = pStr;
    const char * pEnd = pStr;
    const char * pDelim = NULL;

    // Len + 1 - to treat the end of string as if it was a CHAR_WORD_DELIM
    for (int i = 0; i < Len + 1; ++i) {

        DebugLogAssert (pEnd);

        char C = FAFsmConst::CHAR_WORD_DELIM;
        if (i < Len) {
            C = *pEnd;
        }

        if (FAFsmConst::CHAR_TAG_DELIM == C && false == m_NoPosTags) {

            pDelim = pStr + i;

        } else if (FAFsmConst::CHAR_WORD_DELIM == C) {

            const int StrLen = int (pEnd - pBegin);

            // see whether it is a constituent tag
            if (2 < StrLen && \
                '(' == pBegin [StrLen - 1] && \
                '(' == pBegin [StrLen - 2]) {

                int Tag = m_pTagSet->Str2Tag (pBegin, StrLen - 2);

                if (0 >= Tag) {
                    Tag = atoi (pBegin);
                    FAAssert (0 < Tag, FAMsg::IOError);
                }

                // push <Tag, Pos> pair
                S.push_back (Tag);
                S.push_back (Pos);

            // see whether is a closing bracket
            } else if (2 == StrLen && \
                       ')' == pBegin [0] && \
                       ')' == pBegin [1]) {

                // unbalanced brackets
                FAAssert (2 <= S.size (), FAMsg::IOError);
                FAAssert (0 != Pos, FAMsg::IOError);

                const int FromPos = S [S.size () - 1];
                const int Tag = S [S.size () - 2];

                S.pop_back ();
                S.pop_back ();

                Cs.push_back (Tag);
                Cs.push_back (FromPos);
                Cs.push_back (Pos - 1);

            // it is a word_tag pair
            } else {

                if (false == m_NoPosTags) {

                    const int WordLen = int (pDelim - pBegin);
                    const int Offset = int (pBegin - pStr);
                    const int TagLen = int (pEnd - pDelim - 1);

                    FAAddWord (pS, m_pTagSet, pBegin, WordLen, pDelim + 1, \
                        TagLen, Offset);
                } else {

                    const int WordLen = int (pEnd - pBegin);
                    const int Offset = int (pBegin - pStr);

                    FAAddWord (pS, m_pTagSet, pBegin, WordLen, NULL, 0, Offset);
                }

                Pos++;
            }

            pBegin = pEnd + 1;
        }

        pEnd++;

    } // of for (int i = 0; ...

    // unbalanced brackets
    FAAssert (0 == S.size (), FAMsg::IOError);
    DebugLogAssert (Pos == pS->GetWordCount ());

    pT->Init (Pos);

    const int CsCount = Cs.size ();

    // add in reverse order so including will be added before the included

    for (int i = CsCount - 1; i >= 0; --i) {

        const int ToPos = Cs [i--];
        const int FromPos = Cs [i--];
        const int Tag = Cs [i];

        pT->AddNode (-Tag, FromPos, ToPos);
    }

    pT->Update ();
}


static void PrintSubTree_rec (            
        std::ostream& os, 
        const int Root,
        const FATaggedTextCA * pS, 
        const FAParseTreeA * pT,
        const FATagSet * pTagSet,
        bool NoPosTags
    )
{
    int Node = Root;

    while (-1 != Node) {

        if (Root != Node) {
            os << char (FAFsmConst::CHAR_WORD_DELIM);
        }

        const int Label = pT->GetLabel (Node);

        if (0 <= Label) {

            const int * pWord;
            const int WordLen = pS->GetWord (Label, &pWord);

            if (false == NoPosTags) {
                const int Tag = pS->GetTag (Label);
                FAPrintTaggedWord (os, pTagSet, pWord, WordLen, Tag);
            } else {
                FAPrintWord (os, pWord, WordLen);
            }

        } else {

            const int Tag = -Label;
            const char * pTagStr;
            const int TagStrLen = pTagSet->Tag2Str (Tag, &pTagStr);

            if (0 < TagStrLen) {
                for (int j = 0; j < TagStrLen; ++j) {
                    os << pTagStr [j];
                }
            } else {
                os << Tag;
            }
        }

        const int Child = pT->GetChild (Node);

        if (-1 != Child) {
            os << "(( ";
            PrintSubTree_rec (os, Child, pS, pT, pTagSet, NoPosTags);
            os << " ))";
        }

        Node = pT->GetNext (Node);

    } // while (-1 != Node) ...
}


void FACorpusIOTools_utf8::
    Print (
            std::ostream& os, 
            const FATaggedTextCA * pS, 
            const FAParseTreeA * pT
        ) const
{
    FAAssert (m_pTagSet, FAMsg::ObjectIsNotReady);
    FAAssert (pS && pT, FAMsg::InvalidParameters);

    const int * pNodes;
    const int Count = pT->GetUpperNodes (&pNodes);

    if (0 < Count) {
        PrintSubTree_rec (os, *pNodes, pS, pT, m_pTagSet, m_NoPosTags);
    }

    os << '\n';
}

}
