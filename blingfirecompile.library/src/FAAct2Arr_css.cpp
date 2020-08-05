/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAAct2Arr_css.h"
#include "FAUtf8Utils.h"
#include "FATagSet.h"
#include "FAException.h"

namespace BlingFire
{


FAAct2Arr_css::FAAct2Arr_css (FAAllocatorA * pAlloc) : 
    m_pTagSet (NULL)
{
    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();
}


void FAAct2Arr_css::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}
 
inline const int FAAct2Arr_css::
    ParseWord (const char * pStr, const int Len)
{
    // skip the initial qoute
    int Pos = 1;

    const int WordLenIdx = m_arr.size ();
    m_arr.push_back (0);
    int WordLen = 0;

    while (Pos < Len && '"' != pStr [Pos]) {
        // parse escaped: \"
        if (Pos + 1 < Len && '\\' == pStr [Pos] && \
            '"' == pStr [Pos + 1]) {
            m_arr.push_back (int ('"'));
            Pos += 2;
        // parse a regular character
        } else {
            int C;
            const char * pBegin = pStr + Pos;
            const char * pEnd = FAUtf8ToInt (pBegin, &C);
            /// input encoding is not UTF-8
            FAAssert (pEnd, FAMsg::IOError);
            m_arr.push_back (C);
            Pos += int (pEnd - pBegin);
        }

        WordLen++;
    }

    // store the length
    m_arr [WordLenIdx] = WordLen;

    // skip the final qoute
    Pos++;

    return Pos;
}


inline const int FAAct2Arr_css::
    ParseScore (const char * pStr, const int Len)
{
    int To = 0;
    while (To < Len && pStr [To] != ' ' && pStr [To] != '\t' && \
           pStr [To] != '\r' && pStr [To] != '\n') {
        To++;
    }

    DebugLogAssert (0 < To);

    std::string s (pStr, To);
    const float Score = (float) atof (s.c_str ());

    // the Score is too large or too small, see FAAct2Arr_css.h for details
    FAAssert (Score >= - (INT_MAX / 127) && Score <= (INT_MAX / 127),
        FAMsg::InternalError);

    const int IntScore = int (Score * 127.0f);
    m_arr.push_back (IntScore);

    return To;
}


inline const int FAAct2Arr_css::
    ParseEditType (const char * pStr, const int Len)
{
    int To = 0;
    while (To < Len && pStr [To] != ' ' && pStr [To] != '\t' && \
           pStr [To] != '\r' && pStr [To] != '\n') {
        To++;
    }

    DebugLogAssert (0 < To);

    // tagset should be set if edit types are used
    FAAssert (m_pTagSet, FAMsg::InvalidParameters);

    // get the tag name
    const int Tag = m_pTagSet->Str2Tag (pStr, To);
    // the edit type is unknown
    FAAssert (-1 != Tag, FAMsg::InternalError);

    m_arr.push_back (Tag);

    return To;
}


const int FAAct2Arr_css::
    Process (const char * pStr, const int Len, const int ** ppArr, 
        int * pLeftCxAdjust, int * pRightCxAdjust)
{
    DebugLogAssert (ppArr && pLeftCxAdjust && pRightCxAdjust);

    *pLeftCxAdjust = 0;
    *pRightCxAdjust = 0;

    if (!pStr || 0 >= Len) {
        return -1;
    }

    m_arr.resize (0);

    int Pos = 0;

    bool HaveWord1 = false;
    bool HaveWord2 = false;
    bool HaveScore = false;
    bool HaveEditType = false;

    while (Pos < Len) {

        if (!HaveWord1 && '"' == pStr [Pos]) {

            // parse and add first word
            Pos += ParseWord (pStr + Pos, Len - Pos);
            HaveWord1 = true;

        } else if (HaveWord1 && !HaveWord2 && '"' == pStr [Pos]) {

            // parse and add second word
            Pos += ParseWord (pStr + Pos, Len - Pos);
            HaveWord2 = true;

        } else if (HaveWord1 && !HaveScore && \
                   ' ' != pStr [Pos] && '\t' != pStr [Pos]) {

            // if no second word specified then add an empty second word
            if (!HaveWord2) {
                ParseWord (NULL, 0);
                HaveWord2 = true;
            }
            // now add a score
            Pos += ParseScore (pStr + Pos, Len - Pos);
            HaveScore = true;

        } else if (HaveScore && !HaveEditType && \
                   ' ' != pStr [Pos] && '\t' != pStr [Pos]) {

            // parse and add edit type
            Pos += ParseEditType (pStr + Pos, Len - Pos);
            HaveEditType = true;

        } else {

            // skip the character
            Pos++;
        }
    }

    if (!HaveWord1 || !HaveScore) {
        return -1;
    }

    *ppArr = m_arr.begin ();
    return m_arr.size ();
}

}
