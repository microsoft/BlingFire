/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FARSDfa_ro.h"
#include "FARSDfa_ar_judy.h"
#include "FAState2Ows_ar_uniq.h"
#include "FADict2Classifier.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pInOw2FreqFile = NULL;

bool g_extend_ows_map = true;
bool g_extend_finals = false;
bool g_no_output = false;

int g_min_depth = 3;
int g_ows_merge_type = 0;
int g_ows_bound = 100;

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);


void usage () {

  std::cout << "\n\
Usage: fa_dict2classifier [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program builds a classifier from a dictionary.\n\
\n\
  --in=<input-file> - input Moore Multi Dfa in textual form,\n\
    if omited stdin is used\n\
\n\
  --in-ow2f=<input-file> - reads Ow -> Freq array, if specified\n\
    is not used by default\n\
\n\
  --out=<output-file> - output Moore Multi Dfa,\n\
    if omited stdout is used\n\
\n\
  --min-depth=N - sets up minimum prefix length starting from which\n\
    each state in automaton will have a reaction, 3 is used by default\n\
\n\
  --keep-state2ows - won't make any extension of State -> Ows map,\n\
    --min-depth=N parameter will be ignored\n\
\n\
  --ows-merge=<type> - specifies how Ows are merged, if extending state2ows\n\
    or - union of Ows, is used by default\n\
    and - intersection of Ows\n\
\n\
  --ows-bound=N - sets up % of Ows to be used for State2Ow extension, from\n\
    more to less frequent; all are taken by default; used with --in-ow2f only\n\
\n\
  --extend-finals - states with added output weights due to extension will\n\
    be marked as final, won't be by default\n\
\n\
  --no-output - makes no output\n\
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
        if (0 == strncmp ("--in-ow2f=", *argv, 10)) {
            pInOw2FreqFile = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--min-depth=", *argv, 12)) {
            g_min_depth = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strncmp ("--ows-bound=", *argv, 12)) {
            g_ows_bound = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strcmp ("--no-output", *argv)) {
            g_no_output = true;
            continue;
        }
        if (0 == strcmp ("--keep-state2ows", *argv)) {
            g_extend_ows_map = false;
            continue;
        }
        if (0 == strcmp ("--ows-merge=or", *argv)) {
            g_ows_merge_type = 0;
            continue;
        }
        if (0 == strcmp ("--ows-merge=and", *argv)) {
            g_ows_merge_type = 1;
            continue;
        }
        if (0 == strcmp ("--extend-finals", *argv)) {
            g_extend_finals = true;
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

        // select in/out streams
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

        FARSDfa_ro input_dfa (&g_alloc);
        FAState2Ows_ar_uniq input_ows (&g_alloc);
        FARSDfa_ar_judy output_dfa (&g_alloc);
        FAState2Ows_ar_uniq output_ows (&g_alloc);
        FADict2Classifier build_classifier (&g_alloc);

        /// read input Dfa
        g_io.Read (*pIs, &input_dfa, &input_ows);

        /// read Ow -> Freq array, if any
        if (NULL != pInOw2FreqFile) {

            std::ifstream ifs_ow2f (pInOw2FreqFile, std::ios::in);
            FAAssertStream (&ifs_ow2f, pInOw2FreqFile);

            const int * pOw2Freq = NULL;
            int Count = 0;
            g_map_io.Read (ifs_ow2f, &pOw2Freq, &Count);
            FAAssert (pOw2Freq && 0 < Count, FAMsg::IOError);

            build_classifier.SetOw2Freq (pOw2Freq, Count);
            build_classifier.SetOwsBound (g_ows_bound);
        }

        build_classifier.SetMinDepth (g_min_depth);
        build_classifier.SetExtendState2Ows (g_extend_ows_map);
        build_classifier.SetOwsMergeType (g_ows_merge_type);
        build_classifier.SetExtendFinals (g_extend_finals);
        build_classifier.SetInMooreDfa (&input_dfa, &input_ows);
        build_classifier.SetOutMooreDfa (&output_dfa, &output_ows);
        build_classifier.Process ();

        if (false == g_no_output) {
            g_io.Print (*pOs, &output_dfa, &output_ows);
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

