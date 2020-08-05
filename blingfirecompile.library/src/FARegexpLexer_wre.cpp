/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpLexer_wre.h"
#include "FARegexpTree.h"

namespace BlingFire
{


FARegexpLexer_wre::FARegexpLexer_wre ()
{
    // mark all non-regexp special symbols as one which can start the token
    for(int i = 0; i < AsciiSize; ++i) {

        if (FARegexpTree::TYPE_SYMBOL == m_char2type [i])
            m_char2info [i] = (InfoMaskToken | InfoMaskTokenStart);
        else
            m_char2info [i] = 0;
    }

    /// add space information
    m_char2info [(unsigned char)' ' ] = InfoMaskSpace;
    m_char2info [(unsigned char)'\t'] = InfoMaskSpace;
    m_char2info [(unsigned char)'\n'] = InfoMaskSpace;
    m_char2info [(unsigned char)'\r'] = InfoMaskSpace;

    /// mark special WRE symbols
    m_char2info [(unsigned char)'!' ] = (InfoMaskToken | InfoMaskTokenStart);
    m_char2info [(unsigned char)'"' ] = (InfoMaskToken | InfoMaskTokenStart);
    m_char2info [(unsigned char)'\''] = (InfoMaskToken | InfoMaskTokenStart);
    m_char2info [(unsigned char)'|' ] = InfoMaskToken; // token cannot start from |
    m_char2info [(unsigned char)'@' ] = (InfoMaskToken | InfoMaskTokenStart);
    m_char2info [(unsigned char)'_' ] = (InfoMaskToken | InfoMaskTokenStart);
    m_char2info [(unsigned char)'/' ] = InfoMaskToken; // token cannot start from /
}

const bool FARegexpLexer_wre::IsToken (const int Pos) const
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= Pos && m_Length > Pos);

    const unsigned char C = m_pRegexp [Pos];

    if (AsciiSize > C) {

        if (0 != (InfoMaskToken & m_char2info [C])) {

            return true;

        } else if (IsEscaped (Pos)) {

            return true;
        }
    }

    return false;
}

const bool FARegexpLexer_wre::IsTokenStart (const int Pos) const
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= Pos && m_Length > Pos);

    const unsigned char C = m_pRegexp [Pos];

    if (AsciiSize > C) {

        return 0 != (InfoMaskTokenStart & m_char2info [C]);
    }

    return false;
}

const bool FARegexpLexer_wre::IsSpace (const int Pos) const
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= Pos && m_Length > Pos);

    const unsigned char C = m_pRegexp [Pos];

    if (AsciiSize > C) {

        return 0 != (InfoMaskSpace & m_char2info [C]);
    }

    return false;
}

const int FARegexpLexer_wre::FindToken (const int Pos, int * pLength) const
{
    DebugLogAssert (pLength);
    DebugLogAssert (0 <= Pos && Pos < m_Length);

    /// setup token length
    int length = 0;

    /// get next symbol
    char c = m_pRegexp [Pos];

    /// see whether it is a special symbol
    if (true == IsSpecial (c) && false == IsEscaped (Pos)) {

        /// get char type
        const int Type = GetType (c);
        length = 1;

        /// if it is LRB then int should follow, e.g. <7
        if (FARegexpTree::TYPE_LTRBR == Type && Pos + length != m_Length) {

            c = m_pRegexp [Pos + length];
            while (isdigit ((unsigned char) c)) {
                length++;
                if (Pos + length == m_Length) {
                    break;
                }
                c = m_pRegexp [Pos + length];
            }
        }

        *pLength = length;
        return Type;
    }

    /// see whether it is a space
    if (true == IsSpace (Pos)) {
        return -1;
    }

    bool quotation1 = false;
    bool quotation2 = false;
    bool token_started = false;

    /// it looks like we have token here
    while (true) {

        c = m_pRegexp [Pos + length];

        // see whether we have any of quatation marks
        if('"' == c && false == quotation2) {

            quotation1 = !quotation1;
            token_started = true;

            if (Pos + length + 1 < m_Length)
                length++;
            else
                break;

            continue;
        }
        if('\'' == c && false == quotation1) {

            quotation2 = !quotation2;
            token_started = true;

            if (Pos + length + 1 < m_Length)
                length++;
            else
                break;

            continue;
        }

        // just make iteration if we are in quotion mode
        if (quotation1 || quotation2) {

            if (Pos + length + 1 < m_Length)
                length++;
            else
                break;

            continue;
        }

        // break if a space or can not be a part of a token
        if (true == IsSpace (Pos + length))
            break;

        if(false == token_started) {

            if(false == IsTokenStart (Pos + length))
                break;

        } else {

            if(false == IsToken (Pos + length))
                break;
        }

        token_started = true;

        // check whether it is possible to see farther
        if (Pos + length + 1 < m_Length)
            length++;
        else
            break;

    } // of while

    // see the quotations are finished
    DebugLogAssert (false == quotation1);
    DebugLogAssert (false == quotation2);

    if (0 < length) {
        *pLength = length;
        return FARegexpTree::TYPE_SYMBOL;
    }

    return -1;
}

}
