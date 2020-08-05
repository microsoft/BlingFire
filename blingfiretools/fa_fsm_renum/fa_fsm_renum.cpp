/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAUtils.h"
#include "FAFsmConst.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAState2OwsA.h"
#include "FARSDfa_ro.h"
#include "FAMap_judy.h"
#include "FAMultiMap_ar.h"
#include "FAArray_cont_t.h"
#include "FARSDfa_renum.h"
#include "FAState2Ow.h"
#include "FAState2Ows_ar_uniq.h"
#include "FAFsmRenum.h"
#include "FARSDfaRenum_depth_first.h"
#include "FARSDfaRenum_remove_gaps.h"
#include "FASortMultiMap.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

enum {
    ALG_REPAIR = 0,
    ALG_DEPTH_FIRST,
    ALG_REMOVE_GAPS,
    ALG_MMAP_SORT,
    ALG_CXPS_CSPS,
};

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pInActFile = NULL;
const char * g_pOutActFile = NULL;

std::istream * g_pIs = &std::cin;
std::fstream g_ifs;
std::ostream * g_pOs = &std::cout;
std::fstream g_ofs;

FAAllocator g_alloc;
FAAutIOTools g_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

FARSDfa_ro g_in_rs_fsm (&g_alloc);
FAState2Ow g_in_state2ow (&g_alloc);
FAState2Ows_ar_uniq g_in_state2ows (&g_alloc);
FAMultiMap_ar g_in_mmap;
FAArray_cont_t < int > g_in_arr;

FARSDfa_renum g_out_rs_fsm (&g_alloc);
FAState2Ow g_out_state2ow (&g_alloc);
FAState2Ows_ar_uniq g_out_state2ows (&g_alloc);
FAMultiMap_ar g_out_mmap;

bool g_no_output = false;
int g_fsm_type = FAFsmConst::TYPE_RS_NFA;
int g_renum_type = ALG_REPAIR;
int g_dir = FAFsmConst::DIR_L2R;
int g_max_prob = 255;



void usage ()
{
    std::cerr << "\n\
Usage: fa_fsm_renum [OPTIONS] [< fsm.txt] [> renumed-fsm.txt]\n\
\n\
Renumerates states of the input automaton by the one of selected algorithms.\n\
\n\
  --in=<input-file> - reads automaton from the input file,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes automaton to the output file,\n\
    if omited stdout is used\n\
\n\
  --alg=<alg> - selects renumeration algorithm:\n\
    repair - makes textual representation loadable\n\
      1. Final states always greater than non-final states\n\
      2. Transitions are sorted in low to big order\n\
      3. Calculates new MaxState and MaxIw (MaxOw when appropriate)\n\
    depth-first - renumerates states in depth first manner and makes\n\
      states to be contiguous, works with valid automata representations only\n\
    remove-gaps - makes states to be contiguous, works with valid automata\n\
      representations only\n\
    mmap-sort - for the given multi-map makes smaller keys refering to smaller\n\
      in lexicographic sense arrays of values, updates referee structure\n\
      (moore-dfa, moore-mdfa or array) output weights.\n\
    cxps-to-csps - converts Ows that are a combination of class and probability\n\
      into the array of classes followed by the corresponding proabilities,\n\
      works for Moore Multy Dfa only\n\
\n\
  --fsm-type=<type> - specifies input automaton type:\n\
    rs-nfa      - Rabin-Scott NFA, the default value\n\
    rs-dfa      - Rabin-Scott DFA,\n\
    moore-dfa   - Moore DFA\n\
    moore-mdfa  - Moore DFA with multiple outputs\n\
    mealy-nfa   - Mealy NFA\n\
    mealy-dfa   - Mealy DFA,\n\
    arr         - Array, can be used for mmap renumaration only\n\
\n\
  --in-map=<map> - reads info/action map from <map> file,\n\
    used with --alg=sort-map only\n\
\n\
  --out-map=<map> - writes resulting info/action map into <map> file,\n\
    used with --alg=sort-map only\n\
\n\
"

<< "\
  --dir=<direction> - specifies direction for lexicographical sorting,\n\
    used only with --alg=sort-map:\n\
    l2r - left to right (the dafault value)\n\
    r2l - right to left\n\
\n\
  --max-prob=<max-prob> - specifies the maximum integer value for the 1.0 of\n\
    the P(Class|State) probability for --alg=cxps-to-csps only, 255 is used by\n\
    default\n\
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
    if (0 == strncmp ("--in=", *argv, 5)) {
        g_pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        g_pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
        g_no_output = true;
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
    if (0 == strcmp ("--fsm-type=arr", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_ARRAY;
        continue;
    }
    if (0 == strcmp ("--alg=repair", *argv)) {
        g_renum_type = ALG_REPAIR;
        continue;
    }
    if (0 == strcmp ("--alg=depth-first", *argv)) {
        g_renum_type = ALG_DEPTH_FIRST;
        continue;
    }
    if (0 == strcmp ("--alg=remove-gaps", *argv)) {
        g_renum_type = ALG_REMOVE_GAPS;
        continue;
    }
    if (0 == strcmp ("--alg=mmap-sort", *argv)) {
        g_renum_type = ALG_MMAP_SORT;
        continue;
    }
    if (0 == strcmp ("--alg=cxps-to-csps", *argv)) {
        g_renum_type = ALG_CXPS_CSPS;
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
    if (0 == strncmp ("--in-map=", *argv, 9)) {
        g_pInActFile = &((*argv) [9]);
        continue;
    }
    if (0 == strncmp ("--out-map=", *argv, 10)) {
        g_pOutActFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--max-prob=", *argv, 11)) {
        g_max_prob = atoi (&((*argv) [11]));
        continue;
    }
  }
}


void RenumState2Ow (const int * pOld2New)
{
    DebugLogAssert (pOld2New);

    const int MaxOldState = g_in_rs_fsm.GetMaxState ();

    for (int State = 0; State <= MaxOldState; ++State) {

        const int Ow = g_in_state2ow.GetOw (State);

        if (-1 != Ow) {
            const int NewState = pOld2New [State];
            g_out_state2ow.SetOw (NewState, Ow);
        }
    }
}


void RenumState2Ows (const int * pOld2New)
{
    DebugLogAssert (pOld2New);

    const int MaxOldState = g_in_rs_fsm.GetMaxState ();

    for (int State = 0; State <= MaxOldState; ++State) {

        const int * pOws;
        const int OwsCount = g_in_state2ows.GetOws (State, &pOws);

        if (0 < OwsCount) {
            const int NewState = pOld2New [State];
            g_out_state2ows.SetOws (NewState, pOws, OwsCount);
        }
    }
}


template < class _TAlg > 
    void Renumerate ()
{
    // read
    if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm, &g_in_state2ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm, &g_in_state2ows);

    } else {

        std::cerr << "ERROR: \"Unsupported type of automaton for selected renumeration\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }

    // calc renumeration map
    _TAlg renum (&g_alloc);
    renum.SetDfa (&g_in_rs_fsm);
    renum.Process ();

    // get renumeration map
    const int * pOld2New = renum.GetOld2NewMap ();
    DebugLogAssert (pOld2New);

    // setup map-based container
    g_out_rs_fsm.SetOldDfa (&g_in_rs_fsm);
    g_out_rs_fsm.SetOld2New (pOld2New);
    g_out_rs_fsm.Prepare ();

    // renum reaction maps if needed
    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        RenumState2Ow (pOld2New);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        RenumState2Ows (pOld2New);
    }

    // save
    if (false == g_no_output) {

        if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

            g_io.Print (*g_pOs, &g_out_rs_fsm);

        } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

            g_io.Print (*g_pOs, &g_out_rs_fsm, &g_out_state2ow);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

            g_io.Print (*g_pOs, &g_out_rs_fsm, &g_out_state2ows);
        }
    } // of if (false == g_no_output) ...
}


void RenumState2Ow_ow (const FAMapA * pOld2New)
{
    DebugLogAssert (pOld2New);

    const int MaxState = g_in_rs_fsm.GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int Ow = g_in_state2ow.GetOw (State);

        if (-1 != Ow) {

            const int * pNewOw = pOld2New->Get (Ow);
            DebugLogAssert (pNewOw);
            const int NewOw = *pNewOw;

            g_out_state2ow.SetOw (State, NewOw);
        }
    } // of for (int State = 0; ...
}


void RenumState2Ows_ow (const FAMapA * pOld2New)
{
    DebugLogAssert (pOld2New);

    FAArray_cont_t < int > ows;
    ows.SetAllocator (&g_alloc);
    ows.Create ();

    const int MaxOwsCount = g_in_state2ows.GetMaxOwsCount ();
    DebugLogAssert (0 < MaxOwsCount);

    ows.resize (MaxOwsCount);
    int * pOwsBuff = ows.begin ();
    DebugLogAssert (pOwsBuff);

    const int MaxState = g_in_rs_fsm.GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int OwsCount = \
            g_in_state2ows.GetOws (State, pOwsBuff, MaxOwsCount);
        DebugLogAssert (OwsCount <= MaxOwsCount);

        if (0 < OwsCount) {

            for (int i = 0; i < OwsCount; ++i) {

                const int Ow = pOwsBuff [i];
                const int * pNewOw = pOld2New->Get (Ow);
                DebugLogAssert (pNewOw);

                const int NewOw = *pNewOw;
                pOwsBuff [i] = NewOw;
            }

            const int NewCount = ::FASortUniq (pOwsBuff, pOwsBuff + OwsCount);
            DebugLogAssert (0 < NewCount && NewCount <= OwsCount);

            g_out_state2ows.SetOws (State, pOwsBuff, NewCount);

        } // of if (0 < OwsCount) ...

    } // of for (int State = 0; ...
}


void RenumArr_ow (const FAMapA * pOld2New)
{
    DebugLogAssert (pOld2New);

    const int Count = g_in_arr.size ();

    for (int i = 0; i < Count; ++i) {

        // get old ow
        const int Ow = g_in_arr [i];

        // find new ow
        const int * pNewOw = pOld2New->Get (Ow);
        DebugLogAssert (pNewOw);

        // store new ow
        g_in_arr [i] = *pNewOw;
    }
}


void Renumerate_mmap_sort ()
{
    DebugLogAssert (g_pInActFile && g_pOutActFile);

    // read automaton
    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm, &g_in_state2ow);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm, &g_in_state2ows);

    } else if (FAFsmConst::TYPE_ARRAY == g_fsm_type) {

        const int * pArr;
        int ArrSize;
        g_map_io.Read (*g_pIs, &pArr, &ArrSize);
        DebugLogAssert (0 < ArrSize && pArr);

        g_in_arr.resize (ArrSize);
        memcpy (g_in_arr.begin (), pArr, ArrSize * sizeof (int));

    } else {

        std::cerr << "ERROR: \"Unsupported type of automaton for selected renumeration.\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }
    // read multi-map
    {
        std::ifstream ifs (g_pInActFile, std::ios::in);
        FAAssertStream (&ifs, g_pInActFile);
        g_map_io.Read (ifs, &g_in_mmap);
    }

    // sort the map
    FASortMultiMap mmap_sort (&g_alloc);
    mmap_sort.SetDirection (g_dir);
    mmap_sort.SetMultiMap (&g_in_mmap);
    mmap_sort.Process ();

    FAMap_judy old2new;

    // build output multi-map
    const int * pKeys;
    const int KeyCount = mmap_sort.GetKeyOrder (&pKeys);
    for (int NewKey = 0; NewKey < KeyCount; ++NewKey) {

        const int Key = pKeys [NewKey];
        old2new.Set (Key, NewKey);

        const int * pVals;
        const int ValCount = g_in_mmap.Get (Key, &pVals);
        DebugLogAssert (0 <= ValCount && pVals);

        g_out_mmap.Set (NewKey, pVals, ValCount);
    }

    // update referee automaton reaction map
    if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        RenumState2Ow_ow (&old2new);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        RenumState2Ows_ow (&old2new);

    } else if (FAFsmConst::TYPE_ARRAY == g_fsm_type) {

        RenumArr_ow (&old2new);
    }

    // save
    if (false == g_no_output) {

        if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

            g_io.Print (*g_pOs, &g_in_rs_fsm, &g_out_state2ow);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

            g_io.Print (*g_pOs, &g_in_rs_fsm, &g_out_state2ows);

        } else if (FAFsmConst::TYPE_ARRAY == g_fsm_type) {

            g_map_io.Print (*g_pOs, g_in_arr.begin (), g_in_arr.size ());
        }
        // save multi-map
        {
            std::ofstream ofs (g_pOutActFile, std::ios::out);
            g_map_io.Print (ofs, &g_out_mmap);
        }

    } // of if (false == g_no_output) ...
}


void Renumerate_cxps_csps ()
{
    if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        g_io.Read (*g_pIs, &g_in_rs_fsm, &g_in_state2ows);

    } else {

        std::cerr << "ERROR: \"Unsupported type of automaton for selected renumeration.\""
                  << " in program " << __PROG__ << '\n';
        exit (1);
    }

    FAArray_cont_t < int > ows;
    ows.SetAllocator (&g_alloc);
    ows.Create ();

    const int Divisor = g_max_prob + 1;

    const int MaxState = g_in_rs_fsm.GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int * pOws;
        const int OwCount = g_in_state2ows.GetOws (State, &pOws);

        if (0 < OwCount) {

            ows.resize (2 * OwCount);

            for (int i = 0; i < OwCount; ++i) {

                const int Ow = pOws [i];

                const int Tag = Ow / Divisor;
                const int IntProb = Ow % Divisor;

                ows [i] = Tag;
                ows [i + OwCount] = IntProb;

            } // of for (int i = 0; ... 

            g_out_state2ows.SetOws (State, ows.begin (), 2 * OwCount);

        } // of if (0 < OwCount) ...

    } // of for (int State = 0; ...

    // save
    if (false == g_no_output) {

        DebugLogAssert (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type);
        g_io.Print (*g_pOs, &g_in_rs_fsm, &g_out_state2ows);

    } // of if (false == g_no_output) ...
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    g_in_mmap.SetAllocator (&g_alloc);
    g_out_mmap.SetAllocator (&g_alloc);
    g_in_arr.SetAllocator (&g_alloc);
    g_in_arr.Create ();

    process_args (argc, argv);

    try {

        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = & g_ifs;
        }
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = & g_ofs;
        }

        if (ALG_REPAIR == g_renum_type) {

            FAFsmRenum fsm_renum (&g_alloc);
            fsm_renum.SetFsmType (g_fsm_type);
            fsm_renum.Process (g_pOs, g_pIs);

        } else if (ALG_DEPTH_FIRST == g_renum_type) {

            Renumerate < FARSDfaRenum_depth_first > ();

        } else if (ALG_REMOVE_GAPS == g_renum_type) {

            Renumerate < FARSDfaRenum_remove_gaps > ();

        } else if (ALG_MMAP_SORT == g_renum_type) {

            Renumerate_mmap_sort ();

        } else if (ALG_CXPS_CSPS == g_renum_type) {

            Renumerate_cxps_csps ();
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
