/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FASetUtils.h"
#include "FAPrintUtils.h"
#include "FARegexpReverse.h"
#include "FARegexp2Nfa.h"
#include "FARegexpTree.h"
#include "FANfaCreator_base.h"
#include "FANfaCreator_digit.h"
#include "FANfaCreator_char.h"
#include "FANfaCreator_wre.h"
#include "FAMapIOTools.h"
#include "FAAutIOTools.h"
#include "FAChain2Num_hash.h"
#include "FAParsedRegexp2TrBrMaps.h"
#include "FAMultiMap_judy.h"
#include "FARSNfa2RevNfa.h"
#include "FARSNfa_wo_ro.h"
#include "FAAny2AnyOther_t.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);
FAAutIOTools g_aut_io (&g_alloc);

FANfaCreator_digit g_nfa_digit (&g_alloc);
FANfaCreator_char g_nfa_char (&g_alloc);
FAChain2Num_hash g_token2num;
FANfaCreator_wre g_nfa_wre (&g_alloc);

FANfaCreator_base * g_pNfaCrt = NULL;

int g_LabelType = FAFsmConst::LABEL_DIGIT;
const char * pInputEnc = NULL;
int g_spec_any = 0;
int g_spec_l_anchor = 1;
int g_spec_r_anchor = 2;
int g_TokenNumBase = 0;
bool g_TrBr2Iw = false;
int g_TrBrBaseIw = 0;

bool g_reverse = false;
bool g_no_output = false;
bool g_keep_pos = false;

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pOutPosNfaFile = NULL;
const char * pOutTrBrFile = NULL;
const char * pOutTokensFile = NULL;
const char * pOutTreeFile = NULL;
const char * pInTokensFile = NULL;

std::ifstream ifs;
std::ofstream ofs;
std::ofstream ofs_tokens;
std::ofstream ofs_pos_nfa;
std::ofstream ofs_trbr;
std::ofstream ofs_func;
std::ifstream ifs_tokens;

std::istream * g_pIs = &std::cin;
std::ostream * g_pOs = &std::cout;
std::ostream * g_pTokensOs = &std::cout;
std::ostream * g_pPosNfaOs = NULL;
std::ostream * g_pTrBrOs = NULL;
std::ostream * g_pTreeOs = NULL;
std::istream * g_pTokensIs = NULL;

unsigned int LineNum;
std::string line;



void usage () {

  std::cout << "\n\
Usage: fa_re2nfa [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
This program converts regular expression(s) into epsilon free NFA(s).\n\
Reads empty line separated multi-line regular expression(s) from stdin.\n\
Prints an empty-line separated list of non-deterministic automata to stdout.\n\
\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-pos-nfa=<output-file> - writes reversed position NFA, it is necessary\n\
    for regular expression position reconstruction, does not write by default\n\
\n\
  --out-trbr=<output-file> - writes functions necessary for triangular\n\
    brackets extraction, does not write by default\n\
\n\
  --out-tree=<output-file> - writes regexp tree and intermediate regexp\n\
    functions such as nullable, firstpos, lastpos and followpos\n\
\n\
  --label=<label-type> - the following types are available:\n\
\n\
    digit - assumes unsigned ints as a regular expression symbols,\n\
      the default value\n\
    char  - assumes chracters of 8-bit encoding or UTF-8 as input symbols\n\
    wre   - assumes WRE tokens as input symbols\n\
" ;

    std::cout << 
"\n\
  --out-tokens=<output-file> - prints out common Chain2Num map of tokens,\n\
    can be used with --label=wre only, if omited stdout is used\n\
\n\
  --in-tokens=<input-file> - reads in common Chain2Num map of tokens,\n\
    can be used with --label=wre only, if omited is not used\n\
\n\
  --tn-base=Iw - the starting Iw for token nums, makes sense with\n\
    --label=wre only, the default value is 0\n\
\n\
  --trbr2iw - converts triangular brackets into symbols\n\
\n\
  --trbr-base=Iw - base value for trbr Iws, used only with --trbr2iw,\n\
    the default value is 0\n\
\n\
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    can be used with --label=char only, if not specified then\n\
    byte representation of characters is used for automaton input weights\n\
\n\
  --spec-any=N - specifies weight for the special symbol '.',\n\
    the default is 0\n\
\n\
  --spec-l-anchor=N - specifies weight for the special symbol '^',\n\
    the default is 1\n\
\n\
  --spec-r-anchor=N - specifies weight for the special symbol '$',\n\
    the default is 2\n\
\n\
  --rev - processes regular expression in the right to left order\n\
\n\
  --keep-pos - uses original positions for NFA states, not their\n\
    equivalence classes\n\
\n\
  --no-output - does not do any output\n\
\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv){

    if (!strcmp ("--help", *argv)) {
      usage ();
      exit (0);
    }
    if (0 == strcmp ("--label=digit", *argv)) {
      g_LabelType = FAFsmConst::LABEL_DIGIT;
      continue;
    }
    if (0 == strcmp ("--label=char", *argv)) {
      g_LabelType = FAFsmConst::LABEL_CHAR;
      continue;
    }
    if (0 == strcmp ("--label=wre", *argv)) {
      g_LabelType = FAFsmConst::LABEL_WRE;
      continue;
    }
    if (0 == strncmp ("--spec-any=", *argv, 11)) {
      g_spec_any = atoi (&((*argv) [11]));
      continue;
    }
    if (0 == strncmp ("--spec-l-anchor=", *argv, 16)) {
      g_spec_l_anchor = atoi (&((*argv) [16]));
      continue;
    }
    if (0 == strncmp ("--spec-r-anchor=", *argv, 16)) {
      g_spec_r_anchor = atoi (&((*argv) [16]));
      continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
      pInFile = &((*argv) [5]);
      continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
      pOutFile = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--out-tokens=", *argv, 13)) {
      pOutTokensFile = &((*argv) [13]);
      continue;
    }
    if (0 == strncmp ("--in-tokens=", *argv, 12)) {
      pInTokensFile = &((*argv) [12]);
      continue;
    }
    if (0 == strncmp ("--out-trbr=", *argv, 11)) {
      pOutTrBrFile = &((*argv) [11]);
      continue;
    }
    if (0 == strncmp ("--out-tree=", *argv, 11)) {
      pOutTreeFile = &((*argv) [11]);
      continue;
    }
    if (0 == strncmp ("--out-pos-nfa=", *argv, 14)) {
      pOutPosNfaFile = &((*argv) [14]);
      continue;
    }
    if (0 == strncmp ("--tn-base=", *argv, 10)) {
      g_TokenNumBase = atoi (&((*argv) [10]));
      continue;
    }
    if (0 == strncmp ("--input-enc=", *argv, 12)) {
      pInputEnc = &((*argv) [12]);
      continue;
    }
    if (0 == strncmp ("--rev", *argv, 5)) {
      g_reverse = true;
      continue;
    }
    if (0 == strncmp ("--keep-pos", *argv, 10)) {
      g_keep_pos = true;
      continue;
    }
    if (0 == strcmp ("--trbr2iw", *argv)) {
      g_TrBr2Iw = true;
      continue;
    }
    if (0 == strncmp ("--trbr-base=", *argv, 12)) {
      g_TrBrBaseIw = atoi (&((*argv) [12]));
      continue;
    }
  }
}


void WritePosNfaStream (const FARSNfaA * pNfa)
{
    DebugLogAssert (g_pPosNfaOs && pNfa);

    // build reverse Nfa for reconstruction of positions
    FARSNfa_wo_ro rev_follow (&g_alloc);
    FARSNfa2RevNfa rev (&g_alloc);

    rev.SetInNfa (pNfa);
    rev.SetOutNfa (&rev_follow);
    rev.Process ();

    // expand Any symbol
    FARSNfa_wo_ro rev_follow_exp (&g_alloc);

    // state dependent expansion instead of global
    FAAny2AnyOther_t < FARSNfaA, FARSNfa_wo_ro > conv (&g_alloc);

    conv.SetAnyIw (g_spec_any);
    conv.SetInNfa (&rev_follow);
    conv.SetOutNfa (&rev_follow_exp);
    conv.Process ();

    g_aut_io.Print (*g_pPosNfaOs, &rev_follow_exp);
}


void WriteTrbrStream (const FARegexp2Nfa * pRe2Nfa)
{
    DebugLogAssert (g_pTrBrOs && pRe2Nfa);

    FAMultiMap_judy pos2br_start;
    FAMultiMap_judy pos2br_end;

    pos2br_start.SetAllocator (&g_alloc);
    pos2br_end.SetAllocator (&g_alloc);

    FAParsedRegexp2TrBrMaps re2trbrmaps (&g_alloc);

    const FARegexpTree2Funcs * pFuncs = pRe2Nfa->GetRegexpFuncs ();
    DebugLogAssert (pFuncs);

    const FARegexpTree * pTree = pRe2Nfa->GetRegexpTree ();
    DebugLogAssert (pTree);

    const FAMapA * pPos2Class = pRe2Nfa->GetPos2Class ();
    DebugLogAssert (pPos2Class);

    re2trbrmaps.SetRegexpFuncs (pFuncs);
    re2trbrmaps.SetRegexpTree (pTree);
    re2trbrmaps.SetPos2Class (pPos2Class);
    re2trbrmaps.SetStartMap (&pos2br_start);
    re2trbrmaps.SetEndMap (&pos2br_end);

    re2trbrmaps.Process ();

    g_map_io.Print (*g_pTrBrOs, &pos2br_start);
    g_map_io.Print (*g_pTrBrOs, &pos2br_end);
}


void ProcessInput ()
{
    DebugLogAssert (g_pIs);
    DebugLogAssert (g_pNfaCrt);

    FARegexpReverse re2rre (&g_alloc);
    FARegexp2Nfa regexp2nfa (&g_alloc);

    const bool UseUtf8 = ::FAIsUtf8Enc (pInputEnc);

    re2rre.SetLabelType (g_LabelType);
    re2rre.SetUseUtf8 (UseUtf8);

    regexp2nfa.SetLabelType (g_LabelType);
    regexp2nfa.SetUseUtf8 (UseUtf8);
    regexp2nfa.SetTrBr2Iw (g_TrBr2Iw);
    regexp2nfa.SetKeepPos (g_keep_pos);
    regexp2nfa.SetNfa (g_pNfaCrt);

    LineNum = 0;

    while (!(g_pIs->eof ())) {

        std::string Re;

        // read until the empty line is met
        if (!std::getline (*g_pIs, line))
            break;

        LineNum++;

        while (0 < line.length ()) {

            Re = Re + line + "\n";

            if (g_pIs->eof ())
                break;

            if (!std::getline (*g_pIs, line))
                break;
        }

        const char * pRe = Re.c_str ();
        int ReLength = (const int) Re.length ();

        if (0 < ReLength) {

            DebugLogAssert (pRe);

            if (g_reverse) {

                re2rre.SetRegexp (pRe, ReLength);
                re2rre.Process ();

                pRe = re2rre.GetRegexp ();
                DebugLogAssert (pRe);
                ReLength = (const int) strlen (pRe);
            }

            // return it into the initial state
            g_pNfaCrt->Clear ();

            g_pNfaCrt->SetRegexp (pRe, ReLength);
            regexp2nfa.SetRegexp (pRe, ReLength);
            regexp2nfa.Process ();

            // make it ready
            g_pNfaCrt->Prepare ();

            // print out Nfa, if asked
            if (g_pOs) {
                g_aut_io.Print (*g_pOs, g_pNfaCrt->GetNfa ());
            }
            // print reversed position Nfa, if asked
            if (g_pPosNfaOs) {
                WritePosNfaStream (g_pNfaCrt->GetNfa ());
            }
            // print trbr functions, if asked
            if (g_pTrBrOs) {
                WriteTrbrStream (&regexp2nfa);
            }
            // print regexp tree if asked
            if (g_pTreeOs) {

                (*g_pTreeOs) << "Tree:\n";
                ::FAPrintRegexpTree (*g_pTreeOs, regexp2nfa.GetRegexpTree (), pRe);
                (*g_pTreeOs) << "\nFuncs:\n";
                ::FAPrintRegexpFuncs (*g_pTreeOs, regexp2nfa.GetRegexpFuncs (), regexp2nfa.GetRegexpTree ());
                (*g_pTreeOs) << '\n';
            }

        } else {

            // break the outer loop, if RE is empty
            break;
        }
    } // of while
}


void AdjustIOPtrs ()
{
    if (NULL != pInFile) {
        ifs.open (pInFile, std::ios::in);
        FAAssertStream (&ifs, pInFile);
        g_pIs = &ifs;
    }
    if (NULL != pInTokensFile) {
        ifs_tokens.open (pInTokensFile, std::ios::in);
        FAAssertStream (&ifs_tokens, pInTokensFile);
        g_pTokensIs = &ifs_tokens;
    }

    if (false == g_no_output) {

        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            g_pOs = &ofs;
        }
        if (NULL != pOutTokensFile) {
            ofs_tokens.open (pOutTokensFile, std::ios::out);
            g_pTokensOs = &ofs_tokens;
        }
        if (NULL != pOutPosNfaFile) {
            ofs_pos_nfa.open (pOutPosNfaFile, std::ios::out);
            g_pPosNfaOs = &ofs_pos_nfa;
        }
        if (NULL != pOutTrBrFile) {
            ofs_trbr.open (pOutTrBrFile, std::ios::out);
            g_pTrBrOs = &ofs_trbr;
        }
        if (NULL != pOutTreeFile) {
            ofs_func.open (pOutTreeFile, std::ios::out);
            g_pTreeOs = &ofs_func;
        }

    } else {

        g_pOs = NULL;
        g_pTokensOs = NULL;
        g_pPosNfaOs = NULL;
        g_pTrBrOs = NULL;
        g_pTreeOs = NULL;
    }
}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

      // adjust input/output
      AdjustIOPtrs ();

      // select and setup NFA creator

      if (FAFsmConst::LABEL_DIGIT == g_LabelType) {

          g_pNfaCrt = &g_nfa_digit;

      } else if (FAFsmConst::LABEL_CHAR == g_LabelType) {

          g_nfa_char.SetEncodingName (pInputEnc);
          g_pNfaCrt = &g_nfa_char;

      } else if (FAFsmConst::LABEL_WRE == g_LabelType) {

          g_token2num.SetAllocator (&g_alloc);
          g_token2num.SetCopyChains (true);

          // load external tokens (--label=wre only), if specified
          if (g_pTokensIs) {
            g_map_io.Read (*g_pTokensIs, &g_token2num);
          }

          g_nfa_wre.SetToken2NumMap (&g_token2num);
          g_nfa_wre.SetBaseIw (g_TokenNumBase);

          g_pNfaCrt = &g_nfa_wre;
      }

      // setup common part
      g_pNfaCrt->SetAnyIw (g_spec_any);
      g_pNfaCrt->SetAnchorLIw (g_spec_l_anchor);
      g_pNfaCrt->SetAnchorRIw (g_spec_r_anchor);
      g_pNfaCrt->SetTrBr2Iw (g_TrBr2Iw);
      g_pNfaCrt->SetTrBrBaseIw (g_TrBrBaseIw);

      ProcessInput ();

      if (false == g_no_output && FAFsmConst::LABEL_WRE == g_LabelType) {
          g_map_io.Print (*g_pTokensOs, &g_token2num);
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
