/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FALimits.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAException.h"
#include "FAMapIOTools.h"
#include "FACorpusIOTools_utf8.h"
#include "FAWREIO.h"
#include "FAWREConf.h"
#include "FAWREConf_pack.h"
#include "FAAutInterpretTools_t.h"
#include "FAAutInterpretTools_fnfa_t.h"
#include "FAAutInterpretTools2_trbr_t.h"
#include "FABrResult.h"
#include "FATagSet.h"
#include "FATaggedText.h"
#include "FAMorphLDB_t_packaged.h"
#include "FADictInterpreter_t.h"
#include "FAImageDump.h"
#include "FADigitizer_t.h"
#include "FADigitizer_dct_t.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pLdbFile = NULL;
const char * g_pWreFile = NULL;
const char * g_pTagsetFile = "tagset.txt";

bool g_ignore_case = false;
bool g_no_pos_tags = false;
bool g_print_input = false;
bool g_print_ows = false;
bool g_no_output = false;
bool g_no_process = false;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

const int MaxChainSize = FALimits::MaxWordCount * FAFsmConst::DIGITIZER_COUNT;
int OwsChain [MaxChainSize];
int ResSizes [MaxChainSize];
const int * ResPtrs [MaxChainSize];
int g_format = FAFsmConst::FORMAT_TXT ;


void usage () {

  std::cout << "\n\
Usage: test_wre [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
Processes the tagged text by compiled WRE rule(s). Uses FNFA interpreter\n\
for Rabin-Scott and Moore rules and trivial for Mealy. The input is read\n\
in UTF-8 encoding.\n\
\n\
  --in=<input-text> - reads input text from <input-text> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output-text> - prints output text into a file,\n\
    if omited stdout is used\n\
\n\
  --wre=<wre> - compiled WRE file name\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file\n\
\n\
  --ldb=<ldb> - reads PRM LDB dump from file <input>, used only if\n\
   WRE rules have dictionary references\n\
\n\
  --format=<type> - specifies compiled WRE format:\n\
   txt - textual format (the default value)\n\
   dump - memory dump format\n\
\n\
  --ignore-case - converts input text to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --no-pos-tags - the input text is word-broken but words have no POS tags\n\
\n\
  --print-input - prints input string to stdout\n\
\n\
  --print-ows - prints array of digitizer's output weights to stdout\n\
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
        g_pLdbFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--wre=", *argv, 6)) {
        g_pWreFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pTagsetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strcmp ("--ignore-case", *argv)) {
        g_ignore_case = true;
        continue;
    }
    if (0 == strcmp ("--no-output", *argv)) {
        g_no_output = true;
        continue;
    }
    if (0 == strcmp ("--no-process", *argv)) {
        g_no_process = true;
        continue;
    }
    if (0 == strcmp ("--print-input", *argv)) {
        g_print_input = true;
        continue;
    }
    if (0 == strncmp ("--print-ows", *argv, 11)) {
        g_print_ows = true;
        continue;
    }
    if (0 == strcmp ("--format=dump", *argv)) {
        g_format = FAFsmConst::FORMAT_DUMP;
        continue;
    }
    if (0 == strcmp ("--format=txt", *argv)) {
        g_format = FAFsmConst::FORMAT_TXT;
        continue;
    }
    if (0 == strcmp ("--no-pos-tags", *argv)) {
        g_no_pos_tags = true;
        continue;
    }
  }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    /// parse a command line
    process_args (argc, argv);

    try {

        FAMapIOTools map_io (&g_alloc);

        FAMorphLDB_t < int > morph_ldb;
        FADictInterpreter_t < int > tag_dict;
        FAImageDump ldb_image;
        FATagSet tagset (&g_alloc);

        // input WRE
        FAWREConf wre (&g_alloc);
        FAWREConf_pack packed_wre;
        FAImageDump packed_wre_image;
        FAWREConfCA * pWre = NULL;

        FACorpusIOTools_utf8 txt_io (&g_alloc);
        txt_io.SetTagSet (&tagset);
        txt_io.SetNoPosTags (g_no_pos_tags);

        /// read WRE
        if (g_pWreFile) {
            if (FAFsmConst::FORMAT_TXT == g_format) {
                std::ifstream ifs (g_pWreFile, std::ios::in);
                FAAssertStream (&ifs, g_pWreFile);
                ::FAReadWre (ifs, &wre);
                pWre = &wre;
            } else {
                DebugLogAssert (FAFsmConst::FORMAT_DUMP == g_format);
                packed_wre_image.Load (g_pWreFile);
                packed_wre.SetImage (packed_wre_image.GetImageDump ());
                pWre = &packed_wre;
            }
        } else {
            throw FAException ("No WRE specified for execution.", \
                __FILE__, __LINE__);
        }
        /// load PRM LDB
        if (g_pLdbFile) {
            ldb_image.Load (g_pLdbFile);
            morph_ldb.SetImage (ldb_image.GetImageDump ());
            tag_dict.SetConf (morph_ldb.GetTagDictConf (), morph_ldb.GetInTr ());
        }
        /// load POS tagset
        if (g_pTagsetFile) {
            std::ifstream ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&ifs, g_pTagsetFile);
            map_io.Read (ifs, &tagset);
        }
        /// adjust IO
        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }

        /// run-time initialization

        FADigitizer_t < int > txt_digitizer;
        FADigitizer_dct_t < int > dct_digitizer;
        FAAutInterpretTools_fnfa_t < int > g_proc_fnfa (&g_alloc);
        FAAutInterpretTools2_trbr_t < int > g_proc_trbr (&g_alloc);

        FATaggedText text (&g_alloc);
        FABrResult trbr_res (&g_alloc);

        const int Type = pWre->GetType ();
        const int TokenType = pWre->GetTokenType ();
        const int TagOwBase = pWre->GetTagOwBase ();
        int TupleSize = 0;

        /// initialize digitizers

        if (FAFsmConst::WRE_TT_TEXT & TokenType) {

            const FARSDfaCA * pDfa = pWre->GetTxtDigDfa ();
            const FAState2OwCA * pOws = pWre->GetTxtDigOws ();

            if (!pDfa || !pOws) {
                throw FAException ("No text digitizer is found.", \
                    __FILE__, __LINE__);
            }

            txt_digitizer.SetAnyIw (FAFsmConst::IW_ANY);
            txt_digitizer.SetAnyOw (FAFsmConst::IW_ANY);
            txt_digitizer.SetIgnoreCase (g_ignore_case);
            txt_digitizer.SetRsDfa (pDfa);
            txt_digitizer.SetState2Ow (pOws);
            txt_digitizer.Prepare ();

            TupleSize++;
        }
        if (FAFsmConst::WRE_TT_DCTS & TokenType) {

            const FAArrayCA * pDictDig = pWre->GetDictDig ();

            if (!pDictDig) {
                throw FAException ("No dict digitizer is found.", \
                    __FILE__, __LINE__);
            }

            dct_digitizer.SetAnyOw (FAFsmConst::IW_ANY);
            dct_digitizer.SetTagDict (&tag_dict);
            dct_digitizer.SetSet2Ow (pDictDig);

            TupleSize++;
        }
        if (FAFsmConst::WRE_TT_TAGS & TokenType) {

            TupleSize++;
        }

        /// initialize interpreters

        if (FAFsmConst::WRE_TYPE_RS == Type) {

            const FARSDfaCA * pDfa1 = pWre->GetDfa1 ();

            g_proc_fnfa.SetTupleSize (TupleSize);
            g_proc_fnfa.SetAnyIw (FAFsmConst::IW_ANY);
            g_proc_fnfa.SetRsDfa (pDfa1);

        } else if (FAFsmConst::WRE_TYPE_MOORE == Type) {

            const FARSDfaCA * pDfa1 = pWre->GetDfa1 ();
            const FAState2OwsCA * pState2Ows = pWre->GetState2Ows ();

            g_proc_fnfa.SetTupleSize (TupleSize);
            g_proc_fnfa.SetAnyIw (FAFsmConst::IW_ANY);
            g_proc_fnfa.SetRsDfa (pDfa1);
            g_proc_fnfa.SetState2Ows (pState2Ows);

        } else if (FAFsmConst::WRE_TYPE_MEALY == Type) {

            const FARSDfaCA * pDfa1 = pWre->GetDfa1 ();
            const FAMealyDfaCA * pSigma1 = pWre->GetSigma1 ();
            const FARSDfaCA * pDfa2 = pWre->GetDfa2 ();
            const FAMealyDfaCA * pSigma2 = pWre->GetSigma2 ();
            const FAMultiMapCA * pTrBr = pWre->GetTrBrMap ();

            g_proc_trbr.SetTupleSize (TupleSize);
            g_proc_trbr.SetTokenType (TokenType);
            g_proc_trbr.SetMealy1 (pDfa1, pSigma1);
            g_proc_trbr.SetMealy2 (pDfa2, pSigma2);
            g_proc_trbr.SetTrBrMap (pTrBr);

            trbr_res.SetBase (-1);
        }

        for (int k = 0; k < TupleSize; ++k) {
            OwsChain [k] = FAFsmConst::IW_L_ANCHOR;
        }

        /// process input
        while (!g_pIs->eof ()) {

            txt_io.Read (*g_pIs, &text);

            /// print input, if asked
            if (g_print_input && !g_no_output) {
                DebugLogAssert (g_pOs);
                (*g_pOs) << "Input : ";
                txt_io.Print (*g_pOs, &text);
            }
            if (g_no_process) {
                continue;
            }

            /// digitalization: Words --> Ows

            const int WordCount = text.GetWordCount ();
            FAAssert (WordCount <= FALimits::MaxWordCount, \
                FAMsg::LimitIsExceeded);

            if (0 >= WordCount) {
                if (!g_no_output)
                    (*g_pOs) << '\n';
                continue;
            }

            int ChainSize = TupleSize;

            for (int i = 0; i < WordCount; ++i) {

                const int * pWord;
                const int WordLen = text.GetWord (i, &pWord);
                const int Tag = text.GetTag (i);

                if (FAFsmConst::WRE_TT_TEXT & TokenType) {
                    const int Ow = txt_digitizer.Process (pWord, WordLen);
                    OwsChain [ChainSize++] = Ow;
                }
                if (FAFsmConst::WRE_TT_TAGS & TokenType) {
                    const int Ow = Tag + TagOwBase;
                    OwsChain [ChainSize++] = Ow;
                }
                if (FAFsmConst::WRE_TT_DCTS & TokenType) {
                    const int Ow = dct_digitizer.Process (pWord, WordLen);
                    OwsChain [ChainSize++] = Ow;
                }
            }
            for (int j = 0; j < TupleSize; ++j) {
                OwsChain [ChainSize++] = FAFsmConst::IW_R_ANCHOR;
            }

            /// print Ows, if asked
            if (g_print_ows) {
                (*g_pOs) << "Ows: ";
                ::FAPrintArray (*g_pOs, OwsChain, ChainSize);
                (*g_pOs) << ' ';
            }

            /// interpretation: Ows --> Results

            if (FAFsmConst::WRE_TYPE_RS == Type) {

                const bool Res = g_proc_fnfa.Chain2Bool (OwsChain, ChainSize);

                if (false == g_no_output) {
                    if (Res)
                        (*g_pOs) << "accepted\n";
                    else
                        (*g_pOs) << "rejected\n";
                }

            } else if (FAFsmConst::WRE_TYPE_MOORE == Type) {

                g_proc_fnfa.Chain2OwSetChain 
                    (OwsChain, ResPtrs, ResSizes, ChainSize);

                if (false == g_no_output) {

                    bool found = false;

                    for (int i =  0; i < WordCount + 2; ++i) {

                        if (0 < ResSizes [i]) {
                            found = true;
                            const int Size = ResSizes [i];
                            const int * pRuleNums = ResPtrs [i];
                            (*g_pOs) << i << " : ";
                            ::FAPrintArray (*g_pOs, pRuleNums, Size);
                            (*g_pOs) << ' ';
                        }
                    }
                    if (false == found) {
                        (*g_pOs) << "not matched";
                    }
                    (*g_pOs) << '\n';

                } // of if (false == g_no_output) ...

            } else if (FAFsmConst::WRE_TYPE_MEALY == Type) {

                const bool Res = \
                    g_proc_trbr.Process (OwsChain, ChainSize, &trbr_res);

                if (false == g_no_output) {

                    if (!Res) {

                        (*g_pOs) << "rejected\n";

                    } else {

                        const int * pFromTo;
                        int BrId = -1;
                        int Count = trbr_res.GetNextRes (&BrId, &pFromTo);

                        while (-1 != Count) {

                            DebugLogAssert (0 == Count % 2);

                            for (int i = 0; i < Count; ++i) {

                                const int From = pFromTo [i++];
                                const int To = pFromTo [i];

                                (*g_pOs) << BrId << " < " << From \
                                    << ", " << To << " >\n";
                            }

                            Count = trbr_res.GetNextRes (&BrId, &pFromTo);

                        } // of while (-1 != Count) ...
                    }
                }
                trbr_res.Clear ();

            } // of if (FAFsmConst::WRE_TYPE_ ...

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
