/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAWRERules2TokenNfa.h"
#include "FAChain2Num_hash.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <string>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pOutTokensFile = NULL;
const char * pInTokensFile = NULL;

int g_Type = FAFsmConst::WRE_TYPE_RS;


void usage () {

  std::cout << "\n\
Usage: fa_wre2tnnfa [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
This program builds Token NFA from the WRE rule(s).\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --in-tokens=<input-file> - reads in common Chain2Num map of tokens,\n\
    if omited is not used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-tokens=<output-file> - prints out common Chain2Num map of tokens,\n\
    if omited is not used\n\
\n\
  --type=<type> - is one of the following:\n\
    rs - Rabin-Scott automaton (text -> Yes/No), is used by default\n\
    moore - Moore automaton (text -> RuleNum)\n\
    mealy - Mealy automaton (text -> TrBr)\n\
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
    if (0 == strncmp ("--in=", *argv, 5)) {
        pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--out-tokens=", *argv, 13)) {
        pOutTokensFile = &((*argv) [13]);
        continue;
    }
    if (0 == strncmp ("--in-tokens=", *argv, 12)) {
        pInTokensFile = &((*argv) [12]);
        continue;
    }
    if (0 == strcmp ("--type=rs", *argv)) {
        g_Type = FAFsmConst::WRE_TYPE_RS;
        continue;
    }
    if (0 == strcmp ("--type=moore", *argv)) {
        g_Type = FAFsmConst::WRE_TYPE_MOORE;
        continue;
    }
    if (0 == strcmp ("--type=mealy", *argv)) {
        g_Type = FAFsmConst::WRE_TYPE_MEALY;
        continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        FAMapIOTools map_io (&g_alloc);
        FAAutIOTools aut_io (&g_alloc);

        FAChain2Num_hash token2num;

        token2num.SetAllocator (&g_alloc);
        token2num.SetCopyChains (true);

        /// adjust IO

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pInTokensFile) {
            std::ifstream token_ifs (pInTokensFile, std::ios::in);
            FAAssertStream (&token_ifs, pInTokensFile);
            map_io.Read (token_ifs, &token2num);
        }

        /// do processing

        FAWRERules2TokenNfa wre2nfa (&g_alloc);

        wre2nfa.SetType (g_Type);
        wre2nfa.SetToken2NumMap (&token2num);

        std::string line;

        while (!(pIs->eof ())) {

            std::string Re;

            // read until the empty line is met
            if (!std::getline (*pIs, line))
                break;

            while (0 < line.length ()) {

                Re = Re + line + "\n";

                if (pIs->eof ())
                    break;

                if (!std::getline (*pIs, line))
                    break;
            }

            const char * pRe = Re.c_str ();
            int ReLength = (const int) Re.length ();

            if (0 < ReLength) {

                DebugLogAssert (pRe);
                wre2nfa.AddRule (pRe, ReLength);

            } else {
                // break the outer loop, if RE is empty
                break;
            }

        } // of while (!(pIs->eof ())) ...

        // finish processing
        wre2nfa.Process ();

        const FARSNfaA * pTokenNfa = wre2nfa.GetTokenNfa ();

        // print the output

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }

        aut_io.Print (*pOs, pTokenNfa);

        if (NULL != pOutTokensFile) {
            std::ofstream token_ofs (pOutTokensFile, std::ios::out);
            map_io.Print (token_ofs, &token2num);
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
