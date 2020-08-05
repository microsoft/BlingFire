/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaCreator_wre.h"
#include "FAChain2NumA.h"

namespace BlingFire
{


FANfaCreator_wre::FANfaCreator_wre (FAAllocatorA * pAlloc) :
    FANfaCreator_base (pAlloc),
    m_pToken2Num (NULL),
    m_BaseIw (0)
{
    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();
}


void FANfaCreator_wre::SetBaseIw (const int BaseIw)
{
    m_BaseIw = BaseIw;
}


void FANfaCreator_wre::SetToken2NumMap (FAChain2NumA * pToken2Num)
{
    m_pToken2Num = pToken2Num;
}


const int FANfaCreator_wre::Token2Num (const char * pStr, const int Len)
{
    DebugLogAssert (m_pToken2Num);
    DebugLogAssert (pStr);
    DebugLogAssert (0 < Len);

    /// adjust chain's size
    int ChainLen = Len / sizeof (int);

    if (Len % sizeof (int)) {

        ChainLen++;
        m_arr.resize (ChainLen);
        m_arr [ChainLen - 1] = 0;

    } else {

        m_arr.resize (ChainLen);
    }

    int * pBegin = m_arr.begin ();

    /// copy str into the int storage
    memcpy (pBegin, pStr, Len);

    /// build a numerical identifier (does not add if the chain already exists)
    const int Num = m_pToken2Num->Add (pBegin, ChainLen, 0);

    return Num;
}


void FANfaCreator_wre::SetTransition (const int FromState,
                                      const int ToState,
                                      const int LabelOffset,
                                      const int LabelLength)
{
    if (0 > LabelLength || 0 > LabelOffset) {
        FANfaCreator_base:: \
            SetTransition (FromState, ToState, LabelOffset, LabelLength);
        return;
    }

    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 < LabelLength);

    const char * pTokenStr = m_pRegexp + LabelOffset;
    const int TokenNum = Token2Num (pTokenStr, LabelLength);
    const int Iw = TokenNum + m_BaseIw;

    m_tmp_nfa.SetTransition (FromState, Iw, ToState);
}

}
