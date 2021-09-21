/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAUtils.h"
#include "FARSNfaA.h"
#include "FARSDfaA.h"
#include "FAMealyDfaA.h"
#include "FAMealyNfaA.h"
#include "FAMapA.h"
#include "FAMultiMapA.h"
#include "FAFsmConst.h"
#include "FATaggedTextA.h"
#include "FAStringTokenizer.h"
#include "FALimits.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>


namespace BlingFire
{


void FASyntaxError (const char * pBuffer, 
                    const int BuffLength, 
                    const int Offset, 
                    const char * pMsg)
{
    if (0 <= Offset && pBuffer && pMsg) {

        DebugLogAssert (0 < BuffLength);
        const int ERROR_CONTEXT = 80;

        /// calc left context offset of the error
        int LeftOffset = Offset - ERROR_CONTEXT;
        if (0 > LeftOffset) {
            LeftOffset = 0;
        }

        /// calc right context offset of the error
        int RightOffset = LeftOffset + (2 * ERROR_CONTEXT);
        if (RightOffset > BuffLength - 1) {
            RightOffset = BuffLength - 1;
        }

        const int Length = RightOffset - LeftOffset + 1;
        std::string err_str (pBuffer + LeftOffset, Length);

#ifdef  _DLL
        std::cerr << "SYNTAX ERROR: " << pMsg 
                  << " at offset: " << Offset << '\n'
                  << "\"... " << err_str << " ...\"\n" ;
#else
        ::fprintf(stderr, "SYNTAX ERROR: %s  at offset: %d\n\"... %s ...\"\n", pMsg, Offset, err_str.c_str());
#endif
        const int LeftPadding = Offset - LeftOffset + 4;
        const int RightPadding = RightOffset - Offset;

        if (0 < LeftPadding && 0 < RightPadding) {
#ifdef  _DLL
        std::cerr << std::string(Offset - LeftOffset + 4, ' ')
                  << std::string("^^^")
                  << std::string(RightOffset - Offset, ' ') << '\n';
#else
        ::fprintf(stderr, "%*s^^^%*s\n", Offset - LeftOffset + 4, "", RightOffset - Offset, "");
#endif
        }

    } else if (pBuffer && pMsg) {

        DebugLogAssert (0 < BuffLength);

        std::string buffer (pBuffer, BuffLength);
#ifdef  _DLL
        std::cerr << "SYNTAX ERROR: " << pMsg << " while processing: " << buffer << '\n';
#else
        ::fprintf(stderr, "SYNTAX ERROR: %s while processing: %s\n", pMsg, buffer.c_str());
#endif

    } else if (pMsg) {

#ifdef  _DLL
        std::cerr << "SYNTAX ERROR: " << pMsg << '\n';
#else
        ::fprintf(stderr, "SYNTAX ERROR:  %s\n", pMsg);
#endif

    } else {

#ifdef  _DLL
        std::cerr << "SYNTAX ERROR\n";
#else
        ::fprintf(stderr, "SYNTAX ERROR\n");
#endif
    }
}


void FAIOSetup ()
{
#ifdef WIN32
    _set_fmode (_O_BINARY);
    _setmode (_fileno (stdout), _O_BINARY);
#endif
}

void FAInputIOSetup ()
{
#ifdef WIN32
    _setmode (_fileno (stdin), _O_BINARY);
#endif
}

const unsigned int FAEncoding2CodePage (const char * pEncStr)
{
    DebugLogAssert (pEncStr);

    const int EncLen = (const int) strlen (pEncStr);

    if (3 < EncLen && 0 == strncmp ("IBM", pEncStr, 3)) {

        const unsigned int CP = atoi (pEncStr + 3);
        return CP;

    } else if (8 < EncLen && 0 == strncmp ("WINDOWS-", pEncStr, 8)) {

        const unsigned int CP = atoi (pEncStr + 8);
        return CP;

    } else if (3 < EncLen && 0 == strncmp ("CP-", pEncStr, 3)) {

        const unsigned int CP = atoi (pEncStr + 3);
        return CP;

    } else if (2 < EncLen && 0 == strncmp ("CP", pEncStr, 2)) {

        const unsigned int CP = atoi (pEncStr + 2);
        return CP;

    } else if (9 < EncLen && 0 == strncmp ("ISO-8859-", pEncStr, 9)) {

        const unsigned int CP = 28590 + atoi (pEncStr + 9);
        return CP;

    } else if (6 == EncLen && 0 == strcmp ("KOI8-R", pEncStr)) {

        return 20866;

    } else if (4 == EncLen && 0 == strcmp ("BIG5", pEncStr)) {

        return 950;

    } else if (9 == EncLen && 0 == strcmp ("DIN_66003", pEncStr)) {

        return 20106;

    } else if (9 == EncLen && 0 == strcmp ("NS_4551-1", pEncStr)) {

        return 20108;

    } else if (12 == EncLen && 0 == strcmp ("SEN_850200_B", pEncStr)) {

        return 20107;

    } else if (12 == EncLen && 0 == strcmp ("CSISO2022JP2", pEncStr)) {

        return 50221;

    } else if (13 == EncLen && 0 == strcmp ("ISO-2022-JP-2", pEncStr)) {

        return 50221;

    } else if (6 == EncLen && 0 == strcmp ("UTF-16", pEncStr)) {

        return 1200;

    } else if (5 == EncLen && 0 == strcmp ("UTF16", pEncStr)) {

        return 1200;

    } else if (5 == EncLen && 0 == strcmp ("UTF-7", pEncStr)) {

        return 65000;

    } else if (4 == EncLen && 0 == strcmp ("UTF7", pEncStr)) {

        return 65000;

    } else if (5 == EncLen && 0 == strcmp ("UTF-8", pEncStr)) {

        return 65001;

    } else if (4 == EncLen && 0 == strcmp ("UTF8", pEncStr)) {

        return 65001;

    } else if (8 == EncLen && 0 == strcmp ("US-ASCII", pEncStr)) {

        return 20127;

    } else if (5 == EncLen && 0 == strcmp ("ASCII", pEncStr)) {

        return 20127;

    } else if (9 == EncLen && 0 == strcmp ("MACINTOSH", pEncStr)) {

        return 10000;

    } else if (3 == EncLen && 0 == strcmp ("MAC", pEncStr)) {

        return 10000;

    } else if (6 == EncLen && 0 == strcmp ("EUC-JP", pEncStr)) {

        return 51932;

    } else if (5 == EncLen && 0 == strcmp ("EUCJP", pEncStr)) {

        return 51932;
    }

    return 0;
}


FAIdxCmp_b2s::FAIdxCmp_b2s (const int * pValues) :
    m_pValues (pValues)
{}


const bool FAIdxCmp_b2s::operator () (const int Idx1, const int Idx2) const
{
    DebugLogAssert (m_pValues);

    const int Val1 = m_pValues [Idx1];
    const int Val2 = m_pValues [Idx2];

    /// return Val1 > Val2;
    if (Val1 > Val2) {
        return true;
    } else if (Val1 == Val2) {
        return Idx1 > Idx2;
    } else {
        return false;
    }
}


FAIdxCmp_s2b::FAIdxCmp_s2b (const int * pValues) :
    m_pValues (pValues)
{}


const bool FAIdxCmp_s2b::operator () (const int Idx1, const int Idx2) const
{
    DebugLogAssert (m_pValues);

    const int Val1 = m_pValues [Idx1];
    const int Val2 = m_pValues [Idx2];

    /// return Val1 < Val2;
    if (Val1 < Val2) {
        return true;
    } else if (Val1 == Val2) {
        return Idx1 < Idx2;
    } else {
        return false;
    }
}


const bool FAIsEscaped (const int Pos, const char * pStr, const int StrLen)
{
    if (NULL == pStr || 0 >= StrLen || 0 > Pos || StrLen <= Pos) {
        return false;
    }

    if (1 < Pos) {

        if('\\' == pStr [Pos - 1] && '\\' != pStr [Pos - 2]) {
            return true;
        }

    } else if (0 == Pos) {

        return false;

    } else if (1 == Pos && '\\' == *pStr) {

        return true;
    }
    
    return false;
}


const bool FAIsDfa (const FARSNfaA * pNfa)
{
    DebugLogAssert (pNfa);

    const int MaxState = pNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwCount = pNfa->GetIWs (State, &pIws);

        for (int i = 0; i < IwCount; ++i) {

            DebugLogAssert (pIws);
            const int Iw = pIws [i];

            const int * pDsts;
            const int DstCount = pNfa->GetDest (State, Iw, &pDsts);

            if (1 < DstCount) {
                return false;
            }
        }
    }

    return true;
}


void FAGetAlphabet (const FARSNfaA * pNfa, FAArray_cont_t <int> * pA)
{
    DebugLogAssert (pNfa && pA);

    pA->resize (0);

    const int MaxOldState = pNfa->GetMaxState ();

    for (int State = 0; State <= MaxOldState; ++State) {

        const int * pIws;
        const int IwCount = pNfa->GetIWs (State, &pIws);

        if (0 < IwCount) {

            const int OldAlphSize = pA->size ();
            pA->resize (OldAlphSize + IwCount);

            int * pIwDst = pA->begin () + OldAlphSize; 
            memcpy (pIwDst, pIws, sizeof (int) * IwCount);
        }

        const int NewAlphSize = \
            FASortUniq (pA->begin (), pA->end ());
        pA->resize (NewAlphSize);
    }
}


void FACopyNfa (FARSNfaA * pDstNfa, const FARSNfaA * pSrcNfa)
{
    DebugLogAssert (pDstNfa && pSrcNfa);

    pDstNfa->Clear ();

    const int MaxState = pSrcNfa->GetMaxState ();
    const int MaxIw = pSrcNfa->GetMaxIw ();

    pDstNfa->SetMaxState (MaxState);
    pDstNfa->SetMaxIw (MaxIw);
    pDstNfa->Create ();

    const int * pStates;
    int Count = pSrcNfa->GetInitials (&pStates);
    pDstNfa->SetInitials (pStates, Count);
    
    Count = pSrcNfa->GetFinals (&pStates);
    pDstNfa->SetFinals (pStates, Count);

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwCount = pSrcNfa->GetIWs (State, &pIws);

        for (int i = 0; i < IwCount; ++i) {

            const int Iw = pIws [i];

            Count = pSrcNfa->GetDest (State, Iw, &pStates);

            if (0 < Count) {
                pDstNfa->SetTransition (State, Iw, pStates, Count);
            } else if (0 == Count) {
                const int DeadState = FAFsmConst::NFA_DEAD_STATE;
                pDstNfa->SetTransition (State, Iw, &DeadState, 1);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...

    pDstNfa->Prepare ();
}


void FACopyDfa2Nfa (FARSNfaA * pDstNfa, const FARSDfaA * pSrcDfa)
{
    DebugLogAssert (pDstNfa && pSrcDfa);

    pDstNfa->Clear ();

    const int MaxState = pSrcDfa->GetMaxState ();
    const int MaxIw = pSrcDfa->GetMaxIw ();

    pDstNfa->SetMaxState (MaxState);
    pDstNfa->SetMaxIw (MaxIw);
    pDstNfa->Create ();

    const int Ini = pSrcDfa->GetInitial ();
    pDstNfa->SetInitials (&Ini, 1);

    const int * pStates;
    int Count = pSrcDfa->GetFinals (&pStates);
    pDstNfa->SetFinals (pStates, Count);

    const int * pIws;
    const int IwCount = pSrcDfa->GetIWs (&pIws);

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < IwCount; ++i) {

            const int Iw = pIws [i];
            const int Dst = pSrcDfa->GetDest (State, Iw);

            if (-1 != Dst) {
                DebugLogAssert (0 <= Dst);
                pDstNfa->SetTransition (State, Iw, &Dst, 1);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...

    pDstNfa->Prepare ();
}


void FACopyNfa2Dfa (FARSDfaA * pDstDfa, const FARSNfaA * pSrcNfa)
{
    DebugLogAssert (pDstDfa && pSrcNfa);
    DebugLogAssert (FAIsDfa (pSrcNfa));

    pDstDfa->Clear ();

    const int MaxState = pSrcNfa->GetMaxState ();
    const int MaxIw = pSrcNfa->GetMaxIw ();

    pDstDfa->SetMaxState (MaxState);
    pDstDfa->SetMaxIw (MaxIw);
    pDstDfa->Create ();

    const int * pStates = NULL;
    int Count = pSrcNfa->GetInitials (&pStates);
    DebugLogAssert (1 == Count && pStates);
    pDstDfa->SetInitial (*pStates);

    Count = pSrcNfa->GetFinals (&pStates);
    pDstDfa->SetFinals (pStates, Count);

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwCount = pSrcNfa->GetIWs (State, &pIws);

        for (int i = 0; i < IwCount; ++i) {

            const int Iw = pIws [i];

            Count = pSrcNfa->GetDest (State, Iw, &pStates);

            if (0 < Count) {
                DebugLogAssert (1 == Count);
                pDstDfa->SetTransition (State, Iw, *pStates);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...

    pDstDfa->Prepare ();
}


const bool FAIsEmpty (const FAMultiMapA * pMMap)
{
    DebugLogAssert (pMMap);

    const int * pValues;
    int Key = -1;
    int Size = pMMap->Prev (&Key, &pValues);

    return -1 == Size;
}


void FARemapRsFsmIws (
        const FARSDfaA * pOldDfa, 
        FARSDfaA * pNewDfa, 
        const FAMapA * pOld2New
    )
{
    DebugLogAssert (pOldDfa && pNewDfa && pOld2New);
    DebugLogAssert (pOldDfa != pNewDfa);
    DebugLogAssert (FAIsValidDfa (pOldDfa));

    pNewDfa->Clear ();

    // copy initial state
    const int InitialState = pOldDfa->GetInitial ();
    pNewDfa->SetInitial (InitialState);

    // copy final states
    const int * pFinals;
    const int FinalsCount = pOldDfa->GetFinals (&pFinals);
    pNewDfa->SetFinals (pFinals, FinalsCount);

    // get alphabet
    const int * pIws;
    const int IwsCount = pOldDfa->GetIWs (&pIws);
    DebugLogAssert (0 < IwsCount && pIws);

    const int MaxState = pOldDfa->GetMaxState ();

    // copy transitions
    for (int State = 0; State <= MaxState; ++State) {

        for (int iw_idx = 0; iw_idx < IwsCount; ++iw_idx) {

            const int Iw = pIws [iw_idx];
            const int DstState = pOldDfa->GetDest (State, Iw);

            if (-1 != DstState) {

                const int * pNewIw = pOld2New->Get (Iw);
                DebugLogAssert (pNewIw);
                const int NewIw = *pNewIw;

                pNewDfa->SetTransition (State, NewIw, DstState);
            }
        }
    }

    pNewDfa->Prepare ();
}


void FARemapRsFsmIws (
        const FARSNfaA * pSrcNfa,
        FARSNfaA * pDstNfa,
        const FAMapA * pOld2New
    )
{
    DebugLogAssert (pDstNfa && pSrcNfa);

    pDstNfa->Clear ();

    const int MaxState = pSrcNfa->GetMaxState ();
    const int MaxIw = pSrcNfa->GetMaxIw ();

    pDstNfa->SetMaxState (MaxState);
    pDstNfa->SetMaxIw (MaxIw);
    pDstNfa->Create ();

    const int * pStates;
    int Count = pSrcNfa->GetInitials (&pStates);
    pDstNfa->SetInitials (pStates, Count);
    
    Count = pSrcNfa->GetFinals (&pStates);
    pDstNfa->SetFinals (pStates, Count);

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int IwCount = pSrcNfa->GetIWs (State, &pIws);

        for (int i = 0; i < IwCount; ++i) {

            const int Iw = pIws [i];
            const int * pNewIw = pOld2New->Get (Iw);
            DebugLogAssert (pNewIw);
            const int NewIw = *pNewIw;

            Count = pSrcNfa->GetDest (State, Iw, &pStates);

            if (0 < Count) {
                pDstNfa->SetTransition (State, NewIw, pStates, Count);
            } else if (0 == Count) {
                const int DeadState = FAFsmConst::NFA_DEAD_STATE;
                pDstNfa->SetTransition (State, NewIw, &DeadState, 1);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...

    pDstNfa->Prepare ();
}


void FARemapMealySigma1 (
        const FARSDfaA * pOldDfa,
        const FAMealyDfaA * pOldSigma,
        FAMealyDfaA * pNewSigma,
        const FAMapA * pOld2New
    )
{
    DebugLogAssert (pOldDfa && pOldSigma && pNewSigma && pOld2New);
    DebugLogAssert (FAIsValidDfa (pOldDfa));

    const int * pIws;
    const int Iws = pOldDfa->GetIWs (&pIws);

    const int MaxState = pOldDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < Iws; ++i) {

            const int Iw = pIws [i];
            const int OldOw = pOldSigma->GetOw (State, Iw);

            if (-1 != OldOw) {

                const int * pNewOw = pOld2New->Get (OldOw);

                if (pNewOw) {
                    const int NewOw = *pNewOw;
                    pNewSigma->SetOw (State, Iw, NewOw);
                }
            }

        } // of for (int i = 0; i < Iws; ...
    } // of for (int State = 0; State <= MaxState; ...

}


void FARemapMealySigma2 (
        const FARSDfaA * pOldDfa,
        const FAMealyDfaA * pOldSigma,
        FAMealyDfaA * pNewSigma,
        const FAMapA * pOld2New
    )
{
    DebugLogAssert (pOldDfa && pOldSigma && pNewSigma && pOld2New);
    DebugLogAssert (FAIsValidDfa (pOldDfa));

    const int * pIws;
    const int Iws = pOldDfa->GetIWs (&pIws);

    const int MaxState = pOldDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        for (int i = 0; i < Iws; ++i) {

            const int Iw = pIws [i];
            const int Ow = pOldSigma->GetOw (State, Iw);

            if (-1 == Ow)
                continue;

            const int * pNewIw = pOld2New->Get (Iw);
            DebugLogAssert (pNewIw);

            const int NewIw = *pNewIw;
            pNewSigma->SetOw (State, NewIw, Ow);
        }
    }
}


void FARemapMealySigma2 (
        const FARSNfaA * pOldNfa,
        const FAMealyNfaA * pOldSigma,
        FAMealyNfaA * pNewSigma,
        const FAMapA * pOld2New
    )
{
    DebugLogAssert (pOldNfa && pOldSigma && pNewSigma && pOld2New);
    DebugLogAssert (FAIsValidNfa (pOldNfa));

    const int MaxState = pOldNfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int * pIws;
        const int Iws = pOldNfa->GetIWs (State, &pIws);

        for (int i = 0; i < Iws; ++i) {

            const int Iw = pIws [i];

            const int * pDsts;
            const int Dsts = pOldNfa->GetDest (State, Iw, &pDsts);

            for (int j = 0; j < Dsts; ++j) {

                const int Dst = pDsts [j];
                const int Ow = pOldSigma->GetOw (State, Iw, Dst);

                if (-1 == Ow)
                    continue;

                const int * pNewIw = pOld2New->Get (Iw);
                DebugLogAssert (pNewIw);

                const int NewIw = *pNewIw;
                pNewSigma->SetOw (State, NewIw, Dst, Ow);
            }

        } // of for (int i = 0; ...
    } // of for (int State = 0; ...
}


void FAReverseMap (FAMultiMapA * pRevMap, const FAMapA * pMap)
{
    int Key = -1;
    const int * pValue = pMap->Prev (&Key);

    while (NULL != pValue) {
        pRevMap->Add (*pValue, Key);
        pValue = pMap->Prev (&Key);
    }
}


void FACopyTaggedText (FATaggedTextA * pOut, const FATaggedTextCA * pIn)
{
    DebugLogAssert (pIn && pOut);

    pOut->Clear ();

    const int Count = pIn->GetWordCount ();

    for (int i = 0; i < Count; ++i) {

        const int * pText;
        const int Size = pIn->GetWord (i, &pText);
        const int Tag = pIn->GetTag (i);
        const int Offset = pIn->GetOffset (i);

        pOut->AddWord (pText, Size, Tag, Offset);
    }
}


const int FAReadHexChain (
        const char * pLine, 
        const int LineLen, 
        __out_ecount(MaxOutSize) int * pChain, 
        const int MaxChainSize
    )
{
    FAStringTokenizer tokenizer;

    tokenizer.SetSpaces (" ");
    tokenizer.SetString (pLine, LineLen);

    int OutSize = 0;
    const char * pStr = NULL;
    int Len = 0;

    while (tokenizer.GetNextStr (&pStr, &Len)) {

        std::string buff (pStr, Len);
        const unsigned int C = strtol (buff.c_str (), NULL, 16);

        if (OutSize + 1 < MaxChainSize) {
            pChain [OutSize] = C ;
        }

        OutSize++;
    }

    return OutSize;
}

}
