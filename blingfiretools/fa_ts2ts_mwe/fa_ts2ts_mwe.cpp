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
#include "FAWREConf_pack.h"
#include "FAMergeMwe.h"
#include "FATaggedText.h"
#include "FACorpusIOTools_utf8.h"
#include "FAMorphLDB_t_packaged.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pTagsetFile = NULL;
const char * g_pInLDBFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

bool g_no_pos_tags = false;
bool g_no_output = false;
bool g_no_process = false;

// allocator
FAAllocator g_alloc;


void usage () {

  std::cout << "\n\
Usage: fa_ts2ts_mwe [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
This program merges multi-word expressions (MWE) in the tagged text. Word text\n\
must be separated by '" 
                     << char (FAFsmConst::CHAR_TAG_DELIM)
                     << "' symbol from its tag, word_tag pairs must be single\n\
space separated.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --ldb=<input> - reads PRM LDB dump from file <input>,\n\
    has to be specified, the program uses tag-dict's domain to merge MWEs\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file,\n\
    have to be specified\n\
\n\
  --ignore-case - converts input text to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --no-pos-tags - the input text is word-broken but words have no POS tags,\n\
    the output text will have POS tags\n\
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
    if (0 == strcmp ("--no-pos-tags", *argv)) {
        g_no_pos_tags = true;
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
        // IO
        FAMapIOTools g_map_io (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io (&g_alloc);
        // tagset
        FATagSet g_tagset (&g_alloc);
        // PRM LDB
        FAImageDump g_PrmImg;
        FAMorphLDB_t < int > g_ldb;
        // MWE merger
        FAMergeMwe g_mwe_merger;
        // input tagged text
        FATaggedText g_text (&g_alloc);
        FATaggedText g_text_mwe (&g_alloc);

        ///
        /// initialize
        ///
        g_txt_io.SetTagSet (&g_tagset);
        g_txt_io.SetNoPosTags (g_no_pos_tags);

        // adjust IO pointers
        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }
        // load POS tagset
        if (g_pTagsetFile) {
            std::ifstream tagset_ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagsetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
        }

        // LDB must exist
        FAAssert (g_pInLDBFile, FAMsg::InvalidParameters);

        g_PrmImg.Load (g_pInLDBFile);
        const unsigned char * pImg = g_PrmImg.GetImageDump ();
        FAAssert (pImg, FAMsg::IOError);

        g_ldb.SetImage (pImg);

        const FADictConfKeeper * pConf = g_ldb.GetTagDictConf ();
        DebugLogAssert (pConf);

        const bool IgnoreCase = pConf->GetIgnoreCase ();
        const FARSDfaCA * pDfa = pConf->GetRsDfa ();

        g_mwe_merger.SetIgnoreCase (IgnoreCase);
        g_mwe_merger.SetRsDfa (pDfa);

        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }

        ///
        /// process input
        ///

        while (!g_pIs->eof ()) {

            // read the input tagged text
            g_txt_io.Read (*g_pIs, &g_text);

            if (g_no_process)
                continue;

            // merge MWEs
            g_mwe_merger.Process (&g_text_mwe, &g_text);

            if (g_no_output)
                continue;

            // print tagged text with merged MWEs
            g_txt_io.Print (*g_pOs, &g_text_mwe);

        } // of while (!g_pIs->eof ()) ...

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

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
