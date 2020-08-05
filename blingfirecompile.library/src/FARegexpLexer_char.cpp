/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpLexer_char.h"
#include "FARegexpTree.h"
#include "FAUtf8Utils.h"
#include "FAUtils.h"
#include "FAException.h"

namespace BlingFire
{


FARegexpLexer_char::FARegexpLexer_char () :
    m_UseUtf8 (false)
{
    for(int i = 0; i < AsciiSize; ++i) {
        m_is_space [i] = false;
    }

    m_is_space [(unsigned char)'\t'] = true;
    m_is_space [(unsigned char)'\n'] = true;
    m_is_space [(unsigned char)'\r'] = true;
    m_is_space [(unsigned char)'\\'] = true;
    m_is_space [(unsigned char)' ']  = true;
}


void FARegexpLexer_char::SetUseUtf8 (const bool UseUtf8)
{
    m_UseUtf8 = UseUtf8;
}


const bool FARegexpLexer_char::IsSpace (const char C) const
{
    if (AsciiSize > (const unsigned char) C) {
        return m_is_space [(const unsigned char)C];
    }

    return false;
}


const int FARegexpLexer_char::FindToken (const int Pos, int * pLength) const
{
    DebugLogAssert (pLength);
    DebugLogAssert (0 <= Pos && Pos < m_Length);

    int length = 1;

    // get next symbol
    char c = m_pRegexp [Pos];

    // see whether it is a special symbol
    if (IsSpecial (c) && !IsEscaped (Pos)) {

        // get char type
        const int Type = GetType (c);

        // if it is LRB then int should follow, e.g. <7
        if (FARegexpTree::TYPE_LTRBR == Type && Pos + length != m_Length) {

            c = m_pRegexp [Pos + length];
            while (isdigit ((unsigned char) c)) {
                length++;
                if (Pos + length == m_Length) {
                    FASyntaxError (m_pRegexp, m_Length, Pos, \
                        "Wrong place for extracting bracket.");
                    throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
                }
                c = m_pRegexp [Pos + length];
            }
            // skip the following space
            if (IsSpace (c)) {
                length++;
            }
        }

        *pLength = length;
        return Type;

    // process \xHEX labels
    } else if (IsEscaped (Pos) && ('x' == c || 'X' == c) ) {

        c = m_pRegexp [Pos + length];
        while (Pos + length < m_Length && IsHex (c)) {
            length++;
            c = m_pRegexp [Pos + length];
        }
        // skip the following space
        if (IsSpace (c)) {
            length++;
        }

        *pLength = length;
        return FARegexpTree::TYPE_SYMBOL;

    // process ranges [^[:alnum:]Aa0-9\xfff0-\xffff-] as a single label
    } else if (!IsEscaped (Pos) && '[' == c) {

        bool in_spec_range = false;

        c = m_pRegexp [Pos + length];

        while ((Pos + length < m_Length) && \
            (']' != c || IsEscaped (Pos + length) || in_spec_range)) {

            // the beginning of chracter class [:
            if (Pos + length < m_Length && '[' == c && \
                !in_spec_range && ':' == m_pRegexp [Pos + length + 1]) {
                length++;
                in_spec_range = true;
            // the end of the chracter class [:
            } else if (Pos + length < m_Length && ':' == c && \
                in_spec_range && ']' == m_pRegexp [Pos + length + 1]) {
                length++;
                in_spec_range = false;
            // withing the chracter class
            } else if (in_spec_range && ! isalpha ((unsigned char) c)) {
                FASyntaxError (m_pRegexp, m_Length, Pos, \
                    "Bad character class name.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }

            length++;
            c = m_pRegexp [Pos + length];

        }
        // did not find the closing ]
        if (Pos + length == m_Length) {
            FASyntaxError (m_pRegexp, m_Length, Pos, \
                "Missing closing bracket for the character range.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }

        *pLength = length + 1;
        return FARegexpTree::TYPE_SYMBOL;

    } else if (IsSpace (c) && !IsEscaped (Pos)) {

        // ignore
        return -1;

    } else {

        if (m_UseUtf8) {

            *pLength = FAUtf8Size (m_pRegexp + Pos);

            if (0 >= *pLength) {
                FASyntaxError (m_pRegexp, m_Length, Pos, \
                    "Bad UTF-8 character.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }

        } else {

            *pLength = 1;
        }

        return FARegexpTree::TYPE_SYMBOL;
    }
}

}
