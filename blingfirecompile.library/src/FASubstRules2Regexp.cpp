/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASubstRules2Regexp.h"
#include "FARegexpTree.h"
#include "FATagSet.h"
#include "FAUtf8Utils.h"
#include "FAUtils.h"
#include "FAFsmConst.h"
#include "FAException.h"

namespace BlingFire
{


FASubstRules2Regexp::FASubstRules2Regexp (FAAllocatorA * pAlloc) :
    m_pIs (NULL),
    m_pOut (NULL),
    m_pTagSet (NULL),
    m_Reverse (false),
    m_recode (pAlloc),
    m_use_utf8 (false)
{
    m_acts.SetAllocator (pAlloc);

    m_tokens.SetAllocator (pAlloc);
    m_tokens.Create ();

    m_lexer.SetTokens (&m_tokens);
}


void FASubstRules2Regexp::SetEncodingName (const char * pInputEnc)
{
    m_recode.SetEncodingName (pInputEnc);

    m_use_utf8 = FAIsUtf8Enc (pInputEnc);
    m_lexer.SetUseUtf8 (m_use_utf8);
}


void FASubstRules2Regexp::SetRulesIn (std::istream * pIs)
{
    m_pIs = pIs;
}


void FASubstRules2Regexp::SetRegexpOut (std::ostream * pOut)
{
    m_pOut = pOut;
}


void FASubstRules2Regexp::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FASubstRules2Regexp::SetReverse (const bool Reverse)
{
    m_Reverse = Reverse;
}


const FAMultiMapA * FASubstRules2Regexp::GetActions () const
{
    return & m_acts;
}

void FASubstRules2Regexp::Clear ()
{
    m_acts.Clear ();
    m_right2left.clear ();
    m_tokens.resize (0);
}


void FASubstRules2Regexp::Tokenize (const char * pRegexp, const int RegexpLen)
{
    DebugLogAssert (pRegexp && 0 < RegexpLen);

    m_tokens.resize (0);

    m_lexer.SetRegexp (pRegexp, RegexpLen);
    m_lexer.Process ();
}


void FASubstRules2Regexp::WriteRegexp (
        const char * pRegexp, 
        const int RegexpLen, 
        const int Num
    )
{
    DebugLogAssert (m_pOut);
    DebugLogAssert (pRegexp && 0 < RegexpLen);

    int Symbol;
    int Tag;

    const int TokenCount = m_tokens.size ();
    DebugLogAssert (0 < TokenCount);

    int i;
    for (i = 0; i < TokenCount; ++i) {

        const FAToken * pToken = & (m_tokens [i]);
        DebugLogAssert (pToken);

        int Type = pToken->GetType ();
        const int Offset = pToken->GetOffset ();
        DebugLogAssert (0 <= Offset && Offset < RegexpLen);
        const int Length = pToken->GetLength ();
        DebugLogAssert (0 < Length);

        if (FARegexpTree::TYPE_LTRBR == Type) {

            DebugLogAssert (Offset + Length + 1 <= RegexpLen);

            const int TrBr = atoi (pRegexp + Offset + 1);
            const int NewTrBr = TrBr | (Num << 16);

            (*m_pOut) << '<' << NewTrBr << ' ';

        } else if (FARegexpTree::TYPE_SYMBOL == Type && \
            '[' == pRegexp [Offset]) {

            // check whether we could have a tag name
            if (NULL != m_pTagSet) {

                // find first ']' after '['
                int EndPos = Offset + 1;
                while (EndPos < RegexpLen && ']' != pRegexp [EndPos])
                    EndPos++;

                if (EndPos < RegexpLen) {

                    const char * pTagStr = pRegexp + Offset + 1;
                    const int TagStrSize = EndPos - Offset - 1;

                    Tag = m_pTagSet->Str2Tag (pTagStr, TagStrSize);

                    // see whether Tag exists
                    if (-1 != Tag) {
                        // print tag
                        (*m_pOut) << Tag << ' ';
                        continue;
                    }
                }
            } // of Tag name processing

            // print as is
            for (int j = 0; j < Length; ++j) {
                (*m_pOut) << pRegexp [j + Offset];
            }

        } else if (FARegexpTree::TYPE_SYMBOL == Type) {

            const int Count = 
                m_recode.Process (pRegexp + Offset, Length, &Symbol, 1);

            if (-1 == Count) {
                FASyntaxError (pRegexp, Length, Offset, "Can not convert to UTF-16.");
                throw FAException (FAMsg::IOError, __FILE__, __LINE__);
            }

            // print digit
            (*m_pOut) << Symbol << ' ';

        } else {

            // print as is
            for (int j = 0; j < Length; ++j) {
                (*m_pOut) << pRegexp [j + Offset];
            }
        }
    } // of for (int i = 0; ...
}


void FASubstRules2Regexp::WriteLeftParts ()
{
    std::map <std::string, std::string>::const_iterator I = 
        m_right2left.begin ();

    int RightNum = 1;

    for (;m_right2left.end () != I; ++I, ++RightNum) {

        const char * pLeft = I->second.c_str ();
        const int Len = (const int) I->second.length ();
        DebugLogAssert (pLeft);

        if (1 != RightNum) {
            (*m_pOut) << "|\n";
        }

        (*m_pOut) << '(';
        Tokenize (pLeft, Len);
        WriteRegexp (pLeft, Len, RightNum);
        (*m_pOut) << ")\n";
    }
    (*m_pOut) << '\n';
}


void FASubstRules2Regexp::BuildRightParts ()
{
    std::map <std::string, std::string>::const_iterator I = 
        m_right2left.begin ();

    int RightNum = 1;
    int Value;

    for (;m_right2left.end () != I; ++I, ++RightNum) {

        const char * pRight = I->first.c_str ();
        const int Len = (const int) I->first.length ();
        DebugLogAssert (pRight);

        int i = 0;
        while (i < Len) {

            if ('$' == pRight [i]) {

                const int TrBr = - atoi (pRight + i + 1);
                DebugLogAssert (0 >= TrBr);

                m_acts.Add (RightNum, TrBr);

                const char * pStart = strpbrk (pRight + i + 1, "\"$");

                if (NULL == pStart)
                    break;
                
                i = (const int) (pStart - pRight);

            } else if ('"' == pRight [i]) {

                i++;

                while (i < Len && '"' != pRight [i]) {

                    int SymbolLen = 1;
                    if (m_use_utf8) {
                        SymbolLen = FAUtf8Size (pRight + i);
                    }

                    const int Count = 
                        m_recode.Process (pRight + i, SymbolLen, &Value, 1);

                    if (-1 == Count) {
                        FASyntaxError (pRight, Len, i, "Can not convert to UTF-16.");
                        throw FAException (FAMsg::IOError, __FILE__, __LINE__);
                    }

                    m_acts.Add (RightNum, Value);

                    i += SymbolLen;
                }

                if (i < Len && '"' == pRight [i]) {
                    i++;
                } else {
                    FASyntaxError (pRight, Len, -1, "Missing Quotation.");
                    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
                }

            } else {

                FASyntaxError (pRight, Len, i, "Unexpected symbol.");
                throw FAException (FAMsg::IOError, __FILE__, __LINE__);
            }

        } // of while (i < Len) ...
    } // of for (;m_right2left.end () != I; ...
}


void FASubstRules2Regexp::AddEntry (const char * pLeft, const char * pRight)
{
    DebugLogAssert (pLeft && pRight);

    const std::string LeftBr ("(");
    const std::string RightBr (")");

    std::string Right (pRight);
    std::string Left = LeftBr + std::string (pLeft) + RightBr;

    if (m_pTagSet) {

        const int Length = (const int) strlen (pRight);

        int TagFromBegin = -1;
        int TagFromEnd = -1;
        int TagToBegin = -1;
        int TagToEnd = -1;

        for (int i = 0; i < Length - 1; ++i) {

            if ('[' == pRight [i] && -1 == TagFromBegin) {
                TagFromBegin = i;
            } else if (']' == pRight [i] && -1 == TagFromEnd) {
                TagFromEnd = i;
            } else if ('[' == pRight [i] && -1 == TagToBegin) {
                TagToBegin = i;
            } else if (']' == pRight [i] && -1 == TagToEnd) {
                TagToEnd = i;
                break;
            }
        }
        if (-1 == TagFromBegin || -1 == TagFromEnd) {
            // there should be at least one tag
            FASyntaxError (pRight, Length, -1, "Convertion tags were not found.");
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        std::string TagFrom (pRight, TagFromBegin, TagFromEnd - TagFromBegin + 1);
        std::string TagTo ("");

        // check whether there are two tags
        if (-1 != TagToBegin && -1 != TagToEnd) {

            TagTo.assign (pRight + TagToBegin, TagToEnd - TagToBegin + 1);
            Right.assign (pRight + TagToEnd + 1);

        } else {

            Right.assign (pRight + TagFromEnd + 1);
        }

        if (m_Reverse)
            Left = LeftBr + 
                LeftBr + TagTo + TagFrom + RightBr + 
               Left + 
                LeftBr + TagTo + TagFrom + RightBr + 
               RightBr;
        else
            Left = LeftBr + 
                LeftBr + TagFrom + TagTo + RightBr + 
               Left + 
                LeftBr + TagFrom + TagTo + RightBr + 
               RightBr;

    } // of if (m_pTagSet)...

    std::map < std::string, std::string >::iterator I = 
        m_right2left.find (Right);

    if (I != m_right2left.end ()) {
        I->second = I->second + std::string ("|") + Left;
    } else {
        m_right2left.insert (std::make_pair (Right, Left));
    }
}


const char * FASubstRules2Regexp::FindRight (const char * pStr,
                                             const int Len)
{
    FAAssert (5 < Len && pStr, FAMsg::InternalError);

    int Offset;

    const char * pDelim = " --> ";
    const int DelimLen = 5;

    for (Offset = Len - DelimLen - 1; Offset >= 0; --Offset) {
        if (0 == strncmp (pDelim, pStr + Offset, DelimLen)) {
            return pStr + Offset + DelimLen;
        }
    }
    return NULL;
}


void FASubstRules2Regexp::BuildRight2Left ()
{
    DebugLogAssert (m_pIs);

    const int DelimLen = 5;
    std::string line;

    while (!m_pIs->eof ()) {

        if (!std::getline (*m_pIs, line)) {
            break;
        }
        if (0 == line.length ()) {
            break;
        }

        const char * pLine = line.c_str ();
        int LineLen = (const int) line.length ();

        const char * pRight = FindRight (pLine, LineLen);

        std::string LeftSumm;

        while (NULL == pRight) {

            LeftSumm = LeftSumm + line + "\n";

            if (m_pIs->eof ()) {
                break;
            }
            if (!std::getline (*m_pIs, line)) {
                break;
            }

            pLine = line.c_str ();
            LineLen = (const int) line.length ();

            pRight = FindRight (pLine, LineLen);

        } // of while (!pRight) ...

        if (NULL == pRight) {

            const char * pLeftSumm = LeftSumm.c_str ();
            const int LeftSummLen = (const int) LeftSumm.length ();

            FASyntaxError (pLeftSumm, LeftSummLen, -1, 
                             "Right part does not exist.");
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        const char * pLeft = line.c_str ();
        const int LeftLen = ((int)(pRight - pLeft)) - DelimLen;
        DebugLogAssert (0 <= LeftLen);

        if (0 < LeftLen) {
            LeftSumm = LeftSumm + std::string (pLeft, LeftLen);
        }

        if (0 == LeftSumm.length ()) {
            FASyntaxError (NULL, 0, -1, "Left part does not exist.");
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        AddEntry (LeftSumm.c_str (), pRight);

    } // of while (!m_pIs->eof ()) ...
}


void FASubstRules2Regexp::Process ()
{
    DebugLogAssert (m_pIs && m_pOut);

    Clear ();

    BuildRight2Left ();
    WriteLeftParts ();
    BuildRightParts ();
}

}
