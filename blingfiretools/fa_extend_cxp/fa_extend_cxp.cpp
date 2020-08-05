/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAutIOTools.h"
#include "FARSDfa_ro.h"
#include "FAState2Ows_ar_uniq.h"
#include "FAExtendClassifier_cxp.h"
#include "FAAllocator.h"
#include "FAUtils.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";


FAAllocator g_alloc;

const char * g_pInFsmFile = NULL;
const char * g_pDataFile = NULL;
const char * g_pOutFsmFile = NULL;

int g_base = 10;

const int MaxChainSize = 4096;

int g_ChainSize = 0;
int g_Chain [MaxChainSize];
int g_Class = 0;
int g_Count = 0;

int g_max_prob = 255;

std::string line;
unsigned int LineNum = 0;


void usage () {

  std::cout << "\n\
Usage: fa_extend_cxp [OPTION]\n\
\n\
This program extends State -> Ows map for unannotated states by the data taken\n\
from the <list>.  Each such state  will get assigned  an array  of classes and\n\
probabilities P(Class|State) mapped into an integer range 0..<max-prob>.\n\
\n\
\n\
  --in=<input-fsm> - input Moore Multi Dfa in textual form,\n\
    if omited stdin is used\n\
\n\
  --in-data=<list> - reads the CHAIN\\tCLASSt\\tCOUNT\\n list\n\
    and calculates State -> Ows for states with no reaction,\n\
    if omited stdin is used\n\
\n\
  --out=<output-fsm> - output Moore Multi Dfa in textual form,\n\
    if omited stdout is used\n\
\n\
  --max-prob=<max-prob> - specifies the maximum integer value for the 1.0 of\n\
    the P(Class|State) probability, 255 is used by default\n\
\n\
  --base=N - CHAIN, CLASS and COUNT number's base, 10 is used by default\n\
    (use 16 for more compact textual representation)\n\
\n\
\n\
Note: The class and probability values are combined by the following formula:\n\
  Ow == (Class * (<max-prob> + 1)) + Prob\n\
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
        g_pInFsmFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--in-data=", *argv, 10)) {
        g_pDataFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        g_pOutFsmFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--max-prob=", *argv, 11)) {
        g_max_prob = atoi (&((*argv) [11]));
        continue;
    }
    if (0 == strncmp ("--base=", *argv, 7)) {
        g_base = atoi (&((*argv) [7]));
        continue;
    }
  }
}


void ParseLine (const char * pChainStr, const int ChainStrLen)
{
    DebugLogAssert (pChainStr && 0 < ChainStrLen);

    const char * pChainStrEnd = pChainStr + ChainStrLen;
    g_ChainSize = 0;

    while (pChainStr < pChainStrEnd) {

        FAAssert (g_ChainSize < MaxChainSize, FAMsg::IOError);

        g_Chain [g_ChainSize++] = strtol (pChainStr, NULL, g_base);

        pChainStr = strchr (pChainStr, ' ');

        if (NULL == pChainStr)
            break;

        pChainStr++;
    }

    FAAssert (2 < g_ChainSize, FAMsg::IOError);

    g_Class = g_Chain [g_ChainSize - 2];
    g_Count = g_Chain [g_ChainSize - 1];
    g_ChainSize -= 2;

    FAAssert (0 < g_Class && 0 < g_Count, FAMsg::IOError);
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        /// open input streams
        std::istream * pFsmIs = &std::cin;
        std::ifstream fsm_ifs;

        if (NULL != g_pInFsmFile) {
            fsm_ifs.open (g_pInFsmFile, std::ios::in);
            FAAssertStream (&fsm_ifs, g_pInFsmFile);
            pFsmIs = &fsm_ifs;
        }

        std::istream * pDataIs = &std::cin;
        std::ifstream data_ifs;

        if (NULL != g_pDataFile) {
            data_ifs.open (g_pDataFile, std::ios::in);
            FAAssertStream (&data_ifs, g_pDataFile);
            pDataIs = &data_ifs;
        }

        DebugLogAssert (pFsmIs && !pFsmIs->eof ());
        DebugLogAssert (pDataIs && !pDataIs->eof ());

        /// create objects
        FAAutIOTools g_io (&g_alloc);

        FARSDfa_ro input_dfa (&g_alloc);
        FAState2Ows_ar_uniq input_ows (&g_alloc);

        FAExtendClassifier_cxp classifier (&g_alloc);

        /// read the input Dfa
        g_io.Read (*pFsmIs, &input_dfa, &input_ows);

        classifier.SetFsm (&input_dfa, &input_ows);
        classifier.SetMaxProb (g_max_prob);

        /// read the input data

        while (!pDataIs->eof ()) {

            if (!std::getline (*pDataIs, line))
                break;

            LineNum++;

            if (!line.empty ()) {

                const char * pChainStr = line.c_str ();
                const int ChainStrLen = (const int) line.length ();
                DebugLogAssert (pChainStr && 0 < ChainStrLen);

                /// get Chain, Class, Count
                ParseLine (pChainStr, ChainStrLen);

                /// update the statistics
                classifier.AddStat (g_Chain, g_ChainSize, g_Class, g_Count);
            }
        }

        /// make final calculations
        classifier.Process ();

        /// open the output stream

        std::ostream * pFsmOs = &std::cout;
        std::ofstream fsm_ofs;

        if (NULL != g_pOutFsmFile) {
            fsm_ofs.open (g_pOutFsmFile, std::ios::out);
            pFsmOs = &fsm_ofs;
        }

        const FAState2OwsA * pNewOws = classifier.GetOws ();

        g_io.Print (*pFsmOs, &input_dfa, pNewOws);

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        std::cerr << "ERROR: in data at line: " << LineNum << " in \"" \
            << line << "\"\n";

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    return 0;
}
