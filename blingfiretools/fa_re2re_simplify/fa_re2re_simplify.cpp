/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAPrintUtils.h"
#include "FARegexpTree.h"
#include "FARegexpSimplify.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

int g_LabelType = FAFsmConst::LABEL_DIGIT;
const char * pInputEnc = NULL;

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pTreeOutFile = NULL;

bool g_reverse = false;
bool g_no_output = false;


void usage () {

  std::cout << "\n\
Usage: fa_re2re_simplify [OPTIONS] [< regexp.txt] [> regexp.txt]\n\
\n\
This program does regular expression simplification.\n\
\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-tree=<output-file> - writes regular expression tree after\n\
    simplification to <output-file>, does not write if not specified\n\
\n\
  --label=<label-type> - the following types are available:\n\
\n\
    digit - assumes unsigned ints as a regular expression symbols,\n\
      the default value\n\
    char  - assumes chracters of 8-bit encoding or UTF-8 as input symbols\n\
    wre   - assumes WRE tokens as input symbols\n\
\n\
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    can be used with --label=char only, if not specified then\n\
    byte representation of characters is used for automaton input weights\n\
\n\
  --rev - for right to left rules\n\
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
        if (0 == strncmp ("--in=", *argv, 5)) {
            pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out-tree=", *argv, 12)) {
            pTreeOutFile = &((*argv) [12]);
            continue;
        }
        if (0 == strcmp ("--label=digit", *argv)) {
            g_LabelType = FAFsmConst::LABEL_DIGIT;
            continue;
        }
        if (0 == strcmp ("--label=char", *argv)) {
            g_LabelType = FAFsmConst::LABEL_CHAR;
            continue;
        }
        if (0 == strcmp ("--label=wre", *argv)) {
            g_LabelType = FAFsmConst::LABEL_WRE;
            continue;
        }
        if (0 == strncmp ("--input-enc=", *argv, 12)) {
            pInputEnc = &((*argv) [12]);
            continue;
        }
        if (0 == strncmp ("--rev", *argv, 5)) {
            g_reverse = true;
            continue;
        }
        if (0 == strncmp ("--no-output", *argv, 11)) {
            g_no_output = true;
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

    try {

        // adjust input/output
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        std::ostream * pTreeOs = NULL;
        std::ofstream tree_ofs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (NULL != pTreeOutFile) {
            tree_ofs.open (pTreeOutFile, std::ios::out);
            pTreeOs = &tree_ofs;
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        FARegexpSimplify simplifier (&g_alloc);

        simplifier.SetReverse (g_reverse);
        simplifier.SetLabelType (g_LabelType);
        simplifier.SetUseUtf8 (::FAIsUtf8Enc (pInputEnc));

        std::string line;

        while (!pIs->eof ()) {

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

            const char * pRegexp = Re.c_str ();
            const int Length = (const int) Re.length ();

            if (0 < Length) {

                simplifier.SetRegexp (pRegexp, Length);
                simplifier.Process ();

                const char * pSimpleRe = simplifier.GetRegexp ();
                DebugLogAssert (pSimpleRe);

                if (false == g_no_output) {

                    (*pOs) << pSimpleRe << '\n';

                    if (pTreeOs) {
                        ::FAPrintRegexpTree (*pTreeOs, simplifier.GetRegexpTree (), pRegexp);
                    }
                }

            } // if (0 < ReLength)

            (*pOs) << '\n';

        } // of while (!pIs->eof ()) ...


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
