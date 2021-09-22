/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAAllocator.h"
#include "FAMapIOTools.h"
#include "FAMultiMap_judy.h"
#include "FAConfParser.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";


FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);

FAMultiMap_judy g_out_map;
FAConfParser g_parser (&g_alloc);

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_build_conf [OPTIONS]\n\
\n\
This program converts ldb configuration file into interpretable multi-map,\n\
where keys are functionality ids and arrays of values represent initialization\n\
parameters.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
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
        if (0 == strncmp ("--in=", *argv, 5)) {
            g_pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
    }
}


void SetupConfParams ()
{
    // sections
    g_parser.AddSection ("w2t", FAFsmConst::FUNC_W2T);
    g_parser.AddSection ("w2b", FAFsmConst::FUNC_W2B);
    g_parser.AddSection ("b2w", FAFsmConst::FUNC_B2W);
    g_parser.AddSection ("w2w", FAFsmConst::FUNC_W2W);
    g_parser.AddSection ("trs", FAFsmConst::FUNC_TRS);
    g_parser.AddSection ("w2s", FAFsmConst::FUNC_W2S);
    g_parser.AddSection ("wre", FAFsmConst::FUNC_WRE);
    g_parser.AddSection ("wt2b", FAFsmConst::FUNC_WT2B);
    g_parser.AddSection ("b2wt", FAFsmConst::FUNC_B2WT);
    g_parser.AddSection ("tag-dict", FAFsmConst::FUNC_TAG_DICT);
    g_parser.AddSection ("w2h", FAFsmConst::FUNC_W2H);
    g_parser.AddSection ("pos-dict", FAFsmConst::FUNC_POS_DICT);
    g_parser.AddSection ("b2t", FAFsmConst::FUNC_B2T);
    g_parser.AddSection ("t2tb", FAFsmConst::FUNC_T2TB);
    g_parser.AddSection ("tb2t", FAFsmConst::FUNC_TB2T);
    g_parser.AddSection ("w2tp", FAFsmConst::FUNC_W2TP);
    g_parser.AddSection ("w2tpl", FAFsmConst::FUNC_W2TPL);
    g_parser.AddSection ("w2tpr", FAFsmConst::FUNC_W2TPR);
    g_parser.AddSection ("wbd", FAFsmConst::FUNC_WBD);
    g_parser.AddSection ("norm-dict", FAFsmConst::FUNC_NORM_DICT);
    g_parser.AddSection ("global", FAFsmConst::FUNC_GLOBAL);
    g_parser.AddSection ("w2h-alt", FAFsmConst::FUNC_W2H_ALT);
    g_parser.AddSection ("t2p", FAFsmConst::FUNC_T2P);
    g_parser.AddSection ("tt2p", FAFsmConst::FUNC_TT2P);
    g_parser.AddSection ("ttt2p", FAFsmConst::FUNC_TTT2P);
    g_parser.AddSection ("norm-rules", FAFsmConst::FUNC_NORM_RULES);
    g_parser.AddSection ("emit", FAFsmConst::FUNC_EMIT);
    g_parser.AddSection ("oic", FAFsmConst::FUNC_OIC_RULES);
    g_parser.AddSection ("css-rules", FAFsmConst::FUNC_CSS_RULES);
    g_parser.AddSection ("w2v", FAFsmConst::FUNC_W2V);
    g_parser.AddSection ("w2p", FAFsmConst::FUNC_W2P);
    g_parser.AddSection ("n2tp", FAFsmConst::FUNC_N2TP);
    g_parser.AddSection ("lad", FAFsmConst::FUNC_LAD);
    g_parser.AddSection ("u2l", FAFsmConst::FUNC_U2L);
    g_parser.AddSection ("i2w", FAFsmConst::FUNC_I2W);

    // parameters
    g_parser.AddNumParam ("trim", FAFsmConst::PARAM_TRIM);
    g_parser.AddStrParam ("dir", FAFsmConst::PARAM_DIRECTION,
                          "r2l", FAFsmConst::DIR_R2L);
    g_parser.AddStrParam ("dir", FAFsmConst::PARAM_DIRECTION,
                          "l2r", FAFsmConst::DIR_L2R);
    g_parser.AddStrParam ("dir", FAFsmConst::PARAM_DIRECTION,
                          "aff", FAFsmConst::DIR_AFF);
    g_parser.AddNumParam ("fsm", FAFsmConst::PARAM_FSM);
    g_parser.AddNumParam ("action-map", FAFsmConst::PARAM_ACTS);
    g_parser.AddStrParam ("action-map-mode", FAFsmConst::PARAM_MAP_MODE,
                          "triv-dump", FAFsmConst::MODE_PACK_TRIV);
    g_parser.AddStrParam ("action-map-mode", FAFsmConst::PARAM_MAP_MODE,
                          "mph-dump", FAFsmConst::MODE_PACK_MPH);
    g_parser.AddStrParam ("multi-map-mode", FAFsmConst::PARAM_MAP_MODE,
                          "triv-dump", FAFsmConst::MODE_PACK_TRIV);
    g_parser.AddStrParam ("multi-map-mode", FAFsmConst::PARAM_MAP_MODE,
                          "mph-dump", FAFsmConst::MODE_PACK_MPH);
    g_parser.AddStrParam ("multi-map-mode", FAFsmConst::PARAM_MAP_MODE,
                          "fixed-dump", FAFsmConst::MODE_PACK_FIXED);
    g_parser.AddNumParam ("min-len", FAFsmConst::PARAM_MIN_LEN);
    g_parser.AddNumParam ("min-comp-len", FAFsmConst::PARAM_MIN_LEN);
    g_parser.AddNumParam ("min-len2", FAFsmConst::PARAM_MIN_LEN2);
    g_parser.AddNumParam ("min-seg-len", FAFsmConst::PARAM_MIN_LEN2);
    g_parser.AddNumParam ("no-hyph-len", FAFsmConst::PARAM_MIN_LEN2);
    g_parser.AddNumParam ("min-len3", FAFsmConst::PARAM_MIN_LEN3);
    g_parser.AddNumParam ("min-ave-seg-len", FAFsmConst::PARAM_MIN_LEN3);
    g_parser.AddNumParam ("default-tag", FAFsmConst::PARAM_DEFAULT_TAG);
    g_parser.AddNumParam ("array", FAFsmConst::PARAM_ARRAY);
    g_parser.AddNumParam ("multi-map", FAFsmConst::PARAM_MULTI_MAP);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "rs-nfa", FAFsmConst::TYPE_RS_NFA);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "rs-dfa", FAFsmConst::TYPE_RS_DFA);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "moore-dfa", FAFsmConst::TYPE_MOORE_DFA);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "moore-mdfa", FAFsmConst::TYPE_MOORE_MULTI_DFA);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "mealy-nfa", FAFsmConst::TYPE_MEALY_NFA);
    g_parser.AddStrParam ("fsm-type", FAFsmConst::PARAM_FSM_TYPE,
                          "mealy-dfa", FAFsmConst::TYPE_MEALY_DFA);
    g_parser.AddNumParam ("left-anchor", FAFsmConst::PARAM_LEFT_ANCHOR);
    g_parser.AddNumParam ("right-anchor", FAFsmConst::PARAM_RIGHT_ANCHOR);
    g_parser.AddStrParam ("hyph-alg", FAFsmConst::PARAM_HYPH_TYPE,
                          "core", FAFsmConst::HYPH_TYPE_CORE);
    g_parser.AddStrParam ("hyph-alg", FAFsmConst::PARAM_HYPH_TYPE,
                          "w2h-w2s", FAFsmConst::HYPH_TYPE_W2H_W2S);
    g_parser.AddStrParam ("hyph-alg", FAFsmConst::PARAM_HYPH_TYPE,
                          "w2s-w2h", FAFsmConst::HYPH_TYPE_W2S_W2H);
    g_parser.AddParam ("normalize", FAFsmConst::PARAM_NORMALIZE);
    g_parser.AddParam ("no-tr", FAFsmConst::PARAM_NO_TR);
    g_parser.AddParam ("ignore-case", FAFsmConst::PARAM_IGNORE_CASE);
    g_parser.AddParam ("dict-mode", FAFsmConst::PARAM_DICT_MODE);
    g_parser.AddNumParam ("max-prob", FAFsmConst::PARAM_MAX_PROB);
    g_parser.AddNumParam ("depth", FAFsmConst::PARAM_DEPTH);
    g_parser.AddNumParam ("max-depth", FAFsmConst::PARAM_DEPTH);
    g_parser.AddNumParam ("max-pass-count", FAFsmConst::PARAM_MAX_PASS_COUNT);
    g_parser.AddNumParam ("max-score", FAFsmConst::PARAM_MAX_SCORE);
    g_parser.AddNumParam ("max-tag", FAFsmConst::PARAM_MAX_TAG);
    g_parser.AddParam ("log-scale", FAFsmConst::PARAM_LOG_SCALE);
    g_parser.AddNumParam ("float-array", FAFsmConst::PARAM_FLOAT_ARRAY);
    g_parser.AddNumParam ("min-max", FAFsmConst::PARAM_FLOAT_ARRAY);
    g_parser.AddParam ("use-nfst", FAFsmConst::PARAM_USE_NFST);
    g_parser.AddNumParam ("wre-conf", FAFsmConst::PARAM_WRE_CONF);
    g_parser.AddNumParam ("act-data", FAFsmConst::PARAM_ACT_DATA);
    g_parser.AddNumParam ("action-data", FAFsmConst::PARAM_ACT_DATA);
    g_parser.AddNumParam ("max-length", FAFsmConst::PARAM_MAX_LENGTH);
    g_parser.AddNumParam ("max-token-length", FAFsmConst::PARAM_MAX_LENGTH);
    g_parser.AddNumParam ("string-array", FAFsmConst::PARAM_STRING_ARRAY);
    g_parser.AddNumParam ("token-id-min", FAFsmConst::PARAM_TOKENID_MIN);
    g_parser.AddNumParam ("token-id-max", FAFsmConst::PARAM_TOKENID_MAX);

    // WRE-compiler related parameters (not used at runtime)
    g_parser.AddNumParam ("fsm-count", FAFsmConst::PARAM_FSM_COUNT);
    g_parser.AddStrParam ("token-type", FAFsmConst::PARAM_TOKEN_TYPE,
                          "txt", FAFsmConst::WRE_TT_TEXT);
    g_parser.AddStrParam ("token-type", FAFsmConst::PARAM_TOKEN_TYPE,
                          "tag", FAFsmConst::WRE_TT_TAGS);
    g_parser.AddStrParam ("token-type", FAFsmConst::PARAM_TOKEN_TYPE,
                          "dct", FAFsmConst::WRE_TT_DCTS);
    g_parser.AddNumParam ("type", FAFsmConst::PARAM_TYPE);
    g_parser.AddNumParam ("tag-ow-base", FAFsmConst::PARAM_TAG_OW_BASE);

    // transformations related parameters
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "hyph-redup", FAFsmConst::TR_HYPH_REDUP);
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "hyph-redup-rev", FAFsmConst::TR_HYPH_REDUP_REV);
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "pref", FAFsmConst::TR_PREFIX);
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "pref-rev", FAFsmConst::TR_PREFIX_REV);
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "ucf", FAFsmConst::TR_UCF);
    g_parser.AddStrParam ("in-tr", FAFsmConst::PARAM_IN_TR,
                          "ucf-rev", FAFsmConst::TR_UCF_REV);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "hyph-redup", FAFsmConst::TR_HYPH_REDUP);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "hyph-redup-rev", FAFsmConst::TR_HYPH_REDUP_REV);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "pref", FAFsmConst::TR_PREFIX);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "pref-rev", FAFsmConst::TR_PREFIX_REV);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "ucf", FAFsmConst::TR_UCF);
    g_parser.AddStrParam ("out-tr", FAFsmConst::PARAM_OUT_TR,
                          "ucf-rev", FAFsmConst::TR_UCF_REV);
    g_parser.AddNumParam ("redup-delim", FAFsmConst::PARAM_REDUP_DELIM);
    g_parser.AddNumParam ("pref-delim", FAFsmConst::PARAM_PREF_DELIM);
    g_parser.AddNumParam ("pref-fsm", FAFsmConst::PARAM_PREF_FSM);
    g_parser.AddNumParam ("suff-fsm", FAFsmConst::PARAM_SUFFIX_FSM);
    g_parser.AddNumParam ("ucf-delim", FAFsmConst::PARAM_UCF_DELIM);
    g_parser.AddNumParam ("charmap", FAFsmConst::PARAM_CHARMAP);
    g_parser.AddNumParam ("min-uni-prob", FAFsmConst::PARAM_MIN_UNI_PROB);
    g_parser.AddNumParam ("c2s-map", FAFsmConst::PARAM_C2S_MAP);
    g_parser.AddNumParam ("s2l-map", FAFsmConst::PARAM_S2L_MAP);
    g_parser.AddNumParam ("script-min-tag", FAFsmConst::PARAM_SCRIPT_MIN);
    g_parser.AddNumParam ("script-max-tag", FAFsmConst::PARAM_SCRIPT_MAX);

    // switches on W2B, e.g. reductive stemming for the word-breaker
    g_parser.AddParam ("do-w2b", FAFsmConst::PARAM_DO_W2B);
    g_parser.AddNumParam ("punkt", FAFsmConst::PARAM_PUNKT);
    g_parser.AddNumParam ("word", FAFsmConst::PARAM_WORD);
    g_parser.AddNumParam ("eos", FAFsmConst::PARAM_EOS);
    g_parser.AddNumParam ("eop", FAFsmConst::PARAM_EOP);
    g_parser.AddNumParam ("xword", FAFsmConst::PARAM_XWORD);
    g_parser.AddNumParam ("seg", FAFsmConst::PARAM_SEG);
    g_parser.AddNumParam ("ignore", FAFsmConst::PARAM_IGNORE);

    g_parser.AddNumParam ("order", FAFsmConst::PARAM_ORDER);
    g_parser.AddNumParam ("min-order", FAFsmConst::PARAM_MIN_ORDER);
    g_parser.AddNumParam ("unknown", FAFsmConst::PARAM_UNKNOWN);
    g_parser.AddNumParam ("max-count", FAFsmConst::PARAM_MAX_COUNT);
    g_parser.AddNumParam ("ratio", FAFsmConst::PARAM_RATIO);
    g_parser.AddNumParam ("ratio2", FAFsmConst::PARAM_RATIO2);
    g_parser.AddNumParam ("word-ratio", FAFsmConst::PARAM_RATIO2);
    g_parser.AddNumParam ("max-distance", FAFsmConst::PARAM_MAX_DISTANCE);
    g_parser.AddNumParam ("max-ambiguous-distance", FAFsmConst::PARAM_MAX_DISTANCE);
    g_parser.AddNumParam ("threshold", FAFsmConst::PARAM_THRESHOLD);
    g_parser.AddNumParam ("id-offset", FAFsmConst::PARAM_ID_OFFSET);
    g_parser.AddParam ("use-byte-encoding", FAFsmConst::PARAM_USE_BYTE_ENCODING);
    g_parser.AddParam ("no-dummy-prefix", FAFsmConst::PARAM_NO_DUMMY_PREFIX);

    // requires a CRC32-like check for the LDB file to pass
    g_parser.AddParam ("verify-ldb-bin", FAFsmConst::PARAM_VERIFY_LDB_BIN);

    // tokenization algo runtime
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "falex", FAFsmConst::TOKENIZE_DEFAULT);
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "wordpiece", FAFsmConst::TOKENIZE_WORDPIECE);
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "unilm", FAFsmConst::TOKENIZE_UNIGRAM_LM);
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "bpe", FAFsmConst::TOKENIZE_BPE);
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "bpe-opt", FAFsmConst::TOKENIZE_BPE_OPT);
    g_parser.AddStrParam ("tokalgo", FAFsmConst::PARAM_TOKENIZATION_TYPE,
                          "bpe-opt-with-merges", FAFsmConst::TOKENIZE_BPE_OPT_WITH_MERGES);

}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    g_out_map.SetAllocator (&g_alloc);

    // parse a command line
    process_args (argc, argv);

    try {

        // adjust input/output

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

        SetupConfParams ();

        g_parser.SetConfStream (pIs);
        g_parser.SetConfMap (&g_out_map);
        g_parser.Process ();

        g_map_io.Print (*pOs, &g_out_map);

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: \"" << pErrMsg << "\" in file " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    return 0;
}
