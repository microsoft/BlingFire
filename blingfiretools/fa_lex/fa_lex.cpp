/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtils_cl.h"
#include "FAException.h"
#include "FAImageDump.h"
#include "FAWbdConfKeeper.h"
#include "FAMultiMap_pack.h"
#include "FARSDfa_pack_triv.h"
#include "FAState2Ow_pack_triv.h"
#include "FALexTools_t.h"
#include "FACorpusIOTools_utf8.h"
#include "FAMapIOTools.h"
#include "FATagSet.h"
#include "FATaggedText.h"
#include "FALDB.h"

#include <iostream>
#include <string>
#include <fstream>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;
const char * g_pLdbFile = NULL;
const char * g_pStageFile = "out.dump";
const char * g_pTagSetFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

const char * g_pEosTagName = NULL;
int g_EosTag = 0;

bool g_ignore_case = false;
bool g_no_output = false;
bool g_no_process = false;
int g_max_depth = 2;
bool g_no_postags = false;
bool g_print_input = false;
bool g_p2s_mode = false;
bool g_normalize_input = false;

const int MaxBuffSize = FALimits::MaxWordLen * FALimits::MaxWordCount;
int g_RawBuff [MaxBuffSize];
int g_NormBuff [MaxBuffSize];
int g_Offsets [MaxBuffSize];

const int MaxOutputSize = 30 * FALimits::MaxWordCount;
int g_Out [MaxOutputSize];

char g_OutUtf8 [MaxOutputSize * FAUtf8Const::MAX_CHAR_SIZE];

std::string line;
int LineNum = 0;

// allocator
FAAllocator g_alloc;


void usage () {

  std::cout << "\n\
Usage: fa_lex [OPTIONS] [< input.utf8] [> output.utf8]\n\
\n\
This program makes a lexical analysis.\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --ldb=<ldb> - reads comiled rules from the <ldb> file\n\
    tokenization rules should be under [wbd] section,\n\
    cannot be used with --stage= parameter\n\
    not used by default\n\
\n\
  --stage=<input> - reads compiled rules dump (not a part of any LDB),\n\
    cannot be used with --ldb= parameter\n\
    out.dump is used by default\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file,\n\
    have to be specified\n\
\n\
  --ignore-case - converts input symbols to the lower case,\n\
    cannot be used with --ldb= parameter (ignore-case is taken from LDB)\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --max-depth=N - maximum call depth, if functions are present,\n\
    cannot be used with --ldb= parameter (max-depth is taken from LDB)\n\
    2 is used by default\n\
\n\
  --print-input - prints input line, before printing the output\n\
\n\
  --no-postags - output will contain no tokenization tags\n\
\n\
  --p2s-mode - splits paragraphs into sentences\n\
    Note: output tuples are used to separate an input paragraph into\n\
    sentences, each sentence starts from the first character past \n\
    previous sentence end and ends at the to position of the tuple\n\
\n\
  --normalize-input - normalizes entire input with internal charmap\n\
    before doing tokenization. Note this may result into incorrect offset\n\
    if the normalized string has different length\n\
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
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--stage=", *argv, 8)) {
            g_pStageFile = &((*argv) [8]);
            continue;
        }
        if (0 == strncmp ("--ldb=", *argv, 6)) {
            g_pLdbFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--tagset=", *argv, 9)) {
            g_pTagSetFile = &((*argv) [9]);
            continue;
        }
        if (0 == strcmp ("--ignore-case", *argv)) {
            g_ignore_case = true;
            continue;
        }
        if (0 == strncmp ("--max-depth=", *argv, 12)) {
            g_max_depth = atoi (&((*argv) [12]));
            continue;
        }
        if (0 == strncmp ("--no-output", *argv, 11)) {
            g_no_output = true;
            continue;
        }
        if (0 == strncmp ("--no-process", *argv, 12)) {
            g_no_process = true;
            continue;
        }
        if (0 == strncmp ("--no-postags", *argv, 12)) {
            g_no_postags = true;
            continue;
        }
        if (0 == strncmp ("--eos-tag=", *argv, 10)) {
            g_pEosTagName = &((*argv)[10]);
            continue;
        }
        if (0 == strncmp ("--print-input", *argv, 13)) {
            g_print_input = true;
            continue;
        }
        if (0 == strncmp ("--p2s-mode", *argv, 10)) {
            g_p2s_mode = true;
            continue;
        }
        if (0 == strncmp ("--normalize-input", *argv, 17)) {
            g_normalize_input = true;
            continue;
        }
    }
}


int FAGetFirstNonWhiteSpace(int * pStr, const int StrLen)
{
    for(int i = 0; i < StrLen; ++i)
    {
        int C = pStr[i];

        // WHITESPACE [\x0004-\x0020\x007F-\x009F\x00A0\x2000-\x200B\x200E\x200F\x202F\x205F\x2060\x2420\x2424\x3000\xFEFF]
        if(C <= 0x20u || (C >= 0x7fu && C <= 0x9fu) || C == 0xa0u || (C >= 0x2000u && C <= 0x200bu) ||
           C == 0x200eu || C == 0x200fu || C == 0x202fu || C == 0x205fu || C == 0x2060u || C == 0x2420u ||
           C == 0x2424u || C == 0x3000u || C == 0xfeffu) {
            continue;
        }

        return i;
    }

    return StrLen;
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        FATagSet tagset (&g_alloc);
        FACorpusIOTools_utf8 text_io (&g_alloc);
        FAMapIOTools map_io (&g_alloc);
        FATaggedText text (&g_alloc);

        text_io.SetTagSet (&tagset);
        text_io.SetNoPosTags (g_no_postags);

        FAImageDump StageImg;

        FARSDfa_pack_triv Dfa;
        FAState2Ow_pack_triv State2Ow;
        FAMultiMap_pack Acts;
        FAWbdConfKeeper Conf;
        FALDB Ldb;

        FALexTools_t < int > lex;

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
        if (g_pTagSetFile) {
            std::ifstream tagset_ifs (g_pTagSetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagSetFile);
            map_io.Read (tagset_ifs, &tagset);
        }

        // get EOS tag ID from input args
        if (g_pEosTagName) {
            const int EosStrLen = (int) strlen (g_pEosTagName);
            g_EosTag = tagset.Str2Tag (g_pEosTagName, EosStrLen);
            if (-1 == g_EosTag) {
                std::cerr << "ERROR: Unknown EOS tag " << g_pEosTagName << '\n';
                return 1;
            }
        }

        // load and set up the compiled rules
        if (!g_pLdbFile) {

            FAAssert (g_pStageFile, FAMsg::InvalidParameters);

            StageImg.Load (g_pStageFile);
            const unsigned char * pImg = StageImg.GetImageDump ();
            FAAssert (pImg, FAMsg::IOError);

            const int * pA = (const int *) pImg ;
            const int Count = *pA;
            FAAssert (2 == Count, FAMsg::IOError);

            Dfa.SetImage (pImg + *++pA);
            State2Ow.SetImage (pImg + *pA);
            Acts.SetImage (pImg + *++pA);

            Conf.SetRsDfa (&Dfa);
            Conf.SetState2Ow (&State2Ow);
            Conf.SetMMap (&Acts);
            Conf.SetIgnoreCase (g_ignore_case);
            Conf.SetMaxDepth (g_max_depth);

        } else {

            StageImg.Load (g_pLdbFile);
            const unsigned char * pImg = StageImg.GetImageDump ();
            FAAssert (pImg, FAMsg::IOError);

            Ldb.SetImage (pImg);

            const int * pValues = NULL;
            const int iSize = Ldb.GetHeader ()->Get (FAFsmConst::FUNC_WBD, &pValues);
            Conf.Initialize (&Ldb, pValues, iSize);
        }

        // setup parameters and data
        lex.SetConf (&Conf);

        while (!(g_pIs->eof ())) {

            if (!std::getline (*g_pIs, line))
                break;

            LineNum++;

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            // echo the input, if needed
            if (g_print_input && false == g_no_output) {
                (*g_pOs) << line << '\n';
            }

            if (0 < LineLen) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }
            if (0 < LineLen) {

                // UTF-8 --> UTF-32
                int BuffSize = ::FAStrUtf8ToArray (pLine, LineLen, g_RawBuff, g_Offsets, MaxBuffSize);
                FAAssert (0 < BuffSize && MaxBuffSize >= BuffSize, FAMsg::IOError);
                int * g_Buff = g_RawBuff;

                if (false == g_no_process) {

                    // see if we want to normalize the buffer first
                    if (g_normalize_input && 0 < BuffSize) {
                        BuffSize = ::FANormalize(g_Buff, BuffSize, g_NormBuff, MaxBuffSize, Conf.GetCharMap ());
                        FAAssert (0 < BuffSize && MaxBuffSize >= BuffSize, FAMsg::IOError);
                        g_Buff = g_NormBuff;
                    }

                    const int OutSize = \
                        lex.Process (g_Buff, BuffSize, g_Out, MaxOutputSize);
                    FAAssert (OutSize <= MaxOutputSize && 0 == OutSize % 3, \
                        FAMsg::IOError);

                    if (false == g_no_output) {

                        if(g_p2s_mode) {

                            // set previous sentence end to -1
                            int PrevEnd = -1;

                            for (int i = 0; i < OutSize; i += 3) {

                                // we don't care about Tag or From for p2s task
                                const int From = PrevEnd + 1;
                                const int To = g_Out [i + 2];
                                const int Len = To - From + 1;
                                PrevEnd = To;

                                // adjust sentence start if needed
                                const int Delta = FAGetFirstNonWhiteSpace(g_Buff + From, Len);

                                if(Delta < Len) {
                                    // convert buffer to a UTF-8 string
                                    const int StrOutSize = ::FAArrayToStrUtf8 (g_Buff + From + Delta, Len - Delta, g_OutUtf8, sizeof(g_OutUtf8)-1);
                                    FAAssert (0 < StrOutSize && StrOutSize < (int) sizeof(g_OutUtf8)-1, FAMsg::IOError);

                                    // print the sentence
                                    g_OutUtf8 [StrOutSize] = 0;
                                    (*g_pOs) << g_OutUtf8 << '\n';
                                }
                            }

                            // always use the end of paragraph as the end of sentence
                            if(PrevEnd + 1 < BuffSize)
                            {
                                const int From = PrevEnd + 1;
                                const int To = BuffSize - 1;
                                const int Len = To - From + 1;

                                // adjust sentence start if needed
                                const int Delta = FAGetFirstNonWhiteSpace(g_Buff + From, Len);

                                if(Delta < Len) {
                                    // convert buffer to a UTF-8 string
                                    const int StrOutSize = ::FAArrayToStrUtf8 (g_Buff + From + Delta, Len - Delta, g_OutUtf8, sizeof(g_OutUtf8)-1);
                                    FAAssert (0 < StrOutSize && StrOutSize < (int) sizeof(g_OutUtf8)-1, FAMsg::IOError);

                                    // print the sentence
                                    g_OutUtf8 [StrOutSize] = 0;
                                    (*g_pOs) << g_OutUtf8 << '\n';
                                }
                            }

                            (*g_pOs) << '\n';

                        } else {

                            bool print_after_loop = true;

                            text.Clear ();

                            for (int i = 0; i < OutSize; i += 3) {

                                const int Tag = g_Out [i];
                                const int From = g_Out [i + 1];
                                const int Len = g_Out [i + 2] - From + 1;

                                text.AddWord (g_Buff + From, Len, Tag, From);
                                print_after_loop = true;

                                if (NULL != g_pEosTagName && Tag == g_EosTag) {
                                    text_io.Print (*g_pOs, &text);
                                    text.Clear ();
                                    print_after_loop = false;
                                }
                            }

                            if (print_after_loop)
                                text_io.Print (*g_pOs, &text);

                        } // of if(g_p2s_mode) ...
                    }

                } // of if (false == g_no_process) ...

            } // of if (0 < LineLen) ...

        } // of while (!(g_pIs->eof ())) ...

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        std::cerr << "ERROR: in data at line: " << LineNum << " in \"" \
            << line << "\"\n";

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';
        return 1;
    }

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}

