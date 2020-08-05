/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWRERules2TokenNfa.h"
#include "FAFsmConst.h"
#include "FAException.h"
#include "FAUtils.h"

namespace BlingFire
{


FAWRERules2TokenNfa::FAWRERules2TokenNfa (FAAllocatorA * pAlloc) :
    m_Type (FAFsmConst::WRE_TYPE_RS),
    m_simplify (pAlloc),
    m_rule2nfa (pAlloc),
    m_rule_nfa (pAlloc),
    m_merge (pAlloc)
{
    m_simplify.SetLabelType (FAFsmConst::LABEL_WRE);

    m_rule2nfa.SetKeepPos (true);
    m_rule2nfa.SetNfa (&m_rule_nfa);
    m_rule2nfa.SetTrBr2Iw (false);
    m_rule2nfa.SetLabelType (FAFsmConst::LABEL_WRE);

    m_rule_nfa.SetAnyIw (FAFsmConst::IW_ANY);
    m_rule_nfa.SetAnchorLIw (FAFsmConst::IW_L_ANCHOR);
    m_rule_nfa.SetAnchorRIw (FAFsmConst::IW_R_ANCHOR);

    m_merge.SetEpsilonIw (FAFsmConst::IW_EPSILON);
}


void FAWRERules2TokenNfa::Clear ()
{
    m_simplify.Clear ();
    m_rule2nfa.Clear ();
    m_rule_nfa.Clear ();
    m_merge.Clear ();
}


void FAWRERules2TokenNfa::SetType (const int Type)
{
    m_Type = Type;

    if (FAFsmConst::WRE_TYPE_MEALY == Type) {

        m_rule2nfa.SetTrBr2Iw (true);
        m_rule_nfa.SetTrBr2Iw (true);
        m_rule_nfa.SetTrBrBaseIw (100000); /// TODO: make this dynamicly
        m_rule_nfa.SetBaseIw (10000);  /// TODO: make this dynamicly
        m_merge.SetAddNfaNums (false);

    } else if (FAFsmConst::WRE_TYPE_MOORE == Type) {

        m_rule2nfa.SetTrBr2Iw (false);
        m_rule_nfa.SetTrBr2Iw (false);
        m_rule_nfa.SetTrBrBaseIw (0);
        m_rule_nfa.SetBaseIw (10000);  /// TODO: make this dynamicly
        m_merge.SetAddNfaNums (true);
        m_merge.SetNfaNumBase (FAFsmConst::IW_EPSILON + 1);

    } else {
        DebugLogAssert (FAFsmConst::WRE_TYPE_RS == Type);

        m_rule2nfa.SetTrBr2Iw (false);
        m_rule_nfa.SetTrBr2Iw (false);
        m_rule_nfa.SetTrBrBaseIw (0);
        m_rule_nfa.SetBaseIw (FAFsmConst::IW_EPSILON + 1);
        m_merge.SetAddNfaNums (false);
    }
}


void FAWRERules2TokenNfa::SetToken2NumMap (FAChain2NumA * pTokens)
{
    m_rule_nfa.SetToken2NumMap (pTokens);
}


void FAWRERules2TokenNfa::AddRule (const char * pWRE, const int Length)
{
    m_rule_nfa.Clear ();
    m_rule2nfa.Clear ();

    FAAssert (pWRE && 0 < Length, FAMsg::InvalidParameters);

    /// simplify regexp

    m_simplify.SetRegexp (pWRE, Length);
    m_simplify.Process ();
    const char * pWRE2 = m_simplify.GetRegexp ();
    const int Length2 = (const int) strlen (pWRE2);

    FAAssert (pWRE2 && 0 < Length2, FAMsg::InternalError);

    /// build NFA

    m_rule_nfa.SetRegexp (pWRE2, Length2);
    m_rule2nfa.SetRegexp (pWRE2, Length2);
    m_rule2nfa.Process ();
    m_rule_nfa.Prepare ();
    m_simplify.Clear ();
    const FARSNfaA * pNfa = m_rule_nfa.GetNfa ();

    FAAssert (FAIsValidNfa (pNfa), FAMsg::InternalError);

    /// add NFA, e.g. ENFA := ENFA | NFA

    if (FAFsmConst::WRE_TYPE_MOORE == m_Type) {
        m_merge.AddNfa (pNfa);
    }
}


void FAWRERules2TokenNfa::Process ()
{
    if (FAFsmConst::WRE_TYPE_MOORE == m_Type) {
        m_merge.Process ();
    }
}


const FARSNfaA * FAWRERules2TokenNfa::GetTokenNfa () const
{
    if (FAFsmConst::WRE_TYPE_MOORE == m_Type) {

        const FARSNfaA * pNfa = m_merge.GetCommonNfa ();
        return pNfa;

    } else {

        const FARSNfaA * pNfa = m_rule_nfa.GetNfa ();
        return pNfa;
    }
}

}
