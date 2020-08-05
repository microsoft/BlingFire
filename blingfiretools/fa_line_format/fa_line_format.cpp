/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtf32Utils.h"
#include "FAMultiMap_pack_fixed.h"
#include "FAImageDump.h"
#include "FAException.h"
#include "FARSDfa_pack_triv.h"
#include "FATransform_hyph_redup_t.h"
#include "FATransform_hyph_redup_rev_t.h"
#include "FATransform_prefix_t.h"
#include "FATransform_prefix_rev_t.h"
#include "FATransform_capital_t.h"
#include "FATransform_capital_rev_t.h"
#include "FATransform_cascade_t.h"
#include "FAStringTokenizer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

bool g_no_output = false;
bool g_no_process = false;
bool g_to_lower = false;
bool g_to_upper = false;
bool g_to_capitalized = false;
bool g_wtbt = false;
bool g_wts = false;

const char * g_pOutFile = NULL;
const char * g_pInFile = NULL;

std::ostream * pOs = &std::cout;
std::istream * pIs = &std::cin;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;

const int MaxChainSize = 40960;
int g_Chain [MaxChainSize];
int g_Tmp [MaxChainSize];
int g_ChainSize = 0;

const int MaxStrSize = MaxChainSize * FAUtf8Const::MAX_CHAR_SIZE ;
char OutStr [MaxStrSize];

// in --wtbt mode
const int g_WordFrom = 0;
int g_WordTo = 0;
int g_BaseFrom = 0;
int g_BaseTo = 0;

// transformations
FATransform_hyph_redup_t < int > g_tr_hyph_redup;
FATransform_hyph_redup_rev_t < int > g_tr_hyph_redup_rev;
FATransform_prefix_t < int > g_tr_pref;
FATransform_prefix_rev_t < int > g_tr_pref_rev;
FATransform_capital_t < int > g_tr_ucf;
FATransform_capital_rev_t < int > g_tr_ucf_rev;
FATransform_cascade_t < int > g_in_tr_cascade;

const char * g_pInPrefFsmFile = NULL;
FAImageDump g_pref_dfa_image;
FARSDfa_pack_triv g_pref_fsm_dump;
const FARSDfaCA * g_pPrefFsm = NULL;

int g_redup_delim = -1;
int g_pref_delim = -1;
int g_ucf_delim = -1;

const FATransformCA_t < int > * g_pInTr = NULL;



void usage ()
{
  std::cout << "\n\
Usage: fa_line_format [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
This program makes different types of text formatting and normalization.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --to-lower - lowercase the input by simple case folding algorithm due to\n\
    Unicode 4.1.0\n\
\n\
  --ignore-case - does the same as --to-lower\n\
\n\
  --to-upper - uppercase the input by simple case folding algorithm due to\n\
    Unicode 4.1.0\n\
\n\
  --to-capitalized - makes the first letter upper case and the rest lower case,\n\
    if the input word is multi-word expression, then each component is capitalized\n\
    For example: \"washington d. c.\" --> \"Washington D. C.\"\n\
    The text is expected to be word-broken in single-space separated words\n\
    representation (the way fa_lex tool makes the output.)\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
\n\
  --in-tr=<trs> - specifies input transformation type\n\
    <trs> is comma-separated array of the following:\n\
      hyph-redup - hyphenated reduplication\n\
      hyph-redup-rev - reverse hyphenated reduplication\n\
      pref - prefix transformation: represents special prefixes as suffixes\n\
      pref-rev - reversed prefix transformation\n\
      ucf - encodes upper-case-first symbol in a suffix\n\
      ucf-rev - reversed UCF transformation\n\
\n\
  --redup-delim=N - reduplication delimiter.\n\
\n\
  --pref-delim=N - prefix transformation delimiter\n\
\n\
  --pref-fsm=<fsm> - keeps dictionary of prefixes to be treated as suffix,\n\
    used only with --in-tr=pref or --out-tr=pref\n\
\n\
  --ucf-delim=N - UCF transformation delimiter\n\
\n\
  --wtbt - input is in \"Word\\tTag\\tBase\\tTag\\n\" format\n\
\n\
  --wts - input is in \"Word\\tTag1\\t...\\tTagN\\n\" format\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
\n\
";
}


template < class Ty >
void InitTrCascade (FATransform_cascade_t < Ty > * pTrs,
                    const char * pTrsStr,
                    const int TrsStrLen)
{
  DebugLogAssert (pTrs);

  FAStringTokenizer tokenizer;
  tokenizer.SetSpaces (",");
  tokenizer.SetString (pTrsStr, TrsStrLen);

  const char * pTrType = NULL;
  int TrTypeLen = 0;

  while (tokenizer.GetNextStr (&pTrType, &TrTypeLen)) {

    if (0 == strncmp ("hyph-redup-rev", pTrType, 14)) {

      pTrs->AddTransformation (&g_tr_hyph_redup_rev);

    } else if (0 == strncmp ("hyph-redup", pTrType, 10)) {

      pTrs->AddTransformation (&g_tr_hyph_redup);

    } else if (0 == strncmp ("pref-rev", pTrType, 8)) {

      pTrs->AddTransformation (&g_tr_pref_rev);

    } else if (0 == strncmp ("pref", pTrType, 4)) {

      pTrs->AddTransformation (&g_tr_pref);

    } else if (0 == strncmp ("ucf-rev", pTrType, 7)) {

      pTrs->AddTransformation (&g_tr_ucf_rev);

    } else if (0 == strncmp ("ucf", pTrType, 3)) {

      pTrs->AddTransformation (&g_tr_ucf);

    } else {

      std::string s (pTrsStr, TrsStrLen);
      std::cerr << "ERROR: \"Unknown transformation name " << s << '"'
                << " in program " << __PROG__ << '\n';
      exit (1);
    }
  }
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (0 == strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        g_pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
        g_pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
        g_no_output = true;
        continue;
    }
    if (0 == strcmp ("--no-process", *argv)) {
        g_no_process = true;
        continue;
    }
    if (0 == strcmp ("--to-lower", *argv)) {
        g_to_lower = true;
        continue;
    }
    if (0 == strcmp ("--ignore-case", *argv)) {
        g_to_lower = true;
        continue;
    }
    if (0 == strcmp ("--to-upper", *argv)) {
        g_to_upper = true;
        continue;
    }
    if (0 == strcmp ("--to-capitalized", *argv)) {
        g_to_capitalized = true;
        continue;
    }
    if (0 == strcmp ("--wtbt", *argv)) {
        g_wtbt = true;
        g_wts = false;
        continue;
    }
    if (0 == strcmp ("--wts", *argv)) {
        g_wts = true;
        g_wtbt = false;
        continue;
    }
    if (0 == strncmp ("--charmap=", *argv, 10)) {
        g_pCharMapFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--redup-delim=", *argv, 14)) {
        g_redup_delim = atoi (&((*argv) [14]));
        continue;
    }
    if (0 == strncmp ("--pref-fsm=", *argv, 11)) {
        g_pInPrefFsmFile = &((*argv) [11]);
        continue;
    }
    if (0 == strncmp ("--pref-delim=", *argv, 13)) {
        g_pref_delim = atoi (&((*argv) [13]));
        continue;
    }
    if (0 == strncmp ("--ucf-delim=", *argv, 12)) {
        g_ucf_delim = atoi (&((*argv) [12]));
        continue;
    }
    if (0 == strcmp ("--in-tr=hyph-redup", *argv)) {
        g_pInTr = &g_tr_hyph_redup;
        continue;
    }
    if (0 == strcmp ("--in-tr=hyph-redup-rev", *argv)) {
        g_pInTr = &g_tr_hyph_redup_rev;
        continue;
    }
    if (0 == strcmp ("--in-tr=pref", *argv)) {
        g_pInTr = &g_tr_pref;
        continue;
    }
    if (0 == strcmp ("--in-tr=pref-rev", *argv)) {
        g_pInTr = &g_tr_pref_rev;
        continue;
    }
    if (0 == strcmp ("--in-tr=ucf", *argv)) {
        g_pInTr = &g_tr_ucf;
        continue;
    }
    if (0 == strcmp ("--in-tr=ucf-rev", *argv)) {
        g_pInTr = &g_tr_ucf_rev;
        continue;
    }
    if (0 == strncmp ("--in-tr=", *argv, 8)) {
        const char * pTrsStr = &((*argv) [8]);
        const int TrsStrLen = (int) strlen (pTrsStr);
        InitTrCascade (&g_in_tr_cascade, pTrsStr, TrsStrLen);
        g_pInTr = &g_in_tr_cascade;
        continue;
    }
  }
}


void InitWtbtData ()
{
    g_WordTo = 0;
    g_BaseFrom = 0;
    g_BaseTo = 0;

    int c = 0;

    for (int i = 0; i < g_ChainSize; ++i) {

        if (int('\t') == g_Chain [i]) {

            switch (c) {
            case 0:
                g_WordTo = i - 1;
                break;
            case 1:
                g_BaseFrom = i + 1;
                break;
            case 2:
                g_BaseTo = i - 1;
                break;
            default:
                if (g_wtbt) {
                    throw FAException (FAMsg::CorruptFile, __FILE__, __LINE__);
                }
                return;
            };

            c++;

        } // of if (int('\t') == g_Chain [i]) ...
    } // of for (int i = 0; ...
}


void Normalize ()
{
    if (false == g_wtbt && false == g_wts) {

        const int NewSize = ::FANormalize (g_Chain, g_ChainSize, \
            g_Tmp, MaxChainSize, &g_charmap);

        if (0 <= NewSize && NewSize <= MaxChainSize) {
            memcpy (g_Chain, g_Tmp, sizeof (int) * NewSize);
            g_ChainSize = NewSize;
        }

    } else if (g_wtbt) {

        DebugLogAssert (0);

    } else if (g_wts) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;

        const int NewSize = ::FANormalize (g_Chain + g_WordFrom, WordSize, \
            g_Tmp, MaxChainSize, &g_charmap);
        DebugLogAssert (0 <= NewSize);

        const int MaxLeft = MaxChainSize - NewSize;
        const int LeftSize = g_ChainSize - g_WordTo - 1;

        if (MaxLeft >= LeftSize) {
            // copy tags into Tmp chain
            memcpy (g_Tmp + NewSize, g_Chain + g_WordTo + 1, \
                sizeof (int) * LeftSize);
            // copy normalized word and the tags back to g_Chain
            memcpy (g_Chain, g_Tmp, sizeof (int) * (NewSize + LeftSize));
        }
    }
}


void ApplyTransformation ()
{
    if (false == g_wtbt && false == g_wts) {

        // apply the transformation
        const int NewSize = \
            g_pInTr->Process (g_Chain, g_ChainSize, g_Tmp, MaxChainSize);

        // don't do anything if the transformation was not applied
        if (0 <= NewSize && NewSize <= MaxChainSize) {
            memcpy (g_Chain, g_Tmp, sizeof (int) * NewSize);
            g_ChainSize = NewSize;
        }

    } else if (g_wtbt) {

        DebugLogAssert (0);

    } else if (g_wts) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;

        // apply the transformation
        const int NewSize = g_pInTr->Process \
            (g_Chain + g_WordFrom, WordSize, g_Tmp, MaxChainSize);

        // don't do anything if the transformation was not applied
        if (0 <= NewSize && NewSize <= MaxChainSize) {

            const int MaxLeft = MaxChainSize - NewSize;
            const int LeftSize = g_ChainSize - g_WordTo - 1;

            if (MaxLeft >= LeftSize) {
                // copy tags into Tmp chain
                memcpy (g_Tmp + NewSize, g_Chain + g_WordTo + 1, \
                    sizeof (int) * LeftSize);
                // copy normalized word and the tags back to g_Chain
                memcpy (g_Chain, g_Tmp, sizeof (int) * (NewSize + LeftSize));
                // update chain's size
                g_ChainSize += (NewSize - WordSize);
            }
        }
    }
}


void ToLower ()
{
    DebugLogAssert (g_to_lower || g_to_capitalized);

    if (false == g_wtbt && false == g_wts) {

        ::FAUtf32StrLower (g_Chain, g_ChainSize);

    } else if (g_wtbt) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);
        DebugLogAssert (g_ChainSize > g_BaseTo && g_ChainSize > g_BaseFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        ::FAUtf32StrLower (g_Chain + g_WordFrom, WordSize);

        const int BaseSize = g_BaseTo - g_BaseFrom + 1;
        ::FAUtf32StrLower (g_Chain + g_BaseFrom, BaseSize);

    } else if (g_wts) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        ::FAUtf32StrLower (g_Chain + g_WordFrom, WordSize);
    }
}


void ToUpper ()
{
    DebugLogAssert (g_to_upper);

    if (false == g_wtbt && false == g_wts) {

        ::FAUtf32StrUpper (g_Chain, g_ChainSize);

    } else if (g_wtbt) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);
        DebugLogAssert (g_ChainSize > g_BaseTo && g_ChainSize > g_BaseFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        ::FAUtf32StrUpper (g_Chain + g_WordFrom, WordSize);

        const int BaseSize = g_BaseTo - g_BaseFrom + 1;
        ::FAUtf32StrUpper (g_Chain + g_BaseFrom, BaseSize);

    } else if (g_wts) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        ::FAUtf32StrUpper (g_Chain + g_WordFrom, WordSize);
    }
}


void CapitalizeFromLower (int * pStr, const int Len)
{
    DebugLogAssert (0 == Len || pStr);

    bool fStart = true;

    for (int i = 0; i < Len; ++i) {

        int Symbol = pStr [i];

        if (' ' == Symbol) {
            fStart = true;
            continue;
        }

        if (fStart) {
            pStr [i] = ::FAUtf32ToUpper (Symbol);
            fStart = false;
        }
    }
}


void ToCapitalized ()
{
    DebugLogAssert (g_to_capitalized);

    /// make everything in lower case
    ToLower ();

    if (false == g_wtbt && false == g_wts) {

        CapitalizeFromLower (g_Chain, g_ChainSize);

    } else if (g_wtbt) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);
        DebugLogAssert (g_ChainSize > g_BaseTo && g_ChainSize > g_BaseFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        CapitalizeFromLower (g_Chain + g_WordFrom, WordSize);

        const int BaseSize = g_BaseTo - g_BaseFrom + 1;
        CapitalizeFromLower (g_Chain + g_BaseFrom, BaseSize);

    } else if (g_wts) {

        DebugLogAssert (g_ChainSize > g_WordTo && g_ChainSize > g_WordFrom);

        const int WordSize = g_WordTo - g_WordFrom + 1;
        CapitalizeFromLower (g_Chain + g_WordFrom, WordSize);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        std::ofstream ofs;
        if (g_pOutFile) {
            ofs.open (g_pOutFile, std::ios::out);
            pOs = &ofs;
        }
        std::ifstream ifs;
        if (g_pInFile) {
            ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&ifs, g_pInFile);
            pIs = &ifs;
        }
        // load normalization map, if needed
        if (g_pCharMapFile) {
            g_charmap_image.Load (g_pCharMapFile);
            const unsigned char * pImg = g_charmap_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_charmap.SetImage (pImg);
        }
        // load prefix automaton, if needed
        if (g_pInPrefFsmFile) {
            g_pref_dfa_image.Load (g_pInPrefFsmFile);
            const unsigned char * pImg = g_pref_dfa_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_pref_fsm_dump.SetImage (pImg);
            g_pPrefFsm = &g_pref_fsm_dump;
            g_tr_pref.SetRsDfa (g_pPrefFsm);
        }
        // specify delimiters, if needed
        if (-1 != g_pref_delim) {
            g_tr_pref.SetDelim (g_pref_delim);
            g_tr_pref_rev.SetDelim (g_pref_delim);
        }
        if (-1 != g_redup_delim) {
            g_tr_hyph_redup.SetDelim (g_redup_delim);
            g_tr_hyph_redup_rev.SetDelim (g_redup_delim);
        }
        if (-1 != g_ucf_delim) {
            g_tr_ucf.SetDelim (g_ucf_delim);
            g_tr_ucf_rev.SetDelim (g_ucf_delim);
        }

        DebugLogAssert (pOs && pIs);

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

                // UTF-8 --> UTF-32
                g_ChainSize = \
                    ::FAStrUtf8ToArray (pLine, LineLen, g_Chain, MaxChainSize);

                if (0 > g_ChainSize || MaxChainSize - 1 < g_ChainSize) {
                    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
                }

                if (false == g_no_process) {

                    if (g_wtbt || g_wts) {
                        InitWtbtData ();
                    }
                    if (g_to_lower) {
                        ToLower ();
                    }
                    if (g_to_upper) {
                        ToUpper ();
                    }
                    if (g_to_capitalized) {
                        ToCapitalized ();
                    }
                    if (g_pCharMapFile) {
                        Normalize (); // the WTBT bounaries are lost 
                        /// uncomment if needed
                        /// if (g_wtbt || g_wts) {
                        ///    InitWtbtData ();
                        /// }
                    }
                    if (g_pInTr) {
                        ApplyTransformation ();
                    }
                }

                // UTF-32 --> UTF-8
                const int OutStrSize = \
                  ::FAArrayToStrUtf8 (g_Chain, g_ChainSize, OutStr, MaxStrSize - 1);

                if (0 > OutStrSize || MaxStrSize - 1 < OutStrSize) {
                    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
                }

                OutStr [OutStrSize] = 0;

                if (false == g_no_output) {
                    (*pOs) << OutStr;
                }

            } // of if (0 < LineLen) ...

            if (false == g_no_output) {
                (*pOs) << '\n';
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
