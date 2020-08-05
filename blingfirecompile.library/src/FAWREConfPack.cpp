/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWREConfPack.h"
#include "FAWREConfA.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FARSDfaA.h"
#include "FAException.h"


namespace BlingFire
{

FAWREConfPack::FAWREConfPack (FAAllocatorA * pAlloc) :
    m_pWre (NULL),
    m_pack_txt_dig (pAlloc),
    m_pack_dct_dig (pAlloc),
    m_pack_fsm1 (pAlloc),
    m_pack_fsm2 (pAlloc),
    m_pack_trbr (pAlloc)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();

    m_conf.SetAllocator (pAlloc);
    m_conf.Create ();

    m_pack_trbr.SetSizeOfValue (sizeof (int));
}


void FAWREConfPack::SetWre (const FAWREConfA * pWre)
{
    m_pWre = pWre;
}


const int FAWREConfPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_dump.begin ();
    return m_dump.size ();
}


void FAWREConfPack::Prepare ()
{
    m_dump.Clear ();
    m_dump.Create ();
}


const int FAWREConfPack::BuildDumps ()
{
    DebugLogAssert (m_pWre);

    const unsigned char * pTmpDump;
    int TmpSize;

    // initial output dump size
    int Size = sizeof (int) * FAFsmConst::WRE_CONF_COUNT;

    // set up conf
    m_conf.resize (FAFsmConst::WRE_CONF_COUNT);
    memset (m_conf.begin (), 0, sizeof (int) * FAFsmConst::WRE_CONF_COUNT);

    /// get conf
    const int WreType = m_pWre->GetType ();
    const int TokenType = m_pWre->GetTokenType ();
    const int TagOwBase = m_pWre->GetTagOwBase ();

    m_conf [FAFsmConst::WRE_CONF_WRE_TYPE] = WreType;
    m_conf [FAFsmConst::WRE_CONF_TOKEN_TYPE] = TokenType;
    m_conf [FAFsmConst::WRE_CONF_TAG_OW_BASE] = TagOwBase;

    /// pack text digitizer, if any
    const FARSDfaA * pDfa;
    const FAState2OwA * pState2Ow;
    m_pWre->GetTxtDigititizer (&pDfa, &pState2Ow);

    if (pDfa && pState2Ow) {
        m_conf [FAFsmConst::WRE_CONF_TXT_DIG] = Size;

        FAAssert (pDfa && pState2Ow, FAMsg::InternalError);
        FAAssert (FAIsValidDfa (pDfa), FAMsg::InternalError);
        m_pack_txt_dig.SetDfa (pDfa);
        m_pack_txt_dig.SetState2Ow (pState2Ow);
        m_pack_txt_dig.SetUseIwIA (true);
        m_pack_txt_dig.SetRemapIws (true);
        m_pack_txt_dig.SetUseRanges (true);
        m_pack_txt_dig.Process ();

        TmpSize = m_pack_txt_dig.GetDump (&pTmpDump);
        FAAssert (0 < TmpSize, FAMsg::InternalError);
        Size += TmpSize;
    }

    /// pack dict digitizer, if any
    const int * pArr;
    int ArrSize;
    m_pWre->GetDictDigitizer (&pArr, &ArrSize);

    if (pArr && 0 < ArrSize) {
        m_conf [FAFsmConst::WRE_CONF_DCT_DIG] = Size;

        FAAssert (pArr && 0 < ArrSize, FAMsg::InternalError);
        m_pack_dct_dig.SetArray (pArr, ArrSize);
        m_pack_dct_dig.Process ();

        TmpSize = m_pack_dct_dig.GetDump (&pTmpDump);
        FAAssert (0 < TmpSize, FAMsg::InternalError);
        Size += TmpSize;
    }

    m_pWre->GetDfa1 (&pDfa);

    if (pDfa) {
        FAAssert (FAIsValidDfa (pDfa), FAMsg::InternalError);
        m_conf [FAFsmConst::WRE_CONF_FSM1] = Size;

        if (FAFsmConst::WRE_TYPE_RS == WreType) {

            m_pack_fsm1.SetDfa (pDfa);
            m_pack_fsm1.SetUseIwIA (true);
            m_pack_fsm1.SetRemapIws (true);
            m_pack_fsm1.SetUseRanges (true);
            m_pack_fsm1.Process ();

        } else if (FAFsmConst::WRE_TYPE_MOORE == WreType) {

            const FAState2OwsA * pState2Ows;
            m_pWre->GetState2Ows (&pState2Ows);
            FAAssert (pState2Ows, FAMsg::InternalError);

            m_pack_fsm1.SetDfa (pDfa);
            m_pack_fsm1.SetState2Ows (pState2Ows);
            m_pack_fsm1.SetUseIwIA (true);
            m_pack_fsm1.SetRemapIws (true);
            m_pack_fsm1.SetUseRanges (true);
            m_pack_fsm1.Process ();

        } else if (FAFsmConst::WRE_TYPE_MEALY == WreType) {

            const FAMealyDfaA * pSigma;
            m_pWre->GetSigma1 (&pSigma);

            if (pSigma) {

                m_pack_fsm1.SetDfa (pDfa);
                m_pack_fsm1.SetSigma (pSigma);
                m_pack_fsm1.SetUseIwIA (false);
                m_pack_fsm1.SetRemapIws (false);
                m_pack_fsm1.SetUseRanges (false);
                m_pack_fsm1.Process ();

            } else {
                // trivial Mealy, use --type=rs for compilation
                FAAssert (!m_pWre->GetTrBrMap () && !m_pWre->GetDfa2 () && \
                    !m_pWre->GetSigma2 (), FAMsg::InternalError);
            }
        } else {
            // incorrect WreType
            FAAssert (0, FAMsg::InternalError);
        }

        TmpSize = m_pack_fsm1.GetDump (&pTmpDump);
        FAAssert (0 < TmpSize, FAMsg::InternalError);
        Size += TmpSize;
    }

    /// pack rules2 and trbr map, if any
    if (FAFsmConst::WRE_TYPE_MEALY == WreType) {

        m_pWre->GetDfa2 (&pDfa);
        const FAMealyDfaA * pSigma;
        m_pWre->GetSigma2 (&pSigma);
        const FAMultiMapA * pTrBr;
        m_pWre->GetTrBrMap (&pTrBr);

        if (pDfa && pSigma) {
            FAAssert (FAIsValidDfa (pDfa), FAMsg::InternalError);
            m_conf [FAFsmConst::WRE_CONF_FSM2] = Size;

            m_pack_fsm2.SetDfa (pDfa);
            m_pack_fsm2.SetSigma (pSigma);
            m_pack_fsm2.SetUseIwIA (false);
            m_pack_fsm2.SetRemapIws (false);
            m_pack_fsm2.Process ();

            TmpSize = m_pack_fsm2.GetDump (&pTmpDump);
            FAAssert (0 < TmpSize, FAMsg::InternalError);
            Size += TmpSize;
        }
        if (pTrBr) {
            FAAssert (!FAIsEmpty (pTrBr), FAMsg::InternalError);
            m_conf [FAFsmConst::WRE_CONF_TRBR] = Size;

            m_pack_trbr.SetMultiMap (pTrBr);
            m_pack_trbr.Process ();

            TmpSize = m_pack_trbr.GetDump (&pTmpDump);
            FAAssert (0 < TmpSize, FAMsg::InternalError);
            Size += TmpSize;
        }
    }

    return Size;
}


void FAWREConfPack::Process ()
{
    FAAssert (m_pWre, FAMsg::InvalidParameters);

    const unsigned char * pTmpDump;
    int TmpSize;

    Prepare ();

    const int Size = BuildDumps ();

    m_dump.resize (Size);
    unsigned char * pOut = m_dump.begin ();
    const unsigned char * pEnd = m_dump.end ();

    TmpSize = sizeof (int) * m_conf.size ();
    FAAssert (pOut + TmpSize <= pEnd, FAMsg::InternalError);
    memcpy (pOut, m_conf.begin (), TmpSize);
    pOut += TmpSize;

    if (0 != m_conf [FAFsmConst::WRE_CONF_TXT_DIG]) {
        TmpSize = m_pack_txt_dig.GetDump (&pTmpDump);
        DebugLogAssert (pOut + TmpSize <= pEnd && 0 < TmpSize && pTmpDump);
        memcpy (pOut, pTmpDump, TmpSize);
        pOut += TmpSize;
    }
    if (0 != m_conf [FAFsmConst::WRE_CONF_DCT_DIG]) {
        TmpSize = m_pack_dct_dig.GetDump (&pTmpDump);
        DebugLogAssert (pOut + TmpSize <= pEnd && 0 < TmpSize && pTmpDump);
        memcpy (pOut, pTmpDump, TmpSize);
        pOut += TmpSize;
    }
    if (0 != m_conf [FAFsmConst::WRE_CONF_FSM1]) {
        TmpSize = m_pack_fsm1.GetDump (&pTmpDump);
        DebugLogAssert (pOut + TmpSize <= pEnd && 0 < TmpSize && pTmpDump);
        memcpy (pOut, pTmpDump, TmpSize);
        pOut += TmpSize;
    }
    if (0 != m_conf [FAFsmConst::WRE_CONF_FSM2]) {
        TmpSize = m_pack_fsm2.GetDump (&pTmpDump);
        DebugLogAssert (pOut + TmpSize <= pEnd && 0 < TmpSize && pTmpDump);
        memcpy (pOut, pTmpDump, TmpSize);
        pOut += TmpSize;
    }
    if (0 != m_conf [FAFsmConst::WRE_CONF_TRBR]) {
        TmpSize = m_pack_trbr.GetDump (&pTmpDump);
        DebugLogAssert (pOut + TmpSize <= pEnd && 0 < TmpSize && pTmpDump);
        memcpy (pOut, pTmpDump, TmpSize);
        pOut += TmpSize;
    }
}

}
