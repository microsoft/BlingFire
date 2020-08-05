/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FARSDfa_ro.h"
#include "FAMealyDfa_ro.h"
#include "FAState2Ows_ar_uniq.h"
#include "FARSDfa2PerfHash.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * pInFile = NULL;
const char * pOutFile = NULL;

int g_out_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
bool g_no_output = false;


void usage () {

  std::cout << "\n\
Usage: fa_dfa2mph [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program builds Minimal Perfect Hash from the given acyclic DFA.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --type=moore-mdfa|mealy-dfa - specifies output automaton type,\n\
    moore-mdfa is used by default\n\
\n\
  --no-output - does not do any output\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (0 == strcmp ("--help", *argv)) {
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
    if (0 == strcmp ("--type=moore-mdfa", *argv)) {
      g_out_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
      continue;
    }
    if (0 == strcmp ("--type=mealy-dfa", *argv)) {
      g_out_type = FAFsmConst::TYPE_MEALY_DFA;
      continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
      g_no_output = true;
      continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        /// create objects

        FAAutIOTools io (&g_alloc);

        FARSDfa_ro input_dfa (&g_alloc);
        FAState2Ows_ar_uniq output_ows (&g_alloc);
        FAMealyDfa_ro mealy_ows (&g_alloc);

        FARSDfaA * pInDfa = &input_dfa;
        FAState2OwsA * pOutOws = &output_ows;

        FARSDfa2PerfHash dfa2mph (&g_alloc);

        /// select in/out streams
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != pInFile) {
          ifs.open (pInFile, std::ios::in);
          FAAssertStream (&ifs, pInFile);
          pIs = &ifs;
        }
        if (NULL != pOutFile) {
          ofs.open (pOutFile, std::ios::out);
          pOs = &ofs;
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        /// read Dfa from stdin or file in plain-text
        io.Read (*pIs, pInDfa);

        /// initialize processor
        dfa2mph.SetRsDfa (pInDfa);
        dfa2mph.SetState2Ows (pOutOws);

        /// make processing
        dfa2mph.Process ();

        /// convert Multi Moore into Mealy reactions
        if (FAFsmConst::TYPE_MEALY_DFA == g_out_type) {

            FAArray_cont_t < int > tmp_iws;
            tmp_iws.SetAllocator (&g_alloc);
            tmp_iws.Create ();

            const int * pIws;
            const int Iws = input_dfa.GetIWs (&pIws);
            DebugLogAssert (0 < Iws && ::FAIsSortUniqed (pIws, Iws));

            const int MaxState = input_dfa.GetMaxState ();

            for (int State = 0; State <= MaxState; ++State) {

                int i;
                tmp_iws.resize (0);

                for (i = 0; i < Iws; ++i) {
                    const int Iw = pIws [i];
                    if (-1 != input_dfa.GetDest (State, Iw)) {
                        tmp_iws.push_back (Iw);
                    }
                }

                const int * pOws;
                const int Count = pOutOws->GetOws (State, &pOws);

                FAAssert ((-1 == Count && 0 == tmp_iws.size ()) || \
                    (0 <= Count && (unsigned int) Count == tmp_iws.size ()),
                    FAMsg::InternalError);

                for (i = 0; i < Count; ++i) {
                    const int Iw = tmp_iws [i];
                    const int Ow = pOws [i];
                    mealy_ows.SetOw (State, Iw, Ow);
                }
            }

            mealy_ows.Prepare ();

        } // of if (FAFsmConst::TYPE_MEALY_DFA == g_out_type) ...

        /// print the resulting automaton out
        if (false == g_no_output) {
            if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_out_type) {
                io.Print (*pOs, pInDfa, pOutOws);
            } else {
                DebugLogAssert (FAFsmConst::TYPE_MEALY_DFA == g_out_type);
                io.Print (*pOs, pInDfa, &mealy_ows);
            }
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

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
