/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FARSDfa_ro.h"
#include "FARSDfa_renum_iws.h"
#include "FARSNfa_ro.h"
#include "FAState2Ow.h"
#include "FAState2Ows_ar_uniq.h"
#include "FAMealyDfa.h"
#include "FAMap_judy.h"
#include "FAMultiMap_ar.h"
#include "FAMultiMap_judy.h"
#include "FADfaPack_triv.h"
#include "FAPosNfaPack_triv.h"
#include "FAMultiMapPack.h"
#include "FAMultiMapPack_mph.h"
#include "FAMultiMapPack_fixed.h"
#include "FARSDfa_pack_triv.h"
#include "FAState2Ow_pack_triv.h"
#include "FAState2Ows_pack_triv.h"
#include "FAPosNfa_pack_triv.h"
#include "FAState2TrBr_pack_triv.h"
#include "FAMealyDfa_pack_triv.h"
#include "FAMultiMap_pack.h"
#include "FAMultiMap_pack_mph.h"
#include "FAMultiMap_pack_fixed.h"
#include "FAArrayPack.h"
#include "FAArray_pack.h"
#include "FAFloatArrayPack.h"
#include "FAStringArrayPack.h"
#include "FATestCmpDfa.h"
#include "FATestCmpPosNfa.h"
#include "FATestCmpMultiMap.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFsmFile = NULL;
const char * g_pInIwMapFile = NULL;
const char * g_pInTrBrFile = NULL;
const char * g_pOutFsmFile = NULL;

int g_type = FAFsmConst::TYPE_RS_DFA;
int g_alg = FAFsmConst::MODE_PACK_TRIV;
int g_dir = FAFsmConst::DIR_L2R;
int g_DstSize = 3;

bool g_remap_iws = false;
bool g_imp_mmap = false;
bool g_use_iwia = false;
bool g_use_ranges = false;
bool g_force_flat = false;
bool g_no_output = false;
bool g_no_process = false;
bool g_auto_test = false;

// string array value is a string in UTF-8, otherwise is a sequence of bytes
bool g_text_value = false;

FAAllocator g_alloc;
FAAutIOTools g_fsm_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

/// ordinary containers
FARSDfa_ro g_in_fsm_rs (&g_alloc);
FAState2Ow g_state2ow (&g_alloc);
FAState2Ows_ar_uniq g_state2ows (&g_alloc);
FARSNfa_ro g_in_nfa (&g_alloc);
FAMultiMap_ar g_pos2br_begin;
FAMultiMap_ar g_pos2br_end;
FAMultiMap_judy g_in_mmap;
FAMealyDfa g_in_sigma (&g_alloc);
FAMap_judy g_iw_map;
FARSDfa_renum_iws g_in_renum_fsm (&g_alloc);
FAArray_cont_t < unsigned char > g_strings;
FAArray_cont_t < int > g_offsets;


/// memory dump containers (for test)
FARSDfa_pack_triv g_rs_dfa_triv_dump;
FAState2Ow_pack_triv g_state2ow_triv_dump;
FAState2Ows_pack_triv g_state2ows_triv_dump;
FAPosNfa_pack_triv g_pos_nfa_dump;
FAState2TrBr_pack_triv g_state2trbr_begin_dump (FAState2TrBr_pack_triv::MapTypeTrBrBegin);
FAState2TrBr_pack_triv g_state2trbr_end_dump (FAState2TrBr_pack_triv::MapTypeTrBrEnd);
FAMultiMap_pack g_mmap_dump;
FAMultiMap_pack_mph g_mmap_mph_dump;
FAMultiMap_pack_fixed g_mmap_fixed_dump;
FAMealyDfa_pack_triv g_out_sigma_dump;
FAArray_pack g_array_dump;

/// packers
FAPosNfaPack_triv g_pos_nfa_pack (&g_alloc);
FADfaPack_triv g_dfa_pack (&g_alloc);
FAMultiMapPack g_mmap_pack (&g_alloc);
FAMultiMapPack_mph g_mmap_pack_mph (&g_alloc);
FAMultiMapPack_fixed g_mmap_pack_fixed (&g_alloc);
FAArrayPack g_array_pack (&g_alloc);
FAFloatArrayPack g_farray_pack (&g_alloc);
FAStringArrayPack g_sarray_pack (&g_alloc);

/// interface pointers
FARSDfaA * g_pInDfa = & g_in_fsm_rs;
FAState2OwA * g_pState2Ow = NULL;
FAState2OwsA * g_pState2Ows = NULL;
FARSNfaA * g_pInNfa = & g_in_nfa;
FAMultiMapA * g_pPos2BrBeg = NULL;
FAMultiMapA * g_pPos2BrEnd = NULL;
FAMultiMapA * g_pMMap = NULL;
FAMealyDfaA * g_pSigma = NULL;
const int * g_Array = NULL;
int g_ArraySize = 0;
const float * g_FloatArray = NULL;
int g_FloatArraySize = 0;


void usage () {

  std::cout << "\n\
Usage: fa_fsm2fsm_pack [OPTIONS] [< fsm.txt]\n\
\n\
This program builds packed contiguous automaton image.\n\
\n\
  --in=<file-name> - reads specified structure from <input-aut> file,\n\
    if omited stdin is used\n\
\n\
  --out=<file-name> - writes specified structure dump into file,\n\
    if omited stdout is used\n\
\n\
  --alg=<algo> - specifies memory dump representation algorithm:\n\
    triv - trivial algorithm for packing will be used, used by default\n\
    mph - MPH-based algorithm will be used, valid only with --type=mmap\n\
      Note: In order to use --alg=mph mmap arrays of values must be \n\
          lexicographically ordered, to be sure use --auto-test.\n\
    fixed - allocates fixed amount of memory for each array associated with\n\
      each key of the multimap, this is the fastest mmap representation\n\
\n\
  --type=<type> - specifies input structure type:\n\
    rs-dfa     - Rabin-Scott DFA, the default value\n\
    moore-dfa  - Moore DFA\n\
    moore-mdfa - Moore DFA with multiple outputs\n\
    pos-nfa    - Position NFA, can be packed with triangular bracket maps\n\
    mmap       - Multi-Map\n\
    mealy-dfa  - Mealy DFA\n\
    arr        - Array of integers\n\
    farr       - Array of floats\n\
    sarr       - Array of strings or bytes\n\
\n\
  --trbr-maps=<input-file> - reads triangular bracket extraction maps,\n\
    can be used to pack position NFA, does not read them by default\n\
" ;

  std::cout << "\n\
  --remap-iws - remaps input weights into a contiguous array, such that more\n\
    frequent input weights have smaller values, can be used with --alg=triv\n\
    Note: This option cannot be used for Mealy and MHP automata representation\n\
\n\
  --iw-map=<input> - New -> Old Iw external map, not used by default\n\
\n\
  --imp-mmap - uses implicit representation for mmap, if possible\n\
\n\
  --use-iwia - uses Iw-indexed arrays for state representation, used only when\n\
    gives a smaller representation and always for the initial state\n\
    Note: This option cannot be used for Mealy and MHP automata representation\n\
\n\
  --use-ranges - uses ranges of Iws storing just one destination state per\n\
    each range, used only when gives smaller representation\n\
    Note: This option cannot be used for Mealy and MHP automata representation\n\
\n\
  --dir=<direction> - specifies direction of reading, currently used only\n\
    with --type=mmap and --alg=mph, the possible values are:\n\
    l2r - left to right (the dafault value)\n\
    r2l - right to left\n\
\n\
  --force-flat - forces the array to be stored in a flat format\n\
\n\
  --dst-size=N - overrides the deafult Dst size for DFA automata,\n\
    the default value is 3 (only 1, 2, 3, 4 are possible)\n\
\n\
  --text-value - forces string array values to be text in UTF-8 encoding\n\
    otherwise it is a sequence space delimited numbers base 10\n\
\n\
  --auto-test - compares two interfaces to be equivalent (original one and\n\
    its memory dump counterpart) before saving the output\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
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
    if (0 == strcmp ("--no-output", *argv)) {
        g_no_output = true;
        continue;
    }
    if (0 == strcmp ("--no-process", *argv)) {
        g_no_process= true;
        continue;
    }
    if (0 == strcmp ("--remap-iws", *argv)) {
        g_remap_iws = true;
        continue;
    }
    if (0 == strcmp ("--use-iwia", *argv)) {
        g_use_iwia = true;
        continue;
    }
    if (0 == strcmp ("--force-flat", *argv)) {
        g_force_flat = true;
        continue;
    }
    if (0 == strcmp ("--use-ranges", *argv)) {
        g_use_ranges = true;
        continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
        g_pInFsmFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        g_pOutFsmFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--trbr-maps=", *argv, 12)) {
        g_pInTrBrFile = &((*argv) [12]);
        continue;
    }
    if (0 == strncmp ("--iw-map=", *argv, 9)) {
        g_pInIwMapFile = &((*argv) [9]);
        continue;
    }
    if (0 == strcmp ("--alg=triv", *argv)) {
        g_alg = FAFsmConst::MODE_PACK_TRIV;
        continue;
    }
    if (0 == strcmp ("--alg=mph", *argv)) {
        g_alg = FAFsmConst::MODE_PACK_MPH;
        continue;
    }
    if (0 == strcmp ("--alg=fixed", *argv)) {
        g_alg = FAFsmConst::MODE_PACK_FIXED;
        continue;
    }
    if (0 == strcmp ("--type=rs-dfa", *argv)) {
        g_type = FAFsmConst::TYPE_RS_DFA;
        continue;
    }
    if (0 == strcmp ("--type=pos-nfa", *argv)) {
        g_type = FAFsmConst::TYPE_POS_RS_NFA;
        continue;
    }
    if (0 == strcmp ("--type=moore-dfa", *argv)) {
        g_type = FAFsmConst::TYPE_MOORE_DFA;
        continue;
    }
    if (0 == strcmp ("--type=moore-mdfa", *argv)) {
        g_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
        continue;
    }
    if (0 == strcmp ("--type=mealy-dfa", *argv)) {
        g_type = FAFsmConst::TYPE_MEALY_DFA;
        continue;
    }
    if (0 == strcmp ("--type=mmap", *argv)) {
        g_type = FAFsmConst::TYPE_MULTI_MAP;
        continue;
    }
    if (0 == strcmp ("--type=arr", *argv)) {
        g_type = FAFsmConst::TYPE_ARRAY;
        continue;
    }
    if (0 == strcmp ("--type=farr", *argv)) {
        g_type = FAFsmConst::TYPE_FLOAT_ARRAY;
        continue;
    }
    if (0 == strcmp ("--type=sarr", *argv)) {
        g_type = FAFsmConst::TYPE_STRING_ARRAY;
        continue;
    }
    if (0 == strcmp ("--imp-mmap", *argv)) {
        g_imp_mmap = true;
        continue;
    }
    if (0 == strcmp ("--dir=l2r", *argv)) {
      g_dir = FAFsmConst::DIR_L2R;
      continue;
    }
    if (0 == strcmp ("--dir=r2l", *argv)) {
      g_dir = FAFsmConst::DIR_R2L;
      continue;
    }
    if (0 == strncmp ("--dst-size=", *argv, 11)) {
        g_DstSize = atoi (&((*argv) [11]));
        continue;
    }
    if (0 == strncmp ("--text-value", *argv, 12)) {
        g_text_value = true;
        continue;
    }
    if (0 == strcmp ("--auto-test", *argv)) {
        g_auto_test = true;
        continue;
    }
  }
}


void Load ()
{
    std::istream * pIs = &std::cin;
    std::ifstream ifs;

    // load automaton
    if (g_pInFsmFile) {
        ifs.open (g_pInFsmFile, std::ios::in);
        FAAssertStream (&ifs, g_pInFsmFile);
        pIs = &ifs;
    }
    if (FAFsmConst::TYPE_RS_DFA == g_type) {

        g_fsm_io.Read (*pIs, g_pInDfa);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_type) {

        g_pState2Ow = & g_state2ow;
        g_fsm_io.Read (*pIs, g_pInDfa, g_pState2Ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_type) {

        g_pState2Ows = & g_state2ows;
        g_fsm_io.Read (*pIs, g_pInDfa, g_pState2Ows);

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_type) {

        g_in_sigma.SetRsDfa (g_pInDfa);
        g_pSigma = & g_in_sigma;

        g_fsm_io.Read (*pIs, g_pInDfa, g_pSigma);

    } else if (FAFsmConst::TYPE_MULTI_MAP == g_type) {

        g_pMMap = & g_in_mmap;
        g_map_io.Read (*pIs, g_pMMap);

    } else if (FAFsmConst::TYPE_ARRAY == g_type) {

        g_map_io.Read (*pIs, &g_Array, &g_ArraySize);

    } else if (FAFsmConst::TYPE_FLOAT_ARRAY == g_type) {

        g_map_io.Read (*pIs, &g_FloatArray, &g_FloatArraySize);

    } else if (FAFsmConst::TYPE_STRING_ARRAY == g_type) {

        g_map_io.Read (*pIs, &g_strings, &g_offsets, g_text_value);

    } else {

        DebugLogAssert (FAFsmConst::TYPE_POS_RS_NFA == g_type);
        g_fsm_io.Read (*pIs, g_pInNfa);
    }

    // load triangular brackets
    if (NULL != g_pInTrBrFile) {

        std::ifstream trbr_ifs (g_pInTrBrFile, std::ios::in);

        g_pPos2BrBeg = & g_pos2br_begin;
        g_pPos2BrEnd = & g_pos2br_end;

        g_map_io.Read (trbr_ifs, g_pPos2BrBeg);
        g_map_io.Read (trbr_ifs, g_pPos2BrEnd);
    }

    // load Iw map and readjust the input DFA pointer
    if (NULL != g_pInIwMapFile) {

        std::ifstream iws_ifs (g_pInIwMapFile, std::ios::in);

        g_map_io.Read (iws_ifs, &g_iw_map);

        g_in_renum_fsm.SetNew2Old (&g_iw_map);
        g_in_renum_fsm.SetOldDfa (&g_in_fsm_rs);
        g_in_renum_fsm.Prepare ();

        g_pInDfa = & g_in_renum_fsm;
    }
}


const bool AutoTest (const unsigned char * pDump, const int DumpSize)
{
    FAAssert (0 < DumpSize && pDump, FAMsg::InvalidParameters);

    /// Dfa test
    if (FAFsmConst::TYPE_RS_DFA == g_type ||
        FAFsmConst::TYPE_MOORE_DFA == g_type ||
        FAFsmConst::TYPE_MOORE_MULTI_DFA == g_type ||
        FAFsmConst::TYPE_MEALY_DFA == g_type) {

        FATestCmpDfa cmp_dfa (&g_alloc);

        if (FAFsmConst::TYPE_RS_DFA == g_type) {

            g_rs_dfa_triv_dump.SetImage (pDump);

            cmp_dfa.SetFsm1 (g_pInDfa, NULL, NULL, NULL);
            cmp_dfa.SetFsm2 (&g_rs_dfa_triv_dump, NULL, NULL, NULL);

        } else if (FAFsmConst::TYPE_MOORE_DFA == g_type) {

            g_rs_dfa_triv_dump.SetImage (pDump);
            g_state2ow_triv_dump.SetImage (pDump);

            cmp_dfa.SetFsm1 (g_pInDfa, g_pState2Ow, NULL, NULL);
            cmp_dfa.SetFsm2 (&g_rs_dfa_triv_dump, &g_state2ow_triv_dump, NULL, NULL);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_type) {

            g_rs_dfa_triv_dump.SetImage (pDump);
            g_state2ows_triv_dump.SetImage (pDump);

            cmp_dfa.SetFsm1 (g_pInDfa, NULL, g_pState2Ows, NULL);
            cmp_dfa.SetFsm2 (&g_rs_dfa_triv_dump, NULL, &g_state2ows_triv_dump, NULL);

        } else if (FAFsmConst::TYPE_MEALY_DFA == g_type) {

            g_rs_dfa_triv_dump.SetImage (pDump);
            g_out_sigma_dump.SetImage (pDump);

            cmp_dfa.SetFsm1 (g_pInDfa, NULL, NULL, g_pSigma);
            cmp_dfa.SetFsm2 (&g_rs_dfa_triv_dump, NULL, NULL, &g_out_sigma_dump);
        }

        const int MaxState = g_pInDfa->GetMaxState ();
        const int MaxIw = g_pInDfa->GetMaxIw ();

        const bool Res = cmp_dfa.Process (MaxState, MaxIw);
        return Res;

    // position Nfa test
    } else if (FAFsmConst::TYPE_POS_RS_NFA == g_type) {

        FATestCmpPosNfa cmp_pos_nfa (&g_alloc);

        g_pos_nfa_dump.SetImage (pDump);

        if (g_pPos2BrBeg && g_pPos2BrEnd) {
            g_state2trbr_begin_dump.SetImage (pDump);
            g_state2trbr_end_dump.SetImage (pDump);
        }

        cmp_pos_nfa.SetFsm1 (g_pInNfa, g_pPos2BrBeg, g_pPos2BrEnd);
        cmp_pos_nfa.SetFsm2 (&g_pos_nfa_dump, &g_state2trbr_begin_dump, &g_state2trbr_end_dump);

        const int MaxState = g_pInNfa->GetMaxState ();
        const int MaxIw = g_pInNfa->GetMaxIw ();

        const bool Res = cmp_pos_nfa.Process (MaxState, MaxIw);
        return Res;

    } else if (FAFsmConst::TYPE_MULTI_MAP == g_type) {

        // get max key
        const int * pValues;
        int Key = -1;
        int MaxKey = -1;
        int Size = g_pMMap->Prev (&Key, &pValues);

        while (-1 != Size) {

            if (MaxKey < Key)
                MaxKey = Key;

            Size = g_pMMap->Prev (&Key, &pValues);
        }

        FATestCmpMultiMap cmp_mmaps (&g_alloc);

        cmp_mmaps.SetMap1 (g_pMMap);

        if (FAFsmConst::MODE_PACK_TRIV == g_alg) {

            g_mmap_dump.SetImage (pDump);
            cmp_mmaps.SetMap2 (&g_mmap_dump);

        } else if (FAFsmConst::MODE_PACK_MPH == g_alg) {

            g_mmap_mph_dump.SetImage (pDump);
            cmp_mmaps.SetMap2 (&g_mmap_mph_dump);

        } else if (FAFsmConst::MODE_PACK_FIXED == g_alg) {

            g_mmap_fixed_dump.SetImage (pDump);
            cmp_mmaps.SetMap2 (&g_mmap_fixed_dump);
        }

        const bool Res = cmp_mmaps.Process (MaxKey);
        return Res;

    } else if (FAFsmConst::TYPE_ARRAY == g_type) {

        DebugLogAssert (g_Array && 0 < g_ArraySize);

        g_array_dump.SetImage (pDump);

        if (g_array_dump.GetCount () != g_ArraySize) {
            return false;
        }
        for (int i = 0; i < g_ArraySize; ++i) {
            if (g_array_dump.GetAt (i) != g_Array [i]) {
                return false;
            }
        }

    } else if (FAFsmConst::TYPE_FLOAT_ARRAY == g_type) {

        DebugLogAssert (g_FloatArray && 0 < g_FloatArraySize);

        const int Count = *((const int *) pDump);
        const float * pArr = (const float *)(pDump + sizeof (int));

        if (Count != g_FloatArraySize) {
            return false;
        }
        for (int i = 0; i < g_FloatArraySize; ++i) {
            if (pArr [i] != g_FloatArray [i]) {
                return false;
            }
        }
    }

    return true;
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    g_pos2br_begin.SetAllocator (&g_alloc);
    g_pos2br_end.SetAllocator (&g_alloc);
    g_in_mmap.SetAllocator (&g_alloc);
    g_strings.SetAllocator (&g_alloc);
    g_offsets.SetAllocator (&g_alloc);

    // parse a command line
    process_args (argc, argv);

    try {

        // load automaton
        Load ();

        // process
        if (false == g_no_process) {

            const unsigned char * pDump = NULL;
            int DumpSize = 0;

            // build dump
            if (FAFsmConst::MODE_PACK_TRIV == g_alg) {

                if (FAFsmConst::TYPE_POS_RS_NFA == g_type) {

                    g_pos_nfa_pack.SetNfa (g_pInNfa);
                    g_pos_nfa_pack.SetPos2BrBegin (g_pPos2BrBeg);
                    g_pos_nfa_pack.SetPos2BrEnd (g_pPos2BrEnd);
                    g_pos_nfa_pack.Process ();

                    DumpSize = g_pos_nfa_pack.GetDump (&pDump);

                } else if (FAFsmConst::TYPE_MULTI_MAP == g_type) {

                    if (false == g_imp_mmap) {
                        g_mmap_pack.SetSizeOfValue (sizeof (int));
                    }
                    g_mmap_pack.SetMultiMap (g_pMMap);
                    g_mmap_pack.Process ();

                    DumpSize = g_mmap_pack.GetDump (&pDump);

                } else if (FAFsmConst::TYPE_ARRAY == g_type) {

                    DebugLogAssert (g_Array && 0 < g_ArraySize);

                    g_array_pack.SetForceFlat (g_force_flat);
                    g_array_pack.SetArray (g_Array, g_ArraySize);
                    g_array_pack.Process ();

                    DumpSize = g_array_pack.GetDump (&pDump);

                } else if (FAFsmConst::TYPE_FLOAT_ARRAY == g_type) {

                    DebugLogAssert (g_FloatArray && 0 < g_FloatArraySize);

                    g_farray_pack.SetArray (g_FloatArray, g_FloatArraySize);
                    g_farray_pack.Process ();

                    DumpSize = g_farray_pack.GetDump (&pDump);

                } else if (FAFsmConst::TYPE_STRING_ARRAY == g_type) {

                    g_sarray_pack.SetArray (&g_strings, &g_offsets);
                    g_sarray_pack.Process ();

                    DumpSize = g_sarray_pack.GetDump (&pDump);

                } else {

                    g_dfa_pack.SetDfa (g_pInDfa);
                    g_dfa_pack.SetState2Ow (g_pState2Ow);
                    g_dfa_pack.SetState2Ows (g_pState2Ows);
                    g_dfa_pack.SetSigma (g_pSigma);
                    g_dfa_pack.SetRemapIws (g_remap_iws);
                    g_dfa_pack.SetUseIwIA (g_use_iwia);
                    g_dfa_pack.SetUseRanges (g_use_ranges);
                    g_dfa_pack.SetDstSize (g_DstSize);
                    g_dfa_pack.Process ();

                    DumpSize = g_dfa_pack.GetDump (&pDump);
                }

            } else if (FAFsmConst::MODE_PACK_MPH == g_alg) {

                if (FAFsmConst::TYPE_MULTI_MAP == g_type) {

                    g_mmap_pack_mph.SetMultiMap (g_pMMap);
                    g_mmap_pack_mph.SetDirection (g_dir);
                    g_mmap_pack_mph.Process ();

                    DumpSize = g_mmap_pack_mph.GetDump (&pDump);

                } else {

                    std::cerr << "ERROR: Unsupported container type is specified for packing"
                              << " in program " << __PROG__ << '\n';
                    exit (1);
                }

            } else if (FAFsmConst::MODE_PACK_FIXED == g_alg) {

                if (FAFsmConst::TYPE_MULTI_MAP == g_type) {

                    if (false == g_imp_mmap) {
                        g_mmap_pack_fixed.SetSizeOfValue (sizeof (int));
                    }
                    g_mmap_pack_fixed.SetMultiMap (g_pMMap);
                    g_mmap_pack_fixed.Process ();

                    DumpSize = g_mmap_pack_fixed.GetDump (&pDump);

                } else {

                    std::cerr << "ERROR: Unsupported container type is specified for packing"
                              << " in program " << __PROG__ << '\n';
                    exit (1);
                }

            } else {

                std::cerr << "ERROR: Unsupported algorithm type is specified for packing"
                          << " in program " << __PROG__ << '\n';
                exit (1);
            }

            // make auto-test, if needed
            if (g_auto_test) {
                if (false == AutoTest (pDump, DumpSize)) {
                    std::cerr << "ERROR: Dump representation and original interface have different behaviour"
                              << " in program " << __PROG__ << '\n';
                    exit (1);
                }
            }

            // save dump, if needed
            if (false == g_no_output) {

                std::ostream * pFsmOs = &std::cout;
                std::ofstream fsm_ofs;

                if (NULL != g_pOutFsmFile) {
                    // output is always binary
                    fsm_ofs.open (g_pOutFsmFile, std::ios::out | std::ios::binary);
                    pFsmOs = &fsm_ofs;
                }

                pFsmOs->write ((const char *)pDump, DumpSize);
            }
        }

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
