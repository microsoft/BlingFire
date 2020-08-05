/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAAct2Arr_score_tag.h"
#include "FATagSet.h"
#include "FAException.h"

namespace BlingFire
{


FAAct2Arr_score_tag::FAAct2Arr_score_tag (FAAllocatorA * pAlloc) : 
    m_pTagSet (NULL)
{
    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();
}


void FAAct2Arr_score_tag::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


inline const int FAAct2Arr_score_tag::
    ParseScore (const char * pStr, const int Len)
{
    int To = 0;
    while (To < Len && pStr [To] != ' ' && pStr [To] != '\t' && \
           pStr [To] != '\r' && pStr [To] != '\n') {
        To++;
    }

    DebugLogAssert (0 < To);

    std::string s (pStr, To);
    const int Score = atoi (s.c_str ());

    m_arr.push_back (Score);

    return To;
}


inline const int FAAct2Arr_score_tag::
    ParseTag (const char * pStr, const int Len)
{
    int To = 0;
    while (To < Len && pStr [To] != ' ' && pStr [To] != '\t' && \
           pStr [To] != '\r' && pStr [To] != '\n') {
        To++;
    }

    DebugLogAssert (0 < To);

    // tagset should be set
    FAAssert (m_pTagSet, FAMsg::InvalidParameters);

    // get the tag name
    const int Tag = m_pTagSet->Str2Tag (pStr, To);
    // the tag is unknown
    FAAssert (-1 != Tag, FAMsg::InternalError);

    m_arr.push_back (Tag);

    return To;
}


const int FAAct2Arr_score_tag::
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

    bool HaveScore = false;
    bool HaveTag = false;

    while (Pos < Len) {

        if (!HaveScore && ' ' != pStr [Pos] && '\t' != pStr [Pos]) {

            // now add a score
            Pos += ParseScore (pStr + Pos, Len - Pos);
            HaveScore = true;

        } else if (HaveScore && !HaveTag && \
                   ' ' != pStr [Pos] && '\t' != pStr [Pos]) {

            // parse and add tag value
            Pos += ParseTag (pStr + Pos, Len - Pos);
            HaveTag = true;

        } else {

            // skip the character
            Pos++;
        }
    }
    if (!HaveTag || !HaveScore) {
        return -1;
    }

    *ppArr = m_arr.begin ();
    return m_arr.size ();
}

}
