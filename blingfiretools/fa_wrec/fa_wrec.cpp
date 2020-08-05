/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAImageDump.h"
#include "FATagSet.h"
#include "FAMorphLDB_t_packaged.h"
#include "FADictInterpreter_t.h"
#include "FAWRECompiler.h"
#include "FAWREIO.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <string>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;


FAAutIOTools fsm_io (&g_alloc);
FAMapIOTools map_io (&g_alloc);

// tagsets
FATagSet tagset (&g_alloc);
FATagSet tagset2 (&g_alloc);

// PRM
FAImageDump ldb_img;
FAMorphLDB_t < int > ldb;
FADictInterpreter_t < int > tag_dict;

// Input Tokens
FAChain2Num_hash tokens;

// CNF
FAMultiMap_judy cnf;

// Type2Ows map
FAMultiMap_judy type2ows;

// WRE compiler
FAWRECompiler wrecc (&g_alloc);


/// input files
const char * g_pInFile = NULL;
const char * g_pInTagSetFile = NULL;
const char * g_pInTagSet2File = NULL;
const char * g_pInLdbFile = NULL;
/// input external digitizer
const char * g_pInExtDigFile = NULL;

/// output WRE file
const char * g_pOutFile = NULL;
/// output external digitizer
const char * g_pOutExtDigFile = NULL;

const char * g_pDictRoot = NULL;
const char * g_pInputEnc = "UTF-8";

int g_Type = FAFsmConst::WRE_TYPE_RS;
bool g_build_dump = false;


void usage () {

  std::cout << "\n\
Usage: fa_wrec [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
This program builds WRE rules.\n\
\n\
 Input parameters:\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --tagset=<input-file> - reads POS tagger tagset from the <input-file>,\n\
    does not use it by default\n\
\n\
  --tagset2=<input-file> - if this parameter is specified then all WRE @TAG\n\
    references are resolved by the tag-dictionary and corresponding digitizer\n\
    will be constructed, not used by default\n\
\n\
  --ldb=<ldb-dump> - specifies PRM LDB file, it is needed for tag-dictionary\n\
    digitizer, not used by default\n\
\n\
  --dict-root=<path> - specifies text digitizer dictionary path,\n\
    not used by default\n\
\n\
  --input-enc=<enc> - input encoding, UTF-8 - is used by default\n\
\n\
  --type=<type> - is one of the following:\n\
    rs - Rabin-Scott automaton (text -> Yes/No), is used by default\n\
    moore - Moore automaton (text -> RuleNum)\n\
    mealy - Mealy automaton (text -> TrBr)\n\
\n";

    std::cout << "\
 Output parameters:\n\
\n\
  --out=<output> - writes compiled WRE into <output> file,\n\
    stdout is used by default\n\
\n\
  --build-dump - builds a single memory-dump output file\n\
\n\
 To create an external digitizer:\n\
\n\
  --out-ext=<filename> - stores Token <-> Num, Token -> Type CNF and Type ->\n\
    Ows map, needed for external digitizer; does not print this by default\n\
\n\
 To use an external digitizer:\n\
\n\
  --in-ext=<filename> - reads Token <-> Num, Token -> Type CNF and Type ->\n\
    Ows map, needed for external digitizer; does not read this by default\n\
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
            g_pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strcmp ("--type=rs", *argv)) {
            g_Type = FAFsmConst::WRE_TYPE_RS;
            continue;
        }
        if (0 == strcmp ("--type=moore", *argv)) {
            g_Type = FAFsmConst::WRE_TYPE_MOORE;
            continue;
        }
        if (0 == strcmp ("--type=mealy", *argv)) {
            g_Type = FAFsmConst::WRE_TYPE_MEALY;
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
        if (0 == strncmp ("--ldb=", *argv, 6)) {
            g_pInLdbFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--input-enc=", *argv, 12)) {
            g_pInputEnc = &((*argv) [12]);
            continue;
        }
        if (0 == strncmp ("--dict-root=", *argv, 12)) {
            g_pDictRoot = &((*argv) [12]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out-ext=", *argv, 10)) {
            g_pOutExtDigFile = &((*argv) [10]);
            continue;
        }
        if (0 == strncmp ("--in-ext=", *argv, 9)) {
            g_pInExtDigFile = &((*argv) [9]);
            continue;
        }
        if (0 == strcmp ("--build-dump", *argv)) {
            g_build_dump = true;
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

        if ((!g_pInTagSet2File && g_pInLdbFile) ||
            (g_pInTagSet2File && !g_pInLdbFile)) {
            throw FAException ("--tagset2= and --ldb= should both be specified",
                               __FILE__, __LINE__);
        }


        // Input Tokens
        tokens.SetAllocator (&g_alloc);
        tokens.SetCopyChains (true);

        // CNF
        cnf.SetAllocator (&g_alloc);

        // Type2Ows map
        type2ows.SetAllocator (&g_alloc);

        /// adjust IO

        std::istream * pIs = &std::cin;

        if (NULL != g_pInFile) {
            std::ifstream ifs;
            ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&ifs, g_pInFile);
            pIs = &ifs;
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
        if (NULL != g_pInExtDigFile) {

            std::ifstream ifs (g_pInExtDigFile, std::ios::in);
            FAAssertStream (&ifs, g_pInExtDigFile);

            map_io.Read (ifs, &tokens);
            map_io.Read (ifs, &cnf);
            map_io.Read (ifs, &type2ows);

            FAAssert (!::FAIsEmpty (&cnf) && !::FAIsEmpty (&type2ows), \
                FAMsg::IOError);
        }

        /// do processing

        wrecc.SetType (g_Type);
        wrecc.SetEncodingName (g_pInputEnc);
        wrecc.SetDictRoot (g_pDictRoot);

        if (NULL != g_pInTagSetFile) {
            wrecc.SetTagSet (&tagset);
        }
        if (NULL != g_pInTagSet2File) {
            wrecc.SetTagSet2 (&tagset2);
        }
        if (NULL != g_pInLdbFile) {
            wrecc.SetTagDict (&tag_dict);
        }
        if (NULL != g_pInExtDigFile) {
            wrecc.SetTokens (&tokens);
            wrecc.SetCNF (&cnf);
            wrecc.SetType2Ows (&type2ows);
        }

        std::string line;
        const char * pLine;
        int LineLen;

        while (!(pIs->eof ())) {

            std::string Re;

            // read until the empty line is met
            if (!std::getline (*pIs, line))
                break;

            pLine = line.c_str ();
            LineLen = (const int) line.length ();

            if (0 < LineLen) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }

            while (0 < LineLen) {

                Re = Re + std::string (pLine, LineLen) + "\n";

                if (pIs->eof ())
                    break;

                if (!std::getline (*pIs, line))
                    break;

                pLine = line.c_str ();
                LineLen = (const int) line.length ();

                if (0 < LineLen) {
                    DebugLogAssert (pLine);
                    if (0x0D == (unsigned char) pLine [LineLen - 1])
                        LineLen--;
                }
            }

            const char * pRe = Re.c_str ();
            int ReLength = (const int) Re.length ();

            if (0 < ReLength) {

                DebugLogAssert (pRe);
                wrecc.AddRule (pRe, ReLength);

            } else {
                // break the outer loop, if RE is empty
                break;
            }

        } // of while (!(pIs->eof ())) ...

        // make compilation
        wrecc.Process ();

        /// save the results, only if not empty
        if (wrecc.GetDfa1 ()) {

            std::ostream * pOs = & std::cout ;
            std::ofstream ofs;

            if (g_pOutFile) {
                if (g_build_dump) {
                    ofs.open (g_pOutFile, std::ios::out | std::ios::binary);
                } else {
                    ofs.open (g_pOutFile, std::ios::out);
                }
                pOs = & ofs;
            }
            if (!g_build_dump) {
                ::FAPrintWre (*pOs, &wrecc);
            } else {
                ::FASaveWre (*pOs, &wrecc);
            }
        }

        /// save external digitzer, if needed
        if (g_pOutExtDigFile) {

            const FAChain2NumA * pTokens = wrecc.GetTokens ();
            FAAssert (pTokens, FAMsg::InternalError);

            const FAMultiMapA * pCnf = wrecc.GetCNF ();
            FAAssert (pCnf && !::FAIsEmpty (pCnf), FAMsg::InternalError);

            const FAMultiMapA * pOwsMap = wrecc.GetType2Ows ();
            FAAssert (pOwsMap && !::FAIsEmpty (pOwsMap), FAMsg::InternalError);

            std::ofstream ofs (g_pOutExtDigFile, std::ios::out);

            map_io.Print (ofs, pTokens);
            map_io.Print (ofs, pCnf);
            map_io.Print (ofs, pOwsMap);
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

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
