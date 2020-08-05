/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAAct2Arr_data.h"
#include "FATagSet.h"
#include "FAStringTokenizer.h"
#include "FAException.h"

namespace BlingFire
{


FAAct2Arr_data::FAAct2Arr_data (FAAllocatorA * pAlloc) : 
    m_pTagSet (NULL)
{
    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();

    m_data.SetAllocator (pAlloc);
    m_data.Create ();

    m_data2tag.SetAllocator (pAlloc);
}


void FAAct2Arr_data::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


const int FAAct2Arr_data::
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
    m_data.resize (0);

    FAStringTokenizer tokenizer;
    tokenizer.SetString (pStr, Len);

    const char * pTmpStr;
    int TmpStrLen;

    int LeftCxAdjust = 0;
    int RightCxAdjust = 0;
    int DataTag = 0;

    m_arr.push_back (DataTag); // DataTag is always present

    // keyword _call was used
    bool fCallUsed = false;

    /// iterate thru the tokens of the action
    while (tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {

        // skip the "_call" keyword
        if (5 == TmpStrLen && 0 == strncmp ("_call", pTmpStr, TmpStrLen)) {

            // bad action format _call is used twice
            FAAssert (false == fCallUsed, FAMsg::InternalError);
            // add a delimiter
            m_arr.push_back (0);
            fCallUsed = true;

        // check for _main "function" call
        } else if (5 == TmpStrLen && 0 == strncmp ("_main", pTmpStr, TmpStrLen)) {

            // bad action format: _main is used before _call
            FAAssert (true == fCallUsed, FAMsg::InternalError);
            // add a FnTag 0
            m_arr.push_back (0);

        // check for left context bracket adjustment: e.g. <-3 <+100
        } else if ( 3 <= TmpStrLen && '<' == pTmpStr [0] &&
             ('+' == pTmpStr [1] || '-' == pTmpStr [1]) &&
             ('0' <= pTmpStr [2] && '9' >= pTmpStr [2])) {

            // for the run-time bigger values mean narrower bracket context
            LeftCxAdjust = - atoi (pTmpStr+1);

        // check for right context bracket adjustment: e.g. >-3 >+100
        } else if ( 3 <= TmpStrLen && '>' == pTmpStr [0] &&
            ('+' == pTmpStr [1] || '-' == pTmpStr [1]) &&
            ('0' <= pTmpStr [2] && '9' >= pTmpStr [2])) {

            // for the run-time bigger values mean narrower bracket context
            RightCxAdjust = - atoi (pTmpStr+1);

        // see if the data is a number (no _call keyword has been used)
        } else if (false == fCallUsed && 0 < TmpStrLen && 
            ('0' <= pTmpStr [0] && '9' >= pTmpStr [0])) {

            // a number for data
            const int Num = atoi (pTmpStr);
            m_data.push_back (Num);

        // it have to be a known tag
        } else {
            FAAssert (m_pTagSet, FAMsg::InternalError);
            // see if we have a known tag
            const int Tag = m_pTagSet->Str2Tag (pTmpStr, TmpStrLen);
            FAAssert (-1 != Tag, FAMsg::InternalError);
            if (false == fCallUsed) {
                // the tag is a part of data, it goes into m_data array
                m_data.push_back (Tag);
            } else {
                // the tag is a function name, it goes into m_arr array
                m_arr.push_back (Tag);
            }
        }

    } // of while (tokenizer.GetNextStr ...

    // see if there were any data
    if (0 < m_data.size ()) {
        // map the data into a unique tag 
        const int Id = m_data2tag.Add (m_data.begin (), m_data.size (), 0);
        FAAssert (0 <= Id, FAMsg::InternalError);
        // add one as all the ids are 0-based and tags are 1-based
        m_arr [0] = 1 + Id;
    }

    // return the results
    *pLeftCxAdjust = LeftCxAdjust;
    *pRightCxAdjust = RightCxAdjust;
    *ppArr = m_arr.begin ();

    return m_arr.size ();
}


const int FAAct2Arr_data::GetDataCount () const
{
    return m_data2tag.GetChainCount ();
}

const int FAAct2Arr_data::GetData (const int Idx, const int ** ppData) const
{
    return m_data2tag.GetChain (Idx, ppData);
}

}
