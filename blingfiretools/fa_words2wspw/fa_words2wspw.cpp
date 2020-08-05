/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAImageDump.h"
#include "FARSDfa_pack_triv.h"
#include "FAWord2WordSuff.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * pInFile = NULL;

FAImageDump g_dfa_dump;
FARSDfa_pack_triv g_dfa;

int g_dir = FAFsmConst::DIR_L2R;


void usage ()
{
    std::cerr << "\
Usage: fa_words2wspw OPTIONS > output.utf8\n\
\n\
Reads a list of words in the form of automaton, and finds all the pairs\n\
Word Suff, such that WordSuff and Word are valid words.\n\
\n\
  --in=<input-aut> - reads wordlist automaton from <input-aut> file,\n\
    trivial dump format must be used\n\
\n\
  --dir=<direction> - is one of the following:\n\
    l2r - left to right (the dafault value)\n\
    r2l - right to left\n\
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
        pInFile = &((*argv) [5]);
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
  }
}


///
/// finds W1 == W2 + suff; W1, W2 \in L
///
class FAWord2WordSuff_l2r : public FAWord2WordSuff {

public:
    FAWord2WordSuff_l2r (FAAllocatorA * pAlloc);

public:
    void PutSplit (
            const int * pLeft, 
            const int LeftSize,
            const int * pRight, 
            const int RightSize
        );
};


FAWord2WordSuff_l2r::FAWord2WordSuff_l2r (FAAllocatorA * pAlloc) :
    FAWord2WordSuff (pAlloc)
{}


void FAWord2WordSuff_l2r::
    PutSplit (
            const int * pLeft, 
            const int LeftSize,
            const int * pRight, 
            const int RightSize
    )
{
    DebugLogAssert (pLeft && 0 < LeftSize);
    DebugLogAssert (pRight && 0 < RightSize);

    ::FAPrintWordList (std::cout, pLeft, LeftSize);
    std::cout << (char) m_Delim;
    ::FAPrintWordList (std::cout, pRight, RightSize);
    std::cout << '\n';
}

///
/// finds W1 == pref + W2; W1, W2 \in L
///
class FAWord2WordSuff_r2l : public FAWord2WordSuff {

public:
    FAWord2WordSuff_r2l (FAAllocatorA * pAlloc);

public:
    void PutSplit (
            const int * pLeft, 
            const int LeftSize,
            const int * pRight, 
            const int RightSize
        );

private:
    FAArray_cont_t < int > m_rev_chain;
};


FAWord2WordSuff_r2l::FAWord2WordSuff_r2l (FAAllocatorA * pAlloc) :
    FAWord2WordSuff (pAlloc)
{
    m_rev_chain.SetAllocator (pAlloc);
    m_rev_chain.Create ();
}


void FAWord2WordSuff_r2l::
    PutSplit (
            const int * pLeft, 
            const int LeftSize,
            const int * pRight, 
            const int RightSize
    )
{
    DebugLogAssert (pLeft && 0 < LeftSize);
    DebugLogAssert (pRight && 0 < RightSize);

    int i;

    m_rev_chain.resize (RightSize);
    int * pRevChain = m_rev_chain.begin ();
    for (i = 0; i < RightSize; ++i) {
        pRevChain [RightSize - i - 1] = pRight [i];
    }

    ::FAPrintWordList (std::cout, pRevChain, RightSize);
    std::cout << (char) m_Delim;

    m_rev_chain.resize (LeftSize);
    pRevChain = m_rev_chain.begin ();
    for (i = 0; i < LeftSize; ++i) {
        pRevChain [LeftSize - i - 1] = pLeft [i];
    }

    ::FAPrintWordList (std::cout, pRevChain, LeftSize);
    std::cout << '\n';
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        g_dfa_dump.Load (pInFile);
        const unsigned char * pDump = g_dfa_dump.GetImageDump ();
        g_dfa.SetImage (pDump);

        FAWord2WordSuff * pProc = NULL;
        FAWord2WordSuff_l2r w2ws_l2r (&g_alloc);
        FAWord2WordSuff_r2l w2ws_r2l (&g_alloc);

        if (FAFsmConst::DIR_L2R == g_dir) {
            pProc = &w2ws_l2r;
        } else {
            DebugLogAssert (FAFsmConst::DIR_R2L == g_dir);
            pProc = &w2ws_r2l;
        }

        pProc->SetDelim (0x09);
        pProc->SetRsDfa (&g_dfa);
        pProc->Process ();

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
