/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAStringTokenizer.h"
#include "FAMultiMap_judy.h"
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

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

bool g_no_output = false;
bool g_no_process = false;

const int MaxBuffSize = FALimits::MaxWordLen;
int g_Tmp [MaxBuffSize];

int g_value_base = -1; // -1 stands for default behavior, if not -1 then assumes value is a chain of numbers
int g_key_base = -1;   // -1 stands for default behavior, if not -1 then assumes key is a number
bool g_text_value = false; // value is a string in UTF-8
bool g_tab_only_delimiter = false; // tab and space by default

void usage ()
{
  std::cout << "\n\
Usage: fa_charmap2mmap [OPTIONS]\n\
\n\
Converts character map into a multi map (1:n mapping.) Input data\n\
should be in UTF-8.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --key-base=N - if specified then Key is assumed to always be a number\n\
     with Nth base\n\
\n\
  --value-base=N - if specified, Value is assumed to always be a number chain\n\
     with Nth base\n\
\n\
  --text-value - forces values to be interpreted as a text in UTF-8 encoding\n\
\n\
  --tab-only-delim - delimiter between keys and values is only tab,\n\
     space and tab by default\n\
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
        if (0 == strncmp ("--key-base=", *argv, 11)) {
            g_key_base = atoi (&((*argv) [11]));
            LogAssert (0 < g_key_base && g_key_base <= 16);
            continue;
        }
        if (0 == strncmp ("--value-base=", *argv, 13)) {
            g_value_base = atoi (&((*argv) [13]));
            LogAssert (0 < g_value_base && g_value_base <= 16);
            continue;
        }
        if (0 == strncmp ("--text-value", *argv, 12)) {
            g_text_value = true;
            continue;
        }
        if (0 == strncmp ("--tab-only-delim", *argv, 16)) {
            g_tab_only_delimiter = true;
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


static inline const int GetNum (const char * pStr, const int Len, int base = 16)
{
    std::string buff (pStr, Len);
    return strtol (buff.c_str (), NULL, 16);
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

        FAMapIOTools g_map_io (&g_alloc);

        FAStringTokenizer tokenizer;
        if (g_tab_only_delimiter) {
            tokenizer.SetSpaces ("\t");
        } else {
            tokenizer.SetSpaces (" \t");
        }

        FAMultiMap_judy mmap;
        mmap.SetAllocator (&g_alloc);

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
            // skip the comments and empty lines
            if (0 < LineLen && '#' != *pLine) {

                /// read the input
                tokenizer.SetString (pLine, LineLen);

                const char * pStr1 = NULL;
                int Len1 = 0;
                tokenizer.GetNextStr (&pStr1, &Len1);

                // empty line with some spaces
                if (0 >= Len1)
                    continue;

                FAAssert (pStr1, FAMsg::IOError);

                int Key;

                if (-1 != g_key_base) {
                    Key = GetNum (pStr1, Len1, g_key_base);
                } else if (2 < Len1 && '\\' == pStr1 [0] && \
                    ('X' == pStr1 [1] || 'x' == pStr1 [1])) {
                    Key = GetNum (pStr1 + 2, Len1 - 2);
                } else {
                    const int Count1 = \
                        ::FAStrUtf8ToArray (pStr1, Len1, &Key, 1);
                    FAAssert (1 == Count1, FAMsg::IOError);
                }

                const char * pStr2 = NULL;
                int Len2 = 0;
                tokenizer.GetNextStr (&pStr2, &Len2);
                // Note: 0 length is allowed
                FAAssert (0 <= Len2, FAMsg::IOError);

                if (-1 != g_value_base) {

                    // make a chain from numbers, if g_value_base == 10, the deciam numbers, e.g. 255 1 238 2
                    const int Count2 = \
                        ::FAReadIntegerChain(pStr2, Len2, g_value_base, g_Tmp, MaxBuffSize);
                    FAAssert(0 <= Count2 && MaxBuffSize > Count2, FAMsg::IOError);
                    mmap.Set (Key, g_Tmp, Count2);

                } else if (false == g_text_value && 2 < Len2 && '\\' == pStr2 [0] && \
                    ('X' == pStr2 [1] || 'x' == pStr2 [1])) {

                    // TODO: allow arbitrary hexadecimal chains
                    int SingleCharValue = GetNum (pStr2 + 2, Len2 - 2);
                    mmap.Set (Key, &SingleCharValue, 1);

                } else {

                    const int Count2 = \
                        ::FAStrUtf8ToArray (pStr2, Len2, g_Tmp, MaxBuffSize);
                    // Note: 0 Count is allowed
                    FAAssert (0 <= Count2 && MaxBuffSize > Count2, FAMsg::IOError);
                    mmap.Set (Key, g_Tmp, Count2);
                }
            }

        } // of while (!(g_pIs->eof ())) ...

        g_map_io.Print (*g_pOs, &mmap);

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
