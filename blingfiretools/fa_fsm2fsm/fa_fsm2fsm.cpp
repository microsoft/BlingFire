/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FARSNfaA.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAState2OwsA.h"
#include "FAMealyNfaA.h"
#include "FARSNfa_ar_judy.h"
#include "FARSNfa_wo_ro.h"
#include "FARSDfa_ro.h"
#include "FAState2Ow.h"
#include "FAState2Ows.h"
#include "FAMealyNfa.h"
#include "FARSDfa2MooreDfa.h"
#include "FATrBrNfa2MealyNfa.h"
#include "FADfa2MealyNfa.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

const char * pInType = "rs-dfa";
const char * pInFile = NULL;

const char * pOutType = "rs-dfa";
const char * pOutFile = NULL;
const char * pTrBrFile = NULL;

int g_OwBase = 0;
int g_OwMax = 0;

bool g_keep_ows = false;


void usage () {

  std::cout << "\n\
Usage: fa_fsm2fsm [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program makes equivalent convertions between different \n\
representations and types of automata.\n\
\n\
  --in-type=<type> - specifies input automaton type\n\
    the following types are available:\n\
\n\
      rs-nfa      - Rabin-Scott NFA,\n\
      rs-dfa      - Rabin-Scott DFA, the default value\n\
      moore-dfa   - Moore DFA\n\
      moore-mdfa  - Moore DFA with multiple output\n\
      mealy-nfa   - Mealy NFA\n\
\n\
  --out-type=<type> - specifies output automaton type,\n\
    the default value is rs-nfa\n\
\n\
  --out-trbr=<output-file> - writes functions necessary for triangular\n\
    brackets extraction, does not write by default\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --ow-base=OwBase - base value for Ows,\n\
    0 is used by default\n\
\n\
  --ow-max=MaxOw - max value for Ows,\n\
    0 is used by default\n\
\n\
  --keep-ows - does not substract OwBase from Ows\n\
\n\
  Note:\n\
\n\
  1. Only the following convertions are possible:\n\
\n\
     rs-nfa -> rs-nfa\n\
     rs-nfa -> mealy-nfa (the rs-nfa keeps triangular bracket expresions)\n\
     rs-dfa -> rs-dfa\n\
     rs-dfa -> moore-dfa\n\
     rs-dfa -> moore-mdfa\n\
     rs-dfa -> mealy-nfa (the rs-dfa should be build on pairs <Iw, Ow>)\n\
     moore-dfa -> moore-dfa\n\
     moore-mdfa -> moore-mdfa\n\
\n\
  2. --ow-base, --ow-max - should be used for Moore/Mealy <-> RS convertions\n\
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
    if (0 == strncmp ("--out-trbr=", *argv, 11)) {
      pTrBrFile = &((*argv) [11]);
      continue;
    }
    if (0 == strncmp ("--in-type=", *argv, 10)) {
      pInType = &((*argv) [10]);
      continue;
    }
    if (0 == strncmp ("--out-type=", *argv, 11)) {
      pOutType = &((*argv) [11]);
      continue;
    }
    if (0 == strncmp ("--ow-base=", *argv, 10)) {
      g_OwBase = atoi (&((*argv) [10]));
      continue;
    }
    if (0 == strncmp ("--ow-max=", *argv, 9)) {
      g_OwMax = atoi (&((*argv) [9]));
      continue;
    }
    if (0 == strcmp ("--keep-ows", *argv)) {
      g_keep_ows = true;
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

    try {

      // select in/out streams
      std::istream * pIs = &std::cin;
      std::ifstream ifs;

      std::ostream * pOs = &std::cout;
      std::ofstream ofs;

      std::ostream * pTrBrOs = NULL;
      std::ofstream trbr_ofs;

      if (NULL != pInFile) {
          ifs.open (pInFile, std::ios::in);
          FAAssertStream (&ifs, pInFile);
          pIs = &ifs;
      }
      if (NULL != pOutFile) {
          ofs.open (pOutFile, std::ios::out);
          pOs = &ofs;
      }
      if (NULL != pTrBrFile) {
          trbr_ofs.open (pTrBrFile, std::ios::out);
          pTrBrOs = &trbr_ofs;
      }

      DebugLogAssert (pIs);
      DebugLogAssert (pOs);

      // create all possible automata
      FARSNfa_ar_judy rs_nfa (&g_alloc);
      FARSNfa_wo_ro rs_nfa_wo_ro (&g_alloc);
      FARSDfa_ro rs_dfa (&g_alloc);
      FARSDfa_ro moore_dfa (&g_alloc);
      FAState2Ow moore_ows (&g_alloc);
      FARSDfa_ro moore_mdfa_dfa (&g_alloc);
      FAState2Ows moore_mdfa_ows (&g_alloc);
      FAMealyNfa mealy_nfa_ows (&g_alloc);

      // read in specified automaton
      if (0 == strcmp (pInType, "rs-nfa")) {
        g_io.Read (*pIs, &rs_nfa);
      } else if (0 == strcmp (pInType, "rs-dfa")) {
        g_io.Read (*pIs, &rs_dfa);
      } else if (0 == strcmp (pInType, "moore-dfa")) {
        g_io.Read (*pIs, &moore_dfa, &moore_ows);
      } else if (0 == strcmp (pInType, "moore-mdfa")) {
        g_io.Read (*pIs, &moore_mdfa_dfa, &moore_mdfa_ows);
      }

      // make convertions
      const FARSNfaA * pOutNfa = NULL;
      const FARSDfaA * pOutDfa = NULL;
      const FAState2OwA * pState2Ow = NULL;
      const FAState2OwsA * pState2Ows = NULL;
      const FAMealyNfaA * pMealyOws = NULL;

      if (0 == strcmp (pInType, "rs-nfa") && 
          0 == strcmp (pOutType, "rs-nfa")) {

          pOutNfa = &rs_nfa;

      } else if (0 == strcmp (pInType, "rs-dfa") && 
                 0 == strcmp (pOutType, "rs-dfa")) {

          pOutDfa = &rs_dfa;

      } else if (0 == strcmp (pInType, "moore-dfa") && 
                 0 == strcmp (pOutType, "moore-dfa")) {

          pOutDfa = &moore_dfa;
          pState2Ow = &moore_ows;

      } else if (0 == strcmp (pInType, "moore-mdfa") && 
                 0 == strcmp (pOutType, "moore-mdfa")) {

          pOutDfa = &moore_mdfa_dfa;
          pState2Ows = &moore_mdfa_ows;

      } else if (0 == strcmp (pInType, "rs-dfa") && 
                 0 == strcmp (pOutType, "moore-dfa")) {

          FARSDfa2MooreDfa rs2moore (&g_alloc);

          rs2moore.SetRSDfa (&rs_dfa);
          rs2moore.SetMooreDfa (&moore_dfa);
          rs2moore.SetState2Ow (&moore_ows);
          rs2moore.SetOwsRange (g_OwBase, g_OwMax);
          rs2moore.SetKeepOws (g_keep_ows);

          rs2moore.Process ();

          pOutDfa = &moore_dfa;
          pState2Ow = &moore_ows;

      } else if (0 == strcmp (pInType, "rs-dfa") && 
                 0 == strcmp (pOutType, "moore-mdfa")) {

          FARSDfa2MooreDfa rs2moore (&g_alloc);

          rs2moore.SetRSDfa (&rs_dfa);
          rs2moore.SetMooreDfa (&moore_mdfa_dfa);
          rs2moore.SetState2Ows (&moore_mdfa_ows);
          rs2moore.SetOwsRange (g_OwBase, g_OwMax);
          rs2moore.SetKeepOws (g_keep_ows);

          rs2moore.Process ();

          pOutDfa = &moore_mdfa_dfa;
          pState2Ows = &moore_mdfa_ows;

      } else if (0 == strcmp (pInType, "rs-dfa") && 
                 0 == strcmp (pOutType, "mealy-nfa")) {

          FADfa2MealyNfa dfa2mealy (&g_alloc);

          dfa2mealy.SetInDfa (&rs_dfa);
          dfa2mealy.SetOutNfa (&rs_nfa_wo_ro, &mealy_nfa_ows);

          dfa2mealy.Process ();

          pOutNfa = &rs_nfa_wo_ro;
          pMealyOws = &mealy_nfa_ows;

      } else if (0 == strcmp (pInType, "rs-nfa") && 
                 0 == strcmp (pOutType, "mealy-nfa")) {

          FATrBrNfa2MealyNfa nfa2mealy (&g_alloc);

          nfa2mealy.SetEpsilonIw (FAFsmConst::IW_EPSILON);
          nfa2mealy.SetInNfa (&rs_nfa);
          nfa2mealy.SetOutNfa (&rs_nfa, &mealy_nfa_ows);
          nfa2mealy.SetTrBrBaseIw (g_OwBase);
          nfa2mealy.SetTrBrMaxIw (g_OwMax);

          nfa2mealy.Process ();

          pOutNfa = &rs_nfa;
          pMealyOws = &mealy_nfa_ows;

          if (pTrBrOs) {

            const FAMultiMapA * pOw2TrBrs = nfa2mealy.GetOw2TrBrMap ();

            if (!::FAIsEmpty (pOw2TrBrs)) {
                g_map_io.Print (*pTrBrOs, pOw2TrBrs);
            }
          }

      } else {

          std::cerr << "ERROR: \"Unsupported convertion type\""
                    << " in program " << __PROG__ << '\n';
          return 1;
      }

      // write down specified automaton
      if (0 == strcmp (pOutType, "rs-nfa")) {
        g_io.Print (*pOs, pOutNfa);
      } else if (0 == strcmp (pOutType, "rs-dfa")) {
        g_io.Print (*pOs, pOutDfa);
      } else if (0 == strcmp (pOutType, "moore-dfa")) {
        g_io.Print (*pOs, pOutDfa, pState2Ow);
      } else if (0 == strcmp (pOutType, "moore-mdfa")) {
        g_io.Print (*pOs, pOutDfa, pState2Ows);
      } else if (0 == strcmp (pOutType, "mealy-nfa")) {
        g_io.Print (*pOs, pOutNfa, pMealyOws);
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
