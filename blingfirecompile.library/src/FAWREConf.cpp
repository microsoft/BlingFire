/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWREConf.h"
#include "FAFsmConst.h"
#include "FAException.h"


namespace BlingFire
{

FAWREConf::FAWREConf (FAAllocatorA * pAlloc) :
    m_Type (FAFsmConst::WRE_TYPE_RS),
    m_TokenType (FAFsmConst::WRE_TT_DEFAULT),
    m_TagOwBase (1000),
    m_txt_dig_dfa (pAlloc),
    m_txt_dig_ows (pAlloc),
    m_dfa1 (pAlloc),
    m_moore_ows1 (pAlloc),
    m_sigma1 (pAlloc),
    m_dfa2 (pAlloc),
    m_sigma2 (pAlloc)
{
    m_dct_dig_arr.SetAllocator (pAlloc);
    m_dct_dig_arr.Create ();

    m_trbr.SetAllocator (pAlloc);

    m_sigma1.SetRsDfa (&m_dfa1);
    m_sigma2.SetRsDfa (&m_dfa2);
}


FAWREConf::~FAWREConf ()
{}


void FAWREConf::Clear ()
{
    m_Type = FAFsmConst::WRE_TYPE_RS;
    m_TokenType = FAFsmConst::WRE_TT_DEFAULT;
    m_TagOwBase = 1000;

    m_txt_dig_dfa.Clear ();
    m_txt_dig_ows.Clear ();
    m_dfa1.Clear ();
    m_moore_ows1.Clear ();
    m_sigma1.Clear ();
    m_dfa2.Clear ();
    m_sigma2.Clear ();
    m_trbr.Clear ();

    m_dct_dig_arr.Clear ();
    m_dct_dig_arr.Create ();
}


void FAWREConf::SetType (const int Type)
{
    DebugLogAssert (FAFsmConst::WRE_TYPE_RS <= Type && \
        FAFsmConst::WRE_TYPE_COUNT > Type);

    m_Type = Type;
}

void FAWREConf::SetTokenType (const int TokenType)
{
    DebugLogAssert ((FAFsmConst::WRE_TT_TEXT | FAFsmConst::WRE_TT_TAGS | \
        FAFsmConst::WRE_TT_DCTS) & TokenType);

    m_TokenType = TokenType;
}

void FAWREConf::SetTagOwBase (const int TagOwBase)
{
    DebugLogAssert (0 <= TagOwBase);

    m_TagOwBase = TagOwBase;
}

void FAWREConf::
    GetTxtDigititizer (FARSDfaA ** ppDfa, FAState2OwA ** ppState2Ow)
{
    DebugLogAssert (ppDfa && ppState2Ow);

    *ppDfa = & m_txt_dig_dfa;
    *ppState2Ow = & m_txt_dig_ows;
}

void FAWREConf::SetDictDigitizer (const int * pId2Ow, const int Size)
{
    FAAssert (0 < Size && pId2Ow, FAMsg::InvalidParameters);

    m_dct_dig_arr.resize (Size);
    int * pBegin = m_dct_dig_arr.begin ();

    memcpy (pBegin, pId2Ow, sizeof (int) * Size);
    m_ca_arr.SetArray (m_dct_dig_arr.begin (), Size);
}

void FAWREConf::GetDfa1 (FARSDfaA ** ppDfa)
{
    FAAssert (ppDfa, FAMsg::InvalidParameters);
    *ppDfa = & m_dfa1;
}

void FAWREConf::GetDfa2 (FARSDfaA ** ppDfa)
{
    FAAssert (ppDfa, FAMsg::InvalidParameters);
    *ppDfa = & m_dfa2;
}

void FAWREConf::GetState2Ows (FAState2OwsA ** ppState2Ows)
{
    FAAssert (ppState2Ows, FAMsg::InvalidParameters);
    *ppState2Ows = & m_moore_ows1;
}

void FAWREConf::GetSigma1 (FAMealyDfaA ** ppSigma)
{
    FAAssert (ppSigma, FAMsg::InvalidParameters);
    *ppSigma = & m_sigma1;
}

void FAWREConf::GetSigma2 (FAMealyDfaA ** ppSigma)
{
    FAAssert (ppSigma, FAMsg::InvalidParameters);
    *ppSigma = & m_sigma2;
}

void FAWREConf::GetTrBrMap (FAMultiMapA ** ppTrBr)
{
    FAAssert (ppTrBr, FAMsg::InvalidParameters);
    *ppTrBr = & m_trbr;
}


const int FAWREConf::GetType () const
{
    return m_Type;
}

const int FAWREConf::GetTokenType () const
{
    return m_TokenType;
}

const int FAWREConf::GetTagOwBase () const
{
    return m_TagOwBase;
}

const FARSDfaCA * FAWREConf::GetTxtDigDfa () const
{
    const int MaxState = m_txt_dig_dfa.GetMaxState ();

    if (-1 != MaxState) {
        return & m_txt_dig_dfa;
    } else {
        return NULL;
    }
}

const FAState2OwCA * FAWREConf::GetTxtDigOws () const
{
    const int MaxState = m_txt_dig_dfa.GetMaxState ();

    if (-1 != MaxState) {
        return & m_txt_dig_ows;
    } else {
        return NULL;
    }
}

void FAWREConf::
    GetTxtDigititizer (
        const FARSDfaA ** ppDfa, 
        const FAState2OwA ** ppState2Ow
    ) const
{
    DebugLogAssert (ppDfa && ppState2Ow);

    const int MaxState = m_txt_dig_dfa.GetMaxState ();

    if (-1 != MaxState) {

        *ppDfa = & m_txt_dig_dfa;
        *ppState2Ow = & m_txt_dig_ows;

    } else {

        *ppDfa = NULL;
        *ppState2Ow = NULL;
    }
}

const FAArrayCA * FAWREConf::GetDictDig () const
{
    const int Size = m_dct_dig_arr.size ();

    if (0 < Size) {
        return & m_ca_arr;
    } else {
        return NULL;
    }
}

void FAWREConf::
    GetDictDigitizer (
            const int ** ppId2Ow,
            int * pId2OwSize
        ) const
{
    DebugLogAssert (ppId2Ow && pId2OwSize);

    const int Size = m_dct_dig_arr.size ();

    if (0 < Size) {

        *ppId2Ow = m_dct_dig_arr.begin ();
        *pId2OwSize = m_dct_dig_arr.size ();

    } else {

        *ppId2Ow = NULL;
        *pId2OwSize = 0;
    }
}

const FARSDfaCA * FAWREConf::GetDfa1 () const
{
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState) {
        return & m_dfa1;
    } else {
        return NULL;
    }
}

void FAWREConf::GetDfa1 (const FARSDfaA ** ppDfa) const
{
    DebugLogAssert (ppDfa);
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState) {
        *ppDfa = & m_dfa1;
    } else {
        *ppDfa = NULL;
    }
}

const FARSDfaCA * FAWREConf::GetDfa2 () const
{
    const int MaxState = m_dfa2.GetMaxState ();

    if (-1 != MaxState) {
        return & m_dfa2;
    } else {
        return NULL;
    }
}

void FAWREConf::GetDfa2 (const FARSDfaA ** ppDfa) const
{
    DebugLogAssert (ppDfa);
    const int MaxState = m_dfa2.GetMaxState ();

    if (-1 != MaxState) {
        *ppDfa = & m_dfa2;
    } else {
        *ppDfa = NULL;
    }
}

const FAState2OwsCA * FAWREConf::GetState2Ows () const
{
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MOORE == m_Type) {
        return & m_moore_ows1;
    } else {
        return NULL;
    }
}

void FAWREConf::GetState2Ows (const FAState2OwsA ** ppState2Ows) const
{
    DebugLogAssert (ppState2Ows);
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MOORE == m_Type) {
        *ppState2Ows = & m_moore_ows1;
    } else {
        *ppState2Ows = NULL;
    }
}

const FAMealyDfaCA * FAWREConf::GetSigma1 () const
{
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        return & m_sigma1;
    } else {
        return NULL;
    }
}

void FAWREConf::GetSigma1 (const FAMealyDfaA ** ppSigma) const
{
    DebugLogAssert (ppSigma);
    const int MaxState = m_dfa1.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        *ppSigma = & m_sigma1;
    } else {
        *ppSigma = NULL;
    }
}

const FAMealyDfaCA * FAWREConf::GetSigma2 () const
{
    const int MaxState = m_dfa2.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        return & m_sigma2;
    } else {
        return NULL;
    }
}

void FAWREConf::GetSigma2 (const FAMealyDfaA ** ppSigma) const
{
    DebugLogAssert (ppSigma);
    const int MaxState = m_dfa2.GetMaxState ();

    if (-1 != MaxState && FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        *ppSigma = & m_sigma2;
    } else {
        *ppSigma = NULL;
    }
}

const FAMultiMapCA * FAWREConf::GetTrBrMap () const
{
    if (FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        return & m_trbr;
    } else {
        return NULL;
    }
}

void FAWREConf::GetTrBrMap (const FAMultiMapA ** ppTrBr) const
{
    DebugLogAssert (ppTrBr);

    if (FAFsmConst::WRE_TYPE_MEALY == m_Type) {
        *ppTrBr = & m_trbr;
    } else {
        *ppTrBr = NULL;
    }
}

}
