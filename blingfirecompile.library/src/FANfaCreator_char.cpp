/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaCreator_char.h"
#include "FAAllocatorA.h"
#include "FAUtf8Utils.h"
#include "FAUtils.h"
#include "FARegexpTree.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <string>

namespace BlingFire
{


FANfaCreator_char::FANfaCreator_char (FAAllocatorA * pAlloc) :
    FANfaCreator_base (pAlloc),
    m_pEncName (NULL),
    m_IsUtf8 (false),
    m_recode (pAlloc),
    m_IsNegative (false)
{
    m_iws.SetAllocator (pAlloc);
    m_iws.Create ();
}


void FANfaCreator_char::SetEncodingName (const char * pEncStr)
{
    m_pEncName = pEncStr;

    if (pEncStr) {
        m_recode.SetEncodingName (pEncStr);
        m_IsUtf8 = FAIsUtf8Enc (pEncStr);
    }
}


inline const bool FANfaCreator_char::IsEscaped (const int Pos) const
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= Pos && m_ReLength > Pos);

    return FAIsEscaped (Pos, m_pRegexp, m_ReLength);
}


inline const int FANfaCreator_char::
    GetHex (const char * pStr, const int Len) const
{
    std::string buff (pStr, Len);
    return strtol (buff.c_str (), NULL, 16);
}


inline const bool FANfaCreator_char::IsHex (const char C) const
{
    return isdigit ((unsigned char) C) || \
        ( 'A' <= C && C <= 'F' ) ||
        ( 'a' <= C && C <= 'f' );
}


const int FANfaCreator_char::
    GetIw (const char * pStr, const int StrLen, int * pSymLen)
{
    DebugLogAssert (pSymLen && 0 < StrLen);

    int Iw;
    int length;

    /// check for [:BlahBlah:]
    if (4 <= StrLen && '[' == pStr [0] && ':' == pStr [1]) {

        length = 2;
        while (length++ < StrLen) {
            if (':' == pStr [length - 1] && ']' == pStr [length]) {
                break;
            }
        }
        if (4 < length) {

            const char * pName = pStr + 2;
            const int Len = length - 3;
            *pSymLen = length + 1;

            if (0 == strncmp ("alnum", pName, Len)) {
                return IW_ALNUM;
            } else if (0 == strncmp ("digit", pName, Len)) {
                return IW_DIGIT;
            } else if (0 == strncmp ("punct", pName, Len)) {
                return IW_PUNCT;
            } else if (0 == strncmp ("alpha", pName, Len)) {
                return IW_ALPHA;
            } else if (0 == strncmp ("space", pName, Len)) {
                return IW_SPACE;
            } else if (0 == strncmp ("blank", pName, Len)) {
                return IW_BLANK;
            } else if (0 == strncmp ("lower", pName, Len)) {
                return IW_LOWER;
            } else if (0 == strncmp ("upper", pName, Len)) {
                return IW_UPPER;
            } else if (0 == strncmp ("cntrl", pName, Len)) {
                return IW_CNTRL;
            } else if (0 == strncmp ("xdigit", pName, Len)) {
                return IW_XDIGIT;
            } else if (0 == strncmp ("print", pName, Len)) {
                return IW_PRINT;
            } else if (0 == strncmp ("graph", pName, Len)) {
                return IW_GRAPH;
            } else {
                FASyntaxError (pStr, StrLen, 0, "Unsupported character range name.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }
        }
    } // of check for [:BlahBlah:] ...

    if (2 <= StrLen && '\\' == pStr [0] && 's' == pStr [1]) {
        Iw = (unsigned char) ' ';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 't' == pStr [1]) {
        Iw = (unsigned char) '\t';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 'r' == pStr [1]) {
        Iw = (unsigned char) '\r';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 'n' == pStr [1]) {
        Iw = (unsigned char) '\n';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 'b' == pStr [1]) {
        Iw = (unsigned char) '\b';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 'f' == pStr [1]) {
        Iw = (unsigned char) '\f';
        length = 2;
    } else if (2 <= StrLen && '\\' == pStr [0] && 'v' == pStr [1]) {
        Iw = (unsigned char) '\v';
        length = 2;

    /// \xFFFF
    } else if (2 < StrLen && '\\' == pStr [0] && \
        ('x' == pStr [1] || 'X' == pStr [1])) {

        length = 2;
        while (length < StrLen && IsHex (pStr [length])) {
            length++;
        }
        Iw = GetHex (pStr + 2, length - 2);

    } else if (NULL == m_pEncName) {

        length = 1;
        if (1 < StrLen && '\\' == *pStr) {
            pStr++;
            length++;
        }
        Iw = (unsigned char) *pStr;

    } else {

        length = 0;
        if (1 < StrLen && '\\' == *pStr) {
            pStr++;
            length++;
        }
        if (m_IsUtf8) {
            length += FAUtf8Size (pStr);
        } else {
            length++;
        }

        const int Count = m_recode.Process (pStr, length, &Iw, 1);

        if (1 != Count) {
            FASyntaxError (pStr, StrLen, 0, \
                "Cannot make convertion from specified encoding.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }
    }

    *pSymLen = length;
    return Iw;
}


void FANfaCreator_char::AddNamedRange (const int RangeName)
{
    unsigned int c;

    if (IW_ALNUM == RangeName) {

        AddNamedRange (IW_LOWER);
        AddNamedRange (IW_UPPER);
        AddNamedRange (IW_DIGIT);

    } else if (IW_ALPHA == RangeName) {

        AddNamedRange (IW_LOWER);
        AddNamedRange (IW_UPPER);

    } else if (IW_PRINT == RangeName) {

        AddNamedRange (IW_LOWER);
        AddNamedRange (IW_UPPER);
        AddNamedRange (IW_DIGIT);
        AddNamedRange (IW_PUNCT);
        m_iws.push_back (' ');

    } else if (IW_GRAPH == RangeName) {

        AddNamedRange (IW_LOWER);
        AddNamedRange (IW_UPPER);
        AddNamedRange (IW_DIGIT);
        AddNamedRange (IW_PUNCT);

    } else if (IW_DIGIT == RangeName) {

        for (c = '0'; c <= '9'; ++c) {
            m_iws.push_back (c);
        }

    } else if (IW_PUNCT == RangeName) {

        // EXCLAMATION MARK, QUOTATION MARK, NUMBER SIGN, DOLLAR SIGN,
        // PERCENT SIGN, AMPERSAND, APOSTROPHE, LEFT PARENTHESIS,
        // RIGHT PARENTHESIS, ASTERISK, PLUS SIGN, COMMA, HYPHEN-MINUS,
        // FULL STOP, SOLIDUS
        for (c = 0x21; c <= 0x2f; ++c) {
            m_iws.push_back (c);
        }
        // COLON, SEMICOLON, LESS-THAN SIGN, EQUALS SIGN, GREATER-THAN SIGN
        // QUESTION MARK, COMMERCIAL AT
        for (c = 0x3a; c <= 0x40; ++c) {
            m_iws.push_back (c);
        }
        // LEFT SQUARE BRACKET, REVERSE SOLIDUS, RIGHT SQUARE BRACKET
        // CIRCUMFLEX ACCENT, LOW LINE, GRAVE ACCENT
        for (c = 0x5b; c <= 0x60; ++c) {
            m_iws.push_back (c);
        }
        // LEFT CURLY BRACKET, VERTICAL LINE, RIGHT CURLY BRACKET, TILDE
        for (c = 0x7b; c <= 0x7e; ++c) {
            m_iws.push_back (c);
        }

    } else if (IW_CNTRL == RangeName) {

        // START OF HEADING, START OF TEXT, END OF TEXT,
        // END OF TRANSMISSION, ENQUIRY, ACKNOWLEDGE, BELL, BACKSPACE
        // CHARACTER TABULATION, LINE FEED, LINE TABULATION, FORM FEED
        // CARRIAGE RETURN, SHIFT OUT, SHIFT IN, DATA LINK ESCAPE,
        // DEVICE CONTROL ONE - FOUR, NEGATIVE ACKNOWLEDGE, SYNCHRONOUS IDLE
        // END OF TRANSMISSION BLOCK, CANCEL, END OF MEDIUM, SUBSTITUTE
        // ESCAPE, INFORMATION SEPARATOR FOUR - ONE
        for (c = 1; c <= 0x1f; ++c) {
            m_iws.push_back (c);
        }

        // DEL
        m_iws.push_back (0x7f);

    } else if (IW_SPACE == RangeName) {

        m_iws.push_back (' ');
        m_iws.push_back ('\t');
        m_iws.push_back ('\n');
        m_iws.push_back ('\r');
        m_iws.push_back ('\f');
        m_iws.push_back ('\v');

    } else if (IW_BLANK == RangeName) {

        m_iws.push_back (' ');
        m_iws.push_back ('\t');

    } else if (IW_LOWER == RangeName) {

        // a-z
        for (c = 'a'; c <= 'z'; ++c) {
            m_iws.push_back (c);
        }

    } else if (IW_UPPER == RangeName) {

        // A-Z
        for (c = 'A'; c <= 'Z'; ++c) {
            m_iws.push_back (c);
        }

    } else if (IW_XDIGIT == RangeName) {

        // 0-9
        for (c = '0'; c <= '9'; ++c) {
            m_iws.push_back (c);
        }
        // a-f
        for (c = 'a'; c <= 'f'; ++c) {
            m_iws.push_back (c);
        }
        // A-F
        for (c = 'A'; c <= 'F'; ++c) {
            m_iws.push_back (c);
        }
    }
}


void FANfaCreator_char::ParseRange (const char * pStr, const int Length)
{
    FAAssert (0 < Length, FAMsg::SyntaxError);

    int length;

    m_IsNegative = false;
    m_iws.resize (0);

    const char * const pBegin = pStr;
    const char * const pEnd = pStr + Length;

    if ('^' == *pStr) {
        if (pEnd == pStr) {
            m_iws.push_back (int ('^'));
        } else {
            m_IsNegative = true;
        }
        pStr++;
    }

    while (pStr < pEnd) {

        // get Iw
        const int Iw = GetIw (pStr, int (pEnd - pStr), &length);
        pStr += length;

        if (0 > Iw) {

            AddNamedRange (Iw);

        // see whether Iw is followed by the - not in the ending position
        } else if (pStr + 1 < pEnd && '-' == *pStr) {

            // skip the minus and decode the other Iw
            pStr++;
            const int Iw2 = GetIw (pStr, int (pEnd - pStr), &length);
            pStr += length;

            // check whether interval is not too big or too small
            if (0 > Iw2 - Iw || DefMaxRange < Iw2 - Iw) {
                FASyntaxError (pBegin, Length, int (pStr - pBegin), \
                    "Invalid chracter range.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }
            for (int iw = Iw; iw <= Iw2; ++iw) {
                m_iws.push_back (iw);
            }

        } else {

            m_iws.push_back (Iw);
        }

    } // of while (pStr < pEnd) ...

    int Count = m_iws.size ();

    if (1 < Count) {
        const int NewSize = FASortUniq (m_iws.begin (), m_iws.end ());
        m_iws.resize (NewSize);
    }
}


void FANfaCreator_char::
    SetTransition (
        const int FromState,
        const int ToState,
        const int LabelOffset,
        const int LabelLength
    )
{
    if (0 > LabelLength || 0 > LabelOffset) {
        FANfaCreator_base:: \
            SetTransition (FromState, ToState, LabelOffset, LabelLength);
        return;
    }

    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= LabelOffset);

    int Iw;

    if (IsEscaped (LabelOffset) && 's' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) ' ';
    } else if (IsEscaped (LabelOffset) && 't' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\t';
    } else if (IsEscaped (LabelOffset) && 'r' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\r';
    } else if (IsEscaped (LabelOffset) && 'n' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\n';
    } else if (IsEscaped (LabelOffset) && 'b' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\b';
    } else if (IsEscaped (LabelOffset) && 'v' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\v';
    } else if (IsEscaped (LabelOffset) && 'f' == m_pRegexp [LabelOffset]) {
        Iw = (unsigned char) '\f';

    /// \xFFFF label
    } else if (IsEscaped (LabelOffset) && \
        ('x' == m_pRegexp [LabelOffset] || 'X' == m_pRegexp [LabelOffset])) {

        Iw = GetHex (m_pRegexp + LabelOffset + 1, LabelLength - 1);

    /// [^[:alnum:]Aa0-9\xfff0-\xffff-] range label
    } else if (!IsEscaped (LabelOffset) && '[' == m_pRegexp [LabelOffset]) {

        ParseRange (m_pRegexp + LabelOffset + 1, LabelLength - 2);

        const int Count = m_iws.size ();

        if (0 >= Count) {
            FASyntaxError (m_pRegexp, m_ReLength, LabelOffset, \
                "Empty character range.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }

        for (int i = 0; i < Count; ++i) {

            const int iw = m_iws [i];

            if (m_IsNegative) {
                m_tmp_nfa.SetTransition (FromState, iw, FAFsmConst::NFA_DEAD_STATE);
            } else {
                m_tmp_nfa.SetTransition (FromState, iw, ToState);
            }
        }
        if (m_IsNegative) {
            const int AnyOtherIw = m_type2spec [FARegexpTree::TYPE_ANY];
            m_tmp_nfa.SetTransition (FromState, AnyOtherIw, ToState);
            const int LeftAnchorIw = m_type2spec [FARegexpTree::TYPE_L_ANCHOR];
            m_tmp_nfa.SetTransition (FromState, LeftAnchorIw, FAFsmConst::NFA_DEAD_STATE);
            const int RightAnchorIw = m_type2spec [FARegexpTree::TYPE_R_ANCHOR];
            m_tmp_nfa.SetTransition (FromState, RightAnchorIw, FAFsmConst::NFA_DEAD_STATE);
        }
        return ;

    } else if (NULL == m_pEncName) {

        const char Symbol = m_pRegexp [LabelOffset];
        Iw = (unsigned char) Symbol;

    } else {

        const char * pStr = & m_pRegexp [LabelOffset];
        const int Count = m_recode.Process (pStr, LabelLength, &Iw, 1);

        if (-1 == Count) {
            FASyntaxError (m_pRegexp, m_ReLength, LabelOffset, "Cannot make convertion from specified encoding.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }
    }

    m_tmp_nfa.SetTransition (FromState, Iw, ToState);
}

}
