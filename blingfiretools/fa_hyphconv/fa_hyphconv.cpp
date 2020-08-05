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
#include "FAUtf8Utils.h"
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

int g_spec_l_anchor = FAFsmConst::IW_L_ANCHOR;
int g_spec_r_anchor = FAFsmConst::IW_R_ANCHOR;

const char * pInFile = NULL;
const char * pOutFile = NULL;

bool g_m2h = false;
bool g_ignore_case = false;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;
const FAMultiMapCA * g_pMap = NULL;

const int MaxChainSize = FALimits::MaxWordLen * 3;
int g_Chain [MaxChainSize];

int g_Iws [FALimits::MaxWordLen];
int g_Ows [FALimits::MaxWordLen];
int g_Count;


void usage () {

  std::cout << "\n\
Usage: fa_hyphconv [OPTIONS]\n\
\n\
This program converts between hyphenation patterns in human-readable and \n\
machine-readable formats. The input should be in UTF-8.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --m2h - converts machine int ohuman readable format,\n\
    otherwise by default\n\
\n\
  --spec-l-anchor=N - specifies output weight for the left anchor \"^\",\n\
    the default is 1\n\
\n\
  --spec-r-anchor=N - specifies output weight for the right anchor \"$\",\n\
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
        if (0 == strcmp ("--m2h", *argv)) {
            g_m2h = true;
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

    std::string line;
    int LineNum = -1;

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

        // process the input

        int i;
        char Utf8Char [FAUtf8Const::MAX_CHAR_SIZE + 1];

        while (!(pIs->eof ())) {

            if (!std::getline (*pIs, line))
                break;

            LineNum++;

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            if (0 < LineLen) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }
            if (0 < LineLen) {

                // human --> machine
                if (!g_m2h) {

                    /// str -> input and output chains
                    g_Count = FATrWordIOTools_utf8::Str2IwOw (pLine, LineLen, \
                        g_Iws, g_Ows, FALimits::MaxWordLen, g_ignore_case, g_pMap);
                    FAAssert (0 < g_Count, FAMsg::IOError);

                    /// print the Key string
                    for (i = 0; i < g_Count; ++i) {

                        int Iw = g_Iws [i];

                        if (int ('^') == Iw) {
                            Iw = g_spec_l_anchor;
                        } else if (int ('$') == Iw) {
                            Iw = g_spec_r_anchor;
                        }
                        FAAssert (0 < ::FAUtf8Size (Iw) && \
                            FAUtf8Const::MAX_CHAR_SIZE >= ::FAUtf8Size (Iw), FAMsg::IOError);

                        char * pEnd = ::FAIntToUtf8 (Iw, Utf8Char, FAUtf8Const::MAX_CHAR_SIZE);
                        FAAssert (pEnd, FAMsg::IOError);
                        *pEnd = 0;
                        (*pOs) << Utf8Char;
                    }

                    /// print a tab-separated action
                    for (i = 0; i < g_Count; ++i) {
                        const int Ow = g_Ows [i];
                        (*pOs) << '\t' << Ow;
                    }

                // machine --> human
                } else {

                    g_Count = ::FAStrUtf8ToArray (pLine, LineLen, \
                            g_Chain, MaxChainSize);
                    FAAssert (0 < g_Count, FAMsg::IOError);

                    int IwCount = 0;
                    int OwCount = 0;

                    for (i = 0; i < g_Count; ++i) {

                        int Iw = g_Chain [i];

                        if (int ('\t') == Iw) {
                            break;
                        } else if (g_spec_l_anchor == Iw) {
                            Iw = int ('^');
                        } else if (g_spec_r_anchor == Iw) {
                            Iw = int ('$');
                        }

                        g_Iws [IwCount++] = Iw;
                    }
                    for (; i < g_Count; ++i) {

                        const int Ow = g_Chain [i];

                        if (int ('\t') != Ow) {
                            g_Ows [OwCount++] = Ow;
                        }
                    }
                    FAAssert (IwCount == OwCount, FAMsg::IOError);

                    const int MaxOutSize = FAUtf8Const::MAX_CHAR_SIZE * FALimits::MaxWordLen;
                    char Out [MaxOutSize];

                    const int StrLen = FATrWordIOTools_utf8::
                        IwOw2Str (g_Iws, g_Ows, IwCount, Out, MaxOutSize - 1);
                    Out [StrLen] = 0;

                    (*pOs) << Out;

                } // of if (!g_m2h) ...

                (*pOs) << '\n';

            } // of if if (0 < LineLen) ...

        } // of while (!(pIs->eof ())) ...

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

        std::cerr << "ERROR: in data at line: " << LineNum << " in \"" \
            << line << "\"\n";

        return 1;
    }

    return 0;
}
