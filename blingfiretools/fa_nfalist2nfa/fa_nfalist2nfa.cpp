/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAutIOTools.h"
#include "FAEpsilonRemoval.h"
#include "FANfas2CommonENfa.h"
#include "FADfas2CommonNfa.h"
#include "FANfas2TupleNfa.h"
#include "FAArray_cont_t.h"
#include "FARSNfa_ar_judy.h"
#include "FARSNfa_ro.h"
#include "FARSDfa_ro.h"
#include "FAException.h"
#include "FAFsmConst.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);

enum {
  ALG_UNION = 0,
  ALG_TUPLE = 1,
  ALG_DFA_UNION = 2,
};

int g_alg = ALG_UNION;
int g_epsilon_iw = 0;
bool g_add_nfa_nums = false;
bool g_keep_epsilon = false;
int g_nfa_num_base = 0;
bool g_no_output = false;
int g_ignore_base = -1;
int g_ignore_max = -1;

FARSNfa_ar_judy g_out_nfa (&g_alloc);
const FARSNfaA * g_pOutNfa = &g_out_nfa;

FANfas2CommonENfa g_common (&g_alloc);
FADfas2CommonNfa g_merge_dfa (&g_alloc);

const char * pInFile = NULL;
const char * pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_nfalist2nfa [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program builds common Nfa from the list of 1 or more NFA.\n\
Reads/writes Nfa in text mode.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --alg=<type> - specifies how Nfa(s) will be combined,\n\
    union - L(Nfa1) | L(Nfa2) ..., is used by default\n\
    dfa-union - L(Dfa1) | L(Dfa2) ... ,\n\
    tuple - builds tuple NFA, input NFA(s) must be parallel each other\n\
\n\
  --epsilon=Iw - specifies wich Iw is the Epsilon\n\
    0 is used by default, (used with --combine=union)\n\
\n\
  --keep-epsilon - does not do epsilon removal,\n\
    removes by default, (used with --combine=union)\n\
\n\
  --nfa-num-base=Iw - maps final states into Nfa num from the input list\n\
    if not specified does not add this information, (used with --combine=union)\n\
\n\
  --ignore-base=Iw - specifies base Iw for ignore-range for --alg=tuple,\n\
    does not ignore no Iws by default\n\
\n\
  --ignore-max=Iw - specifies max Iw for ignore-range for --alg=tuple,\n\
    does not ignore no Iws by default\n\
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
    if (0 == strncmp ("--no-output", *argv, 11)) {
        g_no_output = true;
        continue;
    }
    if (0 == strncmp ("--epsilon=", *argv, 10)) {
        g_epsilon_iw = atoi (&((*argv) [10]));
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
    if (0 == strncmp ("--alg=union", *argv, 11)) {
        g_alg = ALG_UNION;
        continue;
    }
    if (0 == strncmp ("--alg=dfa-union", *argv, 15)) {
        g_alg = ALG_DFA_UNION;
        continue;
    }
    if (0 == strncmp ("--alg=tuple", *argv, 11)) {
        g_alg = ALG_TUPLE;
        continue;
    }
    if (0 == strncmp ("--nfa-num-base=", *argv, 15)) {
        g_add_nfa_nums = true;
        g_nfa_num_base = atoi (&((*argv) [15]));
        continue;
    }
    if (0 == strcmp ("--keep-epsilon", *argv)) {
        g_keep_epsilon = true;
        continue;
    }
    if (0 == strncmp ("--ignore-max=", *argv, 13)) {
        g_ignore_max = atoi (&((*argv) [13]));
        continue;
    }
    if (0 == strncmp ("--ignore-base=", *argv, 14)) {
        g_ignore_base = atoi (&((*argv) [14]));
        continue;
    }
  }
}


void CalcUnion (std::istream * pIs)
{
    DebugLogAssert (pIs);

    if (g_add_nfa_nums) {
        g_common.SetAddNfaNums (true);
        g_common.SetNfaNumBase (g_nfa_num_base);
    }

    g_common.SetEpsilonIw (g_epsilon_iw);

    while (!pIs->eof ()) {

        // read in input nfa
        g_io.Read (*pIs, &g_out_nfa);

        /// see whether automaon is not empty
        const int MaxState = g_out_nfa.GetMaxState ();

        if (-1 != MaxState) {

            /// add nfa
            g_common.AddNfa (&g_out_nfa);
            // return nfa into initial state
            g_out_nfa.Clear ();
        }
    }

    g_common.Process ();

    // get read interface to the common nfa
    const FARSNfaA * pCommonNfa = g_common.GetCommonNfa ();
    DebugLogAssert (pCommonNfa);

    // remove epsilon transitions
    if (false == g_keep_epsilon) {

        FAEpsilonRemoval e_removal (&g_alloc);

        e_removal.SetInNfa (pCommonNfa);
        e_removal.SetOutNfa (&g_out_nfa);
        e_removal.SetEpsilonIw (g_epsilon_iw);
        e_removal.Process ();

    } else {

        g_pOutNfa = pCommonNfa;
    }
}


void CalcTuple (std::istream * pIs)
{
    DebugLogAssert (pIs);

    FANfas2TupleNfa nfalist2tuple (&g_alloc);

    nfalist2tuple.SetIgnoreBase (g_ignore_base);
    nfalist2tuple.SetIgnoreMax (g_ignore_max);

    FAArray_cont_t < FARSNfa_ro * > nfa_list;
    nfa_list.SetAllocator (&g_alloc);
    nfa_list.Create ();

    int TupleSize = 0;

    // read nfa list
    while (!pIs->eof ()) {

        FARSNfa_ro * pNfa = NEW FARSNfa_ro (&g_alloc);
        DebugLogAssert (pNfa);

        nfa_list.push_back (pNfa);
        g_io.Read (*pIs, pNfa);

        if (-1 != pNfa->GetMaxState ()) {
            nfalist2tuple.AddNfa (pNfa);
            TupleSize++;
        } else {
            break;
        }

    } // of while (!pIs->eof ()) ...

    // see whether list contains more than one element
    if (1 < TupleSize) {
        nfalist2tuple.SetOutNfa (&g_out_nfa);
        nfalist2tuple.Process ();
    } else {
        DebugLogAssert (1 == TupleSize);
        DebugLogAssert (0 < nfa_list.size ());
        ::FACopyNfa (&g_out_nfa, nfa_list [0]);
    }

    const int Count = nfa_list.size ();

    for (int i = 0; i < Count; ++i) {

        FARSNfa_ro * pNfa = nfa_list [i];
        DebugLogAssert (pNfa);
        delete pNfa;
    }
}


void CalcDfaUnion (std::istream * pIs)
{
    DebugLogAssert (pIs);

    FARSDfa_ro tmp_dfa (&g_alloc);

    while (!pIs->eof ()) {

        // read in input dfa
        g_io.Read (*pIs, &tmp_dfa);

        /// see whether automaon is not empty
        const int MaxState = tmp_dfa.GetMaxState ();

        if (-1 != MaxState) {
            g_merge_dfa.AddDfa (&tmp_dfa);
            tmp_dfa.Clear ();
        }
    }

    g_merge_dfa.Process ();

    // get the common NFA pointer
    g_pOutNfa = g_merge_dfa.GetCommonNfa ();
    DebugLogAssert (g_pOutNfa);
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        if (ALG_UNION == g_alg) {
            CalcUnion (pIs);
        } else if (ALG_DFA_UNION == g_alg) {
            CalcDfaUnion (pIs);
        } else {
            DebugLogAssert (ALG_TUPLE == g_alg);
            CalcTuple (pIs);
        }

        if (false == g_no_output) {
            DebugLogAssert (g_pOutNfa);
            g_io.Print (*pOs, g_pOutNfa);
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
