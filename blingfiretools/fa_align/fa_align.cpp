/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtf32Utils.h"
#include "FAPrintUtils.h"
#include "FAStringTokenizer.h"
#include "FAMultiMap_pack_fixed.h"
#include "FAImageDump.h"
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

int g_Filler = FAFsmConst::IW_EPSILON;
int g_Filler2 = 2;
int g_spec_l_anchor = -1;
int g_Gap1 = -1;
int g_Gap2 = -1;
int g_NotEqual = 0;
int g_NotEqual2 = 0;
int g_Equal2 = 1;
int g_MinPos = -1;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;

bool g_ignore_case = false;
bool g_rev = false;
bool g_no_epsilon = false;
bool g_no_output = false;
bool g_no_process = false;

const int MaxBuffSize = 1024;

int g_Buff1 [MaxBuffSize];
int g_Count1;

int g_Buff2 [MaxBuffSize];
int g_Count2;

int g_OutBuff [4 * MaxBuffSize];
int g_OutCount;

int g_D [MaxBuffSize][MaxBuffSize];

const int MaxOutStr = (FAUtf8Const::MAX_CHAR_SIZE * MaxBuffSize * 4) + 1;
char OutStr [MaxOutStr];


void usage ()
{
  std::cout << "\n\
Usage: fa_align [OPTIONS]\n\
\n\
This tool aligns two tab separated input UTF-8 strings of each line.\n\
The tool prints left-string character followed by the corresponding \n\
right-string character, if there is a gap the filler character is used.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --rev - reverses the alignment order, by default the right most characters\n\
    are considered first, the switch reverses the behaviour\n\
\n\
  --filler=N - specifies which character code #N to use to fill\n\
    the alignment gaps, 3 is used by default\n\
\n\
  --no-epsilon - glues together characters of the second string which has\n\
    been aligned with gaps from the first string, if this option is used\n\
    the output is single space separated\n\
\n\
  --filler2=N - specifies which character code #N for the second string\n\
    symbols which match corresponding first string symbols, 2 is used by\n\
    default (this option can only be used with --no-epsilon)\n\
\n\
  --gap1=N - a score for inserting a gap into the first string,\n\
    -1 is used by default\n\
\n\
  --gap2=N - a score for inserting a gap into the second string,\n\
    -1 is used by default\n\
\n" << \

"  --not-equal=N - a score for aligning two non-equal characters,\n\
    0 is used by default\n\
\n\
  --not-equal2=N - a score for aligning two non-equal characters\n\
    at the string 1 position less or equal M (see --min-len=M parameter),\n\
    0 is used by default\n\
\n\
  --equal2=N - a score for aligning two equal characters at the string 1\n\
    position less or equal M (see --min-len=M parameter),\n\
    1 is used by default\n\
\n\
  --min-len=M - specifies the string 1 length boundary (see --not-equal2=N,\n\
    --equal2=N parameters), 0 is used by default\n\
\n\
  --ignore-case - converts input symbols to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
\n\
 Note: the scrore of 1 is used for equal characters\n\
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
        if (0 == strncmp ("--filler=", *argv, 9)) {
            g_Filler = atoi (&((*argv) [9]));
            continue;
        }
        if (0 == strncmp ("--filler2=", *argv, 10)) {
            g_Filler2 = atoi (&((*argv) [10]));
            continue;
        }
        if (0 == strncmp ("--spec-l-anchor=", *argv, 16)) {
            g_spec_l_anchor = atoi (&((*argv) [16]));
            continue;
        }
        if (0 == strncmp ("--gap1=", *argv, 7)) {
            g_Gap1 = atoi (&((*argv) [7]));
            continue;
        }
        if (0 == strncmp ("--gap2=", *argv, 7)) {
            g_Gap2 = atoi (&((*argv) [7]));
            continue;
        }
        if (0 == strncmp ("--not-equal=", *argv, 12)) {
            g_NotEqual = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strncmp ("--not-equal2=", *argv, 13)) {
            g_NotEqual2 = atoi (&((*argv) [13]));
            continue;
        }
        if (0 == strncmp ("--equal2=", *argv, 9)) {
            g_Equal2 = atoi (&((*argv) [9]));
            continue;
        }
        if (0 == strncmp ("--min-len=", *argv, 10)) {
            g_MinPos = atoi (&((*argv) [10])) - 1;
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
        if (0 == strcmp ("--rev", *argv)) {
            g_rev = true;
            continue;
        }
        if (0 == strcmp ("--no-epsilon", *argv)) {
            g_no_epsilon = true;
            continue;
        }
        if (0 == strncmp ("--charmap=", *argv, 10)) {
            g_pCharMapFile = &((*argv) [10]);
            continue;
        }
        if (0 == strcmp ("--ignore-case", *argv)) {
            g_ignore_case = true;
            continue;
        }
    }
}


inline static const int Score (const int IwPos, const int Iw, const int Ow)
{
    DebugLogAssert (0 <= g_Count1 - IwPos);

    bool fLeft = IwPos <= g_MinPos;
    if (g_rev) {
        fLeft = g_Count1 - IwPos - 1 <= g_MinPos;
    }

    if (fLeft) {
        return Iw == Ow ? g_Equal2 : g_NotEqual2 ;
    } else {
        return Iw == Ow ? 1 : g_NotEqual;
    }
}

static void Process ()
{
    int i, j, k;

    if (g_rev) {
        for (k = 0; k < g_Count1 / 2; ++k) {
            const int T = g_Buff1 [k];
            g_Buff1 [k] = g_Buff1 [g_Count1 - k - 1];
            g_Buff1 [g_Count1 - k - 1] = T;
        }
        for (k = 0; k < g_Count2 / 2; ++k) {
            const int T = g_Buff2 [k];
            g_Buff2 [k] = g_Buff2 [g_Count2 - k - 1];
            g_Buff2 [g_Count2 - k - 1] = T;
        }
    }

    /// g_D [i][j] keeps an alignament score of str2 [0..i-1] with str1 [0..j-1]

    g_D [0][0] = 0;

    for (j = 0; j < g_Count1; ++j) {
        g_D [0][j + 1] = (j + 1) * g_Gap2;
    }
    for (i = 0; i < g_Count2; ++i) {
        g_D [i + 1][0] = (i + 1) * g_Gap1;
    }

	for (i = 1; i < g_Count2 + 1; ++i) {
	    for (j = 1;  j < g_Count1 + 1; ++j) {

            const int S1 = g_D [i - 1][j - 1] + \
                Score (j - 1, g_Buff1 [j - 1], g_Buff2 [i - 1]);
            const int S2 = g_D [i][j - 1] + g_Gap2;
            const int S3 = g_D [i - 1][j] + g_Gap1;

            if (S1 >= S2 && S1 >= S3) {
                g_D [i][j] = S1;
            } else if (S2 >= S1 && S2 >= S3) {
                g_D [i][j] = S2;
            } else {
                g_D [i][j] = S3;
            }
        }
    }

    i = g_Count2;
    j = g_Count1;

    const int n = g_rev ? 1 : 0;
    const int m = g_rev ? 0 : 1;

    g_OutCount = 0;

    while (i > 0 && j > 0) {

        if (g_D [i][j] - Score (j - 1, g_Buff1 [j - 1], g_Buff2 [i - 1]) == \
            g_D [i - 1][j - 1]) {
            g_OutBuff [g_OutCount + n] = g_Buff2 [i - 1];
            g_OutBuff [g_OutCount + m] = g_Buff1 [j - 1];
            g_OutCount += 2;
            j--;
            i--;
        } else if (g_D [i][j] - g_Gap2 == g_D [i][j - 1]) {
            g_OutBuff [g_OutCount + n] = g_Filler;
            g_OutBuff [g_OutCount + m] = g_Buff1 [j - 1];
            g_OutCount += 2;
            j--;
        } else {
            DebugLogAssert (g_D [i][j] - g_Gap1 == g_D [i - 1][j]);
            g_OutBuff [g_OutCount + n] = g_Buff2 [i - 1];
            g_OutBuff [g_OutCount + m] = g_Filler;
            g_OutCount += 2;
            i--;
        }
    } // of while (i > 0 && j > 0) ...

    while (i > 0) {
        g_OutBuff [g_OutCount + n] = g_Buff2 [i - 1];
        g_OutBuff [g_OutCount + m] = g_Filler;
        g_OutCount += 2;
        i--;
    }
    while (j > 0) {
        g_OutBuff [g_OutCount + n] = g_Filler;
        g_OutBuff [g_OutCount + m] = g_Buff1 [j - 1];
        g_OutCount += 2;
        j--;
    }

    if (!g_rev) {
        for (k = 0; k < g_OutCount / 2; ++k) {
            const int T = g_OutBuff [k];
            g_OutBuff [k] = g_OutBuff [g_OutCount - k - 1];
            g_OutBuff [g_OutCount - k - 1] = T;
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
        // load normalization map, if needed
        if (g_pCharMapFile) {
            g_charmap_image.Load (g_pCharMapFile);
            const unsigned char * pImg = g_charmap_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_charmap.SetImage (pImg);
        }

        FAStringTokenizer tokenizer;
        tokenizer.SetSpaces ("\t");

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

                const char * pStr1 = NULL;
                int Len1 = 0;
                tokenizer.GetNextStr (&pStr1, &Len1);

                const char * pStr2 = NULL;
                int Len2 = 0;
                tokenizer.GetNextStr (&pStr2, &Len2);

                FAAssert (pStr1 && 0 < Len1 && pStr2 && 0 < Len2, \
                    FAMsg::IOError);

                g_Count1 = \
                    ::FAStrUtf8ToArray (pStr1, Len1, g_Buff1, MaxBuffSize);
                // normalize the case, if needed
                if (g_ignore_case) {
                    ::FAUtf32StrLower (g_Buff1, g_Count1);
                }
                // normalize a word
                if (g_pCharMapFile) {
                    g_Count1 = ::FANormalizeWord (g_Buff1, g_Count1, \
                        g_Buff1, MaxBuffSize, &g_charmap);
                }
                FAAssert (0 < g_Count1 && MaxBuffSize > g_Count1, \
                    FAMsg::IOError);

                g_Count2 = \
                    ::FAStrUtf8ToArray (pStr2, Len2, g_Buff2, MaxBuffSize);
                // normalize the case, if needed
                if (g_ignore_case) {
                    ::FAUtf32StrLower (g_Buff2, g_Count2);
                }
                // normalize a word
                if (g_pCharMapFile) {
                    g_Count2 = ::FANormalizeWord (g_Buff2, g_Count2, \
                        g_Buff2, MaxBuffSize, &g_charmap);
                }
                FAAssert (0 < g_Count2 && MaxBuffSize > g_Count2, \
                    FAMsg::IOError);

                if (g_no_process)
                    continue;

                /// make the alignment

                Process ();

                if (g_no_output)
                    continue;

                /// print the results

                if (false == g_no_epsilon) {

                    const int OutLen = ::FAArrayToStrUtf8 (g_OutBuff, \
                        g_OutCount, OutStr, MaxOutStr);
                    FAAssert (0 < OutLen && OutLen < MaxOutStr, FAMsg::IOError);
                    OutStr [OutLen] = 0;

                } else {

                    char * pOut = OutStr;

                    if (-1 != g_spec_l_anchor) {
                        pOut = ::FAIntToUtf8 (g_spec_l_anchor, pOut, \
                                                FAUtf8Const::MAX_CHAR_SIZE);
                        FAAssert (pOut, FAMsg::IOError);
                        *pOut++ = ' ';
                        pOut = ::FAIntToUtf8 (g_Filler2, pOut, \
                                                FAUtf8Const::MAX_CHAR_SIZE);
                        FAAssert (pOut, FAMsg::IOError);
                    }

                    bool fStart = true;
                    int PrevIw = -1;

                    for (int i = 0; i < g_OutCount; ++i) {

                        int C = g_OutBuff [i];

                        // a character from string 1
                        if (0 == i % 2) {

                            if (g_Filler == C) {
                                continue;
                            }

                            if (!fStart) {
                                *pOut++ = ' ';
                            }
                            fStart = false;

                            PrevIw = C;
                            pOut = ::FAIntToUtf8 (C, pOut, \
                                                  FAUtf8Const::MAX_CHAR_SIZE);
                            FAAssert (pOut, FAMsg::IOError);
                            *pOut++ = ' ';

                        // a character from string 2
                        } else {

                            // if the output is g_Filler and there will be
                            // something attached to the left, then don't
                            // print g_Filler2
                            if (g_Filler == C && i + 1 < g_OutCount && \
                                g_Filler == g_OutBuff [i + 1]) {
                                continue;
                            }

                            if (C == PrevIw) {
                                C = g_Filler2;
                            }

                            pOut = ::FAIntToUtf8 (C, pOut, \
                                                  FAUtf8Const::MAX_CHAR_SIZE);
                            FAAssert (pOut, FAMsg::IOError);
                            fStart = false;
                        }

                    } // of for (int i = 0; ...

                    *pOut = 0;
                }

                (*g_pOs) << OutStr << '\n';
            }

        } // of while (!(g_pIs->eof ())) ...

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
