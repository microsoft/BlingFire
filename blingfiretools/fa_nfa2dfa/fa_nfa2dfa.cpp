/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FARSNfa_ro.h"
#include "FARSDfa_wo_ro.h"
#include "FARSDfa_ro.h"
#include "FAState2Ows.h"
#include "FANfa2Dfa_t.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAState2Ows.h"
#include "FAState2Ows_amb.h"
#include "FAMealyNfa2Dfa.h"
#include "FAMealyNfa.h"
#include "FAMealyDfa.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);

bool g_no_output = false;

int AutType = FAFsmConst::TYPE_RS_NFA;

const char * pInFile = NULL;
const char * pInRevPosNfaFile = NULL;
const char * pOutFile = NULL;
const char * pOutFile2 = NULL;

bool g_use_any = false;
int g_spec_any = -1;
bool g_bi_machine = false;
bool g_verbose = false;

FARSNfa_ro g_nfa (&g_alloc);
FAMealyNfa g_sigma (&g_alloc);
FARSNfa_ro g_rev_pos_nfa (&g_alloc);

FARSNfaA * g_pInNfa = NULL;
FARSNfaA * g_pInRevPosNfa = NULL;

// I/O
std::istream * g_pIs = &std::cin;
std::ifstream ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream ofs;

std::ostream * g_pOs2 = &std::cout;
std::ofstream ofs2;

std::istream * g_pRevPosNfaIs = NULL;
std::ifstream rev_pos_nfa_ifs;


void usage () {

  std::cout << "\n\
Usage: fa_nfa2dfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program converts non-deterministic finite-state machine into \n\
deterministic one.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out2=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --pos-nfa=<input-file> - reads reversed position NFA from <input-file>,\n\
    needed for --fsm=pos-rs-nfa to store only ambiguous positions, if omited\n\
    stores all positions\n\
\n\
  --fsm=rs-nfa - makes convertion from Rabin-Scott NFA (is used by default)\n\
  --fsm=pos-rs-nfa - makes convertion from Rabin-Scott position NFA,\n\
    builds Moore Multi Dfa\n\
  --fsm=mealy-nfa - makes convertion from Mealy NFA into a cascade of\n\
    two Mealy Dfa (general case) or a single Mealy DFA (trivial case)\n\
\n\
  --spec-any=N - treats input weight N as a special any symbol,\n\
    if specified produces Dfa with the same symbol on arcs,\n\
    which must be interpreted as any other\n\
\n\
  --bi-machine - uses bi-machine for Mealy NFA determinization\n\
\n\
  --no-output - does not do any output\n\
\n\
  --verbose - prints out debug information, if supported\n\
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
      pInFile = &((*argv) [5]);
      continue;
    }
    if (0 == strncmp ("--pos-nfa=", *argv, 10)) {
      pInRevPosNfaFile = &((*argv) [10]);
      continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
      pOutFile = &((*argv) [6]);
      continue;
    }
    if (0 == strncmp ("--out2=", *argv, 7)) {
      pOutFile2 = &((*argv) [7]);
      continue;
    }
    if (0 == strcmp ("--fsm=rs-nfa", *argv)) {
      AutType = FAFsmConst::TYPE_RS_NFA;
      continue;
    }
    if (0 == strcmp ("--fsm=pos-rs-nfa", *argv)) {
      AutType = FAFsmConst::TYPE_POS_RS_NFA;
      continue;
    }
    if (0 == strcmp ("--fsm=mealy-nfa", *argv)) {
      AutType = FAFsmConst::TYPE_MEALY_NFA;
      continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
      g_no_output = true;
      continue;
    }
    if (0 == strncmp ("--spec-any=", *argv, 11)) {
      g_use_any = true;
      g_spec_any = atoi (&((*argv) [11]));
      continue;
    }
    if (0 == strcmp ("--verbose", *argv)) {
      g_verbose = true;
      continue;
    }
    if (0 == strcmp ("--bi-machine", *argv)) {
      g_bi_machine = true;
      continue;
    }
  }
}


void ProcessRSNfa (const FARSNfaA * pNfa,
                   FARSDfaA * pDfa,
                   FAAllocatorA * pAlloc)
{
    DebugLogAssert (pNfa);
    DebugLogAssert (pDfa);
    DebugLogAssert (pAlloc);

    // create RSNfa2Dfa converter
    FANfa2Dfa_t < FARSNfaA, FARSDfaA > nfa2dfa (pAlloc);

    // setup nfa, and dfa and get converter ready
    nfa2dfa.SetNFA (pNfa);
    nfa2dfa.SetDFA (pDfa);

    if (true == g_use_any) {
        nfa2dfa.SetAnyIw (g_spec_any);
    }

    // make the convertion
    nfa2dfa.Process ();
}


void ProcessPosRSNfa (const FARSNfaA * pNfa, 
                      FARSDfaA * pDfa,
                      FAState2OwsA * pState2Ows,
                      FAAllocatorA * pAlloc)
{
    DebugLogAssert (pNfa);
    DebugLogAssert (pDfa);
    DebugLogAssert (pState2Ows);

    FANfa2Dfa_t < FARSNfaA, FARSDfaA > nfa2dfa (pAlloc);

    nfa2dfa.SetNew2Old (pState2Ows);
    nfa2dfa.SetNFA (pNfa);
    nfa2dfa.SetDFA (pDfa);

    if (true == g_use_any) {
        nfa2dfa.SetAnyIw (g_spec_any);
    }

    nfa2dfa.Process ();
}


void SetUpPtrs ()
{
    g_pInNfa = &g_nfa;
    g_pInRevPosNfa = &g_rev_pos_nfa;
}


void Load ()
{
    if (NULL != pInFile) {
        ifs.open (pInFile, std::ios::in);
        FAAssertStream (&ifs, pInFile);
        g_pIs = &ifs;
    }
    if (NULL != pOutFile) {
        ofs.open (pOutFile, std::ios::out);
        g_pOs = &ofs;
    }
    if (NULL != pOutFile2) {
        ofs2.open (pOutFile2, std::ios::out);
        g_pOs2 = &ofs2;
    }
    if (NULL != pInRevPosNfaFile) {
        rev_pos_nfa_ifs.open (pInRevPosNfaFile, std::ios::in);
        FAAssertStream (&rev_pos_nfa_ifs, pInRevPosNfaFile);
        g_pRevPosNfaIs = &rev_pos_nfa_ifs;
    }

    DebugLogAssert (g_pIs);
    DebugLogAssert (g_pOs);

    if (FAFsmConst::TYPE_RS_NFA == AutType || \
        FAFsmConst::TYPE_POS_RS_NFA == AutType) {

        g_io.Read (*g_pIs, g_pInNfa);

    } else if (FAFsmConst::TYPE_MEALY_NFA == AutType) {

        g_io.Read (*g_pIs, g_pInNfa, &g_sigma);
    }

    if (g_pRevPosNfaIs) {
        g_io.Read (*g_pRevPosNfaIs, g_pInRevPosNfa);
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

        // adjust pointers
        SetUpPtrs ();

        // load input
        Load ();

        // make processing
        if (FAFsmConst::TYPE_RS_NFA == AutType) {

            // output dfa
            FARSDfa_wo_ro dfa (&g_alloc);

            ProcessRSNfa (g_pInNfa, &dfa, &g_alloc);

            if (false == g_no_output) {
                g_io.Print (*g_pOs, &dfa);
            }

        } else if (FAFsmConst::TYPE_POS_RS_NFA == AutType) {

            // output dfa
            FARSDfa_wo_ro dfa (&g_alloc);

            // output map: state -> { pos }
            FAState2Ows_amb state2pos (&g_alloc);

            // see whether rev pos nfa was provided
            if (pInRevPosNfaFile) {
                state2pos.SetRevPosNfa (g_pInRevPosNfa);
            }
            // make mapping ready
            state2pos.Prepare ();

            ProcessPosRSNfa (g_pInNfa, &dfa, &state2pos, &g_alloc);

            // print out Moore Multi Dfa
            if (false == g_no_output) {
                g_io.Print (*g_pOs, &dfa, &state2pos);
            }

        } else if (FAFsmConst::TYPE_MEALY_NFA == AutType) {

            FAMealyNfa2Dfa nfa2dfa (&g_alloc);

            FARSDfa_ro fsm1_dfa (&g_alloc);
            FAMealyDfa fsm1_ows (&g_alloc);
            FARSDfa_ro fsm2_dfa (&g_alloc);
            FAMealyDfa fsm2_ows (&g_alloc);

            nfa2dfa.SetUseBiMachine (g_bi_machine);
            nfa2dfa.SetInNfa (g_pInNfa, &g_sigma);
            nfa2dfa.SetOutFsm1 (&fsm1_dfa, &fsm1_ows);
            nfa2dfa.SetOutFsm2 (&fsm2_dfa, &fsm2_ows);
            nfa2dfa.Process ();

            if (false == g_no_output) {

                // always return Fsm1
                g_io.Print (*g_pOs, &fsm1_dfa, &fsm1_ows);

                // return Fsm2, if it is not empty
                if (-1 != fsm2_dfa.GetMaxState ()) {
                    g_io.Print (*g_pOs2, &fsm2_dfa, &fsm2_ows);
                }

                FAAssert (!nfa2dfa.IsNonDet (), FAMsg::InternalError);
            }

        } else {

            std::cerr << "ERROR: \"Unsupported automaton type\""
                      << " in program " << __PROG__ << '\n';
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
