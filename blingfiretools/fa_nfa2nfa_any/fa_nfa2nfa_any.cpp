/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAAny2AnyOther_t.h"
#include "FAAny2AnyOther_global_t.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FARSNfa_ro.h"
#include "FARSNfa_wo_ro.h"
#include "FATagSet.h"
#include "FAArray_cont_t.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
FAAutIOTools g_aut_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

FATagSet g_tagset (&g_alloc);
FAArray_cont_t < int > g_exp_iws;

int g_spec_any = 0;
int g_base_iw = -1;
int g_max_iw = -1;

bool g_global = false;
bool g_no_output = false;

const char * g_pInTagSetFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_nfa2nfa_any [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
This program converts non-deterministic automaton with Any symbols into\n\
non-deterministic automaton with AnyOther symbols.\n\
\n\
Reads/writes empty line delimited non-deterministic automatons from/to\n\
stdin/stdout.\n\
\n\
  --no-output - does not do any output\n\
\n\
  --spec-any=N - specifies weight for the special symbol '.',\n\
      the default is 0\n\
\n\
  --global - makes expantion with automaton's alphabet, otherwise with\n\
    state's alphabet\n\
\n\
  --tagset=<input-file> - reads tagset from input file and expands '.' with\n\
    all tags from tagset state-regardless, is not used by default\n\
    if set up --iw-base=Iw is used as a base for tag values\n\
\n\
  --iw-base=Iw - makes expansion with weights >= Iw only,\n\
    by default makes expansion with all Iws\n\
\n\
  --iw-max=Iw - makes expansion with weights <= Iw only,\n\
    by default makes expansion with all Iws\n\
\n\
\n\
Example, input:\n\
 ...\n\
0 0 Any\n\
0 1 1\n\
0 1 2\n\
1 1 1\n\
 ...\n\
output:\n\
 ...\n\
0 0 AnyOther\n\
0 0 1\n\
0 0 2\n\
0 1 1\n\
0 1 2\n\
1 1 1\n\
 ...\n\
";
}

void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv){

    if (0 == strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--global", *argv, 8)) {
        g_global = true;
        continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
        g_no_output = true;
        continue;
    }
    if (0 == strncmp ("--spec-any=", *argv, 11)) {
        g_spec_any = atoi (&((*argv) [11]));
        continue;
    }
    if (0 == strncmp ("--iw-base=", *argv, 10)) {
        g_base_iw = atoi (&((*argv) [10]));
        continue;
    }
    if (0 == strncmp ("--iw-max=", *argv, 9)) {
        g_max_iw = atoi (&((*argv) [9]));
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pInTagSetFile = &((*argv) [9]);
        g_global = true;
        continue;
    }
  }
}


void CalcExpIws ()
{
    int TagIw;

    const int Count = g_tagset.GetStrCount ();

    for (int i = 0; i < Count; ++i) {

        TagIw = g_tagset.GetValue (i);

        if (-1 != g_base_iw) {
            TagIw += g_base_iw;
        }

        g_exp_iws.push_back (TagIw);
    }

    const int NewSize = ::FASortUniq (g_exp_iws.begin (), g_exp_iws.end ());
    g_exp_iws.resize (NewSize);
}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        g_exp_iws.SetAllocator (&g_alloc);
        g_exp_iws.Create ();

        // create objects
        FAAny2AnyOther_t < FARSNfaA, FARSNfa_wo_ro > conv1 (&g_alloc);
        FAAny2AnyOther_global_t < FARSNfaA, FARSNfa_wo_ro > conv2 (&g_alloc);

        conv1.SetAnyIw (g_spec_any);
        conv2.SetAnyIw (g_spec_any);

        if (-1 != g_base_iw) {
            conv2.SetIwBase (g_base_iw);
        }
        if (-1 != g_max_iw) {
            conv2.SetIwMax (g_max_iw);
        }
        if (NULL != g_pInTagSetFile) {

            std::ifstream tagset_ifs (g_pInTagSetFile, std::ios::in);
            g_map_io.Read (tagset_ifs, &g_tagset);

            CalcExpIws ();

            conv2.SetExpIws (g_exp_iws.begin (), g_exp_iws.size ());
        }

        FARSNfa_ro nfa_in (&g_alloc);
        FARSNfa_wo_ro nfa_out (&g_alloc);

        while (!std::cin.eof ()) {

            g_aut_io.Read (std::cin, &nfa_in);

            // see if empty automaton has been read
            if (-1 == nfa_in.GetMaxState ()) {
                continue;
            }

            if (false == g_global) {

                conv1.SetInNfa (&nfa_in);
                conv1.SetOutNfa (&nfa_out);
                conv1.Process ();

            } else {

                conv2.SetInNfa (&nfa_in);
                conv2.SetOutNfa (&nfa_out);
                conv2.Process ();
            }

            g_aut_io.Print (std::cout, &nfa_out);

            nfa_in.Clear ();
            nfa_out.Clear ();
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
