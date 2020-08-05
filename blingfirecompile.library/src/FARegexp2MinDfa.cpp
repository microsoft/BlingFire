/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexp2MinDfa.h"
#include "FAFsmConst.h"
#include "FAUtf8Utils.h"

namespace BlingFire
{


FARegexp2MinDfa::FARegexp2MinDfa (FAAllocatorA * pAlloc) :
    m_regexp2nfa (pAlloc),
    m_nfa_char (pAlloc),
    m_dot_exp (pAlloc),
    m_nfa (pAlloc),
    m_nfa2dfa (pAlloc),
    m_dfa (pAlloc),
    m_dfa2mindfa (pAlloc),
    m_min_dfa (pAlloc)
{
    m_nfa_char.SetAnyIw (FAFsmConst::IW_ANY);
    m_nfa_char.SetAnchorLIw (FAFsmConst::IW_L_ANCHOR);
    m_nfa_char.SetAnchorRIw (FAFsmConst::IW_R_ANCHOR);
    m_nfa_char.SetTrBr2Iw (false);
    m_nfa_char.SetTrBrBaseIw (0);

    m_regexp2nfa.SetTrBr2Iw (false);
    m_regexp2nfa.SetKeepPos (false);
    m_regexp2nfa.SetNfa (&m_nfa_char);
    m_regexp2nfa.SetLabelType (FAFsmConst::LABEL_CHAR);

    m_dot_exp.SetAnyIw (FAFsmConst::IW_ANY);
    m_dot_exp.SetOutNfa (&m_nfa);

    m_nfa2dfa.SetNFA (&m_nfa);
    m_nfa2dfa.SetDFA (&m_dfa);

    m_dfa2mindfa.SetInDfa (&m_dfa);
    m_dfa2mindfa.SetOutDfa (&m_min_dfa);
}


void FARegexp2MinDfa::SetEncodingName (const char * pEncStr)
{
    m_regexp2nfa.SetUseUtf8 (FAIsUtf8Enc (pEncStr));
    m_nfa_char.SetEncodingName (pEncStr);
}


void FARegexp2MinDfa::SetRegexp (const char * pRegexp, const int Length)
{
    m_nfa_char.SetRegexp (pRegexp, Length);
    m_regexp2nfa.SetRegexp (pRegexp, Length);
}


///
/// This function Clears containers and processors as soon as possible
/// in order to minimize memory usage.
///
void FARegexp2MinDfa::Process ()
{
    // empty the output
    m_min_dfa.Clear ();

    // build an Nfa from a Re
    m_regexp2nfa.Process ();
    m_regexp2nfa.Clear ();

    m_nfa_char.Prepare ();
    const FARSNfaA * pNfa = m_nfa_char.GetNfa ();
    DebugLogAssert (pNfa);

    // make global expansion for the '.'-symbol
    m_dot_exp.SetInNfa (pNfa);
    m_dot_exp.Process ();
    /// m_dot_exp.Clear ();  // TODO: add Clear to FAAny2AnyOther_global_t
    m_nfa_char.Clear ();

    // build DFA
    m_nfa2dfa.Process ();
    m_nfa.Clear ();
    m_nfa2dfa.Clear ();

    // add one more state, to be used as a dead
    const int MaxState = m_dfa.GetMaxState ();
    m_dfa.SetMaxState (MaxState + 1);

    // build Min DFA
    m_dfa2mindfa.Process ();
    m_dfa.Clear ();
    m_dfa2mindfa.Clear ();
}


const FARSDfaA * FARegexp2MinDfa::GetRsDfa () const
{
    return & m_min_dfa;
}

}
