/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAAllocator.h"
#include "FAMapIOTools.h"
#include "FAPrintUtils.h"
#include "FAUtf8Utils.h"
#include "FATagSet.h"
#include "FAImageDump.h"
#include "FAWREConf_pack.h"
#include "FAMultiMap_pack.h"
#include "FAParser_triv_t.h"
#include "FAParser_nest_t.h"
#include "FAWreLexTools_t.h"
#include "FAWreLexTools_t.h"
#include "FAParserConfKeeper.h"
#include "FAParseTree.h"
#include "FATaggedText.h"
#include "FACorpusIOTools_utf8.h"
#include "FAMorphLDB_t_packaged.h"
#include "FADictInterpreter_t.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pStageFile = NULL;
const char * g_pTagsetFile = NULL;
const char * g_pInLDBFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

bool g_resume = false;
bool g_ignore_case = false;
bool g_no_pos_tags = false;
bool g_no_output = false;
bool g_no_process = false;
bool g_verbose = false;

int g_MaxPassCount = 1;
int g_AlgType = FAFsmConst::PARSER_TRIV;

const int MaxOutputSize = 30 * FALimits::MaxWordCount;
int g_Out [MaxOutputSize];

// allocator
FAAllocator g_alloc;


void usage () {

  std::cout << "\n\
Usage: fa_ts2ps [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
This program makes tagged sentence to parsed sentence convertion. Word text\n\
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
  --stage=<input> - input compiled grammar file in memory dump format,\n\
    out.dump is used by default\n\
\n\
  --ldb=<input> - reads PRM LDB dump from file <input>,\n\
    is not used by default\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file,\n\
    have to be specified\n\
\n\
  --alg=<type> - specifies parsing algorithm, (triv is used by default):\n\
    triv - no overlaps, no inclusions, left most longest\n\
    nest - no overlaps, no same tag inclusions, left most longest\n\
    wre-lex - uses FAWreLexTools_t algorithm, allows to have _function calls\n\
\n\
  --max-pass-count=N - specifies the maximum number of passes for parser,\n\
    1 is used by default\n\
\n\
  --resume - continue parsing from the given tree (requires tagged tree\n\
    intput but not tagged text)\n\
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
\n\
  --verbose - prints additional information to the same direction as --out=\n\
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
    if (0 == strncmp ("--stage=", *argv, 8)) {
        g_pStageFile = &((*argv) [8]);
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
    if (0 == strcmp ("--alg=triv", *argv)) {
        g_AlgType = FAFsmConst::PARSER_TRIV;
        continue;
    }
    if (0 == strcmp ("--alg=nest", *argv)) {
        g_AlgType = FAFsmConst::PARSER_NEST;
        continue;
    }
    if (0 == strcmp ("--alg=wre-lex", *argv)) {
        g_AlgType = FAFsmConst::PARSER_WRE_LEX;
        continue;
    }
    if (0 == strncmp ("--max-pass-count=", *argv, 17)) {
        g_MaxPassCount = atoi (&((*argv) [17]));
        continue;
    }
    if (0 == strncmp ("--resume", *argv, 8)) {
        g_resume = true;
        continue;
    }
    if (0 == strcmp ("--ignore-case", *argv)) {
        g_ignore_case = true;
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
    if (0 == strcmp ("--verbose", *argv)) {
        g_verbose = true;
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
        FAAllocatorA * g_pool;
        FAAllocator alloc;
        g_pool = & alloc;

        // IO
        FAMapIOTools g_map_io (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io (&g_alloc);
        // tagset
        FATagSet g_tagset (&g_alloc);
        // PRM LDB
        FAImageDump g_PrmImg;
        FAMorphLDB_t < int > g_ldb;
        FADictInterpreter_t < int > g_tag_dict;
        /// compiled grammar
        FAImageDump g_StageImg;
        FAWREConf_pack g_Wre;
        FAMultiMap_pack g_Acts;
        // parser
        FAParserConfKeeper g_conf;
        FAWreLexTools_t < int > g_wre_lex;
        FAParser_triv_t < int > g_parser_triv (&g_alloc);
        FAParser_nest_t < int > g_parser_nest (&g_alloc);
        FAParser_base_t < int > * g_pParser = NULL;
        // input tagged text
        FATaggedText g_text (&g_alloc);
        FATaggedTextA * pInText = &g_text;
        // output parse tree
        FAParseTree g_tree (g_pool);

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
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }
        if (FAFsmConst::PARSER_TRIV == g_AlgType) {
            g_pParser = &g_parser_triv ;
        } else if (FAFsmConst::PARSER_NEST == g_AlgType) {
            g_pParser = &g_parser_nest ;
        }

        // load POS tagset
        if (g_pTagsetFile) {
            std::ifstream tagset_ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagsetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
        }

        // load PRM LDB
        if (g_pInLDBFile) {

            g_PrmImg.Load (g_pInLDBFile);
            const unsigned char * pImg = g_PrmImg.GetImageDump ();
            FAAssert (pImg, FAMsg::IOError);

            g_ldb.SetImage (pImg);

            const FADictConfKeeper * pConf = g_ldb.GetTagDictConf ();

            if (pConf) {
                g_tag_dict.SetConf (pConf, g_ldb.GetInTr ());
            }
        }

        // load and set up the compiled grammar
        if (g_pStageFile) {

            g_StageImg.Load (g_pStageFile);
            const unsigned char * pImg = g_StageImg.GetImageDump ();
            FAAssert (pImg, FAMsg::IOError);

            const int * pA = (const int *) pImg ;
            const int Count = *pA;
            FAAssert (2 == Count, FAMsg::IOError);

            g_Wre.SetImage (pImg + *++pA);
            g_Acts.SetImage (pImg + *++pA);

            g_conf.SetWre (&g_Wre);
            g_conf.SetActs (&g_Acts);
            g_conf.SetIgnoreCase (g_ignore_case);
            g_conf.SetMaxDepth (g_MaxPassCount);

        } else {

            const int * pValues = NULL;
            const int Size = g_ldb.GetHeader ()->Get (FAFsmConst::FUNC_WRE, &pValues);
            FAAssert (0 < Size, FAMsg::IOError);

            g_conf.Initialize (&g_ldb, pValues, Size);
        }

        if (FAFsmConst::PARSER_TRIV == g_AlgType || \
            FAFsmConst::PARSER_NEST == g_AlgType) {

            const FAWREConfCA * pWre = g_conf.GetWre ();
            const FAMultiMapCA * pActs = g_conf.GetActs ();

            g_pParser->SetRules (pWre->GetDfa1 (), pWre->GetState2Ows ());
            g_pParser->SetDigitizer (pWre->GetTxtDigDfa (), pWre->GetTxtDigOws ());
            g_pParser->SetDctDigitizer (&g_tag_dict, pWre->GetDictDig ());
            g_pParser->SetActions (pActs);
            g_pParser->SetIgnoreCase (g_ignore_case);
            g_pParser->SetResume (g_resume);
            g_pParser->SetTokenType (pWre->GetTokenType ());
            g_pParser->SetTagOwBase (pWre->GetTagOwBase ());
            g_pParser->SetMaxPassCount (g_MaxPassCount);
            g_pParser->SetParseTree (&g_tree);

        } else if (FAFsmConst::PARSER_WRE_LEX == g_AlgType) {

            g_wre_lex.Initialize (g_pool, &g_conf, &g_tag_dict);
        }

        ///
        /// process input
        ///

        while (!g_pIs->eof ()) {

            // read input
            if (!g_resume) {
                g_txt_io.Read (*g_pIs, &g_text);
            } else {
                g_txt_io.Read (*g_pIs, &g_text, &g_tree);
            }

            const int WordCount = pInText->GetWordCount ();

            if (!g_no_process && 0 < WordCount) {

                if (FAFsmConst::PARSER_TRIV == g_AlgType || \
                    FAFsmConst::PARSER_NEST == g_AlgType) {

                    for (int i = 0; i < WordCount; ++i) {

                        const int * pWord;
                        const int WordLen = pInText->GetWord (i, &pWord);
                        DebugLogAssert (0 < WordLen && pWord);

                        const int Tag = pInText->GetTag (i);
                        DebugLogAssert (0 < Tag);

                        g_pParser->AddWord (pWord, WordLen, Tag);        
                    }

                    g_pParser->Process ();

                    if (false == g_no_output) {
                        g_txt_io.SetNoPosTags (false);
                        g_txt_io.Print (*g_pOs, pInText, &g_tree);
                        g_txt_io.SetNoPosTags (g_no_pos_tags);
                    }

                } else if (FAFsmConst::PARSER_WRE_LEX == g_AlgType) {

                    if (false == g_resume) {
                        g_tree.Init (WordCount);
                    }

                    g_wre_lex.Reset (WordCount);

                    for (int i = 0; i < WordCount; ++i) {

                        const int * pWord;
                        const int WordLen = pInText->GetWord (i, &pWord);
                        DebugLogAssert (0 < WordLen && pWord);

                        const int Tag = pInText->GetTag (i);
                        DebugLogAssert (0 < Tag);

                        g_wre_lex.AddWord (pWord, WordLen, Tag);
                    }

                    g_wre_lex.SetParseTree (&g_tree);
                    g_wre_lex.Process ();

                    if (g_no_output) {
                        continue;
                    }

                    g_txt_io.SetNoPosTags (false);
                    g_txt_io.Print (*g_pOs, pInText, &g_tree);
                    g_txt_io.SetNoPosTags (g_no_pos_tags);
                }

            } // if (!g_no_process && 0 < WordCount) ...

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
