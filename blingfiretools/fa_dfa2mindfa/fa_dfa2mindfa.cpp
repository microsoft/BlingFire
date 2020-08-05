/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FARSDfa_ro.h"
#include "FADfa2MinDfa_hg_t.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);

bool g_no_output = false;
bool g_print_eq_classes = false;

const char * pInFile = NULL;
const char * pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_dfa2mindfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program calculates equivalence classes of states for the input Dfa\n\
and prints out the minimized automaton. Reads/writes Dfa in text mode.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --print-eq-classes - prints equivalence classes to stderr\n\
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
    if (0 == strcmp ("--no-output", *argv)) {
      g_no_output = true;
      continue;
    }
    if (0 == strcmp ("--print-eq-classes", *argv)) {
      g_print_eq_classes = true;
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
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    /// create objects
    FARSDfa_ro input_dfa (&g_alloc);
    FARSDfa_ro output_dfa (&g_alloc); // see FADfa2MinDfa_hg_t for details

    FARSDfaA * pInDfa = &input_dfa;
    FARSDfaA * pOutDfa = &output_dfa;

    FADfa2MinDfa_hg_t < FARSDfaA, FARSDfaA > dfa2mindfa (&g_alloc);

    // select in/out streams
    std::istream * pIs = &std::cin;
    std::ifstream ifs;

    std::ostream * pOs = &std::cout;
    std::ofstream ofs;

    try {

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

      // read Dfa from stdin in plain-text
      g_io.Read (*pIs, pInDfa);

      // initialize processor
      dfa2mindfa.SetInDfa (pInDfa);
      dfa2mindfa.SetOutDfa (pOutDfa);

      // make processing
      dfa2mindfa.Process ();

      if (false == g_no_output) {

          // print the output automaton
          g_io.Print (*pOs, pOutDfa);

          if (true == g_print_eq_classes) {

              // print equivalence classes
              const int state_count = 1 + pInDfa->GetMaxState ();
              for (int state = 0; state < state_count; ++state) {

                  const int State = dfa2mindfa.GetEqClass (state);
                  std::cerr << State << " <- " << state << "\n";
              }

          } // of if (false == g_print_eq_classes) ...
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
