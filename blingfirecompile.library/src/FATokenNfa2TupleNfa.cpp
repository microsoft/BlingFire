/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATokenNfa2TupleNfa.h"
#include "FAMultiMapA.h"
#include "FATagSet.h"
#include "FAUtils.h"
#include "FAException.h"

namespace BlingFire
{


FATokenNfa2TupleNfa::FATokenNfa2TupleNfa (FAAllocatorA * pAlloc) :
    m_pTagSet (NULL),
    m_TnBaseIw (0),
    m_pToken2CNF (NULL),
    m_pType2Ows (NULL),
    m_TokenType (0),
    m_pOutNfa (NULL),
    m_dot_exp (pAlloc),
    m_tnnfa2nfa (pAlloc),
    m_nfas2nfa (pAlloc),
    m_is_dot (pAlloc),
    m_ExpNfa (pAlloc),
    m_TxtNfa (pAlloc),
    m_TagNfa (pAlloc),
    m_DctNfa (pAlloc)
{
    m_tnnfa2nfa.SetAnyIw (FAFsmConst::IW_ANY);
    m_tnnfa2nfa.SetTokenNfa (&m_ExpNfa);

    m_dot_exp.SetAnyIw (FAFsmConst::IW_ANY);
    m_dot_exp.SetOutNfa (&m_ExpNfa);

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();
}


FATokenNfa2TupleNfa::~FATokenNfa2TupleNfa ()
{}


void FATokenNfa2TupleNfa::Clear ()
{
    /// m_dot_exp.Clear ();  // TODO: add Clear to FAAny2AnyOther_global_t
    m_tnnfa2nfa.Clear ();
    m_nfas2nfa.Clear ();

    m_ExpNfa.Clear ();
    m_TxtNfa.Clear ();
    m_TagNfa.Clear ();
    m_DctNfa.Clear ();

    m_ows.Clear ();
    m_ows.Create ();
}


void FATokenNfa2TupleNfa::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FATokenNfa2TupleNfa::SetOutNfa (FARSNfaA * pOutNfa)
{
    m_pOutNfa = pOutNfa;
    m_nfas2nfa.SetOutNfa (pOutNfa);
}


void FATokenNfa2TupleNfa::SetTokenNfa (const FARSNfaA * pTokenNfa)
{
    m_dot_exp.SetInNfa (pTokenNfa);
}


void FATokenNfa2TupleNfa::SetTnBaseIw (const int TnBaseIw)
{
    m_TnBaseIw = TnBaseIw;
    m_tnnfa2nfa.SetTnBaseIw (TnBaseIw);
    m_dot_exp.SetIwBase (TnBaseIw);
}


void FATokenNfa2TupleNfa::SetCNF (const FAMultiMapA * pToken2CNF)
{
    m_pToken2CNF = pToken2CNF;
    m_tnnfa2nfa.SetTokenNum2CNF (pToken2CNF);
}


void FATokenNfa2TupleNfa::SetType2Ows (const FAMultiMapA * pType2Ows)
{
    m_pType2Ows = pType2Ows;
    m_tnnfa2nfa.SetTypeNum2Ows (pType2Ows);
}


void FATokenNfa2TupleNfa::SetIgnoreBase (const int IgnoreBase)
{
    m_nfas2nfa.SetIgnoreBase (IgnoreBase);
}


void FATokenNfa2TupleNfa::SetIgnoreMax (const int IgnoreMax)
{
    m_nfas2nfa.SetIgnoreMax (IgnoreMax);
}


void FATokenNfa2TupleNfa::Prepare ()
{
    FAAssert (m_pToken2CNF && 0 <= m_TnBaseIw, FAMsg::InvalidParameters);

    m_TokenType = 0;
    int MaxTokenNum = -1;

    const int * pValues;
    int Key = -1;
    int Size = m_pToken2CNF->Prev (&Key, &pValues);

    while (-1 != Size) {

        const int TokenNum = 0x00FFFFFF & Key;
        const int Digitizer = (0xFF000000 & Key) >> 24;

        // update m_TokenType
        if (FAFsmConst::DIGITIZER_TEXT == Digitizer)
            m_TokenType |= FAFsmConst::WRE_TT_TEXT;
        else if (FAFsmConst::DIGITIZER_TAGS == Digitizer)
            m_TokenType |= FAFsmConst::WRE_TT_TAGS;
        else if (FAFsmConst::DIGITIZER_DCTS == Digitizer)
            m_TokenType |= FAFsmConst::WRE_TT_DCTS;
    
        // update MaxTokenNum
        if (MaxTokenNum < TokenNum)
            MaxTokenNum = TokenNum;

        Size = m_pToken2CNF->Prev (&Key, &pValues);
    }

    FAAssert (0 <= MaxTokenNum, FAMsg::InternalError);
    FAAssert (0 != m_TokenType, FAMsg::InternalError);

    m_tnnfa2nfa.SetTnMaxIw (m_TnBaseIw + MaxTokenNum);
    m_dot_exp.SetIwMax (m_TnBaseIw + MaxTokenNum);
}


const bool FATokenNfa2TupleNfa::
    IsDotNfa (const FARSNfaA * pNfa, const int Digitizer)
{
    DebugLogAssert (pNfa);

    m_ows.resize (0);

    if (FAFsmConst::DIGITIZER_TAGS != Digitizer) {

        FAAssert (m_pType2Ows, FAMsg::InternalError);

        const int * pOws;
        int Key = -1;
        int Size = m_pType2Ows->Prev (&Key, &pOws);

        while (-1 != Size) {

            if (Digitizer == Key >> 24) {
                for (int i = 0; i < Size; ++i) {
                    const int Ow = pOws [i];
                    m_ows.push_back (Ow);
                }
            }
            Size = m_pType2Ows->Prev (&Key, &pOws);
        }

        m_ows.push_back (FAFsmConst::IW_ANY);

    } else {

        FAAssert (m_pTagSet, FAMsg::InternalError);

        const int Count = m_pTagSet->GetStrCount ();

        for (int i = 0; i < Count; ++i) {

            const int Tag = m_pTagSet->GetValue (i);
            const int Ow = Tag + m_TnBaseIw;
            m_ows.push_back (Ow);
        }
    }

    const int NewSize = FASortUniq (m_ows.begin (), m_ows.end ());
    m_ows.resize (NewSize);

    m_is_dot.SetNfa (pNfa);
    m_is_dot.SetExpIws (m_ows.begin (), NewSize);

    return m_is_dot.Process ();
}


void FATokenNfa2TupleNfa::Process ()
{
    Prepare ();

    /// expand the dot
    m_dot_exp.Process ();

    /// build Ows NFAs
    int TupleSize = 0;
    const FARSNfaA * pSingleton = NULL;

    if (FAFsmConst::WRE_TT_TAGS & m_TokenType) {

        m_tnnfa2nfa.SetDigitizer (FAFsmConst::DIGITIZER_TAGS);
        m_tnnfa2nfa.SetOutNfa (&m_TagNfa);
        m_tnnfa2nfa.Process ();

        if (!IsDotNfa (&m_TagNfa, FAFsmConst::DIGITIZER_TAGS)) {
            pSingleton = &m_TagNfa;
            TupleSize++;
        } else {
            m_TokenType &= (~FAFsmConst::WRE_TT_TAGS);
        }
    }
    if (FAFsmConst::WRE_TT_DCTS & m_TokenType) {

        m_tnnfa2nfa.SetDigitizer (FAFsmConst::DIGITIZER_DCTS);
        m_tnnfa2nfa.SetOutNfa (&m_DctNfa);
        m_tnnfa2nfa.Process ();

        if (!IsDotNfa (&m_DctNfa, FAFsmConst::DIGITIZER_DCTS)) {
            pSingleton = &m_DctNfa;
            TupleSize++;
        } else {
            m_TokenType &= (~FAFsmConst::WRE_TT_DCTS);
        }
    }
    if (FAFsmConst::WRE_TT_TEXT & m_TokenType) {

        m_tnnfa2nfa.SetDigitizer (FAFsmConst::DIGITIZER_TEXT);
        m_tnnfa2nfa.SetOutNfa (&m_TxtNfa);
        m_tnnfa2nfa.Process ();

        if (0 == TupleSize || \
            !IsDotNfa (&m_TxtNfa, FAFsmConst::DIGITIZER_TEXT)) {
            pSingleton = &m_TxtNfa;
            TupleSize++;
        } else {
            m_TokenType &= (~FAFsmConst::WRE_TT_TEXT);
        }
    }

    /// merge NFAs together
    if (1 < TupleSize) {

        if (FAFsmConst::WRE_TT_TEXT & m_TokenType)
            m_nfas2nfa.AddNfa (&m_TxtNfa);
        if (FAFsmConst::WRE_TT_TAGS & m_TokenType)
            m_nfas2nfa.AddNfa (&m_TagNfa);
        if (FAFsmConst::WRE_TT_DCTS & m_TokenType)
            m_nfas2nfa.AddNfa (&m_DctNfa);

        m_nfas2nfa.Process ();

    } else {
        DebugLogAssert (1 == TupleSize && pSingleton);
        FACopyNfa (m_pOutNfa, pSingleton);
    }

    Clear ();
}


const int FATokenNfa2TupleNfa::GetTokenType () const
{
    return m_TokenType;
}

}

