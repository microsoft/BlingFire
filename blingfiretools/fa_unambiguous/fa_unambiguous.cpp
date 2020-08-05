/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FARSNfa_ro.h"
#include "FAMealyNfa.h"
#include "FAFsmConst.h"
#include "FAUnAmbiguous.h"
#include "FAMultiMap_judy.h"
#include "FACmpTrBrOws_greedy.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_aut_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pTrBrFile = NULL;

bool g_no_output = false;
bool g_no_process = false;

FARSNfa_ro g_in_nfa (&g_alloc);
FAMealyNfa g_in_sigma (&g_alloc);
FAMultiMap_judy g_trbr;
FACmpTrBrOws_greedy g_trbr_less;
FAUnAmbiguous g_proc (&g_alloc);


void usage () {

  std::cout << "\n\
Usage: fa_unambiguous [OPTION]\n\
\n\
This program builds functional Mealy NFA from the input non-functional\n\
(ambiguous) Mealy NFA.\n\
\n\
\n\
  --in=<input-file> - reads input Mealy NFA from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output Mealy NFA to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --in-trbr=<input-file> - reads TrBr map to be able to compare Mealy NFA's\n\
    output weights\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
\n\
Example:\n\
  Input: S(('a:1''b:1')|(('a:2'|'p:2')'b:2'))S\n\
  S a b S -> S 1 1 S\n\
  S a b S -> S 2 2 S\n\
  S p b S -> S 2 2 S\n\
  Output:\n\
  S a b S -> S 1 1 S\n\
  S p b S -> S 2 2 S\n\
\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (!strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
        pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--in-trbr=", *argv, 10)) {
        pTrBrFile = &((*argv) [10]);
        continue;
    }
    if (0 == strcmp ("--no-process", *argv)) {
        g_no_process = true;
        continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
        g_no_output = true;
        continue;
    }
  }
}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    // make converter ready
    g_trbr.SetAllocator (&g_alloc);

    try {

        // select in/out streams
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (pTrBrFile) {

            std::ifstream trbr_ifs (pTrBrFile, std::ios::in);
            FAAssertStream (&trbr_ifs, pTrBrFile);
            g_map_io.Read (trbr_ifs, &g_trbr);

            g_trbr_less.SetTrBrMap (&g_trbr);
            g_proc.SetLess (&g_trbr_less);
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        g_aut_io.Read (*pIs, &g_in_nfa, &g_in_sigma);

        g_proc.SetInMealy (&g_in_nfa, &g_in_sigma);

        if (false == g_no_process) {
            g_proc.Process ();
        }

        const FARSNfaA * pOutNfa = g_proc.GetNfa ();
        const FAMealyNfaA * pOutSigma = g_proc.GetSigma ();

        if (false == g_no_output) {
            g_aut_io.Print (*pOs, pOutNfa, pOutSigma);
        }

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    return 0;
}

