/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAAllocator.h"
#include "FAMapIOTools.h"
#include "FATagSet.h"
#include "FAImageDump.h"
#include "FATaggedText.h"
#include "FACorpusIOTools_utf8.h"
#include "FAUtils.h"
#include "FAException.h"
#include "FAMorphLDB_t_packaged.h"
#include "FAWordGuesser_prob_t.h"
#include "FAT2PTable.h"
#include "FATs2PTable.h"
#include "FAHmmTagger_l1.h"
#include "FAArray_cont_t.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pTagsetFile = NULL;
const char * g_pWbdTagsetFile = NULL;
const char * g_pInLDBFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

bool g_no_output = false;
bool g_no_process = false;

// allocator
FAAllocator g_alloc;

int g_InputLineNum = 0;


void usage () {

  std::cout << "\n\
Usage: fa_s2ts [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
This program assigns POS tags to the word-broken text. If word-broken text\n\
contains word-breaker tags then the --wbd-tagset= parameter should be\n\
specified. Each input line should contain one sentence.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --ldb=<input> - reads LDB dump from file <input>\n\
\n\
  --tagset=<tagset> - reads POS tagset from the <tagset> file,\n\
    have to be specified\n\
\n\
  --wbd-tagset=<tagset> - reads word-breaker tagset from the <tagset> file,\n\
    if not specified the input text should not contain any tags.\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
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
    if (0 == strncmp ("--ldb=", *argv, 6)) {
        g_pInLDBFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pTagsetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strncmp ("--wbd-tagset=", *argv, 13)) {
        g_pWbdTagsetFile = &((*argv) [13]);
        continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
        g_no_output = true;
        continue;
    }
    if (0 == strncmp ("--no-process", *argv, 12)) {
        g_no_process= true;
        continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {
        // Mem Lfp manager
        FAAllocator alloc;
        FAAllocatorA * g_pool = &alloc;

        // IO
        FAMapIOTools g_map_io (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io_in (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io_out (&g_alloc);
        // tagsets
        FATagSet g_tagset (&g_alloc);
        FATagSet g_wbd_tagset (&g_alloc);
        // LDB (keeps everything)
        FAImageDump g_PrmImg;
        FAMorphLDB_t < int > g_ldb;
        // input/output text
        FATaggedText g_text (&g_alloc);
        // output tags
        FAArray_cont_t < int > g_tags;

        g_tags.SetAllocator (&g_alloc);
        g_tags.Create ();

        // P(T|W) model
        FAWordGuesser_prob_t < int > g_w2tp;
        // P(T) table
        FAT2PTable g_t2p;
        // P(T|T-1) table
        FATs2PTable g_tt2p;
        // HMM tagger
        FAHmmTagger_l1 g_tagger;

        ///
        /// initialize
        ///

        // adjust IO pointers
        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }
        // load WBD tagset, if any
        if (g_pWbdTagsetFile) {
            std::ifstream tagset_ifs (g_pWbdTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pWbdTagsetFile);
            g_map_io.Read (tagset_ifs, &g_wbd_tagset);
        }
        // load POS tagset
        FAAssert (g_pTagsetFile, FAMsg::InvalidParameters);
        {
            std::ifstream tagset_ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagsetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
        }

        // initialize corpus IO
        g_txt_io_in.SetTagSet (&g_wbd_tagset);
        g_txt_io_in.SetNoPosTags (NULL == g_pWbdTagsetFile);

        g_txt_io_out.SetTagSet (&g_tagset);
        g_txt_io_out.SetNoPosTags (false);

        // LDB must exist
        FAAssert (g_pInLDBFile, FAMsg::InvalidParameters);

        g_PrmImg.Load (g_pInLDBFile);
        const unsigned char * pImg = g_PrmImg.GetImageDump ();
        FAAssert (pImg, FAMsg::IOError);

        g_ldb.SetImage (pImg);

        const FAWgConfKeeper * pW2TPConf = g_ldb.GetW2TPConf ();
        LogAssert (pW2TPConf);

        const FAState2OwsCA * pOws = pW2TPConf->GetState2Ows ();
        LogAssert (pOws);

        const int MaxTagsPerWord = pOws->GetMaxOwsCount ();

        g_w2tp.Initialize (pW2TPConf, g_ldb.GetInTr ());
        g_t2p.SetConf (g_ldb.GetT2PConf ());
        g_tt2p.SetConf (g_ldb.GetTT2PConf ());

        const int EosTag = pW2TPConf->GetEosTag ();

        g_tagger.Initialize (&g_w2tp, &g_t2p, &g_tt2p, EosTag, MaxTagsPerWord, g_pool);

        ///
        /// process input
        ///

        while (!g_pIs->eof ()) {

            g_InputLineNum++;

            // read the input tagged text
            g_txt_io_in.Read (*g_pIs, &g_text);

            if (g_no_process)
                continue;

            const int WordCount = g_text.GetWordCount ();

            for (int i = 0; i < WordCount; ++i) {

                const int * pWord = NULL;
                const int WordLen = g_text.GetWord (i, &pWord);

                g_tagger.AddWord (pWord, WordLen);
            }

            g_tags.resize (WordCount);

            const int OutSize = g_tagger.Process (g_tags.begin (), WordCount);
            LogAssert (OutSize == WordCount);

            if (g_no_output)
                continue;

            // print the tagged text
            g_text.SetTags (g_tags.begin (), WordCount);
            g_txt_io_out.Print (*g_pOs, &g_text);

        } // of while (!g_pIs->eof ()) ...

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        if (0 < g_InputLineNum) {
            std::cerr << "ERROR: at input line " << g_InputLineNum << '\n';
        }

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';

        if (0 < g_InputLineNum) {
            std::cerr << "ERROR: at input line " << g_InputLineNum << '\n';
        }

        return 1;
    }

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
