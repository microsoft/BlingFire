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
#include "FAStringTokenizer.h"
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

int g_field = -1;
const char * g_pOutEnc = "UTF-8";
bool g_no_encode = false;

bool g_no_output = false;
bool g_no_process = false;

const int MaxBuffSize = 4096 * 1024;
int g_Buff1 [MaxBuffSize];
char g_Buff2 [MaxBuffSize];


void usage ()
{
  std::cout << "\n\
Usage: fa_encode [OPTIONS]\n\
\n\
This tool encodes the whole input line or the specified field in \n\
the tab-delimited input line with the given encoding. The output encoded\n\
string is represented as a space delimited sequence of hexadecial bytes.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --output-enc=<enc> - output encoding name or codepage number\n\
    UTF-8 is used by default (input encoding should be UTF-8)\n\
\n\
  --no-encode - does not do UTF-8 --> <enc> convertion, just prints\n\
    byte ngrams (this paprameter can be used if the encoding is known,\n\
    or for testing purposes, --output-enc= is ignored)\n\
\n\
  --field=N - 1-based field value, fields should be tab separated,\n\
    if not specified the whole line got encoded\n\
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
        if (0 == strncmp ("--no-encode", *argv, 11)) {
            g_no_encode = true;
            continue;
        }
        if (0 == strncmp ("--field=", *argv, 8)) {
            g_field = atoi (&((*argv) [8]));
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


static void PrintBuffBytes (
        std::ostream& os, 
        const unsigned char * pBuff, 
        const int Size
    )
{
    for (int i = 0; i < Size; ++i) {

        if (0 != i) {
            os << ' ';
        }

        const int B = pBuff [i];
        os << std::setw (2) << std::setfill ('0') << std::hex << B;
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

        FAStringTokenizer tokenizer;
        tokenizer.SetSpaces ("\t");

        while (!(g_pIs->eof ())) {

            if (!std::getline (*g_pIs, line))
                break;

            LineNum++;

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            if (0 < LineLen && false == g_no_encode) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }

            if (0 >= LineLen)
                continue;

            // the input line is too big
            FAAssert (MaxBuffSize > LineLen, FAMsg::InternalError);

            int Count2;

            if (-1 == g_field) {

                if (false == g_no_encode) {
                    // UTF-8 --> UTF-32LE
                    const int Count1 = \
                        ::FAStrUtf8ToArray (pLine, LineLen, g_Buff1, MaxBuffSize);
                    if (0 >= Count1 || MaxBuffSize < Count1) {
                        continue;
                    }
                    // UTF-32LE --> ENC
                    Count2 = conv.Process (g_Buff1, Count1, g_Buff2, MaxBuffSize);
                    if (0 >= Count2 || MaxBuffSize < Count2) {
                        continue;
                    }
                } else {
                    // copy the whole byte sequence
                    memcpy (g_Buff2, pLine, LineLen);
                    // copy the new line 0x0A symbol
                    g_Buff2 [LineLen] = 0x0A;
                    Count2 = LineLen + 1;
                }

                // print bytes
                PrintBuffBytes (*g_pOs, (const unsigned char*) g_Buff2, Count2);
                (*g_pOs) << '\n';

            } else {

                bool fError = false;
                std::ostringstream TmpOs;

                tokenizer.SetString (pLine, LineLen);

                bool fPrinted = false;
                int CurrField = 0;
                const char * pStr = NULL;
                int Len = 0;

                while (tokenizer.GetNextStr (&pStr, &Len)) {

                    DebugLogAssert (pStr && 0 <= Len);

                    CurrField++;

                    if (fPrinted) {
                        TmpOs << '\t';
                    }

                    if (CurrField != g_field) {

                        TmpOs << std::string (pStr, Len);

                    } else {

                        if (false == g_no_encode) {
                            // UTF-8 --> UTF-32LE
                            const int Count1 = \
                              ::FAStrUtf8ToArray (pStr, Len, g_Buff1, MaxBuffSize);
                            if (0 >= Count1 || MaxBuffSize < Count1) {
                                fError = true;
                                break;
                            }
                            // UTF-32LE --> ENC
                            Count2 = conv.Process (g_Buff1, Count1, g_Buff2, MaxBuffSize);
                            if (0 >= Count2 || MaxBuffSize < Count2) {
                                fError = true;
                                break;
                            }
                        } else {
                            // copy the whole byte sequence
                            memcpy (g_Buff2, pStr, Len);
                            Count2 = Len;

                        }

                        // print bytes
                        PrintBuffBytes (TmpOs, (const unsigned char*) \
                            g_Buff2, Count2);
                    }

                    fPrinted = true;

                } // of while (tokenizer.GetNextStr (&pStr, &Len)) ...

                if (!fError) {
                    const std::string & TmpStr = TmpOs.str ();
                    (*g_pOs) << TmpStr << '\n';
                }
            }  // of if (-1 == g_field) ...

        } // of  while (!(g_pIs->eof ())) ...

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
