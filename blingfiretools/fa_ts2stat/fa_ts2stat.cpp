/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAMapIOTools.h"
#include "FACorpusIOTools_utf8.h"
#include "FATagSet.h"
#include "FATaggedText.h"
#include "FATaggedTextStat.h"
#include "FAPrintUtils.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FALimits.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * g_pInFile = NULL;
const char * g_pTagsetFile = NULL;

const FATagSet * g_pTagSet = NULL;

const char * g_pOutFile_w = NULL;
const char * g_pOutFile_ww = NULL;
const char * g_pOutFile_www = NULL;
const char * g_pOutFile_wt = NULL;
const char * g_pOutFile_wtt = NULL;
const char * g_pOutFile_twt = NULL;
const char * g_pOutFile_wtwt = NULL;
const char * g_pOutFile_t = NULL;
const char * g_pOutFile_tt = NULL;
const char * g_pOutFile_ttt = NULL;
const char * g_pOutFile_tttt = NULL;
const char * g_pOutFile_w_t = NULL;
const char * g_pOutFile_tw = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ofstream g_ofs_w ;
std::ofstream g_ofs_ww ;
std::ofstream g_ofs_www ;
std::ofstream g_ofs_wt ;
std::ofstream g_ofs_wtt ;
std::ofstream g_ofs_twt ;
std::ofstream g_ofs_wtwt ;
std::ofstream g_ofs_t ;
std::ofstream g_ofs_tt ;
std::ofstream g_ofs_ttt ;
std::ofstream g_ofs_tttt ;
std::ofstream g_ofs_w_t ;
std::ofstream g_ofs_tw ;

int g_StatMask = FAFsmConst::STAT_TYPE_NONE ;

const char * g_pBosTagName = NULL;
int g_BosTag = 0;
const char * g_pEosTagName = NULL;
int g_EosTag = 0;

int BosWord [FALimits::MaxWordLen] ;
unsigned int BosWordLen = 0;
int EosWord [FALimits::MaxWordLen] ;
unsigned int EosWordLen = 0;

bool g_ignore_case = false;

const int SENT_COUNT_DELAY = 10000;
int g_line_step = -1;
int g_min_freq = 1;
bool g_print_text = false;
bool g_print_input = false;
bool g_verbose = false;
bool g_hex = false;
int g_digits = 5;
bool g_force = false;

long LineNum = 0;


void usage () {

  std::cout << "\n\
Usage: fa_ts2stat [OPTIONS] [< input.utf8]\n\
\n\
This program collects different kinds of statistics from the tagged text.\n\
\n\
Input/Output:\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file,\n\
    have to be specified\n\
\n\
  --bos-tag=<name> - BOS tagname for POS tags,\n\
     if in use the parameter have to be specified\n\
\n\
  --eos-tag=<name> - EOS tagname for POS tags,\n\
     if in use the parameter have to be specified\n\
\n\
  --bos-word=<text> - BOS word text, empty string is used by default\n\
\n\
  --eos-word=<text> - EOS word text, empty string is used by default\n\
\n\
  --out-t=<output> - writes T\\tF\\n to the <output> file\n\
\n\
  --out-tt=<output> - writes T\\tT\\tF\\n to the <output> file\n\
\n\
  --out-ttt=<output> - writes T\\tT\\tT\\tF\\n to the <output> file\n\
\n\
  --out-tttt=<output> - writes T\\tT\\tT\\tT\\tF\\n to the <output> file\n\
\n\
  --out-w=<output> - writes W\\tF\\n to the <output> file\n\
\n\
  --out-ww=<output> - writes W\\tW\\tF\\n to the <output> file\n\
\n\
  --out-www=<output> - writes W\\tW\\tW\\tF\\n to the <output> file\n\
\n\
  --out-wt=<output> - writes W\\tT\\tF\\n to the <output> file\n\
\n\
  --out-w_t=<output> - writes W\\tT\\tF\\n to the <output> file, W here is a\n\
    preceding word\n\
\n\
  --out-tw=<output> - writes W\\tT\\tF\\n to the <output> file, W here is a\n\
    following word\n\
\n\
  --out-wtt=<output> - writes W\\tT\\tT\\tF\\n to the <output> file,\n\
    the second tag corresponds the tag of the following word\n\
\n\
  --out-twt=<output> - writes W\\tT\\tT\\tF\\n to the <output> file,\n\
    the second tag corresponds the tag of the previous word\n\
\n\
  --out-wtwt=<output> - writes W\\tW\\tT\\tT\\tF\\n to the <output> file\n\
\n" 
    << "Additional parameters:\n\
\n\
  --line-step=N - the amount of lines processed at once,\n\
    by default full input is processed at once\n\
\n\
  --min-freq=N - frequency limit for DATA\\tF\\n to be printed out,\n\
    1 is used by default (everything is printed out)\n\
\n\
  --ignore-case - converts input text to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --print-text - prints words and tags as a text vs integer chains\n\
\n\
  --print-input - repeats input at stdout (for piping)\n\
\n\
  --base=hex - prints integers (including counts) in hexadecimal format,\n\
    uses decimal base by default\n\
\n\
  --verbose - prints to stderr the number of processed words for\n\
    every " << SENT_COUNT_DELAY << " sentences\n\
\n\
  --force - will drop incorrectly formatted corpus lines and will get \n\
    statistics anyway\n\
\n\
Notes:\n\
\n\
 1. Input should be in UTF-8 in corpus IO format (see FACorpusIOTools_utf8.h)\n\
 2. Currently frequencies cannot be bigger than MAX_INT\n\
 3. All statistics should fit the memory.\n\
 4. If --line-step=N was used then the resulting statistics should be \n\
    postprocessed with fa_merge_stat\n\
";

}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv){

    if (!strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pTagsetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strcmp ("--ignore-case", *argv)) {
        g_ignore_case = true;
        continue;
    }
    if (0 == strncmp ("--bos-tag=", *argv, 10)) {
        g_pBosTagName = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--eos-tag=", *argv, 10)) {
        g_pEosTagName = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--bos-word=", *argv, 11)) {
        BosWordLen = \
          ::FAStrUtf8ToArray (&((*argv) [11]), BosWord, FALimits::MaxWordLen);
        FAAssert (0 < BosWordLen, FAMsg::InvalidParameters);
        continue;
    }
    if (0 == strncmp ("--eos-word=", *argv, 11)) {
        EosWordLen = \
          ::FAStrUtf8ToArray (&((*argv) [11]), EosWord, FALimits::MaxWordLen);
        FAAssert (0 < EosWordLen, FAMsg::InvalidParameters);
        continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
        g_pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out-w=", *argv, 8)) {
        g_pOutFile_w = &((*argv) [8]);
        g_StatMask |= FAFsmConst::STAT_TYPE_W;
        continue;
    }
    if (0 == strncmp ("--out-ww=", *argv, 9)) {
        g_pOutFile_ww = &((*argv) [9]);
        g_StatMask |= FAFsmConst::STAT_TYPE_WW;
        continue;
    }
    if (0 == strncmp ("--out-www=", *argv, 10)) {
        g_pOutFile_www = &((*argv) [10]);
        g_StatMask |= FAFsmConst::STAT_TYPE_WWW;
        continue;
    }
    if (0 == strncmp ("--out-wt=", *argv, 9)) {
        g_pOutFile_wt = &((*argv) [9]);
        g_StatMask |= FAFsmConst::STAT_TYPE_WT;
        continue;
    }
    if (0 == strncmp ("--out-wtt=", *argv, 10)) {
        g_pOutFile_wtt = &((*argv) [10]);
        g_StatMask |= FAFsmConst::STAT_TYPE_WTT;
        continue;
    }
    if (0 == strncmp ("--out-twt=", *argv, 10)) {
        g_pOutFile_twt = &((*argv) [10]);
        g_StatMask |= FAFsmConst::STAT_TYPE_TWT;
        continue;
    }
    if (0 == strncmp ("--out-wtwt=", *argv, 11)) {
        g_pOutFile_wtwt = &((*argv) [11]);
        g_StatMask |= FAFsmConst::STAT_TYPE_WTWT;
        continue;
    }
    if (0 == strncmp ("--out-t=", *argv, 8)) {
        g_pOutFile_t = &((*argv) [8]);
        g_StatMask |= FAFsmConst::STAT_TYPE_T;
        continue;
    }
    if (0 == strncmp ("--out-tt=", *argv, 9)) {
        g_pOutFile_tt = &((*argv) [9]);
        g_StatMask |= FAFsmConst::STAT_TYPE_TT;
        continue;
    }
    if (0 == strncmp ("--out-ttt=", *argv, 10)) {
        g_pOutFile_ttt = &((*argv) [10]);
        g_StatMask |= FAFsmConst::STAT_TYPE_TTT;
        continue;
    }
    if (0 == strncmp ("--out-tttt=", *argv, 11)) {
        g_pOutFile_tttt = &((*argv) [11]);
        g_StatMask |= FAFsmConst::STAT_TYPE_TTTT;
        continue;
    }
    if (0 == strncmp ("--line-step=", *argv, 12)) {
        g_line_step = atoi (&((*argv) [12]));
        continue;
    }
    if (0 == strncmp ("--min-freq=", *argv, 11)) {
        g_min_freq = atoi (&((*argv) [11]));
        continue;
    }
    if (!strcmp ("--print-text", *argv)) {
        g_print_text = true;
        continue;
    }
    if (!strcmp ("--print-input", *argv)) {
        g_print_input = true;
        continue;
    }
    if (!strcmp ("--base=hex", *argv)) {
        g_hex = true;
        g_digits = 4;
        continue;
    }
    if (!strcmp ("--verbose", *argv)) {
        g_verbose = true;
        continue;
    }
    if (0 == strncmp ("--out-w_t=", *argv, 10)) {
        g_pOutFile_w_t = &((*argv) [10]);
        g_StatMask |= FAFsmConst::STAT_TYPE_W_T;
        continue;
    }
    if (0 == strncmp ("--out-tw=", *argv, 9)) {
        g_pOutFile_tw = &((*argv) [9]);
        g_StatMask |= FAFsmConst::STAT_TYPE_TW;
        continue;
    }
    if (!strcmp ("--force", *argv)) {
        g_force = true;
        continue;
    }
  }
}


void PrintWord (std::ostream & os, const int * pWord, const int Length)
{
    if (false == g_print_text) {
        // print a word as a chain of UTF-32 symbols
        ::FAPrintChain (os, pWord, Length, FAFsmConst::DIR_L2R, g_digits, g_hex);
    } else {
        // print a word in UTF-8
        ::FAPrintWord (os, pWord, Length);
    }
}


void PrintTag (std::ostream & os, const int Tag)
{
    DebugLogAssert (g_pTagSet);

    if (false == g_print_text) {

        ::FAPrintValue (os, Tag, g_digits, g_hex);

    } else {
        const char * pStr;
        const int StrLen = g_pTagSet->Tag2Str(Tag, &pStr);
        FAAssert (0 < StrLen && pStr, FAMsg::InternalError);

        for (int j = 0; j < StrLen; ++j) {
            DebugLogAssert (FAFsmConst::CHAR_TAG_DELIM != pStr [j]);
            os << pStr [j];
        }
    }
}


void PrintWWW (std::ostream& ofs, const FAChain2NumA * pMap)
{
    DebugLogAssert (pMap);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int Freq = pMap->GetValue (ChainIdx);

        if (g_min_freq > Freq)
            continue;

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);

        int From = 0;

        for (int To = 0; To < ChainSize; ++To) {

            if (0 == pChain [To]) {
                const int Len = To - From;
                PrintWord (ofs, pChain + From, Len);
                ofs << '\t' ;
                From = To + 1;
            }
        }

        PrintWord (ofs, pChain + From, ChainSize - From);
        ofs << '\t' ;
        ::FAPrintValue (ofs, Freq, 1, g_hex);
        ofs << '\n' ;
    }
}


void PrintWT (std::ostream& ofs, const FAChain2NumA * pMap)
{
    DebugLogAssert (pMap);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int Freq = pMap->GetValue (ChainIdx);

        if (g_min_freq > Freq)
            continue;

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);
        DebugLogAssert (1 <= ChainSize);

        // check whether the W(ord) is not empty
        if (0 < ChainSize - 1) {
            PrintWord (ofs, pChain, ChainSize - 1);
        }
        ofs << '\t' ;
        PrintTag (ofs, pChain [ChainSize - 1]);
        ofs << '\t' ;
        ::FAPrintValue (ofs, Freq, 1, g_hex);
        ofs << '\n' ;
    }
}


void PrintWTT_TWT (std::ostream& ofs, const FAChain2NumA * pMap)
{
    DebugLogAssert (pMap);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int Freq = pMap->GetValue (ChainIdx);

        if (g_min_freq > Freq)
            continue;

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);
        DebugLogAssert (2 < ChainSize);

        PrintWord (ofs, pChain, ChainSize - 2);
        ofs << '\t' ;
        PrintTag (ofs, pChain [ChainSize - 2]);
        ofs << '\t' ;
        PrintTag (ofs, pChain [ChainSize - 1]);
        ofs << '\t' ;
        ::FAPrintValue (ofs, Freq, 1, g_hex);
        ofs << '\n' ;
    }
}


void PrintWTWT (std::ostream& ofs, const FAChain2NumA * pMap)
{
    DebugLogAssert (pMap);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int Freq = pMap->GetValue (ChainIdx);

        if (g_min_freq > Freq)
            continue;

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);
        DebugLogAssert (3 <= ChainSize && pChain);

        int DelimPos = 0;
        for (; DelimPos < ChainSize - 2; ++DelimPos) {
            if (0 == pChain [DelimPos]) {
                break;
            }
        }
        FAAssert (DelimPos < ChainSize - 2, FAMsg::InternalError);

        const int Length1 = DelimPos;
        const int Length2 = ChainSize - Length1 - 3;

        PrintWord (ofs, pChain, Length1);
        ofs << '\t' ;
        PrintWord (ofs, pChain + Length1 + 1, Length2);
        ofs << '\t' ;
        PrintTag (ofs, pChain [ChainSize - 2]);
        ofs << '\t' ;
        PrintTag (ofs, pChain [ChainSize - 1]);
        ofs << '\t' ;
        ::FAPrintValue (ofs, Freq, 1, g_hex);
        ofs << '\n' ;
    }
}


void PrintTTTT (std::ostream& ofs, const FAChain2NumA * pMap)
{
    DebugLogAssert (pMap);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);

        const int Freq = pMap->GetValue (ChainIdx);

        if (g_min_freq > Freq)
            continue;

        // print TAGs
        for (int i = 0; i < ChainSize; ++i) {
            PrintTag (ofs, pChain [i]);
            ofs << '\t' ;
        }
        // print FREQ
        ::FAPrintValue (ofs, Freq, 1, g_hex);
        ofs << '\n' ;
    }
}


void InitOutput ()
{
    if (g_StatMask & FAFsmConst::STAT_TYPE_W) {
        g_ofs_w.open (g_pOutFile_w, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WW) {
        g_ofs_ww.open (g_pOutFile_ww, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WWW) {
        g_ofs_www.open (g_pOutFile_www, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WT) {
        g_ofs_wt.open (g_pOutFile_wt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WTT) {
        g_ofs_wtt.open (g_pOutFile_wtt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TWT) {
        g_ofs_twt.open (g_pOutFile_twt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WTWT) {
        g_ofs_wtwt.open (g_pOutFile_wtwt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_T) {
        g_ofs_t.open (g_pOutFile_t, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TT) {
        g_ofs_tt.open (g_pOutFile_tt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TTT) {
        g_ofs_ttt.open (g_pOutFile_ttt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TTTT) {
        g_ofs_tttt.open (g_pOutFile_tttt, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_W_T) {
        g_ofs_w_t.open (g_pOutFile_w_t, std::ios::out);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TW) {
        g_ofs_tw.open (g_pOutFile_tw, std::ios::out);
    }
}


void PrintStat (const FATaggedTextStat * pStats)
{
    DebugLogAssert (pStats);

    if (g_StatMask & FAFsmConst::STAT_TYPE_W) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_W);
        PrintWWW (g_ofs_w, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WW) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_WW);
        PrintWWW (g_ofs_ww, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WWW) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_WWW);
        PrintWWW (g_ofs_www, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_WT);
        PrintWT (g_ofs_wt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WTT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_WTT);
        PrintWTT_TWT (g_ofs_wtt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TWT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_TWT);
        PrintWTT_TWT (g_ofs_twt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_WTWT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_WTWT);
        PrintWTWT (g_ofs_wtwt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_T) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_T);
        PrintTTTT (g_ofs_t, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_TT);
        PrintTTTT (g_ofs_tt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TTT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_TTT);
        PrintTTTT (g_ofs_ttt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TTTT) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_TTTT);
        PrintTTTT (g_ofs_tttt, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_W_T) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_W_T);
        PrintWT (g_ofs_w_t, pMap);
    }
    if (g_StatMask & FAFsmConst::STAT_TYPE_TW) {
        const FAChain2NumA * pMap = \
            pStats->GetStat (FAFsmConst::STAT_TYPE_TW);
        PrintWT (g_ofs_tw, pMap);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {
        // IO
        FAMapIOTools g_map_io (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io (&g_alloc);

        // tagset
        FATagSet g_tagset (&g_alloc);
        g_pTagSet = &g_tagset;
        // input tagged text
        FATaggedText g_text (&g_alloc);

        g_txt_io.SetTagSet (&g_tagset);

        /// adjust IO pointers; stdin is used if g_pInFile is not specified.
        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }

        /// load POS tagset and get the EOS/BOS tag IDs from the input args.
        if (g_pTagsetFile) {
            std::ifstream tagset_ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagsetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
        } else {
            std::cerr << "ERROR: Tagset file must be specified\n";
            return 1;
        }

        if (g_pBosTagName) {
            const int BosStrLen = (int) strlen (g_pBosTagName);
            g_BosTag = g_tagset.Str2Tag (g_pBosTagName, BosStrLen);
            if (-1 == g_BosTag) {
                std::cerr << "ERROR: Unknown BOS tag " << g_pBosTagName << '\n';
                return 1;
            }
        }

        if (g_pEosTagName) {
            const int EosStrLen = (int) strlen (g_pEosTagName);
            g_EosTag = g_tagset.Str2Tag (g_pEosTagName, EosStrLen);
            if (-1 == g_EosTag) {
                std::cerr << "ERROR: Unknown EOS tag " << g_pEosTagName << '\n';
                return 1;
            }
        }

        // create output stream objects
        InitOutput ();

        // create statistics storage
        FATaggedTextStat * pStats = NEW FATaggedTextStat (&g_alloc);
        FAAssert (pStats, FAMsg::OutOfMemory);

        pStats->SetStatMask (g_StatMask);
        pStats->SetIgnoreCase (g_ignore_case);
        pStats->SetBosTag (g_BosTag);
        pStats->SetEosTag (g_EosTag);
        pStats->SetBosWord (BosWord, BosWordLen);
        pStats->SetEosWord (EosWord, EosWordLen);

        unsigned int wc = 0;
        unsigned int sc = 0;

        /// process input, line by line
        while (!g_pIs->eof ()) {

            LineNum++;

            // see if the IO errors should not be ignored
            if (false == g_force) {
                g_txt_io.Read (*g_pIs, &g_text);
            } else {
                try {
                    g_txt_io.Read (*g_pIs, &g_text);
                } catch (const FAException & ) {
                    g_text.Clear ();
                }
            }

            if (g_print_input) {
                g_txt_io.Print (std::cout, &g_text);
            }

            pStats->UpdateStat (&g_text);

            sc++;

            if (-1 != g_line_step && 0 == (sc % g_line_step)) {

                /// print the statistics earlier
                PrintStat (pStats);
                /// make sure thememory is completely returned
                delete pStats;

                // re-create statistics storage
                pStats = NEW FATaggedTextStat (&g_alloc);
                FAAssert (pStats, FAMsg::OutOfMemory);

                pStats->SetStatMask (g_StatMask);
                pStats->SetIgnoreCase (g_ignore_case);
                pStats->SetBosTag (g_BosTag);
                pStats->SetEosTag (g_EosTag);
                pStats->SetBosWord (BosWord, BosWordLen);
                pStats->SetEosWord (EosWord, EosWordLen);
            }

            if (g_verbose) {
                wc += g_text.GetWordCount ();
                if (0 == (sc % SENT_COUNT_DELAY)) {
                    std::cerr << "              \r" << wc ;
                }
            }

        } // of while (!g_pIs->eof ()) ...

        if (g_verbose) {
            std::cerr << "              \r" << wc << '\n';
        }

        /// print out the statistics
        PrintStat (pStats);

        delete pStats;

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        if (0 < LineNum) {
            std::cerr << "ERROR: while reading the data at line: " << LineNum ;
        }

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
