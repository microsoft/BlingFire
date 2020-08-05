/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FARSDfaA.h"
#include "FAAutIOTools.h"
#include "FAStringTokenizer.h"
#include "FAChains2MinDfa_sort.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);

const char * pOutFileName = NULL;

const char * g_pInFile = NULL;
std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

bool g_no_output = false;
int g_base = 10;

const int MaxChainSize = 4096;
int ChainBuffer [MaxChainSize];


void usage () {

  std::cout << "\n\
Usage: fa_chains2mindfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program builds minimal deterministic Rabin Scott automaton from\n\
the list of one-space-separated integer chains.\n\
\n\
  --base=hex - reads digits in hexadecimal format,\n\
    uses decimal base by default\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output in binary format to the <output-file>,\n\
    if omited writes the output in textual format to stdout\n\
\n\
  --no-output - does not do any output\n\
\n\
\n\
Input example for --algo=sort* :\n\
...\n\
100 100 102 13212\n\
100 100 102 13212 10\n\
101\n\
...\n\
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
    if (0 == strncmp ("--out=", *argv, 6)) {
      pOutFileName = & ((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
      g_pInFile = &((*argv) [5]);
      continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--base=hex", *argv, 10)) {
      g_base = 16;
      continue;
    }
  }
}


template < class _TChains2MinDfa >
void MakeConstruction (_TChains2MinDfa * pChains2MinDfa)
{
    DebugLogAssert (pChains2MinDfa);
    DebugLogAssert (g_pIs);

    FAStringTokenizer tokenizer;

    int TmpInt;
    std::string line;

    while (!g_pIs->eof ()) {

        if (!std::getline (*g_pIs, line))
            break;

        if (!line.empty ()) {

            const char * pChainStr = line.c_str ();
            const int ChainStrLen = (const int) line.length ();
            DebugLogAssert (pChainStr && 0 < ChainStrLen);

            tokenizer.SetString (pChainStr, ChainStrLen);
            int ChainSize = 0;

            while (tokenizer.GetNextInt (&TmpInt, g_base)) {

                DebugLogAssert (ChainSize < MaxChainSize);

                ChainBuffer [ChainSize] = TmpInt;
                ChainSize++;
            }

            // add the chain
            pChains2MinDfa->AddChain (ChainBuffer, ChainSize); 
        }
    }

    pChains2MinDfa->Prepare ();
}



void Print (const FARSDfaA * pAutomaton)
{
    if (NULL == pOutFileName) {

        g_io.Print (std::cout, pAutomaton);

    } else {

        std::ofstream ofs (pOutFileName, std::ios::out);
        g_io.Print (ofs, pAutomaton);
        ofs.close ();
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

        if (NULL != g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }

        FAChains2MinDfa_sort chains2mdfa (&g_alloc);

        MakeConstruction (&chains2mdfa);

        if (false == g_no_output) {
            Print (&chains2mdfa);
        }

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: \"" << pErrMsg << "\" in file " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    return 0;
}
