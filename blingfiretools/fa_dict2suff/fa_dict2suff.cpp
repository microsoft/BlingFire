/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAStr2Utf16.h"
#include "FADictStr2SuffixRule.h"
#include "FAImageDump.h"
#include "FARSDfa_pack_triv.h"
#include "FATransformCA_t.h"
#include "FATransform_hyph_redup_t.h"
#include "FATransform_hyph_redup_rev_t.h"
#include "FATransform_prefix_t.h"
#include "FATransform_prefix_rev_t.h"
#include "FATransform_capital_t.h"
#include "FATransform_capital_rev_t.h"
#include "FATransform_cascade_t.h"
#include "FAStringTokenizer.h"
#include "FAMultiMap_pack_fixed.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFile = NULL;
const char * g_pInPrefFsmFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pInEnc = "CP1251";

bool g_is_utf8 = false;
int g_redup_delim = -1;
int g_pref_delim = -1;
int g_ucf_delim = -1;
bool g_ignore_case = false;
bool g_use_pref = false;

FAAllocator g_alloc;

// recode
FAStr2Utf16 g_recode (&g_alloc);

FAImageDump g_pref_dfa_image;
FARSDfa_pack_triv g_pref_fsm_dump;

// transformations
FATransform_hyph_redup_t < int > g_tr_hyph_redup;
FATransform_hyph_redup_rev_t < int > g_tr_hyph_redup_rev;
FATransform_prefix_t < int > g_tr_pref;
FATransform_prefix_rev_t < int > g_tr_pref_rev;
FATransform_capital_t < int > g_tr_ucf;
FATransform_capital_rev_t < int > g_tr_ucf_rev;
FATransform_cascade_t < int > g_in_tr_cascade;
FATransform_cascade_t < int > g_out_tr_cascade;

const FATransformCA_t < int > * g_pInTr = NULL;
const FATransformCA_t < int > * g_pOutTr = NULL;
const FARSDfaCA * g_pPrefFsm = NULL;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;
const FAMultiMapCA * g_pCharMap = NULL;

// input chain
const int MaxChainSize = 4096;
int g_InChain [MaxChainSize];
int g_InChainSize;

// output chain
const int * g_pOutChain;
int g_OutChainSize;

std::string line;
unsigned int LineNum = 0;


void usage () {

  std::cout << "\n\
Usage: fa_dict2suff [OPTIONS]\n\
\n\
This program converts input word pairs (optionally with tags) into suffix rules.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - prints output into the <output> file,\n\
    if omited stdout is used\n\
\n\
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    CP1251 is used by default\n\
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
  --out-tr=<trs> - specifies output transformation type\n\
    <trs> is the same as in --in-tr= option\n\
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
  --ignore-case - converts input symbols to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
\n\
  --use-pref - generates exteded rules with for both prefixes and suffixes\n\
\n\
\n\
Input format:\n\
  ...\n\
  word_from\\tword_to[\\ttag_from[\\ttag_to]]\\n\n\
  ...\n\
\n\
Output format:\n\
  ...\n\
  word_from --> [tag_from][tag_to]-N+suff\\n\n\
  ...\n\
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

        if (!strcmp ("--help", *argv)) {
            usage ();
            exit (0);
        }
        if (0 == strncmp ("--input-enc=", *argv, 12)) {
            g_pInEnc = &((*argv) [12]);
            g_is_utf8 = ::FAIsUtf8Enc (g_pInEnc);
            continue;
        }
        if (0 == strncmp ("--in=", *argv, 5)) {
            g_pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
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
        if (0 == strcmp ("--out-tr=hyph-redup", *argv)) {
            g_pOutTr = &g_tr_hyph_redup;
            continue;
        }
        if (0 == strcmp ("--out-tr=hyph-redup-rev", *argv)) {
            g_pOutTr = &g_tr_hyph_redup_rev;
            continue;
        }
        if (0 == strcmp ("--out-tr=pref", *argv)) {
            g_pOutTr = &g_tr_pref;
            continue;
        }
        if (0 == strcmp ("--out-tr=pref-rev", *argv)) {
            g_pOutTr = &g_tr_pref_rev;
            continue;
        }
        if (0 == strcmp ("--out-tr=ucf", *argv)) {
            g_pOutTr = &g_tr_ucf;
            continue;
        }
        if (0 == strcmp ("--out-tr=ucf-rev", *argv)) {
            g_pOutTr = &g_tr_ucf_rev;
            continue;
        }
        if (0 == strncmp ("--in-tr=", *argv, 8)) {
            const char * pTrsStr = &((*argv) [8]);
            const int TrsStrLen = (int) strlen (pTrsStr);
            InitTrCascade (&g_in_tr_cascade, pTrsStr, TrsStrLen);
            g_pInTr = &g_in_tr_cascade;
            continue;
        }
        if (0 == strncmp ("--out-tr=", *argv, 9)) {
            const char * pTrsStr = &((*argv) [9]);
            const int TrsStrLen = (int) strlen (pTrsStr);
            InitTrCascade (&g_out_tr_cascade, pTrsStr, TrsStrLen);
            g_pOutTr = &g_out_tr_cascade;
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
        if (0 == strcmp ("--ignore-case", *argv)) {
            g_ignore_case = true;
            continue;
        }
        if (0 == strncmp ("--charmap=", *argv, 10)) {
            g_pCharMapFile = &((*argv) [10]);
            g_pCharMap = &g_charmap;
            continue;
        }
        if (0 == strcmp ("--use-pref", *argv)) {
            g_use_pref = true;
            continue;
        }
    }
}


void Line2Chain (const char * pStr, const int StrLen)
{
    DebugLogAssert (0 < StrLen && pStr);

    if (MaxChainSize < StrLen) {
        std::cerr << "ERROR: \"Input string is too long\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }

    if (g_is_utf8) {

        g_InChainSize = \
            ::FAStrUtf8ToArray (pStr, StrLen, g_InChain, MaxChainSize);

    } else {

        g_InChainSize = \
            g_recode.Process (pStr, StrLen, g_InChain, MaxChainSize);

    } // if (g_is_utf8) ...

    if (-1 == g_InChainSize) {
        std::cerr << "ERROR: \"Conversion is not possible\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }
}


void Print (std::ostream * pOs)
{
    DebugLogAssert (pOs);
    DebugLogAssert (0 < g_OutChainSize && g_pOutChain);

    const int MaxOutStr = MaxChainSize;
    char OutStr [MaxChainSize];

    const int OutStrLen = \
        ::FAArrayToStrUtf8 (g_pOutChain, g_OutChainSize, OutStr, MaxOutStr - 1);

    if (0 > OutStrLen || MaxChainSize - 1 < OutStrLen) {
        throw FAException (FAMsg::IOError, __FILE__, __LINE__);
    }

    OutStr [OutStrLen] = 0;

    (*pOs) << OutStr;
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        // select in/out streams
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != g_pInFile) {
            ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&ifs, g_pInFile);
            pIs = &ifs;
        }
        if (NULL != g_pOutFile) {
            ofs.open (g_pOutFile, std::ios::out);
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

        g_recode.SetEncodingName (g_pInEnc);

        if (g_pInPrefFsmFile) {

            g_pref_dfa_image.Load (g_pInPrefFsmFile);
            const unsigned char * pImg = g_pref_dfa_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_pref_fsm_dump.SetImage (pImg);
            g_pPrefFsm = &g_pref_fsm_dump;
            g_tr_pref.SetRsDfa (g_pPrefFsm);
        }
        if (-1 != g_redup_delim) {
            g_tr_hyph_redup.SetDelim (g_redup_delim);
            g_tr_hyph_redup_rev.SetDelim (g_redup_delim);
        }
        if (-1 != g_pref_delim) {
            g_tr_pref.SetDelim (g_pref_delim);
            g_tr_pref_rev.SetDelim (g_pref_delim);
        }
        if (-1 != g_ucf_delim) {
            g_tr_ucf.SetDelim (g_ucf_delim);
            g_tr_ucf_rev.SetDelim (g_ucf_delim);
        }

        FADictStr2SuffixRule dict2suff (&g_alloc);

        dict2suff.SetInTr (g_pInTr);
        dict2suff.SetOutTr (g_pOutTr);
        dict2suff.SetIgnoreCase (g_ignore_case);
        dict2suff.SetUsePref (g_use_pref);
        dict2suff.SetCharmap (g_pCharMap);

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

                Line2Chain (pLine, LineLen);

                g_OutChainSize = \
                    dict2suff.Process (g_InChain, g_InChainSize, &g_pOutChain);

                Print (pOs);
                (*pOs) << '\n';
            }
        }

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
        return 1;
    }

    return 0;
}
