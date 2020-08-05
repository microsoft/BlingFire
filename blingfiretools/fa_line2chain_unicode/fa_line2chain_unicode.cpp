/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAStr2Utf16.h"
#include "FAUtf32Utils.h"
#include "FAMapIOTools.h"
#include "FATagSet.h"
#include "FAFsmConst.h"
#include "FATransform_hyph_redup_t.h"
#include "FATransform_hyph_redup_rev_t.h"
#include "FATransform_prefix_t.h"
#include "FATransform_prefix_rev_t.h"
#include "FATransform_capital_t.h"
#include "FATransform_capital_rev_t.h"
#include "FATransform_unescape_t.h"
#include "FATransform_cascade_t.h"
#include "FARSDfa_pack_triv.h"
#include "FAImageDump.h"
#include "FAStringTokenizer.h"
#include "FAArray_cont_t.h"
#include "FAMultiMap_pack_fixed.h"
#include "FASecurity.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

using namespace BlingFire;

const char * __PROG__ = "";

bool g_no_output = false;
bool g_hex_output = false;
bool g_use_keys = false;
bool g_use_keypairs = false;
bool g_ignore_case = false;
bool g_compounds = false;
int g_key_base = 0;
bool g_key_delim = false;
int g_num_size = -1;
// -1 means no limit
int g_chain_size_limit = -1;
int g_dir = FAFsmConst::DIR_L2R;

const char * g_pInEnc = "UTF-8";
bool g_fDecEnc = false;
bool g_fHexEnc = false;
bool g_fUInt8Enc = false;

const int MaxChainSize = 4096;
int Chain [MaxChainSize];

bool g_use_tags = false;
bool g_float_nums = false;

const char * pInTagSetFile = NULL;
const char * pKey2FreqFile = NULL;

FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);

FATagSet g_tagset (&g_alloc);

// transformations
FATransform_hyph_redup_t < int > g_tr_hyph_redup;
FATransform_hyph_redup_rev_t < int > g_tr_hyph_redup_rev;
FATransform_prefix_t < int > g_tr_pref;
FATransform_prefix_rev_t < int > g_tr_pref_rev;
FATransform_capital_t < int > g_tr_ucf;
FATransform_capital_rev_t < int > g_tr_ucf_rev;
FATransform_unescape_t < int > g_tr_unesc;
FATransform_cascade_t < int > g_in_tr_cascade;

int g_redup_delim = -1;
int g_pref_delim = -1;
int g_ucf_delim = -1;

// If true, not allow TAB's to be replaced with 0 in the function PrintValue
bool g_use_tr_unesc = false;

const char * g_pInPrefFsmFile = NULL;
FAImageDump g_pref_dfa_image;
FARSDfa_pack_triv g_pref_fsm_dump;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;

const FATransformCA_t < int > * g_pInTr = NULL;
const FARSDfaCA * g_pPrefFsm = NULL;

FAArray_cont_t < int > g_Key2Freq;

unsigned int LineNum;
std::string line;


void usage ()
{
    std::cerr << "\
Usage: fa_line2chain_unicode [OPTION] < lines.txt > chains.txt\n\
\n\
Converts textual lines in a given encoding into a space separated chain\n\
of unicode symbol codes in UTF-16.\n\
\n\
  --out-key2f=<key2freq> - writes Key -> Freq array, if specified\n\
    is not used by default\n\
\n\
  --input-enc=<encoding> - input encoding name, \n\
      if --input-enc=DEC is specifed then the input words are a sequence\n\
         of space dilimited decimal numbers\n\
      if --input-enc=HEX is specifed then the input words are a sequence\n\
         of space dilimited hexadecimal numbers\n\
      if --input-enc=UInt8 is specifed then the input sequence is treated\n\
         as a binary byte sequence\n\
\n\
  --base=10 - uses decimal base for the output numbers,\n\
    (is used by default)\n\
\n\
  --base=16 - uses hexadecimal base for the output numbers\n\
\n\
  --num-size=N - number of digitis per value/key,\n\
    4 for --base=16 and 5 for --base=10 is used by default\n\
\n\
  --use-keys - if this option is specified then the program read lines\n\
    only upto the first tab, the rest is treated as a key. The key is \n\
    a number or (if tagset is specified) a tag name.\n\
\n\
  --use-keypairs - if this option is specified then the program read lines\n\
    only up to the second tab, the rest is treated as a key. The key is \n\
    a number or (if tagset is specified) a tag name. The result is output\n\
    as key0info rather than info0key.\n\
\n\
  --tagset=<input-file> - reads input tagset from the <input-file>,\n\
    if omited does not use tagset\n\
\n\
  --key-base=N - adds N to all assosiated keys,\n\
    0 by default\n\
\n\
  --key-delim - adds delimiter 0 before each key\n\
\n\
  --dir=<direction> - is one of the following:\n\
    l2r - left to right (the dafault value)\n\
    r2l - right to left\n\
    aff - affix first, e.g. last, first, last - 1, first + 1, ...\n\
\n\
";

    std::cerr << "\
  --in-tr=<trs> - specifies input transformation type\n\
    <trs> is comma-separated array of the following:\n\
      hyph-redup - hyphenated reduplication\n\
      hyph-redup-rev - reverse hyphenated reduplication\n\
      pref - prefix transformation: represents special prefixes as suffixes\n\
      pref-rev - reversed prefix transformation\n\
      ucf - encodes upper-case-first symbol in a suffix\n\
      ucf-rev - reversed UCF transformation\n\
      unesc - unescape escape sequences \\r, \\n, \\t and \\\\\n\
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
  --trim=N - output chains will be trimmed to have up to N elements,\n\
    no trimming is made by default\n\
\n\
  --ignore-case - converts input symbols to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
\n\
  --compounds - encodes the length of the right segment for tab-separated\n\
    pairs of words, uses --key-base=N as a base for this value\n\
\n\
  --float-nums - if specified, allows floating point numbers to be \n\
    inter-mixed with tags\n\
\n\
  --no-output - makes no output\n\
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

    } else if (0 == strncmp ("unesc", pTrType, 5)) {

      pTrs->AddTransformation (&g_tr_unesc);
      g_use_tr_unesc = true;

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
    if (0 == strncmp ("--out-key2f=", *argv, 12)) {
      pKey2FreqFile = &((*argv) [12]);
      continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--input-enc=", *argv, 12)) {
      g_pInEnc = &((*argv) [12]);
      g_fDecEnc = (0 == strcmp ("DEC", g_pInEnc));
      g_fHexEnc = (0 == strcmp ("HEX", g_pInEnc));
      g_fUInt8Enc = (0 == strcmp ("UInt8", g_pInEnc));
      continue;
    }
    if (0 == strncmp ("--base=16", *argv, 9)) {
      g_hex_output = true;
      continue;
    }
    if (0 == strncmp ("--dir=l2r", *argv, 9)) {
      g_dir = FAFsmConst::DIR_L2R;
      continue;
    }
    if (0 == strncmp ("--dir=r2l", *argv, 9)) {
      g_dir = FAFsmConst::DIR_R2L;
      continue;
    }
    if (0 == strncmp ("--dir=aff", *argv, 9)) {
      g_dir = FAFsmConst::DIR_AFF;
      continue;
    }
    if (0 == strcmp ("--use-keys", *argv)) {
      g_use_keys = true;
      continue;
    }
    if (0 == strcmp ("--use-keypairs", *argv)) {
      g_use_keypairs = true;
      continue;
    }
    if (0 == strncmp ("--key-base=", *argv, 11)) {
      g_key_base = atoi (&((*argv) [11]));
      continue;
    }
    if (0 == strcmp ("--key-delim", *argv)) {
      g_key_delim = true;
      continue;
    }
    if (0 == strcmp ("--float-nums", *argv)) {
      g_float_nums = true;
      continue;
    }
    if (0 == strncmp ("--trim=", *argv, 7)) {
      g_chain_size_limit = atoi (&((*argv) [7]));
      continue;
    }
    if (0 == strncmp ("--num-size=", *argv, 11)) {
      g_num_size = atoi (&((*argv) [11]));
      continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        pInTagSetFile = &((*argv) [9]);
        g_use_tags = true;
        continue;
    }
    if (0 == strcmp ("--ignore-case", *argv)) {
        g_ignore_case = true;
        continue;
    }
    if (0 == strcmp ("--compounds", *argv)) {
        g_compounds = true;
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
    if (0 == strcmp ("--in-tr=unesc", *argv)) {
        g_pInTr = &g_tr_unesc;
        g_use_tr_unesc = true;
        continue;
    }
    if (0 == strncmp ("--charmap=", *argv, 10)) {
        g_pCharMapFile = &((*argv) [10]);
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


inline void PrintValue (std::ostream * pOs, const int Value)
{
    DebugLogAssert (pOs);

    if (false == g_hex_output) {

      (*pOs) << std::setw (g_num_size) << std::setfill ('0') << ((g_use_keypairs && '\t'==Value && !g_use_tr_unesc) ? 0 : Value);

    } else {

      (*pOs) << std::setw (g_num_size) << std::setfill ('0') << std::hex << ((g_use_keypairs && '\t'==Value && !g_use_tr_unesc) ? 0 : Value);
    }
}


void PrintChain (const int * pChain, const int ChainSize)
{
    DebugLogAssert (pChain && 0 < ChainSize);
    DebugLogAssert (-1 == g_chain_size_limit || 0 < g_chain_size_limit);

    int OutputSize = ChainSize;
    if (-1 != g_chain_size_limit && g_chain_size_limit < ChainSize)
        OutputSize = g_chain_size_limit;

    if (FAFsmConst::DIR_L2R == g_dir) {

        PrintValue (&std::cout, pChain [0]);
        for (int i = 1; i < OutputSize; ++i) {
            std::cout << ' ';
            PrintValue (&std::cout, pChain [i]);
        }

    } else if (FAFsmConst::DIR_R2L == g_dir) {

        const int MinIdx = ChainSize - OutputSize;

        PrintValue (&std::cout, pChain [ChainSize - 1]);
        for (int i = ChainSize - 2; i >= MinIdx; --i) {
            std::cout << ' ';
            PrintValue (&std::cout, pChain [i]);
        }

    } else {

        DebugLogAssert (FAFsmConst::DIR_AFF == g_dir);

        int Right = ChainSize - 1;
        int Left = 0;
        int Count = 0;
        bool First = true;

        while (Right > Left) {

            if (OutputSize != Count) {
                if (!First) {
                    std::cout << ' ';
                } else {
                    First = false;
                }
                PrintValue (&std::cout, pChain [Right--]);
                Count++;
            } else {
                break;
            }
            if (OutputSize != Count) {
                std::cout << ' ';
                PrintValue (&std::cout, pChain [Left++]);
                Count++;
            } else {
                break;
            }
        } // of while ...

        if (Left == Right && OutputSize != Count) {
          if (!First) {
                std::cout << ' ';
          }
          PrintValue (&std::cout, pChain [Left]);
        }
    }
}


void PrintCompound (__out_ecount(Count) int * pChain, const int Count)
{
    DebugLogAssert (FAFsmConst::DIR_L2R == g_dir || \
            FAFsmConst::DIR_R2L == g_dir);

    int SplitPos = -1;

    for (int i = 0; i < Count; ++i) {

        if ('\t' == pChain [i]) {

            for (int j = i; j < Count - 1; ++j) {
                pChain [j] = pChain [j + 1];
            }

            SplitPos = i;
            break;
        }
    }

    if (-1 != SplitPos) {

        PrintChain (pChain, Count - 1);

        if (FAFsmConst::DIR_L2R == g_dir) {
            SplitPos = Count - 1 - SplitPos;
        }

        std::cout << ' ';
        PrintValue (&std::cout, g_key_base + SplitPos);

    } else {

        PrintChain (pChain, Count);
    }
}


void UpdateStat (const int Key)
{
    if (pKey2FreqFile) {

        FAAssert (0 <= Key, FAMsg::InvalidParameters);

        const int OldSize = g_Key2Freq.size ();
        const int GapSize = Key - OldSize + 1;

        if (0 < GapSize) {
            g_Key2Freq.resize (OldSize + GapSize);
            memset (g_Key2Freq.begin () + OldSize, 0, sizeof (int) * GapSize);
        }

        g_Key2Freq [Key]++;
    }
}


void PrintKey (const char * pKey, const int KeyLen)
{
    DebugLogAssert (pKey && 0 < KeyLen);

    int Key;

    if (g_key_delim) {
        std::cout << ' ';
        PrintValue (&std::cout, 0);
    }

    FAStringTokenizer tokenizer;
    tokenizer.SetSpaces ("\t");
    tokenizer.SetString (pKey, KeyLen);

    bool NoKeys = true;
    const char * pTagStr = NULL;
    int TagStrLen = 0;

    while (tokenizer.GetNextStr (&pTagStr, &TagStrLen)) {

        if (g_use_tags) {

            Key = g_tagset.Str2Tag (pTagStr, TagStrLen);

            if (0 >= Key) {
                if (false == g_float_nums) {
                    Key = atoi (pTagStr);
                } else {
                    double dKey = atof(pTagStr);
                    if (0.0 == dKey) {
                      std::cerr << std::endl << "ERROR: 0.0 weights are not allowed. Read from tag \"" << pTagStr << "\" " \
                          << " in program " << __PROG__ << std::endl;
                      exit (1);
                    }
                    float flKey = (float) dKey;
                    Key = *((int*)&flKey); // Note: this is hacky but works for all 32 and 64 bit platforms
                }
            }
            if (0 == Key) {
                std::cerr << std::endl << "ERROR: Unknown tag \"" << pTagStr << "\" " \
                    << " in program " << __PROG__ << std::endl;
                exit (1);
            }

        } else {

            Key = atoi (pTagStr);
        }

        // update Key --> Freq array
        UpdateStat (Key);

        Key += g_key_base;
        if (!g_use_keypairs) {
            std::cout << ' ';
        }
        PrintValue (&std::cout, Key);
        if (g_use_keypairs) {
            std::cout << ' ';
        }

        NoKeys = false;
    }

    if (NoKeys) {
        std::cerr << "ERROR: \"Line contains no keys.\"" \
            << " in program " << __PROG__ << '\n';
        exit (1);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    if (g_fUInt8Enc)
    {
        // Switch stdin to binary mode to avoid premature file truncation problem.
        ::FAInputIOSetup ();
    }

    try {

        if (-1 == g_num_size) {
          if (g_hex_output)
            g_num_size = 4;
          else
            g_num_size = 5;
        }

        g_Key2Freq.SetAllocator (&g_alloc);
        g_Key2Freq.Create ();

        // load tagset, if needed
        if (NULL != pInTagSetFile) {
            std::ifstream tagset_ifs (pInTagSetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, pInTagSetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
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
        // load normalization map, if needed
        if (g_pCharMapFile) {
            g_charmap_image.Load (g_pCharMapFile);
            const unsigned char * pImg = g_charmap_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_charmap.SetImage (pImg);
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

        FAStr2Utf16 cp2utf16 (&g_alloc);
        if (!g_fDecEnc && !g_fHexEnc && !g_fUInt8Enc) {
            cp2utf16.SetEncodingName (g_pInEnc);
        }

        const char * pDelim = NULL;
        LineNum = 0;

        while (!std::cin.eof ()) {

            if (!std::getline (std::cin, line))
                break;

            LineNum++;

            std::string::size_type EndOfLine = line.find_last_not_of("\r\n");
            if (EndOfLine != std::string::npos) {
                line.erase(EndOfLine + 1);
            }

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            if (!line.empty ()) {

                int DataLen = LineLen;

                if (MaxChainSize < DataLen) {
                    std::cerr << "ERROR: Line is too long, #" \
                              << LineNum \
                              << " in program " << __PROG__ << '\n';
                    exit (1);
                }
                if (g_use_keys) {

                    pDelim = strchr (pLine, '\t');
                    if (pDelim) {
                        DataLen = int (pDelim - pLine);
                    }
                }
                if (g_use_keypairs) {
                    pDelim = strchr (pLine, '\t');
                    if (pDelim) {
                        pDelim = strchr (pDelim+1, '\t');
                        if (pDelim) {
                            DataLen = int (pDelim - pLine);
                        }
                    }
                }

                int Count = 0;

                if (g_fDecEnc) {
                    // make a chain from the decimal numbers in ASCII, e.g. 255 1 238 2
                    Count = ::FAReadIntegerChain \
                        (pLine, DataLen, 10, Chain, MaxChainSize);
                } else if (g_fHexEnc) {
                    // make a chain from the hex in ASCII, e.g. FF 01 EE 02
                    Count = ::FAReadHexChain \
                        (pLine, DataLen, Chain, MaxChainSize);
                } else if (g_fUInt8Enc) {
                    // make a chain from the binary byte sequence
                    for (int i = 0; i < DataLen && i < MaxChainSize; ++i) {
                        Chain [i] = (unsigned char) pLine [i];
                    }
                    Count = DataLen;
                } else {
                    // make a UTF-32 chain from the plain-text in other encoding
                    Count = cp2utf16.Process \
                        (pLine, DataLen, Chain, MaxChainSize);
                }

                // input sequence is too long
                FAAssert (Count <= MaxChainSize, FAMsg::InternalError);

                if (-1 == Count) {
                    std::cerr << "ERROR: Conversion is not possible in line #"\
                              << LineNum \
                              << " in program " << __PROG__ << '\n';
                    exit (1);
                }

                // lower case, if needed
                if (g_ignore_case) {
                    ::FAUtf32StrLower (Chain, Count);
                }
                // normalize a word (in-place allowed)
                if (g_pCharMapFile) {
                    Count = ::FANormalizeWord (Chain, Count, \
                        Chain, MaxChainSize, &g_charmap);
                }
                // apply transformation, if needed
                if (g_pInTr && false == g_compounds) {

                    const int NewCount = \
                        g_pInTr->Process (Chain, Count, Chain, MaxChainSize);

                    if (-1 != NewCount) {
                        DebugLogAssert (NewCount <= MaxChainSize);
                        Count = NewCount;
                    }
                }

                // print the output chain and the key
                if (false == g_no_output) {

                    if (g_use_keypairs && pDelim) {
                        const char * pKey = pLine + DataLen + 1;
                        const int KeyLen = LineLen - DataLen - 1;
                        PrintKey (pKey, KeyLen);
                    }

                    if (false == g_compounds) {
                        PrintChain (Chain, Count);
                    } else {
                        PrintCompound (Chain, Count);
                    }

                    if (g_use_keys && pDelim) {
                        const char * pKey = pLine + DataLen + 1;
                        const int KeyLen = LineLen - DataLen - 1;
                        PrintKey (pKey, KeyLen);
                    }

                    std::cout << '\n';
                }

            } // of if (!line.empty ()) ...

        } // of while (!std::cin.eof ()) ...

        // print Key -> Freq array, if needed
        if (false == g_no_output && pKey2FreqFile) {

            const int Size = g_Key2Freq.size ();
            const int * pKey2Freq = g_Key2Freq.begin ();

            std::ofstream ofs_key2f (pKey2FreqFile, std::ios::out);
            g_map_io.Print (ofs_key2f, pKey2Freq, Size);
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
