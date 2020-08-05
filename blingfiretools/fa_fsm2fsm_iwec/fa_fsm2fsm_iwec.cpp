/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAMap_judy.h"
#include "FARSDfa_ro.h"
#include "FARSNfa_ro.h"
#include "FARSDfa_wo_ro.h"
#include "FARSNfa_wo_ro.h"
#include "FAState2Ow.h"
#include "FAState2Ows.h"
#include "FAMealyDfa.h"
#include "FAMealyNfa.h"
#include "FACalcIwEqClasses.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

/// global objects

const char * pInFile = NULL;
const char * pOutMapFile = NULL;
const char * pOutMMapFile = NULL;
const char * pOutFsmFile = NULL;

bool g_no_output = false;
int g_fsm_type = FAFsmConst::TYPE_RS_DFA;

int g_iw_base = -1;
int g_iw_max = -1;
int g_new_iw_base = -1;


void usage () {

  std::cout << "\n\
Usage: fa_fsm2iwec [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program calculates IW equivalence classes of the input automaton\n\
and prints out Old -> New, New -> Old maps or the remapped automaton.\n\
\n\
  --in=<input-file> - reads input automaton from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes remapped automaton to the output file,\n\
    if omited stdout is used\n\
\n\
  --out-map=<output-file> - writes Iw -> EqClass map,\n\
    no output if omited\n\
\n\
  --out-mmap=<output-file> - writes EqClass --> { Iw } multi-map,\n\
    no output if omited\n\
\n\
  --fsm-type=<type> - specifies input automaton type:\n\
    rs-nfa      - Rabin-Scott NFA\n\
    rs-dfa      - Rabin-Scott DFA, the default value\n\
    moore-dfa   - Moore DFA\n\
    moore-mdfa  - Moore DFA with multiple output\n\
    mealy-nfa   - Mealy NFA\n\
    mealy-dfa   - Mealy DFA,\n\
\n\
  --iw-base=Iw - process transitions with weights >= Iw only,\n\
    weights outside this range are mapped into themselves,\n\
    default value is 0\n\
\n\
  --iw-max=Iw - process transitions with weights <= Iw only,\n\
    weights outside this range are mapped into themselves,\n\
    default value is MaxIw\n\
\n\
  --new-iw-base=Base - the Base value will be added to all the newly generated\n\
    input weights, the default value is 0\n\
\n\
  --no-output - does not do any output\n\
\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (0 == strcmp ("--help", *argv)) {
      usage ();
      exit (0);
    }
    if (0 == strcmp ("--no-output", *argv)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
      pInFile = &((*argv) [5]);
      continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
      pOutFsmFile = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--out-map=", *argv, 10)) {
      pOutMapFile = &((*argv) [10]);
      continue;
    }
    if (0 == strncmp ("--out-mmap=", *argv, 11)) {
      pOutMMapFile = &((*argv) [11]);
      continue;
    }
    if (0 == strncmp ("--iw-base=", *argv, 10)) {
      g_iw_base = strtol (&((*argv) [10]), NULL, 0);
      continue;
    }
    if (0 == strncmp ("--iw-max=", *argv, 9)) {
      g_iw_max = strtol (&((*argv) [9]), NULL, 0);
      continue;
    }
    if (0 == strncmp ("--new-iw-base=", *argv, 14)) {
      g_new_iw_base = strtol (&((*argv) [14]), NULL, 0);
      continue;
    }
    if (0 == strcmp ("--fsm-type=rs-nfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_RS_NFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=rs-dfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_RS_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=moore-dfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MOORE_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=moore-mdfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=mealy-nfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MEALY_NFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=mealy-dfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MEALY_DFA;
        continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    FAAllocator g_alloc;
    FAAutIOTools g_aut_io (&g_alloc);
    FAMapIOTools g_map_io (&g_alloc);

    FARSDfa_ro g_dfa (&g_alloc);
    FARSNfa_ro g_nfa (&g_alloc);
    FAMealyDfa g_dfa_sigma (&g_alloc);
    FAMealyNfa g_nfa_sigma (&g_alloc);
    FACalcIwEqClasses g_IwClassify (&g_alloc);
    FAMap_judy g_output_map;
    FAMultiMap_judy g_output_mmap;
    FARSDfa_wo_ro g_new_dfa (&g_alloc);
    FARSNfa_wo_ro g_new_nfa (&g_alloc);
    FAState2Ow g_state2ow (&g_alloc);
    FAState2Ows g_state2ows (&g_alloc);
    FAMealyDfa g_new_dfa_sigma (&g_alloc);
    FAMealyNfa g_new_nfa_sigma (&g_alloc);

    g_output_mmap.SetAllocator (&g_alloc);

    // parse a command line
    process_args (argc, argv);

    try {

        // select in/out streams
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }

        if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

            g_aut_io.Read (*pIs, &g_dfa);
            g_IwClassify.SetRsDfa (&g_dfa);

            if (-1 == g_iw_max)
                g_iw_max = g_dfa.GetMaxIw ();

        } else if (FAFsmConst::TYPE_RS_NFA == g_fsm_type) {

            g_aut_io.Read (*pIs, &g_nfa);
            g_IwClassify.SetRsNfa (&g_nfa);

            if (-1 == g_iw_max)
                g_iw_max = g_nfa.GetMaxIw ();

        } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

            g_aut_io.Read (*pIs, &g_dfa, &g_state2ow);
            g_IwClassify.SetRsDfa (&g_dfa);

            if (-1 == g_iw_max)
                g_iw_max = g_dfa.GetMaxIw ();

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

            g_aut_io.Read (*pIs, &g_dfa, &g_state2ows);
            g_IwClassify.SetRsDfa (&g_dfa);

            if (-1 == g_iw_max)
                g_iw_max = g_dfa.GetMaxIw ();

        } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm_type) {

            g_aut_io.Read (*pIs, &g_dfa, &g_dfa_sigma);
            g_IwClassify.SetRsDfa (&g_dfa);
            g_IwClassify.SetDfaSigma (&g_dfa_sigma);

            if (-1 == g_iw_max)
                g_iw_max = g_dfa.GetMaxIw ();

        } else {
            DebugLogAssert (FAFsmConst::TYPE_MEALY_NFA == g_fsm_type);

            g_aut_io.Read (*pIs, &g_nfa, &g_nfa_sigma);
            g_IwClassify.SetRsNfa (&g_nfa);
            g_IwClassify.SetNfaSigma (&g_nfa_sigma);

            if (-1 == g_iw_max)
                g_iw_max = g_nfa.GetMaxIw ();
        }

        // assign default values

        if (-1 == g_iw_base)
            g_iw_base = 0;

        if (-1 == g_new_iw_base)
            g_new_iw_base = 0;

        g_IwClassify.SetIwBase (g_iw_base);
        g_IwClassify.SetIwMax (g_iw_max);
        g_IwClassify.SetNewIwBase (g_new_iw_base);
        g_IwClassify.SetIw2NewIw (&g_output_map);
        g_IwClassify.Process ();

        if (false == g_no_output) {

            // print out OldIw --> NewIw map, if needed
            if (NULL != pOutMapFile) {
                std::ofstream ofs (pOutMapFile, std::ios::out);
                g_map_io.Print (ofs, &g_output_map);
            }
            // print out NewIw --> OldIw map, if needed
            if (NULL != pOutMMapFile) {

                ::FAReverseMap (&g_output_mmap, &g_output_map);
                g_output_mmap.SortUniq ();

                std::ofstream ofs (pOutMMapFile, std::ios::out);
                g_map_io.Print (ofs, &g_output_mmap);
            }

            std::ostream * pOs = &std::cout;
            std::ofstream ofs;

            if (NULL != pOutFsmFile) {
                ofs.open (pOutFsmFile, std::ios::out);
                pOs = &ofs;
            }

            // print out the resulting automaton, if needed
            if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

                ::FARemapRsFsmIws (&g_dfa, &g_new_dfa, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_dfa);

            } else if (FAFsmConst::TYPE_RS_NFA == g_fsm_type) {

                ::FARemapRsFsmIws (&g_nfa, &g_new_nfa, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_nfa);

            } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

                ::FARemapRsFsmIws (&g_dfa, &g_new_dfa, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_dfa, &g_state2ow);

            } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

                ::FARemapRsFsmIws (&g_dfa, &g_new_dfa, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_dfa, &g_state2ows);

            } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm_type) {

                ::FARemapRsFsmIws (&g_dfa, &g_new_dfa, &g_output_map);
                ::FARemapMealySigma2 (&g_dfa, &g_dfa_sigma, \
                    &g_new_dfa_sigma, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_dfa, &g_new_dfa_sigma);

            } else {
                DebugLogAssert (FAFsmConst::TYPE_MEALY_NFA == g_fsm_type);

                ::FARemapRsFsmIws (&g_nfa, &g_new_nfa, &g_output_map);
                ::FARemapMealySigma2 (&g_nfa, &g_nfa_sigma, \
                    &g_new_nfa_sigma, &g_output_map);
                g_aut_io.Print (*pOs, &g_new_nfa, &g_new_nfa_sigma);
            }
        } // of if (false == g_no_output) ...

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

