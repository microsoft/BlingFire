/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAStr2Int_hash.h"
#include "FAException.h"
#include "FAImageDump.h"

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

int g_order = 4;

unsigned char * g_pInBuff = NULL;
unsigned int g_InBuffLen = 0;
size_t g_TotalCount = 0;


bool g_CalcProb = false;
bool g_LogScale = false;


void usage ()
{
  std::cout << "\n\
Usage: fa_bin2ngrams [OPTIONS]\n\
\n\
This tool collects byte ngrams from the binary files.\n\
\n\
  --in=<input> - reads input file list from the <input> file,\n\
    note each file should have name as follows: TAG.rest\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --order=N - takes all the n-grams of the order N and below,\n\
    4 is used by default\n\
\n\
  --calc-prob - calculates probability P(Ngram|File)\n\
\n\
  --log-scale - calculates natural logarithm before returning the value\n\
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
        if (0 == strncmp ("--order=", *argv, 8)) {
            g_order = atoi (&((*argv) [8]));
            LogAssert (0 < g_order);
            continue;
        }
        if (!strcmp ("--log-scale", *argv)) {
            g_LogScale = true;
            continue;
        }
        if (0 == strcmp ("--calc-prob", *argv)) {
            g_CalcProb = true;
            continue;
        }
    }
}


static void PrintNgram (
        std::ostream& os, 
        const unsigned char * pBuff, 
        const int Size,
        const int Freq,
        const char * pTag,
        const int TagLen
    )
{
    // print ngram
    for (int i = 0; i < Size; ++i) {
        if (0 != i) {
            os << ' ';
        }
        const int B = pBuff [i];
        os << std::setw (2) << std::setfill ('0') << std::hex << B;
    }

    // print tag
    os << '\t' << std::string (pTag, TagLen) << '\t';

    // print count/weight
    if (g_CalcProb) {

        LogAssert (0 < g_TotalCount);
        double dlValue = (0.0 + Freq) / g_TotalCount;
        if (g_LogScale) {
            dlValue = log ((double)0.000000000001 + dlValue);
        }
        os << std::dec << dlValue ;

    } else {

        int Value = Freq;
        if (g_LogScale) {
            LogAssert (0 < Value);
            Value = (int) (0.5f + log((float)Value));
        }
        os << std::dec << Value ;
    }
}


static void PrintNgrams (
        std::ostream& os, 
        const FAStr2IntA * pMap, 
        const char * pTag, 
        const int TagLen
    )
{
    DebugLogAssert (pMap);

    const int Count = pMap->GetStrCount ();

    for (int i = 0; i < Count; ++i) {

        const char * pStr = NULL;
        const int StrLen = pMap->GetStr (i, &pStr);
        const int Freq = pMap->GetValue (i);

        PrintNgram (os, (const unsigned char*) pStr, StrLen, Freq, pTag, TagLen);
        os << '\n';
    }
}

static void Load (const char * pFileName)
{
    LogAssert (pFileName);

    if (g_pInBuff) {
        delete [] g_pInBuff;
        g_pInBuff = NULL;
    }

    FILE * file = NULL;
    int res = fopen_s (&file, pFileName, "rb");
    LogAssert (0 == res && NULL != file);

    res = fseek (file, 0, SEEK_END);
    LogAssert (0 == res);

    g_InBuffLen = ftell (file);
    LogAssert (0 < g_InBuffLen);

    res = fseek (file, 0, SEEK_SET);
    LogAssert (0 == res);

    g_pInBuff = NEW unsigned char [g_InBuffLen];
    LogAssert (g_pInBuff);

    const size_t ActSize = fread (g_pInBuff, sizeof (char), g_InBuffLen, file);
    LogAssert (ActSize == g_InBuffLen);

    fclose (file);
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

            if (0 >= LineLen) {
                continue;
            }

            // load the input file
            Load (pLine);
            LogAssert (g_pInBuff && 0 < g_InBuffLen);

            g_TotalCount = 0;

            // update statistics
            for (unsigned int i = 0; i <= g_InBuffLen - g_order; ++i) {

                const char * pNgram = (const char *) (g_pInBuff + i);

                g_TotalCount++;

                int Freq = 0;
                if (stats.Get (pNgram, g_order, &Freq)) {
                    stats.Add (pNgram, g_order, Freq + 1);
                } else {
                    stats.Add (pNgram, g_order, 1);
                }
            }

            // find a dot in the file name, use it as a tag
            int TagLen = 0;
            for (;TagLen < LineLen; ++TagLen) {
                if ('.' == pLine[TagLen]) {
                    break;
                }
            }

            // print statistics
            PrintNgrams (*g_pOs, &stats, pLine, TagLen);
            stats.Clear ();

        } // of  while (!(g_pIs->eof ())) ...

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';
        std::cerr << "ERROR: in data file: " << LineNum << " \"" \
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
