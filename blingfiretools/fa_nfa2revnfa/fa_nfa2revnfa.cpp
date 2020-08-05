/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FARSNfa_ro.h"
#include "FARSNfa_wo_ro.h"
#include "FARSNfa2RevNfa.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

bool g_no_output = false;

bool g_use_any = false;
int g_spec_any = -1;

const char * pInFile = NULL;
const char * pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_nfa2revnfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program calculates reverse automaton for the given Nfa.\n\
Reads empty line separated automata from stdin and outputs empty-line\n\
separated reverse automata to the stdout.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
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
    if (0 == strncmp ("--spec-any=", *argv, 11)) {
      g_use_any = true;
      g_spec_any = atoi (&((*argv) [11]));
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



inline void Process (std::istream * pIs, std::ostream * pOs)
{
    DebugLogAssert (pIs && pOs);

    FAAutIOTools io (&g_alloc);

    FARSNfa_ro nfa_in (&g_alloc);
    FARSNfa_wo_ro nfa_out (&g_alloc);

    // setup main processor
    FARSNfa2RevNfa rev (&g_alloc);

    rev.SetInNfa (&nfa_in);
    rev.SetOutNfa (&nfa_out);

    if (true == g_use_any) {
        rev.SetAnyIw (g_spec_any);
    }

    while (!(pIs->eof ())) {

        // read input from stdin
        io.Read (*pIs, &nfa_in);

        if (-1 == nfa_in.GetMaxIw () && -1 == nfa_in.GetMaxState ())
            break;

        // make processing
        rev.Process ();

        if (false == g_no_output) {
            io.Print (*pOs, &nfa_out);
        }

        // clear Nfas
        nfa_in.Clear ();
        nfa_out.Clear ();
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

        Process (pIs, pOs);

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
