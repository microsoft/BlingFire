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
#include "FARSNfa_wo_ro.h"
#include "FATokenNfa2TupleNfa.h"
#include "FATagSet.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * g_pIn1File = NULL;
const char * g_pIn2File = NULL;
const char * g_pIn3File = NULL;
const char * g_pInTagSetFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pOutConfFile = NULL;

int g_TokenNumBase = 0;
int g_ignore_base = -1;
int g_ignore_max = -1;


void usage () {

  std::cout << "\n\
Usage: fa_tnnfa2nfa [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
Substitutes Iws of the input Token Nfa with tuples of Ows of digitizers in\n\
the following order < TXT, TAG, DCT >, where any of the tuple components is\n\
optional.\n\
\n\
Reads:\n\
  1. Non-deterministic automaton with TokenNums as input weights.\n\
  2. Mapping from Digitizer\\TokenNums to CNFs of TypeNums.\n\
  3. Mapping from TypeNums to sets of output weights of the digitizer.\n\
  4. Tagset, if needed.\n\
\n\
  --in1=<input-file> - reads non-deterministic automaton from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --in2=<input-file> - reads mapping from TokenNums to CNFs on TypeNums\n\
    from the <input-file>, if omited stdin is used\n\
\n\
  --in3=<input-file> - reads mapping from TypeNums to sets of output weights\n\
    of the digitizer from the <input-file>, if omited stdin is used\n\
\n\
  --tagset=<input-file> - reads tagset from input file for automatic generation\n\
    of TypeNum -> Ows mapping, does use --in3=X in this case\n\
\n\
  --out=<output-file1> - writes an ordered list of elementary types extracted\n\
    from tokens into <output-file1>, if omited stdout is used\n\
\n\
  --out-conf=<output> - writes down run-time configuration options,\n\
    does not write by default\n\
\n\
  --tn-base=Iw - the starting Iw for token nums, if --digitizer=tag specified\n\
    then it also specifies base value for tagset Ows\n\
\n\
  --ignore-base=Iw - specifies base Iw for ignore-range for --alg=tuple,\n\
    does not ignore no Iws by default\n\
\n\
  --ignore-max=Iw - specifies max Iw for ignore-range for --alg=tuple,\n\
    does not ignore no Iws by default\n\
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
    if (0 == strncmp ("--in1=", *argv, 6)) {
        g_pIn1File = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--in2=", *argv, 6)) {
        g_pIn2File = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--in3=", *argv, 6)) {
        g_pIn3File = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pInTagSetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        g_pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--out-conf=", *argv, 11)) {
        g_pOutConfFile = &((*argv) [11]);
        continue;
    }
    if (0 == strncmp ("--tn-base=", *argv, 10)) {
        g_TokenNumBase = atoi (&((*argv) [10]));
        continue;
    }
    if (0 == strncmp ("--ignore-max=", *argv, 13)) {
        g_ignore_max = atoi (&((*argv) [13]));
        continue;
    }
    if (0 == strncmp ("--ignore-base=", *argv, 14)) {
        g_ignore_base = atoi (&((*argv) [14]));
        continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    /// parse a command line
    process_args (argc, argv);

    try {

        FAAutIOTools aut_io (&g_alloc);
        FAMapIOTools map_io (&g_alloc);

        FAMultiMap_judy Token2CNF;
        FAMultiMap_judy Type2Ows;
        FATagSet tagset (&g_alloc);
        FARSNfa_ro TokenNfa (&g_alloc);
        FARSNfa_wo_ro TupleNfa (&g_alloc);

        FATokenNfa2TupleNfa tnnfa2nfa (&g_alloc);

        Type2Ows.SetAllocator (&g_alloc);
        Token2CNF.SetAllocator (&g_alloc);

        /// adjust IO pointers

        std::istream * pIs1 = &std::cin;
        std::ifstream ifs1;

        std::istream * pIs2 = &std::cin;
        std::ifstream ifs2;

        std::istream * pIs3 = &std::cin;
        std::ifstream ifs3;

        if (NULL != g_pIn1File) {
            ifs1.open (g_pIn1File, std::ios::in);
            FAAssertStream (&ifs1, g_pIn1File);
            pIs1 = &ifs1;
        }
        if (NULL != g_pIn2File) {
            ifs2.open (g_pIn2File, std::ios::in);
            FAAssertStream (&ifs2, g_pIn2File);
            pIs2 = &ifs2;
        }
        if (NULL != g_pIn3File) {
            ifs3.open (g_pIn3File, std::ios::in);
            FAAssertStream (&ifs3, g_pIn3File);
            pIs3 = &ifs3;
        }
        if (NULL != g_pInTagSetFile) {
            std::ifstream tagset_ifs (g_pInTagSetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pInTagSetFile);
            map_io.Read (tagset_ifs, &tagset);
        }

        DebugLogAssert (pIs1);
        DebugLogAssert (pIs2);
        DebugLogAssert (pIs3);

        /// load objects

        aut_io.Read (*pIs1, &TokenNfa);
        map_io.Read (*pIs2, &Token2CNF);
        map_io.Read (*pIs3, &Type2Ows);

        /// set up the processor and process

        tnnfa2nfa.SetTokenNfa (&TokenNfa);
        tnnfa2nfa.SetOutNfa (&TupleNfa);
        tnnfa2nfa.SetTnBaseIw (g_TokenNumBase);
        tnnfa2nfa.SetCNF (&Token2CNF);
        tnnfa2nfa.SetType2Ows (&Type2Ows);
        tnnfa2nfa.SetIgnoreBase (g_ignore_base);
        tnnfa2nfa.SetIgnoreMax (g_ignore_max);

        if (g_pInTagSetFile) {
            tnnfa2nfa.SetTagSet (&tagset);
        }

        /// processing

        tnnfa2nfa.Process ();

        /// write results

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != g_pOutFile) {
            ofs.open (g_pOutFile, std::ios::out);
            pOs = &ofs;
        }

        aut_io.Print (*pOs, &TupleNfa);

        if (NULL != g_pOutConfFile) {

            std::ofstream conf_ofs (g_pOutConfFile, std::ios::out);

            const int TokenType = tnnfa2nfa.GetTokenType ();

            if (FAFsmConst::WRE_TT_TEXT & TokenType)
                conf_ofs << "token-type txt\n";
            if (FAFsmConst::WRE_TT_TAGS & TokenType)
                conf_ofs << "token-type tag\n";
            if (FAFsmConst::WRE_TT_DCTS & TokenType)
                conf_ofs << "token-type dct\n";
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
