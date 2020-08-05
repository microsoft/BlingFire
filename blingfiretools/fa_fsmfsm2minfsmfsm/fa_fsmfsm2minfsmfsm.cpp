/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMap_judy.h"
#include "FARSDfa_ro.h"
#include "FARSDfa_wo_ro.h"
#include "FACalcIwEqClasses.h"
#include "FAState2Ow.h"
#include "FAState2Ows.h"
#include "FAMealyDfa.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_aut_io (&g_alloc);

int g_fsm1_type = FAFsmConst::TYPE_MOORE_DFA;
int g_fsm2_type = FAFsmConst::TYPE_RS_DFA;

const char * g_pIn1File = NULL;
const char * g_pOut1File = NULL;

const char * g_pIn2File = NULL;
const char * g_pOut2File = NULL;

bool g_no_output = false;

int g_iw_base = -1;
int g_iw_max = -1;
int g_new_iw_base = -1;

/// automata global objects and pointers to interfaces

FARSDfa_ro fsm1_dfa (&g_alloc);
FAState2Ow fsm1_state2ow (&g_alloc);
FAState2Ows fsm1_state2ows (&g_alloc);
FAMealyDfa fsm1_sigma (&g_alloc);

FARSDfa_ro fsm2_dfa (&g_alloc);
FAState2Ow fsm2_state2ow (&g_alloc);
FAState2Ows fsm2_state2ows (&g_alloc);
FAMealyDfa fsm2_sigma (&g_alloc);

FAMap_judy old2new;
FACalcIwEqClasses g_iw_classify (&g_alloc);

FARSDfa_wo_ro new_dfa (&g_alloc);
FAState2Ow new_state2ow (&g_alloc);
FAState2Ows new_state2ows (&g_alloc);
FAMealyDfa new_sigma (&g_alloc);
FAMealyDfa new_sigma2 (&g_alloc);


FARSDfaA * g_pIn1Dfa = & fsm1_dfa;
FAState2OwA * g_pIn1State2Ow = & fsm1_state2ow;
FAState2OwsA * g_pIn1State2Ows = & fsm1_state2ows;
FAMealyDfaA * g_pIn1Sigma = & fsm1_sigma;

FARSDfaA * g_pIn2Dfa = & fsm2_dfa;
FAState2OwA * g_pIn2State2Ow = & fsm2_state2ow;
FAState2OwsA * g_pIn2State2Ows = & fsm2_state2ows;
FAMealyDfaA * g_pIn2Sigma = & fsm2_sigma;

FAMapA * g_pOld2New = &old2new;

FARSDfaA * g_pNewDfa = & new_dfa;
FAState2OwA * g_pNewState2Ow = &new_state2ow;
FAState2OwsA * g_pNewState2Ows = &new_state2ows;
FAMealyDfaA * g_pNewSigma = &new_sigma;
FAMealyDfaA * g_pNewSigma2 = &new_sigma2;


void usage () {

  std::cout << "\n\
Usage: fa_fsmfsm2minfsmfsm [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program reads a pair of automata which work in cascade, then calculates\n\
equivalence classes over input weights of the second automaton and makes two\n\
substitutions. Substitutes output weights of the first automaton and \n\
substitutes input weights of the second automaton.\n\
\n\
  --in1=<input-file> - reads first automaton from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --in2=<input-file>  - reads second automaton from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out1=<output-file> - writes first automaton to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out2=<output-file> - writes second automaton to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --fsm1-type=<type> - specifies the first automaton automaton type\n\
    the following types are available:\n\
      moore-dfa  - Moore DFA, the default value\n\
      moore-mdfa - Moore DFA with multiple output\n\
      mealy-dfa  - Mealy DFA\n\
\n\
  --fsm2-type=<type> - specifies the second automaton automaton type\n\
    the following types are available:\n\
      rs-dfa     - Rabin-Scott DFA, the default value\n\
      moore-dfa  - Moore DFA\n\
      moore-mdfa - Moore DFA with multiple output\n\
      mealy-dfa  - Mealy DFA\n\
\n\
  --iw-base=Iw - process transitions with weights >= Iw only,\n\
    weights outside this range remain unchanged,\n\
    default value is 0\n\
\n\
  --iw-max=Iw - process transitions with weights <= Iw only,\n\
    weights outside this range remain unchanged,\n\
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
    if (0 == strncmp ("--in1=", *argv, 6)) {
      g_pIn1File = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--in2=", *argv, 6)) {
      g_pIn2File = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--out1=", *argv, 7)) {
      g_pOut1File = &((*argv) [7]);
      continue;
    }
    if (0 == strncmp ("--out2=", *argv, 7)) {
      g_pOut2File = &((*argv) [7]);
      continue;
    }
    if (0 == strcmp ("--fsm1-type=moore-dfa", *argv)) {
        g_fsm1_type = FAFsmConst::TYPE_MOORE_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm1-type=moore-mdfa", *argv)) {
        g_fsm1_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm1-type=mealy-dfa", *argv)) {
        g_fsm1_type = FAFsmConst::TYPE_MEALY_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm2-type=rs-dfa", *argv)) {
        g_fsm2_type = FAFsmConst::TYPE_RS_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm2-type=moore-dfa", *argv)) {
        g_fsm2_type = FAFsmConst::TYPE_MOORE_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm2-type=moore-mdfa", *argv)) {
        g_fsm2_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm2-type=mealy-dfa", *argv)) {
        g_fsm2_type = FAFsmConst::TYPE_MEALY_DFA;
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
  }
}


void load (std::istream * pIs1, std::istream * pIs2)
{
    DebugLogAssert (pIs1 && pIs2);

    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm1_type) {

        g_aut_io.Read (*pIs1, g_pIn1Dfa, g_pIn1State2Ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm1_type) {

        g_aut_io.Read (*pIs1, g_pIn1Dfa, g_pIn1State2Ows);

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm1_type) {

        g_aut_io.Read (*pIs1, g_pIn1Dfa, g_pIn1Sigma);

    } else {

        std::cerr << "ERROR: \"Unsupported format of the first automaton\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }

    if (FAFsmConst::TYPE_RS_DFA == g_fsm2_type) {

        g_aut_io.Read (*pIs2, g_pIn2Dfa);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm2_type) {

        g_aut_io.Read (*pIs2, g_pIn2Dfa, g_pIn2State2Ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm2_type) {

        g_aut_io.Read (*pIs2, g_pIn2Dfa, g_pIn2State2Ows);

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm2_type) {

        g_aut_io.Read (*pIs2, g_pIn2Dfa, g_pIn2Sigma);

    } else {

        std::cerr << "ERROR: \"Unsupported format of the second automaton\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }
}


void save (std::ostream * pOs1, std::ostream * pOs2)
{
    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm1_type) {

        g_aut_io.Print (*pOs1, g_pIn1Dfa, g_pNewState2Ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm1_type) {

        g_aut_io.Print (*pOs1, g_pIn1Dfa, g_pNewState2Ows);

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm1_type) {

        g_aut_io.Print (*pOs1, g_pIn1Dfa, g_pNewSigma);
    }

    if (FAFsmConst::TYPE_RS_DFA == g_fsm2_type) {

        g_aut_io.Print (*pOs2, g_pNewDfa);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm2_type) {

        g_aut_io.Print (*pOs2, g_pNewDfa, g_pIn2State2Ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm2_type) {

        g_aut_io.Print (*pOs2, g_pNewDfa, g_pIn2State2Ows);

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm2_type) {

        g_aut_io.Print (*pOs2, g_pNewDfa, g_pNewSigma2);
    }
}


void build_old2new ()
{
    DebugLogAssert (g_pIn2Dfa);

    if (-1 == g_iw_base)
        g_iw_base = 0;

    if (-1 == g_iw_max)
        g_iw_max = g_pIn2Dfa->GetMaxIw ();

    if (-1 == g_new_iw_base)
        g_new_iw_base = 0;

    g_iw_classify.SetRsDfa (g_pIn2Dfa);

    if (FAFsmConst::TYPE_MEALY_DFA == g_fsm2_type) {
        g_iw_classify.SetDfaSigma (g_pIn2Sigma);
    }

    g_iw_classify.SetIwBase (g_iw_base);
    g_iw_classify.SetIwMax (g_iw_max);
    g_iw_classify.SetNewIwBase (g_new_iw_base);
    g_iw_classify.SetIw2NewIw (g_pOld2New);

    g_iw_classify.Process ();
}


void build_newdfa ()
{
    ::FARemapRsFsmIws (g_pIn2Dfa, g_pNewDfa, g_pOld2New);
}


void build_new_state2ow_map ()
{
    DebugLogAssert (g_pOld2New && g_pNewState2Ow && g_pIn1Dfa && g_pIn1State2Ow);

    // build a unique Ow value
    const int MaxNewOw = 1 + g_iw_classify.GetMaxNewIw ();

    const int MaxState = g_pIn1Dfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int Ow = g_pIn1State2Ow->GetOw (State);

        if (-1 != Ow) {

            const int * pNewOw = g_pOld2New->Get (Ow);

            // This may happen if some weights are not used in second fsm.
            // In this case we should map it a unique Ow value.
            if (pNewOw) {

                g_pNewState2Ow->SetOw (State, *pNewOw);

            } else {

                g_pNewState2Ow->SetOw (State, MaxNewOw);
            }

        } // of if (-1 != Ow) ...
    } // of for (int State = 0; ...
}


void build_new_state2ows_map ()
{
    DebugLogAssert (g_pOld2New && g_pNewState2Ows && g_pIn1Dfa && g_pIn1State2Ows);

    FAArray_cont_t < int > tmp_array;
    tmp_array.SetAllocator (&g_alloc);
    tmp_array.Create ();;

    // build a unique Ow value
    const int MaxNewOw = 1 + g_iw_classify.GetMaxNewIw ();

    const int MaxState = g_pIn1Dfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int * pOws;
        const int OwCount = g_pIn1State2Ows->GetOws (State, &pOws);

        if (0 < OwCount) {

            tmp_array.resize (OwCount);

            for (int i = 0; i < OwCount; ++i) {

                DebugLogAssert (pOws);
                const int Ow = pOws [i];

                const int * pNewOw = g_pOld2New->Get (Ow);

                // This may happen if some weights are not used in second fsm.
                // In this case we should map it a unique Ow value.
                if (pNewOw) {

                    tmp_array [i] = *pNewOw;

                } else {

                    tmp_array [i] = MaxNewOw;
                }
            }

            const int NewSize = ::FASortUniq (tmp_array.begin (), tmp_array.end ());
            tmp_array.resize (NewSize);

            g_pNewState2Ows->SetOws (State, tmp_array.begin (), tmp_array.size ());

        } // of if (0 < OwCount) ...
    } // of for (int State = 0; ...
}


void build_new_ow_map ()
{
    // fsm1 map update
    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm1_type) {

        build_new_state2ow_map ();

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm1_type) {

        build_new_state2ows_map ();

    } else if (FAFsmConst::TYPE_MEALY_DFA == g_fsm1_type) {

        ::FARemapMealySigma1 (g_pIn1Dfa, g_pIn1Sigma, g_pNewSigma, g_pOld2New);

    } else {

        DebugLogAssert (0);
    }

    // fsm2 map update
    if (FAFsmConst::TYPE_MEALY_DFA == g_fsm2_type) {

        ::FARemapMealySigma2 (g_pIn2Dfa, g_pIn2Sigma, g_pNewSigma2, g_pOld2New);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        // select in/out streams
        std::istream * pIs1 = &std::cin;
        std::ifstream ifs1;

        std::istream * pIs2 = &std::cin;
        std::ifstream ifs2;

        std::ostream * pOs1 = &std::cout;
        std::ofstream ofs1;

        std::ostream * pOs2 = &std::cout;
        std::ofstream ofs2;

        if (NULL != g_pIn1File) {
            ifs1.open (g_pIn1File, std::ios::in);
            FAAssertStream (&ifs1, g_pIn1File);
            pIs1 = &ifs1;
        }
        if (NULL != g_pIn2File) {
            ifs2.open (g_pIn2File, std::ios::in);
            FAAssertStream (&ifs2, g_pIn2File);
            pIs2 = &ifs2;
        }
        if (NULL != g_pOut1File) {
            ofs1.open (g_pOut1File, std::ios::out);
            pOs1 = &ofs1;
        }
        if (NULL != g_pOut2File) {
            ofs2.open (g_pOut2File, std::ios::out);
            pOs2 = &ofs2;
        }

        DebugLogAssert (pIs1);
        DebugLogAssert (pIs2);
        DebugLogAssert (pOs1);
        DebugLogAssert (pOs2);

        // load automata
        load (pIs1, pIs2);

        // build old input weigts to new input weights mapping
        build_old2new ();

        // build new dfa
        build_newdfa ();

        // build new ow map
        build_new_ow_map ();

        // save automata
        save (pOs1, pOs2);

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
