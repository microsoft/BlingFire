/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtf32ToEnc.h"
#include "FAStr2Int_hash.h"
#include "FAException.h"

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>


using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

int g_min_order = 3;
int g_max_order = 3;
const char * g_pOutEnc = "UTF-8";
int g_line_step = -1;

bool g_no_output = false;
bool g_no_process = false;

const int MaxBuffSize = 4096 * 1024;
int g_Buff1 [MaxBuffSize];
char g_Buff2 [MaxBuffSize];


void usage ()
{
  std::cout << "\n\
Usage: fa_utf8tongrams_cp [OPTIONS]\n\
\n\
This tool collects byte ngrams in the specified encoding.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --output-enc=<encoding> - input encoding name or codepage number\n\
    UTF-8 is used by default\n\
\n\
  --max-order=N - takes all the n-grams of the order N and below,\n\
    3 is used by default\n\
\n\
  --min-order=N - takes all the n-grams of the order N and above,\n\
    3 is used by default\n\
\n\
  --line-step=N - the amount of lines processed at once,\n\
    by default full input is processed at once\n\
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
        if (0 == strncmp ("--output-enc=", *argv, 13)) {
            g_pOutEnc = &((*argv) [13]);
            continue;
        }
        if (0 == strncmp ("--max-order=", *argv, 12)) {
            g_max_order = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strncmp ("--min-order=", *argv, 12)) {
            g_min_order = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strncmp ("--line-step=", *argv, 12)) {
            g_line_step = atoi (&((*argv) [12]));
            continue;
        }
    }
}


static void PrintNgram (
        std::ostream& os, 
        const unsigned char * pBuff, 
        const int Size,
        const int Freq
    )
{
    for (int i = 0; i < Size; ++i) {

        if (0 != i) {
            os << ' ';
        }

        const int B = pBuff [i];
        os << std::setw (2) << std::setfill ('0') << std::hex << B;
    }

    os << '\t' << std::dec << Freq ;
}


static void PrintNgrams (std::ostream& os, const FAStr2IntA * pMap)
{
    DebugLogAssert (pMap);

    const int Count = pMap->GetStrCount ();

    for (int i = 0; i < Count; ++i) {

        const char * pStr = NULL;
        const int StrLen = pMap->GetStr (i, &pStr);
        const int Freq = pMap->GetValue (i);

        PrintNgram (os, (const unsigned char*) pStr, StrLen, Freq);
        os << '\n';
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

        FAUtf32ToEnc conv (&g_alloc);
        conv.SetEncodingName (g_pOutEnc);

        FAStr2Int_hash stats (&g_alloc);

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

            if (0 >= LineLen)
                continue;

            // UTF-8 --> UTF-32LE
            const int Count1 = \
                ::FAStrUtf8ToArray (pLine, LineLen, g_Buff1, MaxBuffSize);
            if (0 >= Count1 || MaxBuffSize < Count1) {
                continue;
            }

            // UTF-32LE --> ENC
            const int Count2 = \
                conv.Process (g_Buff1, Count1, g_Buff2, MaxBuffSize);
            if (0 >= Count2 || MaxBuffSize < Count2) {
                continue;
            }

            // update statistics
            for (int i = 0; i < Count2; ++i) {

                const char * pNgram = g_Buff2 + i;

                for (int N = g_min_order; N <= g_max_order; ++N) {

                    if (Count2 < N + i) {
                        break;
                    }

                    int Value = 0;

                    if (stats.Get (pNgram, N, &Value)) {
                        stats.Add (pNgram, N, Value + 1);
                    } else {
                        stats.Add (pNgram, N, 1);
                    }
                }
            }

            // print statistics
            if (-1 != g_line_step && 0 == (LineNum % g_line_step)) {
                PrintNgrams (*g_pOs, &stats);
                stats.Clear ();
            }

        } // of  while (!(g_pIs->eof ())) ...

        // print statistics
        PrintNgrams (*g_pOs, &stats);

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
