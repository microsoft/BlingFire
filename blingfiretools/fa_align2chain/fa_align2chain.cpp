/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAPrintUtils.h"
#include "FAStringTokenizer.h"
#include "FAChain2Num_hash.h"
#include "FAMultiMap_ar.h"
#include "FAMapIOTools.h"
#include "FAException.h"

#include <iostream>
#include <string>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pOwsFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

std::ostream * g_pOwsOs = &std::cout;
std::ofstream g_ows_ofs;

int g_OwBase = 0;

bool g_no_output = false;
bool g_no_process = false;

const int MaxBuffSize = FALimits::MaxWordLen * 4;
int g_Chain [MaxBuffSize];
int g_Tmp [MaxBuffSize];


void usage ()
{
  std::cout << "\n\
Usage: fa_align2chain [OPTIONS]\n\
\n\
This tool aligns two tab separated input UTF-8 strings of each line.\n\
The tool prints left-string character followed by the corresponding \n\
right-string character, if there is a gap the filler character <C> is used.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --out-ows=<file> - writes Ows --> string map into a <file>,\n\
    stdout is used by default\n\
\n\
  --ow-base=N - base value for output weights, 0 is used by default\n\
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
            g_pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out-ows=", *argv, 10)) {
            g_pOwsFile = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--ow-base=", *argv, 10)) {
            g_OwBase = atoi (&((*argv) [10]));
            continue;
        }
        if (0 == strncmp ("--no-output", *argv, 11)) {
            g_no_output = true;
            continue;
        }
        if (0 == strncmp ("--no-process", *argv, 12)) {
            g_no_process = true;
            continue;
        }
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // process command line
    process_args (argc, argv);

    int LineNum = -1;
    std::string line;

    try {

        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }
        if (g_pOwsFile) {
            g_ows_ofs.open (g_pOwsFile, std::ios::out);
            g_pOwsOs = &g_ows_ofs;
        }

        FAMapIOTools g_map_io (&g_alloc);

        FAStringTokenizer tokenizer;
        tokenizer.SetSpaces (" ");

        // out str --> code
        FAChain2Num_hash m_act2num;
        m_act2num.SetAllocator (&g_alloc);
        // code --> out str
        FAMultiMap_ar m_acts;
        m_acts.SetAllocator (&g_alloc);

        while (!(g_pIs->eof ())) {

            if (!std::getline (*g_pIs, line))
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

                /// read the input

                tokenizer.SetString (pLine, LineLen);

                int * pOut = g_Chain;

                while (pOut - g_Chain < MaxBuffSize) {

                    const char * pStr1 = NULL;
                    int Len1 = 0;
                    if (!tokenizer.GetNextStr (&pStr1, &Len1))
                        break;
                    FAAssert (0 < Len1 && pStr1, FAMsg::IOError);

                    const char * pStr2 = NULL;
                    int Len2 = 0;
                    if (!tokenizer.GetNextStr (&pStr2, &Len2))
                        break;
                    FAAssert (0 < Len2 && pStr2, FAMsg::IOError);

                    const int Count1 = \
                        ::FAStrUtf8ToArray (pStr1, Len1, pOut, 1);
                    FAAssert (1 == Count1, FAMsg::IOError);
                    pOut++;

                    const int Count2 = \
                        ::FAStrUtf8ToArray (pStr2, Len2, g_Tmp, MaxBuffSize);
                    FAAssert (0 < Count2 && MaxBuffSize > Count2, \
                        FAMsg::IOError);

                    // g_Tmp will not be added if it's already in the map
                    const int Ow = g_OwBase + m_act2num.Add (g_Tmp, Count2, 0);

                    *pOut++ = Ow;
                }

                const int ChainSize = int (pOut - g_Chain);
                FAAssert (0 < ChainSize && MaxBuffSize > ChainSize && \
                    0 == ChainSize % 2, FAMsg::IOError);

                ::FAPrintChain (*g_pOs, g_Chain, ChainSize, \
                    FAFsmConst::DIR_L2R, 5, false);

                *g_pOs << '\n';
            }

        } // of while (!(g_pIs->eof ())) ...

        // reverse the Ows map

        const int ChainCount = m_act2num.GetChainCount ();
        DebugLogAssert (0 < ChainCount);

        for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

            const int * pChain;
            const int ChainSize = m_act2num.GetChain (ChainIdx, &pChain);
            m_acts.Set (ChainIdx, pChain, ChainSize);

        } // of for (int ChainIdx = 0; ...

        g_map_io.Print (*g_pOwsOs, &m_acts);

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

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
