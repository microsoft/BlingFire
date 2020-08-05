/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWREIO.h"
#include "FAWREConfA.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAStringTokenizer.h"
#include "FAUtils.h"
#include "FARSDfaA.h"
#include "FAWREConfPack.h"
#include "FAException.h"

#include <iostream>
#include <string>

namespace BlingFire
{


void FAPrintWre (std::ostream & os, const FAWREConfA * pWRE)
{
    FAAssert (pWRE, FAMsg::InvalidParameters);

    FAAllocator al;

    FAAutIOTools FsmIo (&al);
    FAMapIOTools MapIo (&al);

    const FARSDfaA * pDfa;
    const FAState2OwA * pState2Ow;
    const FAState2OwsA * pState2Ows;
    const FAMealyDfaA * pSigma;
    const FAMultiMapA * pTrBr;
    const int * pArr;
    int ArrSize;

    /// print out the configuration

    const int WreType = pWRE->GetType ();
    FAAssert (FAFsmConst::WRE_TYPE_RS <= WreType && \
        FAFsmConst::WRE_TYPE_COUNT > WreType, FAMsg::InternalError);
    os << "WreType " << WreType << '\n';

    const int TokenType = pWRE->GetTokenType ();
    FAAssert ((FAFsmConst::WRE_TT_TEXT | FAFsmConst::WRE_TT_TAGS | \
        FAFsmConst::WRE_TT_DCTS) & TokenType, FAMsg::InternalError);
    os << "TokenType " << TokenType << '\n';

    const int TagOwBase = pWRE->GetTagOwBase ();
    os << "TagOwBase " << TagOwBase << '\n';

    /// print out digitizers, if any

    pWRE->GetTxtDigititizer (&pDfa, &pState2Ow);
    if (pDfa && pState2Ow) {
        FAAssert (FAIsValidDfa (pDfa), FAMsg::InternalError);
        os << "txt-digitizer\n";
        FsmIo.Print (os, pDfa, pState2Ow);
    }

    pWRE->GetDictDigitizer (&pArr, &ArrSize);
    if (pArr && 0 < ArrSize) {
        os << "dct-digitizer\n";
        MapIo.Print (os, pArr, ArrSize);
    }

    /// print out rules automaton

    pWRE->GetDfa1 (&pDfa);
    FAAssert (pDfa && FAIsValidDfa (pDfa), FAMsg::InternalError);
    os << "rules1\n";

    if (FAFsmConst::WRE_TYPE_RS == WreType) {
        FsmIo.Print (os, pDfa);
    } else if (FAFsmConst::WRE_TYPE_MOORE == WreType) {
        pWRE->GetState2Ows (&pState2Ows);
        FAAssert (pState2Ows, FAMsg::InternalError);
        FsmIo.Print (os, pDfa, pState2Ows);
    } else if (FAFsmConst::WRE_TYPE_MEALY == WreType) {
        pWRE->GetSigma1 (&pSigma);
        if (pSigma) {
            FsmIo.Print (os, pDfa, pSigma);
        } else {
            // trivial Mealy, use --type=rs for compilation
            FAAssert (!pWRE->GetTrBrMap () && !pWRE->GetDfa2 () && \
                !pWRE->GetSigma2 (), FAMsg::InternalError);
        }
    } else {
        // incorrect WreType
        FAAssert (0, FAMsg::InternalError);
    }

    /// print out rules2 and trbr map, if any

    if (FAFsmConst::WRE_TYPE_MEALY == WreType) {

        pWRE->GetDfa2 (&pDfa);
        pWRE->GetSigma2 (&pSigma);
        pWRE->GetTrBrMap (&pTrBr);

        if (pDfa && pSigma) {
            FAAssert (FAIsValidDfa (pDfa), FAMsg::InternalError);
            os << "rules2\n";
            FsmIo.Print (os, pDfa, pSigma);
        }
        if (pTrBr) {
            FAAssert (!FAIsEmpty (pTrBr), FAMsg::InternalError);
            os << "trbr\n";
            MapIo.Print (os, pTrBr);
        }
    }
}



void FAReadWre (std::istream & is, FAWREConfA * pWRE)
{
    FAAssert (pWRE, FAMsg::InvalidParameters);

    pWRE->Clear ();

    std::string line;
    const char * pTmpStr;
    int TmpStrLen;
    bool res;
    int n = -1;
    int Type = -1;

    FAStringTokenizer tokenizer;

    /// read the header

    for (int i = 0; i < 3; ++i) {

        if (!std::getline (is, line) || is.eof ())
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);

        tokenizer.SetString (line.c_str (), (int) line.length ());
        res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&n);
        FAAssert (res, FAMsg::IOError);

        if (0 == strncmp ("WreType", pTmpStr, TmpStrLen)) {
            pWRE->SetType (n);
            Type = n;
            continue;
        }
        if (0 == strncmp ("TokenType", pTmpStr, TmpStrLen)) {
            pWRE->SetTokenType (n);
            continue;
        }
        if (0 == strncmp ("TagOwBase", pTmpStr, TmpStrLen)) {
            pWRE->SetTagOwBase (n);
            continue;
        }
        FAAssert (0, FAMsg::IOError);
    }

    FAAllocator al;
    FAAutIOTools FsmIo (&al);
    FAMapIOTools MapIo (&al);

    /// read the rest
    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;
        if (line.empty ())
            break;

        if (0 == strncmp ("txt-digitizer", line.c_str (), 13)) {

            FARSDfaA * pDfa = NULL;
            FAState2OwA * pState2Ow = NULL;
            pWRE->GetTxtDigititizer (&pDfa, &pState2Ow);
            FsmIo.Read (is, pDfa, pState2Ow);
            continue;
        }
        if (0 == strncmp ("dct-digitizer", line.c_str (), 13)) {

            const int * pA = NULL;
            int Count = 0;
            MapIo.Read (is, &pA, &Count);
            pWRE->SetDictDigitizer (pA, Count);
            continue;
        }
        if (0 == strncmp ("rules1", line.c_str (), 6)) {

            if (FAFsmConst::WRE_TYPE_RS == Type) {
                FARSDfaA * pDfa = NULL;
                pWRE->GetDfa1 (&pDfa);
                FsmIo.Read (is, pDfa);
                continue;
            } else if (FAFsmConst::WRE_TYPE_MOORE == Type) {
                FARSDfaA * pDfa = NULL;
                pWRE->GetDfa1 (&pDfa);
                FAState2OwsA * pState2Ows = NULL;
                pWRE->GetState2Ows (&pState2Ows);
                FsmIo.Read (is, pDfa, pState2Ows);
                continue;
            } else if (FAFsmConst::WRE_TYPE_MEALY == Type) {
                FARSDfaA * pDfa = NULL;
                pWRE->GetDfa1 (&pDfa);
                FAMealyDfaA * pSigma = NULL;
                pWRE->GetSigma1 (&pSigma);
                FsmIo.Read (is, pDfa, pSigma);
                continue;
            }
        }
        if (0 == strncmp ("rules2", line.c_str (), 6)) {

            FAAssert (FAFsmConst::WRE_TYPE_MEALY == Type, FAMsg::IOError);
            FARSDfaA * pDfa = NULL;
            pWRE->GetDfa2 (&pDfa);
            FAMealyDfaA * pSigma = NULL;
            pWRE->GetSigma2 (&pSigma);
            FsmIo.Read (is, pDfa, pSigma);
            continue;
        }
        if (0 == strncmp ("trbr", line.c_str (), 4)) {

            FAAssert (FAFsmConst::WRE_TYPE_MEALY == Type, FAMsg::IOError);
            FAMultiMapA * pTrBr = NULL;
            pWRE->GetTrBrMap (&pTrBr);
            MapIo.Read (is, pTrBr);
            continue;
        }
        FAAssert (0, FAMsg::IOError);

    } // of while (!is.eof ()) ...
}


void FASaveWre (std::ostream & os, const FAWREConfA * pWRE)
{
    FAAllocator al;
    FAWREConfPack saver (&al);

    saver.SetWre (pWRE);
    saver.Process ();

    const unsigned char * pDump = NULL;
    const int DumpSize = saver.GetDump (&pDump);

    os.write ((const char *)pDump, DumpSize);
}

}
