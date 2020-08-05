/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FASetUtils.h"
#include "FAMapIOTools.h"
#include "FASubstRules2Regexp.h"
#include "FATagSet.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);

const char * pInputEnc = "CP1251";

const char * pInFile = NULL;
const char * pInTagSetFile = NULL;
const char * pOutFile = NULL;
const char * pActionFile = NULL;

bool g_reverse = false;


void usage () {

  std::cout << "\n\
Usage: fa_subst2re [OPTIONS]\n\
\n\
This program converts substitution rules into a single digital regular\n\
expression and builds rules action maps.\n\
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
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    CP1251 is used by default\n\
\n\
  --rev - right part tags will be put to the beginning of the corresponding\n\
    left part of the rule, such a regular expression is ought to be compiled\n\
    with --rev switch too\n\
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
    if (0 == strncmp ("--rev", *argv, 5)) {
      g_reverse = true;
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

        FASubstRules2Regexp converter (&g_alloc);

        if (NULL != pInTagSetFile) {
            converter.SetTagSet (&tagset);
        }
        converter.SetEncodingName (pInputEnc);
        converter.SetReverse (g_reverse);
        converter.SetRulesIn (pIs);
        converter.SetRegexpOut (pOs);

        converter.Process ();

        const FAMultiMapA * pActs = converter.GetActions ();
        DebugLogAssert (pActs);

        g_map_io.Print (*pActionsOs, pActs);

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
