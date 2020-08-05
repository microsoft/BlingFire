/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWRECompiler.h"
#include "FAFsmConst.h"
#include "FAException.h"
#include "FAUtils.h"

/*
#ifndef NDEBUG

#include <fstream>
#include "FAAutIOTools.h"
#include "FAAllocator.h"

#define FADebugTee1(FN, pX) \
    { \
        FAAllocator alloc; \
        FAAutIOTools io (&alloc); \
        std::ofstream ofs (FN, std::ios::out); \
        io.Print (ofs, pX); \
        ofs.close (); \
    }

#define FADebugTee2(FN, pX, pY) \
    { \
        FAAllocator alloc; \
        FAAutIOTools io (&alloc); \
        std::ofstream ofs (FN, std::ios::out); \
        io.Print (ofs, pX, pY); \
        ofs.close (); \
    }

#else

#define FADebugTee1(FN, pX)
#define FADebugTee2(FN, pX, pY)

#endif
*/

namespace BlingFire
{


FAWRECompiler::FAWRECompiler (FAAllocatorA * pAlloc) :
    m_Type (FAFsmConst::WRE_TYPE_RS),
    m_pDfa1 (NULL),
    m_pDfa2 (NULL),
    m_pMooreOws (NULL),
    m_pSigma1 (NULL),
    m_pSigma2 (NULL),
    m_pTrBrMap (NULL),
    m_wre2tnnfa (pAlloc),
    m_pToken2Num (NULL),
    m_tns2dgs (pAlloc),
    m_pCNF (NULL),
    m_pType2Ows (NULL),
    m_tnnfa2nfa (pAlloc),
    m_nfa (pAlloc),
    m_e_removal (pAlloc),
    m_nfa_rw (pAlloc),
    m_nfa2dfa (pAlloc),
    m_dfa (pAlloc),
    m_dfa2mindfa (pAlloc),
    m_min_dfa (pAlloc),
    m_renum (pAlloc),
    m_renum_dfa (pAlloc),
    m_rs2moore (pAlloc),
    m_dfa1 (pAlloc),
    m_moore_ows (pAlloc),
    m_nfa2mealy (pAlloc),
    m_mealy_ows (pAlloc),
    m_unamb (pAlloc),
    m_nfa2dfa_mealy (pAlloc),
    m_sigma1 (pAlloc),
    m_dfa2 (pAlloc),
    m_sigma2 (pAlloc),
    m_iw_classify (pAlloc),
    m_dfa2_min (pAlloc),
    m_sigma2_min (pAlloc)
{
    m_token2num.SetAllocator (pAlloc);
    m_token2num.SetCopyChains (true);

    m_cnf.SetAllocator (pAlloc);

    m_pToken2Num = &m_token2num;
    m_pCNF = &m_cnf;
    m_pType2Ows = m_tns2dgs.GetType2Ows ();

    m_wre2tnnfa.SetToken2NumMap (m_pToken2Num);
    m_tns2dgs.SetTokens (m_pToken2Num);
    m_tns2dgs.SetToken2CNF (m_pCNF);
    m_tns2dgs.SetEncodingName ("UTF-8");

    m_tnnfa2nfa.SetCNF (m_pCNF);
    m_tnnfa2nfa.SetType2Ows (m_pType2Ows);
    m_tnnfa2nfa.SetOutNfa (&m_nfa);

    m_e_removal.SetEpsilonIw (FAFsmConst::IW_EPSILON);
    m_e_removal.SetInNfa (&m_nfa);
    m_e_removal.SetOutNfa (&m_nfa_rw);

    m_nfa2dfa.SetNFA (&m_nfa);
    m_nfa2dfa.SetDFA (&m_dfa);

    m_dfa2mindfa.SetInDfa (&m_dfa);
    m_dfa2mindfa.SetOutDfa (&m_min_dfa);

    m_renum.SetDfa (&m_min_dfa);
    m_renum_dfa.SetOldDfa (&m_min_dfa);

    m_rs2moore.SetKeepOws (false);
    m_rs2moore.SetRSDfa (&m_renum_dfa);
    m_rs2moore.SetMooreDfa (&m_dfa1);
    m_rs2moore.SetState2Ows (&m_moore_ows);

    m_nfa2mealy.SetEpsilonIw (FAFsmConst::IW_EPSILON);
    m_nfa2mealy.SetInNfa (&m_nfa_rw);
    m_nfa2mealy.SetOutNfa (&m_nfa_rw, &m_mealy_ows);

    m_unamb.SetLess (&m_trbr_less);
    m_unamb.SetInMealy (&m_nfa_rw, &m_mealy_ows);

    m_nfa2dfa_mealy.SetUseBiMachine (true); /// always use bi-machine, for now
    m_nfa2dfa_mealy.SetOutFsm1 (&m_dfa1, &m_sigma1);
    m_nfa2dfa_mealy.SetOutFsm2 (&m_dfa2, &m_sigma2);

    m_iw_classify.SetIwBase (0);
    m_iw_classify.SetNewIwBase (0);
    m_iw_classify.SetIw2NewIw (&m_old2new);
    m_iw_classify.SetRsDfa (&m_dfa2);
    m_iw_classify.SetDfaSigma (&m_sigma2);
}


FAWRECompiler::~FAWRECompiler ()
{}


void FAWRECompiler::SetType (const int Type)
{
    FAAssert (FAFsmConst::WRE_TYPE_RS <= Type && \
              FAFsmConst::WRE_TYPE_COUNT > Type, FAMsg::InvalidParameters);

    m_wre2tnnfa.SetType (Type);
    m_Type = Type;

    if (FAFsmConst::WRE_TYPE_MEALY == Type) {

        m_tns2dgs.SetInitialOw (10000); /// TODO: make this dynamicly
        m_tns2dgs.SetInitialTypeIw (65535); /// TODO: make this dynamicly
        m_tnnfa2nfa.SetTnBaseIw (10000);  /// TODO: make this dynamicly
        m_tnnfa2nfa.SetIgnoreBase (100000);  /// TODO: make this dynamicly
        m_tnnfa2nfa.SetIgnoreMax (200000);  /// TODO: make this dynamicly

        m_nfa2dfa.SetNFA (&m_nfa); // no epsilons

        m_nfa2mealy.SetTrBrBaseIw (100000); /// TODO: make this dynamicly
        m_nfa2mealy.SetTrBrMaxIw (200000); /// TODO: make this dynamicly

    } else if (FAFsmConst::WRE_TYPE_MOORE == Type) {

        m_tns2dgs.SetInitialOw (10000); /// TODO: make this dynamicly
        m_tns2dgs.SetInitialTypeIw (65535); /// TODO: make this dynamicly
        m_tnnfa2nfa.SetTnBaseIw (10000); /// TODO: make this dynamicly
        m_tnnfa2nfa.SetIgnoreBase (-1);
        m_tnnfa2nfa.SetIgnoreMax (-1);

        m_nfa2dfa.SetNFA (&m_nfa_rw); // e-removed
        m_rs2moore.SetOwsRange (FAFsmConst::IW_EPSILON + 1, 9999); /// TODO: make this dynamicly

    } else {
        DebugLogAssert (FAFsmConst::WRE_TYPE_RS == Type);

        m_tns2dgs.SetInitialOw (FAFsmConst::IW_EPSILON + 1);
        m_tns2dgs.SetInitialTypeIw (65535); /// TODO: make this dynamicly
        m_tnnfa2nfa.SetTnBaseIw (FAFsmConst::IW_EPSILON + 1);
        m_tnnfa2nfa.SetIgnoreBase (-1);
        m_tnnfa2nfa.SetIgnoreMax (-1);

        m_nfa2dfa.SetNFA (&m_nfa);  // no epsilons
    }
}


void FAWRECompiler::SetTagSet (const FATagSet * pTagSet)
{
    m_tns2dgs.SetTagSet (pTagSet);
    m_tnnfa2nfa.SetTagSet (pTagSet);
}

void FAWRECompiler::SetTagSet2 (const FATagSet * pTagSet2)
{
    m_tns2dgs.SetTagSet2 (pTagSet2);
}

void FAWRECompiler::SetTagDict (const FADictInterpreter_t < int > * pTagDict)
{
    m_tns2dgs.SetTagDict (pTagDict);
}

void FAWRECompiler::SetDictRoot (const char * pDictRoot)
{
    m_tns2dgs.SetDictRoot (pDictRoot);
}

void FAWRECompiler::SetEncodingName (const char * pEncName)
{
    if (pEncName) {
        m_tns2dgs.SetEncodingName (pEncName);
    } else {
        m_tns2dgs.SetEncodingName ("UTF-8");
    }
}


void FAWRECompiler::SetTokens (FAChain2NumA * pTokens)
{
    if (pTokens) {
        m_pToken2Num = pTokens;
    } else {
        m_pToken2Num = &m_token2num;
    }

    m_wre2tnnfa.SetToken2NumMap (m_pToken2Num);
    m_tns2dgs.SetTokens (m_pToken2Num);
}

void FAWRECompiler::SetCNF (FAMultiMapA * pTokenNum2CNF)
{
    if (pTokenNum2CNF) {
        m_pCNF = pTokenNum2CNF;
    } else {
        m_pCNF = &m_cnf;
    }

    m_tns2dgs.SetToken2CNF (m_pCNF);
    m_tnnfa2nfa.SetCNF (m_pCNF);
}

void FAWRECompiler::SetType2Ows (const FAMultiMapA * pType2Ows)
{
    if (pType2Ows) {
        m_pType2Ows = pType2Ows;
    } else {
        m_pType2Ows = m_tns2dgs.GetType2Ows ();
    }

    m_tnnfa2nfa.SetType2Ows (m_pType2Ows);
}


void FAWRECompiler::AddRule (const char * pWRE, const int Length)
{
    FAAssert (pWRE && 0 < Length, FAMsg::InvalidParameters);
    m_wre2tnnfa.AddRule (pWRE, Length);
}


void FAWRECompiler::Process ()
{
    // external digitizer is incorrectly specified
    FAAssert (
        (
            m_pToken2Num != &m_token2num &&
            m_pCNF != &m_cnf &&
            m_pType2Ows != m_tns2dgs.GetType2Ows ()
        ) || \
        (
            m_pToken2Num == &m_token2num &&
            m_pCNF == &m_cnf &&
            m_pType2Ows == m_tns2dgs.GetType2Ows ()
        ),
            FAMsg::InvalidParameters);

    // WRE --> TnNfa
    m_wre2tnnfa.Process ();
    const FARSNfaA * pTokenNfa = m_wre2tnnfa.GetTokenNfa ();
    DebugLogAssert (FAIsValidNfa (pTokenNfa));

    // Tokens --> Digitizers, if not external
    if (m_pToken2Num == &m_token2num) {
        m_tns2dgs.Process ();
    }

    // Save dictionary digitizer array pointer, if any
    const int * pArr;
    int ArrSize;

    if (m_pToken2Num == &m_token2num && \
        m_tns2dgs.GetDictDigitizer (&pArr, &ArrSize)) {
        m_dct_arr.SetArray (pArr, ArrSize);
    } else {
        m_dct_arr.SetArray (NULL, 0);
    }

    // TnNfa --> Nfa
    m_tnnfa2nfa.SetTokenNfa (pTokenNfa);
    m_tnnfa2nfa.Process ();
    DebugLogAssert (FAIsValidNfa (&m_nfa));

    // build different types of Dfa
    if (FAFsmConst::WRE_TYPE_MEALY == m_Type) {

        BuildMealy ();

    } else if (FAFsmConst::WRE_TYPE_MOORE == m_Type) {

        BuildMoore ();

    } else {
        DebugLogAssert (FAFsmConst::WRE_TYPE_RS == m_Type);

        BuildRs ();
    }
}


void FAWRECompiler::BuildRs ()
{
    // Nfa --> Dfa
    m_nfa2dfa.Process ();
    m_nfa.Clear ();
    m_nfa2dfa.Clear ();

    // add one more state, to be used as a dead
    const int MaxState = m_dfa.GetMaxState ();
    m_dfa.SetMaxState (MaxState + 1);
    DebugLogAssert (FAIsValidDfa (&m_dfa));

    // Dfa --> Min Dfa
    m_dfa2mindfa.Process ();
    m_dfa.Clear ();
    m_dfa2mindfa.Clear ();
    DebugLogAssert (FAIsValidDfa (&m_min_dfa));

    // Min Dfa --> Min Dfa no gaps
    m_renum.Process ();
    const int * pOld2New = m_renum.GetOld2NewMap ();
    DebugLogAssert (pOld2New);
    m_renum_dfa.SetOldDfa (&m_min_dfa);
    m_renum_dfa.SetOld2New (pOld2New);
    m_renum_dfa.Prepare ();
    DebugLogAssert (FAIsValidDfa (&m_renum_dfa));

    m_pDfa1 = &m_renum_dfa;
}


void FAWRECompiler::BuildMoore ()
{
    // eNfa --> Nfa
    m_e_removal.Process ();
    m_nfa.Clear ();
    DebugLogAssert (FAIsValidNfa (&m_nfa_rw));

    // Nfa --> Dfa
    m_nfa2dfa.Process ();
    m_nfa_rw.Clear ();
    m_nfa2dfa.Clear ();
    DebugLogAssert (FAIsValidDfa (&m_dfa));

    // add one more state, to be used as a dead
    const int MaxState = m_dfa.GetMaxState ();
    m_dfa.SetMaxState (MaxState + 1);

    // Dfa --> Min Dfa
    m_dfa2mindfa.Process ();
    m_dfa.Clear ();
    m_dfa2mindfa.Clear ();
    DebugLogAssert (FAIsValidDfa (&m_min_dfa));

    // Min Dfa --> Min Dfa no gaps
    m_renum.Process ();
    const int * pOld2New = m_renum.GetOld2NewMap ();
    DebugLogAssert (pOld2New);
    m_renum_dfa.SetOldDfa (&m_min_dfa);
    m_renum_dfa.SetOld2New (pOld2New);
    m_renum_dfa.Prepare ();
    DebugLogAssert (FAIsValidDfa (&m_renum_dfa));

    // Rs Dfa --> Moore Dfa
    m_rs2moore.Process ();
    m_min_dfa.Clear ();
    m_renum_dfa.Clear ();
    DebugLogAssert (FAIsValidDfa (&m_dfa1));

    m_pDfa1 = &m_dfa1;
    m_pMooreOws = &m_moore_ows;
}


void FAWRECompiler::BuildMealy ()
{
    // Nfa --> Min Rs Dfa
    FAWRECompiler::BuildRs ();

    // Rs Nfa --> Mealy Nfa + TrBr map
    FACopyDfa2Nfa (&m_nfa_rw, &m_renum_dfa);
    m_nfa2mealy.Process ();
    m_pTrBrMap = m_nfa2mealy.GetOw2TrBrMap ();

    // check whether map is empty, e.g. no brackets
    if (FAIsEmpty (m_pTrBrMap)) {

        m_pTrBrMap = NULL;
        m_pDfa1 = NULL;
        m_pSigma1 = NULL;

    } else {
        // no longer needed
        m_renum_dfa.Clear ();

        // Mealy Nfa --> functional Mealy Nfa
        m_trbr_less.SetTrBrMap (m_pTrBrMap);
        m_unamb.Process ();
        m_nfa_rw.Clear ();
        m_mealy_ows.Clear ();

        // Mealy Nfa --> Mealy Dfa(s)
        const FARSNfaA * pNfa = m_unamb.GetNfa ();
        DebugLogAssert (FAIsValidNfa (pNfa));
        const FAMealyNfaA * pSigma = m_unamb.GetSigma ();
        DebugLogAssert (pSigma);
        m_nfa2dfa_mealy.SetInNfa (pNfa, pSigma);
        m_nfa2dfa_mealy.Process ();
        m_nfa2dfa_mealy.Clear ();
        m_unamb.Clear ();
        DebugLogAssert (FAIsValidDfa (&m_dfa1));

        m_pDfa1 = &m_dfa1;
        m_pSigma1 = &m_sigma1;

        // make mutual alphabet minimization if Fsm2 exists
        if (-1 != m_dfa2.GetMaxState ()) {

            // calculate Iw equivalence classes of m_dfa2
            DebugLogAssert (FAIsValidDfa (&m_dfa2));
            m_iw_classify.SetIwMax (m_dfa2.GetMaxIw ());
            m_iw_classify.Process ();

            // remap Sigma2
            FARemapMealySigma2 (&m_dfa2, &m_sigma2, \
                &m_sigma2_min, &m_old2new);
            m_sigma2.Clear ();

            // remap Dfa2
            FARemapRsFsmIws (&m_dfa2, &m_dfa2_min, &m_old2new);
            m_dfa2.Clear ();
            DebugLogAssert (FAIsValidDfa (&m_dfa2_min));

            // remap Sigma1 in-place
            FARemapMealySigma1 (&m_dfa1, &m_sigma1, &m_sigma1, &m_old2new);
            m_old2new.Clear ();

            m_pDfa2 = &m_dfa2_min;
            m_pSigma2 = &m_sigma2_min;
        }
    } // of if (FAIsEmpty (m_pTrBrMap)) ... else ...
}


void FAWRECompiler::Clear ()
{
    m_pDfa1 = NULL;
    m_pDfa2 = NULL;
    m_pMooreOws = NULL;
    m_pSigma1 = NULL;
    m_pSigma2 = NULL;
    m_pTrBrMap = NULL;

    m_token2num.Clear ();
    m_cnf.Clear ();
    m_nfa.Clear ();
    m_e_removal.Clear ();
    m_nfa_rw.Clear ();
    m_nfa2dfa.Clear ();
    m_dfa.Clear ();
    m_dfa2mindfa.Clear ();
    m_min_dfa.Clear ();
    m_renum.Clear ();
    m_renum_dfa.Clear ();
    m_rs2moore.Clear ();
    m_dfa1.Clear ();
    m_moore_ows.Clear ();
    m_mealy_ows.Clear ();
    m_unamb.Clear ();
    m_nfa2dfa_mealy.Clear ();
    m_sigma1.Clear ();
    m_dfa2.Clear ();
    m_sigma2.Clear ();
    m_old2new.Clear ();
    m_dfa2_min.Clear ();
    m_sigma2_min.Clear ();

    m_dct_arr.SetArray (NULL, 0);
}


const int FAWRECompiler::GetType () const
{
    return m_Type;
}

const int FAWRECompiler::GetTokenType () const
{
    return m_tnnfa2nfa.GetTokenType ();
}

const int FAWRECompiler::GetTagOwBase () const
{
    return 10000; // TODO: make this dynamic
}

void FAWRECompiler::
    GetTxtDigititizer (
            const FARSDfaA ** ppDfa, 
            const FAState2OwA ** ppState2Ow
        ) const
{
    DebugLogAssert (ppDfa && ppState2Ow);

    if (m_pToken2Num != &m_token2num || \
        false == m_tns2dgs.GetTxtDigititizer (ppDfa, ppState2Ow)) {
        *ppDfa = NULL;
        *ppState2Ow = NULL;    
    }
}

const FARSDfaCA * FAWRECompiler::GetTxtDigDfa () const
{
    const FARSDfaA * pDfa = NULL;
    const FAState2OwA * pState2Ow = NULL;

    if (m_pToken2Num != &m_token2num || \
        false == m_tns2dgs.GetTxtDigititizer (&pDfa, &pState2Ow)) {
        return NULL;
    }

    return pDfa;
}

const FAState2OwCA * FAWRECompiler::GetTxtDigOws () const
{
    const FARSDfaA * pDfa = NULL;
    const FAState2OwA * pState2Ow = NULL;

    if (m_pToken2Num != &m_token2num || \
        false == m_tns2dgs.GetTxtDigititizer (&pDfa, &pState2Ow)) {
        return NULL;
    }

    return pState2Ow;
}

void FAWRECompiler::
    GetDictDigitizer (
            const int ** ppId2Ow,
            int * pId2OwSize
        ) const
{
    DebugLogAssert (ppId2Ow && pId2OwSize);

    if (m_pToken2Num != &m_token2num || \
        false == m_tns2dgs.GetDictDigitizer (ppId2Ow, pId2OwSize)) {
        *ppId2Ow = NULL;
        *pId2OwSize = 0;
    }
}

const FAArrayCA * FAWRECompiler::GetDictDig () const
{
    if (0 < m_dct_arr.GetCount ()) {
        return & m_dct_arr;
    } else {
        return NULL;
    }
}

const FARSDfaCA * FAWRECompiler::GetDfa1 () const
{
    return m_pDfa1;
}

void FAWRECompiler::GetDfa1 (const FARSDfaA ** ppDfa) const
{
    DebugLogAssert (ppDfa);
    *ppDfa = m_pDfa1;
}

const FARSDfaCA * FAWRECompiler::GetDfa2 () const
{
    return m_pDfa2;
}

void FAWRECompiler::GetDfa2 (const FARSDfaA ** ppDfa) const
{
    DebugLogAssert (ppDfa);
    *ppDfa = m_pDfa2;
}

const FAState2OwsCA * FAWRECompiler::GetState2Ows () const
{
    return m_pMooreOws;
}

void FAWRECompiler::GetState2Ows (const FAState2OwsA ** ppState2Ows) const
{
    DebugLogAssert (ppState2Ows);
    *ppState2Ows = m_pMooreOws;
}

const FAMealyDfaCA * FAWRECompiler::GetSigma1 () const
{
    return m_pSigma1;
}

void FAWRECompiler::GetSigma1 (const FAMealyDfaA ** ppSigma) const
{
    DebugLogAssert (ppSigma);
    *ppSigma = m_pSigma1;
}

const FAMealyDfaCA * FAWRECompiler::GetSigma2 () const
{
    return m_pSigma2;
}

void FAWRECompiler::GetSigma2 (const FAMealyDfaA ** ppSigma) const
{
    DebugLogAssert (ppSigma);
    *ppSigma = m_pSigma2;
}

const FAMultiMapCA * FAWRECompiler::GetTrBrMap () const
{
    return m_pTrBrMap;
}

void FAWRECompiler::GetTrBrMap (const FAMultiMapA ** ppTrBr) const
{
    DebugLogAssert (ppTrBr);
    *ppTrBr = m_pTrBrMap;
}


const FAChain2NumA * FAWRECompiler::GetTokens () const
{
    return m_pToken2Num;
}

const FAMultiMapA * FAWRECompiler::GetCNF () const
{
    return m_pCNF;
}

const FAMultiMapA * FAWRECompiler::GetType2Ows () const
{
    return m_pType2Ows;
}

/// write side of FAWREConfA is not implemented

void FAWRECompiler::SetTokenType (const int)
{
    DebugLogAssert (0);
}
void FAWRECompiler::SetTagOwBase (const int)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetTxtDigititizer (FARSDfaA **, FAState2OwA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::SetDictDigitizer (const int *, const int)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetDfa1 (FARSDfaA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetDfa2 (FARSDfaA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetState2Ows (FAState2OwsA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetSigma1 (FAMealyDfaA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetSigma2 (FAMealyDfaA **)
{
    DebugLogAssert (0);
}
void FAWRECompiler::GetTrBrMap (FAMultiMapA **)
{
    DebugLogAssert (0);
}

}
