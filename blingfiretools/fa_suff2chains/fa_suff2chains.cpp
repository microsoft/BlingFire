/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FASetUtils.h"
#include "FAMapIOTools.h"
#include "FATagSet.h"
#include "FASuffixRules2Chains.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);

int g_key_base = 0;
int g_num_size = 5;
bool g_imp_delim = false;
const char * pInputEnc = "CP1251";

const char * pInFile = NULL;
const char * pInTagSetFile = NULL;
const char * pOutFile = NULL;
const char * pActionFile = NULL;
const char * pOw2FreqFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_suff2chains [OPTIONS]\n\
\n\
This program converts suffix rules into a list of digital chains and builds\n\
action map.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --tagset=<input-file> - reads input tagset from the <input-file>,\n\
    if omited does not use tagset\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-actions=<actions> - writes action maps into <actions> file,\n\
    stdout is used by default\n\
\n\
  --out-ow2f=<ow2freq> - writes Ow -> Freq array, if specified\n\
    is not used by default\n\
\n\
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    CP1251 is used by default\n\
\n\
  --num-size=N - number of digitis per value/key,\n\
    5 is used by default\n\
\n\
  --key-base=N - adds N to all action numbers,\n\
    0 by default\n\
\n\
  --imp-delim - adds delimiter after each suffix regardless whether '^' was\n\
    specified or not\n\
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
        if (0 == strncmp ("--input-enc=", *argv, 12)) {
            pInputEnc = &((*argv) [12]);
            continue;
        }
        if (0 == strncmp ("--out-actions=", *argv, 14)) {
            pActionFile = &((*argv) [14]);
            continue;
        }
        if (0 == strncmp ("--out-ow2f=", *argv, 11)) {
            pOw2FreqFile = &((*argv) [11]);
            continue;
        }
        if (0 == strncmp ("--in=", *argv, 5)) {
            pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--tagset=", *argv, 9)) {
            pInTagSetFile = &((*argv) [9]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--key-base=", *argv, 11)) {
            g_key_base = atoi (&((*argv) [11]));
            continue;
        }
        if (0 == strncmp ("--imp-delim", *argv, 11)) {
            g_imp_delim = true;
            continue;
        }
        if (0 == strncmp ("--num-size=", *argv, 11)) {
            g_num_size = atoi (&((*argv) [11]));
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

    FATagSet tagset (&g_alloc);

    try {

        // adjust input/output

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        std::ostream * pActionsOs = &std::cout;
        std::ofstream ofs_actions;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pInTagSetFile) {
            std::ifstream ifs_tagset (pInTagSetFile, std::ios::in);
            FAAssertStream (&ifs_tagset, pInTagSetFile);
            g_map_io.Read (ifs_tagset, &tagset);
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (NULL != pActionFile) {
            ofs_actions.open (pActionFile, std::ios::out);
            pActionsOs = &ofs_actions;
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);
        DebugLogAssert (pActionsOs);

        FASuffixRules2Chains converter (&g_alloc);

        if (NULL != pInTagSetFile) {
            converter.SetTagSet (&tagset);
        }
        converter.SetEncodingName (pInputEnc);
        converter.SetKeyBase (g_key_base);
        converter.SetNumSize (g_num_size);
        converter.SetImplicitDelim (g_imp_delim);
        converter.SetRulesIn (pIs);
        converter.SetChainsOut (pOs);

        converter.Process ();

        // get action map
        const FAMultiMapA * pActs = converter.GetActions ();
        DebugLogAssert (pActs);
        // print action map
        g_map_io.Print (*pActionsOs, pActs);

        if (pOw2FreqFile) {
            // get Ow -> Freq array
            const int * pOw2Freq = NULL;
            const int Count = converter.GetOw2Freq (&pOw2Freq);
            DebugLogAssert (0 < Count && pOw2Freq);
            // print Ow -> Freq array
            std::ofstream ofs_ow2f (pOw2FreqFile, std::ios::out);
            g_map_io.Print (ofs_ow2f, pOw2Freq, Count);
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
