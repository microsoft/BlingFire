/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FALimits.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FATrWordIOTools_utf8.h"
#include "FAMultiMap_pack_fixed.h"
#include "FAImageDump.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

int g_MinPatLen = 3;
int g_MaxPatLen = 8;
int g_spec_l_anchor = FAFsmConst::IW_L_ANCHOR;
int g_spec_r_anchor = FAFsmConst::IW_R_ANCHOR;

const char * pInFile = NULL;
const char * pOutFile = NULL;

bool g_ignore_case = false;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;
const FAMultiMapCA * g_pMap = NULL;

int g_Iws [FALimits::MaxWordLen];
int g_Ows [FALimits::MaxWordLen];
int g_Count;


void usage () {

  std::cout << "\n\
Usage: fa_hyph2chains [OPTIONS]\n\
\n\
This program converts hyphenation dictionary entries into a list of digital\n\
chain suffixes. Input should be in UTF-8.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --min-length=N - sets up mimimal suffix length, 3 is used by default\n\
\n\
  --max-length=N - sets up maximal suffix length, 8 is used by default\n\
\n\
  --spec-l-anchor=N - specifies weight for the beginning of the sequence,\n\
    the default is 1\n\
\n\
  --spec-r-anchor=N - specifies weight for the end of the sequence,\n\
    the default is 2\n\
\n\
  --ignore-case - converts input symbols to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
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
        if (0 == strncmp ("--min-length=", *argv, 13)) {
            g_MinPatLen = atoi (&((*argv) [13]));
            continue;
        }
        if (0 == strncmp ("--max-length=", *argv, 13)) {
            g_MaxPatLen = atoi (&((*argv) [13]));
            continue;
        }
        if (0 == strncmp ("--spec-l-anchor=", *argv, 16)) {
            g_spec_l_anchor = atoi (&((*argv) [16]));
            continue;
        }
        if (0 == strncmp ("--spec-r-anchor=", *argv, 16)) {
            g_spec_r_anchor = atoi (&((*argv) [16]));
            continue;
        }
        if (0 == strncmp ("--charmap=", *argv, 10)) {
            g_pCharMapFile = &((*argv) [10]);
            g_pMap = &g_charmap;
            continue;
        }
        if (0 == strcmp ("--ignore-case", *argv)) {
            g_ignore_case = true;
            continue;
        }
    }
}


///
/// prints all non-empty suffixes not greater than g_MaxPatLen and
/// not less than g_MinPatLen
/// 
void PrintSuffs (std::ostream * pOs)
{
    DebugLogAssert (pOs);
    DebugLogAssert (0 < g_Count && g_Count <= FALimits::MaxWordLen);

    const int iMax = g_Count - g_MinPatLen;

    for (int i = 0; i <= iMax; ++i) {

        int L = g_MaxPatLen;
        if (g_MaxPatLen + i > g_Count) {
            L = g_Count - i;
        }

        ::FAPrintChain (*pOs, g_Iws + i, L, FAFsmConst::DIR_L2R, 4, true);
        (*pOs) << ' ';
        ::FAPrintChain (*pOs, g_Ows + i, L, FAFsmConst::DIR_L2R, 1, true);
        (*pOs) << '\n';
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
        // load normalization map, if needed
        if (g_pCharMapFile) {
            g_charmap_image.Load (g_pCharMapFile);
            const unsigned char * pImg = g_charmap_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_charmap.SetImage (pImg);
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        /// add left anchor
        g_Iws [0] = g_spec_l_anchor;
        g_Ows [0] = FAFsmConst::HYPH_NO_HYPH;

        // process the input

        std::string line;

        while (!(pIs->eof ())) {

            if (!std::getline (*pIs, line))
                break;

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            if (0 < LineLen) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }
            if (0 < LineLen) {

                /// str -> chains
                g_Count = FATrWordIOTools_utf8::Str2IwOw (pLine, LineLen, \
                    g_Iws + 1, g_Ows + 1, FALimits::MaxWordLen - 2,
                    g_ignore_case, g_pMap);

                /// add right anchor
                if (g_Count + 1 < FALimits::MaxWordLen) {
                    g_Iws [g_Count + 1] = g_spec_r_anchor;
                    g_Ows [g_Count + 1] = FAFsmConst::HYPH_NO_HYPH;
                }

                g_Count += 2;

                // print all possible suffixes
                PrintSuffs (pOs);
            }

        } // of while (!(pIs->eof ())) ...

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
