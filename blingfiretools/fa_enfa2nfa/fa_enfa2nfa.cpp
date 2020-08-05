/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FARSNfa_ar_judy.h"
#include "FARSNfa_ro.h"
#include "FAEpsilonRemoval.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);

bool g_no_output = false;
int g_epsilon_iw = 0;
bool g_use_any = false;
int g_any_iw = 0;

const char * pInFile = NULL;
const char * pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_enfa2nfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program removes epsilon transitions from Nfa.\n\
Reads/writes Nfa in text mode.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --epsilon=Iw - specifies wich Iw is the Epsilon\n\
    0 is used by default\n\
\n\
  --spec-any=N - specifies which Iw to be considered as ANY-other symbol\n\
    (no ANY-other symbol by default)\n\
\n\
  --no-output - does not do any output\n\
\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv){

    if (!strcmp ("--help", *argv)) {
      usage ();
      exit (0);
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--epsilon=", *argv, 10)) {
      g_epsilon_iw = atoi (&((*argv) [10]));
      continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
      pInFile = &((*argv) [5]);
      continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
      pOutFile = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--spec-any=", *argv, 11)) {
      g_use_any = true;
      g_any_iw = atoi (&((*argv) [11]));
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

      FARSNfa_ro nfa_in (&g_alloc);
      FARSNfa_ar_judy nfa_out (&g_alloc);
      FAEpsilonRemoval e_removal (&g_alloc);

      // adjust input/output
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

      // read Nfa from the textual representation
      g_io.Read (*pIs, &nfa_in);

      // make epsilon removal
      e_removal.SetInNfa (&nfa_in);
      e_removal.SetOutNfa (&nfa_out);
      e_removal.SetEpsilonIw (g_epsilon_iw);

      if (g_use_any) {
        e_removal.SetAnyIw (g_any_iw);
      }

      e_removal.Process ();

      // make the output automaton sound
      nfa_out.Prepare ();

      // write Nfa into the textual representation
      if (false == g_no_output) {
          g_io.Print (*pOs, &nfa_out);
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
