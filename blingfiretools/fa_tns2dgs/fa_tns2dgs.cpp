/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAMultiMap_judy.h"
#include "FAChain2Num_hash.h"
#include "FAWRETokens2Digitizers.h"
#include "FAImageDump.h"
#include "FATagSet.h"
#include "FAMorphLDB_t_packaged.h"
#include "FADictInterpreter_t.h"
#include "FAUtils.h"
#include "FAException.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

bool g_no_output = false;
int g_initial_type_iw = -1;
int g_initial_ow = -1;

const char * g_pDictRoot = NULL;
const char * g_pInputEnc = "UTF-8";

const char * g_pInFile = NULL;
const char * g_pInTagSetFile = NULL;
const char * g_pInTagSet2File = NULL;
const char * g_pInLdbFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pOut2File = NULL;
const char * g_pTxtOutFile = NULL;
const char * g_pDctOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_tns2dgs [OPTION] [< input.txt] [> output.txt]\n\
\n\
This program reads WRE tokens parses them and creates run-time digitizers\n\
and compile-time maps necessary to convert Token Nfa into digital Nfa.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --tagset=<input-file> - reads POS tagger tagset from the <input-file>,\n\
    does not use it by default\n\
\n\
  --dict-root=<path> - specifies text digitizer dictionary path,\n\
    not used by default\n\
\n\
  --tagset2=<input-file> - if this parameter is specified then all WRE @TAG\n\
    references are resolved by the tag-dictionary and corresponding digitizer\n\
    will be constructed, not used by default\n\
\n\
  --ldb=<ldb-dump> - specifies PRM LDB file, it is needed for tag-dictionary\n\
    digitizer, not used by default\n\
\n\
  --input-enc=<enc> - input encoding, UTF-8 - is used by default\n\
\n\
  --out=<output-file> - writes a map from TokenNums into CNF on TypeNums,\n\
    if omited stdout is used\n\
\n\
  --out2=<output-file> - writes a map from < Dig, TypeNum > pair to Ows set,\n\
    if omited stdout is used\n\
\n\
  --txt-out=<output-file> - writes a Moore automaton of text-digitizer,\n\
    if any. If omited stdout is used.\n\
\n\
  --dct-out=<output-file> - writes InfoId -> Ow dict-digitizer array,\n\
    if any. If omited stdout is used.\n\
\n\
  --initial-type=Iw - specifies which smallest Iw can the initial type value,\n\
    if omited the default value will be used\n\
\n\
  --initial-ow=Ow - specifies the first Ow of the text/dict-digitizer\n\
    if omited the default value will be used\n\
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
        if (0 == strncmp ("--initial-type=", *argv, 15)) {
            g_initial_type_iw = atoi (&((*argv) [15]));
            continue;
        }
        if (0 == strncmp ("--initial-ow=", *argv, 13)) {
            g_initial_ow = atoi (&((*argv) [13]));
            continue;
        }
        if (0 == strncmp ("--in=", *argv, 5)) {
            g_pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--ldb=", *argv, 6)) {
            g_pInLdbFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out2=", *argv, 7)) {
            g_pOut2File = &((*argv) [7]);
            continue;
        }
        if (0 == strncmp ("--txt-out=", *argv, 10)) {
            g_pTxtOutFile = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--dct-out=", *argv, 10)) {
            g_pDctOutFile = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--tagset=", *argv, 9)) {
            g_pInTagSetFile = &((*argv) [9]);
            continue;
        }
        if (0 == strncmp ("--tagset2=", *argv, 10)) {
            g_pInTagSet2File = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--dict-root=", *argv, 12)) {
            g_pDictRoot = &((*argv) [12]);
            continue;
        }
        if (0 == strncmp ("--input-enc=", *argv, 12)) {
            g_pInputEnc = &((*argv) [12]);
            continue;
        }
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        FAAutIOTools fsm_io (&g_alloc);
        FAMapIOTools map_io (&g_alloc);

        // tagsets
        FATagSet tagset (&g_alloc);
        FATagSet tagset2 (&g_alloc);

        // PRM
        FAImageDump ldb_img;
        FAMorphLDB_t < int > ldb;
        FADictInterpreter_t < int > tag_dict;

        // CNF
        FAMultiMap_judy cnf;
        cnf.SetAllocator (&g_alloc);

        // Input Tokens
        FAChain2Num_hash tokens;
        tokens.SetAllocator (&g_alloc);
        tokens.SetCopyChains (true);        

        if ((!g_pInTagSet2File && g_pInLdbFile) || \
            (g_pInTagSet2File && !g_pInLdbFile)) {
            throw FAException ("--tagset2= and --ldb= should both be specified", __FILE__, __LINE__);
        }

        /// adjust input/output

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        std::ostream * pOs2 = &std::cout;
        std::ofstream ofs2;

        if (NULL != g_pInFile) {
            ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&ifs, g_pInFile);
            pIs = &ifs;
        }
        if (NULL != g_pOutFile) {
            ofs.open (g_pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (NULL != g_pOut2File) {
            ofs2.open (g_pOut2File, std::ios::out);
            pOs2 = &ofs2;
        }
        if (NULL != g_pInTagSetFile) {
            std::ifstream tagset_ifs (g_pInTagSetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pInTagSetFile);
            map_io.Read (tagset_ifs, &tagset);
        }
        if (NULL != g_pInTagSet2File) {
            std::ifstream tagset2_ifs (g_pInTagSet2File, std::ios::in);
            FAAssertStream (&tagset2_ifs, g_pInTagSet2File);
            map_io.Read (tagset2_ifs, &tagset2);
        }
        if (NULL != g_pInLdbFile) {
            ldb_img.Load (g_pInLdbFile);
            ldb.SetImage (ldb_img.GetImageDump ());
            tag_dict.SetConf (ldb.GetTagDictConf (), ldb.GetInTr ());
        }

        /// read in the WRE tokens
        map_io.Read (*pIs, &tokens);

        /// configure the digitizer builder
        FAWRETokens2Digitizers digitizers (&g_alloc);

        digitizers.SetDictRoot (g_pDictRoot);
        digitizers.SetEncodingName (g_pInputEnc);
        digitizers.SetTokens (&tokens);
        digitizers.SetToken2CNF (&cnf);

        if (-1 != g_initial_ow) {
            digitizers.SetInitialOw (g_initial_ow);
        }
        if (-1 != g_initial_type_iw) {
            digitizers.SetInitialTypeIw (g_initial_type_iw);
        }
        if (NULL != g_pInTagSetFile) {
            digitizers.SetTagSet (&tagset);
        }
        if (NULL != g_pInTagSet2File) {
            DebugLogAssert (NULL != g_pInLdbFile);
            digitizers.SetTagSet2 (&tagset2);
        }
        if (NULL != g_pInLdbFile) {
            DebugLogAssert (NULL != g_pInTagSet2File);
            digitizers.SetTagDict (&tag_dict);
        }

        /// make the processing

        digitizers.Process ();

        /// write down TokenNum -> CNF and TypeNum -> { Ows } maps

        if (false == g_no_output) {
            map_io.Print (*pOs, &cnf);
            map_io.Print (*pOs2, digitizers.GetType2Ows ());
        }

        const FARSDfaA * pDfa = NULL;
        const FAState2OwA * pState2Ow = NULL;

        /// write down text-digitizer, if any

        if (digitizers.GetTxtDigititizer (&pDfa, &pState2Ow)) {

            std::ostream * pOs1 = &std::cout;
            std::ofstream ofs1;
            if (NULL != g_pTxtOutFile) {
                ofs1.open (g_pTxtOutFile, std::ios::out);
                pOs1 = &ofs1;
            }
            if (false == g_no_output) {
                fsm_io.Print (*pOs1, pDfa, pState2Ow);
            }
        }

        /// write down dict-digitizer, if any

        const int * pId2Ow = NULL;
        int Id2OwSize = 0;

        if (digitizers.GetDictDigitizer (&pId2Ow, &Id2OwSize)) {

            std::ostream * pOs1 = &std::cout;
            std::ofstream ofs1;
            if (NULL != g_pDctOutFile) {
                ofs1.open (g_pDctOutFile, std::ios::out);
                pOs1 = &ofs1;
            }
            if (false == g_no_output) {
                map_io.Print (*pOs1, pId2Ow, Id2OwSize);
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
