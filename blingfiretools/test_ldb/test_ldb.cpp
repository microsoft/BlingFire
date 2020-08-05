/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAArray_cont_t.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtf32Utils.h"
#include "FAPrintUtils.h"
#include "FAMapIOTools.h"
#include "FAMorphLDB_t_packaged.h"
#include "FAPrmInterpreter_t.h"
#include "FAStemmerLDB.h"
#include "FAStemmerConst_t.h"
#include "FALad.h"
#include "FALadLDB.h"
#include "FADictInterpreter_t.h"
#include "FAHyphInterpreter_t.h"
#include "FAW2VInterpreter_t.h"
#include "FAWordToProb_t.h"
#include "FARegexpTags_t.h"
#include "FATagSet.h"
#include "FAImageDump.h"
#include "FAStringTokenizer.h"
#include "FATestMorph_b2w.h"
#include "FATestMorph_w2b.h"
#include "FATestMorph_w2s.h"
#include "FATestMorph_w2t.h"
#include "FATestMorph_tag_dict.h"
#include "FATestMorph_w2h.h"
#include "FAWordGuesser_prob_t.h"
#include "FAT2PTable.h"
#include "FATs2PTable.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

// Mem Lfp manager
FAAllocatorA * g_pool;

const bool InitializePool ()
{
    FAAllocator alloc;
    g_pool = &alloc;
    return true;
}

static const bool g_fPoolInitialized = InitializePool ();


const char * g_pInLDBFile = "ldb.dump";
const char * g_pInTagSetFile = NULL;
const char * g_pInDataFile = NULL;
const char * g_pOutErrLog = NULL;

bool g_print_input = false;
bool g_no_output = false;
bool g_no_process = false;

enum {
    NO_AUTO_TEST = 0,
    W2B_AUTO_TEST,
    B2W_AUTO_TEST,
    W2S_AUTO_TEST,
    W2T_AUTO_TEST,
    TAG_DICT_AUTO_TEST,
    W2H_AUTO_TEST,
    B2T_AUTO_TEST,
    W2H_ALT_AUTO_TEST,

    AUTO_TEST_COUNT,
};

int g_auto_test = NO_AUTO_TEST;

FAAllocator g_alloc;

FAMapIOTools * p_map_io = NULL;
FAImageDump * p_ldb_img = NULL;
FAMorphLDB_t < int > * p_morph_ldb = NULL;
FAStemmerLDB * p_stemmer_ldb = NULL;
FAPrmInterpreter_t < int > * p_morph = NULL;
FADictInterpreter_t < int > * p_tag_dict = NULL;
FAHyphInterpreter_t < int > * p_w2h = NULL;
FADictInterpreter_t < int > * p_pos_dict = NULL;
FAW2VInterpreter_t < int > * p_w2v = NULL;
FAWordGuesser_prob_t < int > * p_w2tp = NULL;
FAWordGuesser_prob_t < int > * p_w2tpl = NULL;
FAWordGuesser_prob_t < int > * p_w2tpr = NULL;
FAWordToProb_t < int > * p_w2p = NULL;
FAStemmerConst_t < int > * p_stemmer = NULL;
FATagSet * p_tagset = NULL;
FAT2PTable * p_t2p = NULL;
FATs2PTable * p_tt2p = NULL;
FATs2PTable * p_ttt2p = NULL;
FAWordGuesser_prob_t < int > * p_n2tp = NULL;
FALadLDB * p_lad_ldb = NULL;
FALad * p_lad = NULL;
FARegexpTags_t < int > * p_u2l;


const int MaxChainSize = 409600;
// input chain
int g_ChainBuffer [MaxChainSize];
int g_ChainSize;
/// input tags, if any
int g_FromTag = -1;
int g_ToTag = -1;
int g_Tag = -1;
int g_Tag_1 = -1;
int g_Tag_2 = -1;
// output word
int g_OutChainBuffer [MaxChainSize];
int g_OutChainSize;
// for auto-testing purposes
int g_OutChain2Buffer [MaxChainSize];
int g_OutChain2Size;
// output word in UTF-8
char g_OutUtf8Buffer [FALimits::MaxWordLen * FAUtf8Const::MAX_CHAR_SIZE];
int g_OutUtf8BufferSize;
int g_Command = -1;
// extra command value
enum {
   CMD_NORMALIZE_DIACRITICS = FAFsmConst::FUNC_COUNT * 10,
};
// true if the input word is encoded as a sequence of hexadecimal numbers
bool g_fHexEnc = false;
// indicates that object of FAStemmerConst_t is used instead of FAPrmInterpreter_t
bool g_fUseStemmer = false;
// true if the switch --use-mem-map is provided 
bool g_UseMemMap = false;

FATestMorph * g_pAutoTest = NULL;
FATestMorph_b2w * p_test_b2w = NULL;
FATestMorph_w2b * p_test_w2b = NULL;
FATestMorph_w2s * p_test_w2s = NULL;
FATestMorph_w2t * p_test_w2t = NULL;
FATestMorph_w2t * p_test_b2t = NULL;
FATestMorph_tag_dict * p_test_tag_dict = NULL;
FATestMorph_w2h * p_test_w2h = NULL;


void Create ()
{
    DebugLogAssert (NULL == p_map_io);
    p_map_io = NEW FAMapIOTools (&g_alloc);
    DebugLogAssert (NULL == p_ldb_img);
    p_ldb_img = NEW FAImageDump;
    DebugLogAssert (NULL == p_morph_ldb);
    p_morph_ldb = NEW FAMorphLDB_t < int > ();
    DebugLogAssert (NULL == p_stemmer_ldb);
    p_stemmer_ldb = NEW FAStemmerLDB ();
    DebugLogAssert (NULL == p_morph);
    p_morph = NEW FAPrmInterpreter_t < int > (&g_alloc);
    DebugLogAssert (NULL == p_stemmer);
    p_stemmer = NEW FAStemmerConst_t < int > ();
    DebugLogAssert (NULL == p_tag_dict);
    p_tag_dict = NEW FADictInterpreter_t < int > ();
    DebugLogAssert (NULL == p_w2h);
    p_w2h = NEW FAHyphInterpreter_t < int > ();
    DebugLogAssert (NULL == p_w2v);
    p_w2v = NEW FAW2VInterpreter_t < int > (&g_alloc);
    DebugLogAssert (NULL == p_pos_dict);
    p_pos_dict = NEW FADictInterpreter_t < int > ();
    DebugLogAssert (NULL == p_w2tp);
    p_w2tp = NEW FAWordGuesser_prob_t < int > ();
    DebugLogAssert (NULL == p_w2tpl);
    p_w2tpl = NEW FAWordGuesser_prob_t < int > ();
    DebugLogAssert (NULL == p_w2tpr);
    p_w2tpr = NEW FAWordGuesser_prob_t < int > ();
    DebugLogAssert (NULL == p_w2p);
    p_w2p = NEW FAWordToProb_t < int > ();
    DebugLogAssert (NULL == p_tagset);
    p_tagset = NEW FATagSet (&g_alloc);
    DebugLogAssert (NULL == p_test_b2w);
    p_test_b2w = NEW FATestMorph_b2w (&g_alloc);
    DebugLogAssert (NULL == p_test_w2b);
    p_test_w2b = NEW FATestMorph_w2b (&g_alloc);
    DebugLogAssert (NULL == p_test_w2s);
    p_test_w2s = NEW FATestMorph_w2s (&g_alloc);
    DebugLogAssert (NULL == p_test_w2t);
    p_test_w2t = NEW FATestMorph_w2t (&g_alloc);
    DebugLogAssert (NULL == p_test_b2t);
    p_test_b2t = NEW FATestMorph_w2t (&g_alloc);
    DebugLogAssert (NULL == p_test_tag_dict);
    p_test_tag_dict = NEW FATestMorph_tag_dict (&g_alloc);
    DebugLogAssert (NULL == p_test_w2h);
    p_test_w2h = NEW FATestMorph_w2h (&g_alloc);
    DebugLogAssert (NULL == p_t2p);
    p_t2p = NEW FAT2PTable ();
    DebugLogAssert (NULL == p_tt2p);
    p_tt2p = NEW FATs2PTable ();
    DebugLogAssert (NULL == p_ttt2p);
    p_ttt2p = NEW FATs2PTable ();
    DebugLogAssert (NULL == p_lad_ldb);
    p_lad_ldb = NEW FALadLDB ();
    DebugLogAssert (NULL == p_lad);
    p_lad = NEW FALad ();
    DebugLogAssert (NULL == p_n2tp);
    p_n2tp = NEW FAWordGuesser_prob_t < int > ();
    DebugLogAssert (NULL == p_u2l);
    p_u2l = NEW FARegexpTags_t < int > ();

}


void Delete ()
{
    delete p_map_io;
    delete p_ldb_img;
    delete p_morph_ldb;
    delete p_stemmer_ldb;
    delete p_morph;
    delete p_stemmer;
    delete p_tag_dict;
    delete p_w2h;
    delete p_w2v;
    delete p_pos_dict;
    delete p_w2tp;
    delete p_w2tpl;
    delete p_w2tpr;
    delete p_w2p;
    delete p_tagset;
    delete p_test_b2w;
    delete p_test_w2b;
    delete p_test_w2s;
    delete p_test_w2t;
    delete p_test_b2t;
    delete p_test_tag_dict;
    delete p_test_w2h;
    delete p_t2p;
    delete p_tt2p;
    delete p_ttt2p;
    delete p_lad_ldb;
    delete p_lad;
    delete p_n2tp;
    delete p_u2l;
}


void usage ()
{
  std::cout << "\n\
Usage: test_ldb [OPTIONS]\n\
\n\
Linguistic database testing program. The program reads user commands from the\n\
standard input (stdin) in the form see below, and executes them. The output is\n\
printed out to the standard output (stdout). Both the input and output should\n\
be in UTF-8.\n\
\n\
  --ldb=<input> - reads LDB dump from file <input>,\n\
    if omited ldb.dump is used\n\
\n\
  --tagset=<input-file> - reads input tagset from the <input-file>,\n\
    not used by default\n\
\n\
  --data=<input-text> - reads input text from <input-text> file,\n\
    if omited stdin is used\n\
\n\
  --command=<cmd> - predefined command name, if this parameter is specified\n\
    then only arguments are read from the input, not used by default\n\
\n\
  --use-light-stemmer - indicates that an object of the FAStemmerConst_t is used\n\
    instead of FAPrmInterpreter_t\n\
\n\
  --input-enc=HEX - the input word is encoded as a sequence of hexadecimal\n\
    numbers\n\
\n\
  --error-log=<log-file> - specifies the output error log file name,\n\
    this file will contain errors detected by auto-test\n\
\n\
  --use-mem-map - uses memory mapping mechanism to load the LDB file\n\
\n\
  --print-input - prints input string to stdout\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
\n\
";

std::cout << "\
  --auto-test=N - makes N-th auto-test procedure,\n\
    1 - W2B test against the dictionary (usually of unseen words),\n\
        reports precision and recall, input:\n\
        Word1\\tBase1\\tBase2\\n\n\
        ...\n\
    2 - the same as above but B2W test\n\
        Base1\\tWord1\\tWord2\\n\n\
        Base2\\n\
        ...\n\
    3 - W2S test, input:\n\
        Word1Word2...WordN\\tWord1\\tWord2\\t...WordN\\n\n\
        ...\n\
    4 - W2T test, input:\n\
        Word1\\tTag1\\tTag2\\t...TagN\\n\n\
        ...\n\
    5 - Tag Dict test, input:\n\
        Word1\\tTag1\\tTag2\\t...TagN\\n\n\
        ...\n\
    6 - W2H test, input:\n\
        a[XX]b[YY]...c[ZZ]\\n\n\
        ...\n\
    7 - B2T test, input:\n\
        Word1\\tTag1\\tTag2\\t...TagN\\n\n\
        ...\n\
    8 - W2H-ALT test, input:\n\
        a[XX]b[YY]...c[ZZ]\\n\n\
        ...\n\
\n";

std::cout << "\
List of commands:\n\
\n\
  w2t <word> - prints out POS tags of a word\n\
  b2t <word> - prints out POS tags the base-form can be converted to\n\
  w2b <word> - prints out base forms of a word\n\
  b2w <word> - prints out word forms for a base form\n\
  w2w <word> - prints out word forms for a word form\n\
  w2s <word> - splits word into known segments\n\
  wt2b <tag> <word> - prints out base forms of a word\n\
  b2wt <tag> <word> - prints out word forms of a base form\n\
  wtt2w <tag> <tag> <word> - prints out word forms for a word form\n\
  tag-dict <word> - prints a set of tags for the given word\n\
  tag-dict2 <word> - prints a set id of tags for the given word\n\
  w2h <word> - prints a hyphenated word\n\
  w2h-alt <word> - prints a hyphenated word (by alternative rules)\n\
  w2h2 <word> - prints a chain of hyphenation ids\n\
  w2h2-alt <word> - prints a chain of hyphenation ids (by alternative rules)\n\
  pos-dict <word> - prints an array of tags and probs for the given word\n\
  w2tp <word> - prints POS tags and P(T|W) probabilities of the given word\n\
  w2tpl <word> - prints POS tags and P(T|W-1) probabilities\n\
  w2tpr <word> - prints POS tags and P(T|W+1) probabilities\n\
  t2p <tag> - prints tag's probability, P(T)\n\
  tt2p <tag-1> <tag> - prints tag's probability, P(T|T-1)\n\
  ttt2p <tag-2> <tag-1> <tag> - prints tag's probability, P(T|T-2,T-1)\n\
  w2v <word> - prints spelling variants of a word\n\
  w2p <word> - prints P(W)\n\
  n2tp <ngram> - prints tags and P(T|W) probabilities of the given ngram\n\
  lad <line> - runs language detection, returns the best language\n\
  lad2 <line> - runs language detection, returns scores for all languages\n\
  u2l <url> - returns language guesses for the given URL\n\
  norm <algo> <word> - diacritics normalization, <algo> is an integer\n\
  ldb - prints out all implemented functions for the given LDB file\n\
\n\
";
}


static const int Str2Ccommand (const char * pStr, int StrLen)
{
    int Command = -1;

    // check for Byte-Order-Mark (Utf-8 encoded U+FEFF symbol)
    if (6 <= StrLen) {
        if (0xEF == (unsigned char) pStr [0] && 
            0xBB == (unsigned char) pStr [1] && 
            0xBF == (unsigned char) pStr [2]) {
            pStr += 3;
            StrLen -= 3;
        }
    }

    if (0 == strncmp ("w2t", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2T;
    else if (0 == strncmp ("w2b", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2B;
    else if (0 == strncmp ("b2w", pStr, StrLen))
        Command = FAFsmConst::FUNC_B2W;
    else if (0 == strncmp ("w2w", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2W;
    else if (0 == strncmp ("w2s", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2S;
    else if (0 == strncmp ("wt2b", pStr, StrLen))
        Command = FAFsmConst::FUNC_WT2B;
    else if (0 == strncmp ("b2wt", pStr, StrLen))
        Command = FAFsmConst::FUNC_B2WT;
    else if (0 == strncmp ("wtt2w", pStr, StrLen))
        Command = FAFsmConst::FUNC_WTT2W;
    else if (0 == strncmp ("tag-dict", pStr, StrLen))
        Command = FAFsmConst::FUNC_TAG_DICT;
    else if (0 == strncmp ("tag-dict2", pStr, StrLen))
        Command = FAFsmConst::FUNC_TAG_DICT + FAFsmConst::FUNC_COUNT;
    else if (0 == strncmp ("pos-dict", pStr, StrLen))
        Command = FAFsmConst::FUNC_POS_DICT;
    else if (0 == strncmp ("w2h", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2H;
    else if (0 == strncmp ("w2h2", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2H + FAFsmConst::FUNC_COUNT;
    else if (0 == strncmp ("w2h-alt", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2H_ALT;
    else if (0 == strncmp ("w2h2-alt", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2H_ALT + FAFsmConst::FUNC_COUNT;
    else if (0 == strncmp ("b2t", pStr, StrLen))
        Command = FAFsmConst::FUNC_B2T;
    else if (0 == strncmp ("w2tp", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2TP;
    else if (0 == strncmp ("n2tp", pStr, StrLen))
        Command = FAFsmConst::FUNC_N2TP;
    else if (0 == strncmp ("w2tpl", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2TPL;
    else if (0 == strncmp ("w2tpr", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2TPR;
    else if (0 == strncmp ("t2p", pStr, StrLen))
        Command = FAFsmConst::FUNC_T2P;
    else if (0 == strncmp ("w2p", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2P;
    else if (0 == strncmp ("tt2p", pStr, StrLen))
        Command = FAFsmConst::FUNC_TT2P;
    else if (0 == strncmp ("ttt2p", pStr, StrLen))
        Command = FAFsmConst::FUNC_TTT2P;
    else if (0 == strncmp ("w2v", pStr, StrLen))
        Command = FAFsmConst::FUNC_W2V;
    else if (0 == strncmp ("lad", pStr, StrLen))
        Command = FAFsmConst::FUNC_LAD;
    else if (0 == strncmp ("lad2", pStr, StrLen))
        Command = FAFsmConst::FUNC_LAD + FAFsmConst::FUNC_COUNT;
    else if (0 == strncmp ("u2l", pStr, StrLen))
        Command = FAFsmConst::FUNC_U2L;
    else if (0 == strncmp ("norm", pStr, StrLen))
        Command = CMD_NORMALIZE_DIACRITICS;

    return Command;
}


void process_args (int& argc, char**& argv)
{
    for (; argc--; ++argv) {

        if (!strcmp ("--help", *argv)) {
            usage ();
            exit (0);
        }
        if (0 == strncmp ("--ldb=", *argv, 6)) {
            g_pInLDBFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--tagset=", *argv, 9)) {
            g_pInTagSetFile = &((*argv) [9]);
            continue;
        }
        if (0 == strncmp ("--data=", *argv, 7)) {
            g_pInDataFile = &((*argv) [7]);
            continue;
        }
        if (0 == strcmp ("--input-enc=HEX", *argv)) {
            g_fHexEnc = true;
            continue;
        }
        if (0 == strncmp ("--command=", *argv, 10)) {
            const char * pStr = &((*argv) [10]);
            const int StrLen = (int) strlen (pStr);
            g_Command = Str2Ccommand (pStr, StrLen);
            continue;
        }
        if (0 == strcmp ("--use-light-stemmer", *argv)) {
            g_fUseStemmer = true;
            continue;
        }
        if (0 == strcmp ("--print-input", *argv)) {
            g_print_input = true;
            continue;
        }
        if (0 == strcmp ("--no-process", *argv)) {
            g_no_process = true;
            continue;
        }
        if (0 == strcmp ("--no-output", *argv)) {
            g_no_output = true;
            continue;
        }
        if (0 == strcmp ("--use-mem-map", *argv)) {
            g_UseMemMap = true;
            continue;
        }
        if (0 == strncmp ("--auto-test=", *argv, 12)) {
            g_auto_test = atoi (&((*argv) [12]));
            DebugLogAssert (NO_AUTO_TEST <= g_auto_test && \
                    AUTO_TEST_COUNT > g_auto_test);
            continue;
        }
        if (0 == strncmp ("--error-log=", *argv, 12)) {
            g_pOutErrLog = &((*argv) [12]);
            continue;
        }
    }
}


void Load ()
{
    // load LDB
    p_ldb_img->Load (g_pInLDBFile, g_UseMemMap);

    p_morph_ldb->SetImage (p_ldb_img->GetImageDump ());
    p_stemmer_ldb->SetImage (p_ldb_img->GetImageDump ());
    p_lad_ldb->SetImage (p_ldb_img->GetImageDump ());

    p_morph->SetLDB (p_morph_ldb);
    p_w2h->SetLDB (p_morph_ldb);
    p_w2v->SetLDB (p_morph_ldb);
    p_tag_dict->SetConf (p_morph_ldb->GetTagDictConf (), p_morph_ldb->GetInTr ());
    p_pos_dict->SetConf (p_morph_ldb->GetPosDictConf (), p_morph_ldb->GetInTr ());
    p_w2tp->Initialize (p_morph_ldb->GetW2TPConf (), p_morph_ldb->GetInTr ());
    p_w2tpl->Initialize (p_morph_ldb->GetW2TPLConf (), p_morph_ldb->GetInTr ());
    p_w2tpr->Initialize (p_morph_ldb->GetW2TPRConf (), p_morph_ldb->GetInTr ());
    p_t2p->SetConf (p_morph_ldb->GetT2PConf ());
    p_tt2p->SetConf (p_morph_ldb->GetTT2PConf ());
    p_ttt2p->SetConf (p_morph_ldb->GetTTT2PConf ());

    if (g_fUseStemmer) {
        p_stemmer->Initialize (p_stemmer_ldb, g_pool);
    }
    p_w2p->SetConf (p_stemmer_ldb->GetW2PConf (), NULL);

    p_lad->Initialize (p_lad_ldb);
    p_n2tp->Initialize (p_lad_ldb->GetN2TPConf (), NULL);
    p_u2l->Initialize (p_lad_ldb->GetU2LConf ());

    // until there is no DUMP representation for tagsets
    // TODO: fix this or not?
    if (NULL != g_pInTagSetFile) {
        std::ifstream tagset_ifs (g_pInTagSetFile, std::ios::in);
        FAAssertStream (&tagset_ifs, g_pInTagSetFile);
        p_map_io->Read (tagset_ifs, p_tagset);
    }
}


void Process (const char * pLineStr, const int LineLen)
{
    DebugLogAssert (pLineStr && 0 < LineLen);

    const char * pTmpStr = NULL;
    int TmpStrLen = -1;
    bool Res;
    int Command;

    g_FromTag = -1;
    g_ToTag = -1;
    g_Tag = -1;
    g_Tag_1 = -1;
    g_Tag_2 = -1;

    FAStringTokenizer tokenizer;
    tokenizer.SetString (pLineStr, LineLen);

    /// read command if needed
    if (-1 != g_Command) {
        Command = g_Command;
    } else {
        // read the command token
        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        Command = Str2Ccommand (pTmpStr, TmpStrLen);
    }

    // norm command
    if(CMD_NORMALIZE_DIACRITICS == Command) {

        // read the algo value to use
        int NormAlgo = 0;
        Res = tokenizer.GetNextInt (&NormAlgo);
        DebugLogAssert (Res);

        // read the token to normalize in UTF-8
        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);

        if (g_no_process)
            return;

        // do the normalization in UTF-8
        const int MaxOutSize = (sizeof(g_OutUtf8Buffer)/sizeof(g_OutUtf8Buffer[0])) - 1;
        g_OutUtf8BufferSize = ::FAStrUtf8Normalize (pTmpStr, TmpStrLen, g_OutUtf8Buffer, MaxOutSize, NormAlgo);

        if (true == g_no_output)
            return;

        if(0 > g_OutUtf8BufferSize) {
            // the sequence is not in UTF-8
            std::cout << "ERROR: INVALID\n";
        } else if (g_OutUtf8BufferSize > MaxOutSize) {
            // the sequence is too long for the current buffer
            std::cout << "ERROR: Too small, need at least " << g_OutUtf8BufferSize << " byte buffer\n";
        } else  {
            g_OutUtf8Buffer [g_OutUtf8BufferSize] = 0;
            std::cout << g_OutUtf8Buffer << '\n';
        }

        // done with this command
        return;
    }

    // read next token, if needed
    if (FAFsmConst::FUNC_WT2B == Command || \
        FAFsmConst::FUNC_B2WT == Command || \
        FAFsmConst::FUNC_WTT2W == Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);

        // check for BOM, as this token can be first as well
        if (6 <= TmpStrLen) {
            if (0xEF == (unsigned char) pTmpStr [0] && 
                0xBB == (unsigned char) pTmpStr [1] && 
                0xBF == (unsigned char) pTmpStr [2]) {
                pTmpStr += 3;
                TmpStrLen -= 3;
            }
        }

        const int Tag = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == Tag) {
            std::string TagStr (pTmpStr, TmpStrLen);
            std::cerr << "ERROR: Unknown tag: " << TagStr << '\n';
            return ;
        }
        if (FAFsmConst::FUNC_B2WT == Command) {
            g_ToTag = Tag;
        } else {
            g_FromTag = Tag;
        }
    }

    // read next token, if needed
    if (FAFsmConst::FUNC_WTT2W == Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_ToTag = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == g_ToTag) {
            std::string TagStr (pTmpStr, TmpStrLen);
            std::cerr << "ERROR: Unknown tag: \"" << TagStr << "\" "
                      << " in program " << __PROG__ << '\n';
            return ;
        }
    }

    // process T[,T[,T]] commands
    if (FAFsmConst::FUNC_T2P == Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == g_Tag) {
            std::cerr << "ERROR: Unknown tag in program " << __PROG__ << '\n';
            return ;
        }
    } else if (FAFsmConst::FUNC_TT2P == Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag_1 = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == g_Tag || -1 == g_Tag_1) {
            std::cerr << "ERROR: Unknown tag in program " << __PROG__ << '\n';
            return ;
        }
    } else if (FAFsmConst::FUNC_TTT2P == Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag_2 = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag_1 = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);
        g_Tag = p_tagset->Str2Tag (pTmpStr, TmpStrLen);

        if (-1 == g_Tag || -1 == g_Tag_1 || -1 == g_Tag_2) {
            std::cerr << "ERROR: Unknown tag in program " << __PROG__ << '\n';
            return ;
        }
    }

    /// list, or unknown command
    if (-1 == Command) {

        std::cout << "commands:";

        if (p_morph_ldb->GetW2TConf ()) {
            std::cout << " w2t";
        }
        if (p_morph_ldb->GetB2TConf ()) {
            std::cout << " b2t";
        }
        if (p_morph_ldb->GetW2BConf ()) {
            std::cout << " w2b";
        }
        if (p_morph_ldb->GetB2WConf ()) {
            std::cout << " b2w";
        }
        if (p_morph_ldb->GetW2BConf () && p_morph_ldb->GetB2WConf ()) {
            std::cout << " w2w";
        }
        if (p_morph_ldb->GetW2SConf ()) {
            std::cout << " w2s";
        }
        if (p_morph_ldb->GetWT2BConf ()) {
            std::cout << " wt2b";
        }
        if (p_morph_ldb->GetB2WTConf ()) {
            std::cout << " b2wt";
        }
        if (p_morph_ldb->GetWT2BConf () && p_morph_ldb->GetB2WTConf ()) {
            std::cout << " wtt2w";
        }
        if (p_morph_ldb->GetTagDictConf ()) {
            std::cout << " tag-dict tag-dict2";
        }
        if (p_morph_ldb->GetW2HConf ()) {
            std::cout << " w2h w2h2";
        }
        if (p_morph_ldb->GetW2HAltConf ()) {
            std::cout << " w2h-alt w2h2-alt";
        }
        if (p_morph_ldb->GetPosDictConf ()) {
            std::cout << " pos-dict";
        }
        if (p_morph_ldb->GetW2TPConf ()) {
            std::cout << " w2tp";
        }
        if (p_morph_ldb->GetW2TPLConf ()) {
            std::cout << " w2tpl";
        }
        if (p_morph_ldb->GetW2TPRConf ()) {
            std::cout << " w2tpr";
        }
        if (p_morph_ldb->GetT2PConf ()) {
            std::cout << " t2p";
        }
        if (p_morph_ldb->GetTT2PConf ()) {
            std::cout << " tt2p";
        }
        if (p_morph_ldb->GetTTT2PConf ()) {
            std::cout << " ttt2p";
        }
        if (p_morph_ldb->GetW2VConf ()) {
            std::cout << " w2v";
        }
        if (p_stemmer_ldb->GetW2PConf ()) {
            std::cout << " w2p";
        }
        if (p_lad_ldb->GetN2TPConf ()) {
            std::cout << " n2tp";
        }
        if (p_lad_ldb->GetU2LConf ()) {
            std::cout << " u2l";
        }
        if (p_lad_ldb->GetLadConf ()) {
            std::cout << " lad lad2";
        }
        std::cout << '\n';
        return;
    }

    // treat the rest of the text as a <word> argument, if needed
    if (FAFsmConst::FUNC_T2P != Command && \
        FAFsmConst::FUNC_TT2P != Command && \
        FAFsmConst::FUNC_TTT2P != Command) {

        Res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (Res);

        const int WordLen = LineLen - int (pTmpStr - pLineStr);
        DebugLogAssert (WordLen);

        if (!g_fHexEnc) {
            // UTF-8 -> UTF-32
            g_ChainSize = ::FAStrUtf8ToArray \
                (pTmpStr, WordLen, g_ChainBuffer, MaxChainSize);
        } else {
            // HEX -> ARR
            g_ChainSize = ::FAReadHexChain \
                (pTmpStr, WordLen, g_ChainBuffer, MaxChainSize);
        }

        FAAssert (0 <= g_ChainSize && g_ChainSize < MaxChainSize,
            FAMsg::InternalError);
    }

    // select and execute the command
    if (FAFsmConst::FUNC_W2T == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const int Count = \
            p_morph->ProcessW2T (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags
            );

        if (false == g_no_output) {
            ::FAPrintTagList (
                std::cout, 
                p_tagset, 
                pTags, 
                Count
            );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_B2T == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const int Count = \
            p_morph->ProcessB2T (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags
            );

        if (false == g_no_output) {
            ::FAPrintTagList (
                std::cout, 
                p_tagset, 
                pTags, 
                Count
            );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2B == Command && g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessW2B (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_W2B == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessW2B (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_B2W == Command &&  g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessB2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_B2W == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessB2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_W2W == Command && g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessW2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_W2W == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessW2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_WT2B == Command && g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessWT2B (
                g_ChainBuffer, 
                g_ChainSize,
                g_FromTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_WT2B == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessWT2B (
                g_ChainBuffer, 
                g_ChainSize,
                g_FromTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_B2WT == Command && g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessB2WT (
                g_ChainBuffer, 
                g_ChainSize,
                g_ToTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_B2WT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessB2WT (
                g_ChainBuffer, 
                g_ChainSize,
                g_ToTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_WTT2W == Command && g_fUseStemmer) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_stemmer->ProcessWTT2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_FromTag,
                g_ToTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_WTT2W == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessWTT2W (
                g_ChainBuffer, 
                g_ChainSize,
                g_FromTag,
                g_ToTag,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_W2S == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_morph->ProcessW2S (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintSplitting (
                    std::cout,
                    g_ChainBuffer,
                    g_ChainSize,
                    g_OutChainBuffer,
                    g_OutChainSize
                );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_TAG_DICT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_tag_dict->GetInfo (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );
        DebugLogAssert (g_OutChainSize < MaxChainSize);

        if (false == g_no_output) {
            ::FAPrintTagList (
                std::cout,
                p_tagset,
                g_OutChainBuffer,
                g_OutChainSize
            );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_TAG_DICT + FAFsmConst::FUNC_COUNT == Command) {

        if (g_no_process)
            return;

        const int SetId = \
            p_tag_dict->GetInfoId (
                g_ChainBuffer,
                g_ChainSize);

        if (false == g_no_output) {
            std::cout << SetId << '\n';
        }

    } else if (FAFsmConst::FUNC_POS_DICT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_pos_dict->GetInfo (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );
        DebugLogAssert (g_OutChainSize < MaxChainSize);

        if (false == g_no_output) {

            if (0 < g_OutChainSize) {

                DebugLogAssert (0 == (g_OutChainSize % 2));
                const int TagCount = g_OutChainSize / 2;

                ::FAPrintTagList (
                    std::cout,
                    p_tagset,
                    g_OutChainBuffer,
                    TagCount
                );

                std::cout << ' ';
                ::FAPrintArray (
                    std::cout,
                    g_OutChainBuffer + TagCount,
                    TagCount
                );

                std::cout << " as floats: ";
                ::FAPrintArray (
                    std::cout,
                    (const float *)(g_OutChainBuffer + TagCount),
                    TagCount
                );

                std::cout << " as hex int32: ";
                ::FAPrintChain (
                    std::cout,
                    g_OutChainBuffer + TagCount,
                    TagCount,
                    FAFsmConst::DIR_L2R,
                    8,
                    true
                );

            } else {

                std::cout << "NONE" ;
            }
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2H == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_w2h->Process (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );
        DebugLogAssert ((g_OutChainSize < MaxChainSize && \
                 g_OutChainSize == g_ChainSize) || \
                (-1 == g_OutChainSize));

        if (false == g_no_output) {
            if (0 < g_OutChainSize)
                ::FAPrintHyphWord (
                    std::cout,
                    g_ChainBuffer,
                    g_OutChainBuffer,
                    g_OutChainSize
                );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2H  + FAFsmConst::FUNC_COUNT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_w2h->Process (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );
        DebugLogAssert ((g_OutChainSize < MaxChainSize && \
                 g_OutChainSize == g_ChainSize) || \
                (-1 == g_OutChainSize));

        if (false == g_no_output) {
            if (0 < g_OutChainSize)
                ::FAPrintArray (std::cout, g_OutChainBuffer, g_OutChainSize);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2H_ALT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_w2h->Process (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize,
                true
            );
        DebugLogAssert ((g_OutChainSize < MaxChainSize && \
                 g_OutChainSize == g_ChainSize) || \
                (-1 == g_OutChainSize));

        if (false == g_no_output) {
            if (0 < g_OutChainSize)
                ::FAPrintHyphWord (
                    std::cout,
                    g_ChainBuffer,
                    g_OutChainBuffer,
                    g_OutChainSize
                );
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2H_ALT + FAFsmConst::FUNC_COUNT == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_w2h->Process (
                g_ChainBuffer,
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize,
                true
            );
        DebugLogAssert ((g_OutChainSize < MaxChainSize && \
                 g_OutChainSize == g_ChainSize) || \
                (-1 == g_OutChainSize));

        if (false == g_no_output) {
            if (0 < g_OutChainSize)
                ::FAPrintArray (std::cout, g_OutChainBuffer, g_OutChainSize);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2TP == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const float * pProbs;
        const int Count = \
            p_w2tp->Process (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags,
                &pProbs
            );

        if (false == g_no_output) {
            ::FAPrintTagList (std::cout, p_tagset, pTags, Count);
            std::cout << '\t';
            ::FAPrintArray (std::cout, pProbs, Count);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2TPL == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const float * pProbs;
        const int Count = \
            p_w2tpl->Process (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags,
                &pProbs
            );

        if (false == g_no_output) {
            ::FAPrintTagList (std::cout, p_tagset, pTags, Count);
            std::cout << '\t';
            ::FAPrintArray (std::cout, pProbs, Count);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2TPR == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const float * pProbs;
        const int Count = \
            p_w2tpr->Process (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags,
                &pProbs
            );

        if (false == g_no_output) {
            ::FAPrintTagList (std::cout, p_tagset, pTags, Count);
            std::cout << '\t';
            ::FAPrintArray (std::cout, pProbs, Count);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_W2P == Command) {

        if (g_no_process)
            return;

        const float P = p_w2p->GetProb (g_ChainBuffer, g_ChainSize);

        if (false == g_no_output) {
            std::cout << P << '\n';
        }

    } else if (FAFsmConst::FUNC_T2P == Command) {

        if (g_no_process)
            return;

        const float P = p_t2p->GetProb (g_Tag);

        if (false == g_no_output) {
            std::cout << P << '\n';
        }

    } else if (FAFsmConst::FUNC_TT2P == Command) {

        if (g_no_process)
            return;

        const float P = p_tt2p->GetProb (g_Tag_1, g_Tag);

        if (false == g_no_output) {
            std::cout << P << '\n';
        }

    } else if (FAFsmConst::FUNC_TTT2P == Command) {

        if (g_no_process)
            return;

        const float P = p_ttt2p->GetProb (g_Tag_2, g_Tag_1, g_Tag);

        if (false == g_no_output) {
            std::cout << P << '\n';
        }

    } else if (FAFsmConst::FUNC_W2V == Command) {

        if (g_no_process)
            return;

        g_OutChainSize = \
            p_w2v->ProcessW2V (
                g_ChainBuffer, 
                g_ChainSize,
                g_OutChainBuffer,
                MaxChainSize
            );

        if (false == g_no_output) {
            ::FAPrintWordList (
                std::cout, 
                g_OutChainBuffer, 
                g_OutChainSize, 
                '\n'
            );
        }

    } else if (FAFsmConst::FUNC_LAD == Command) {

        if (g_no_process)
            return;

        const int WordLen = LineLen - int (pTmpStr - pLineStr);

        const int Lang = p_lad->Process (pTmpStr, WordLen);

        if (false == g_no_output) {
            if (-1 != Lang) {
                ::FAPrintTagList (std::cout, p_tagset, &Lang, 1);
            } else {
                std::cout << "<UNKNOWN>";
            }
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_LAD + FAFsmConst::FUNC_COUNT == Command) {

        if (g_no_process)
            return;

        const int WordLen = LineLen - int (pTmpStr - pLineStr);

        const int Lang = p_lad->Process (pTmpStr, WordLen);

        if (false == g_no_output) {

            if (-1 != Lang) {

                const float * pScores;
                const int * pCounts;
                const int Size = p_lad->GetScores (&pScores, &pCounts);

                bool fPrinted = false;

                for (int Tag = 1; Tag < Size; ++Tag) {

                    const char * pStr;
                    const int StrLen = p_tagset->Tag2Str (Tag, &pStr);

                    if (-1 == StrLen) {
                        continue;
                    }

                    if (fPrinted) {
                        std::cout << ' ';
                    }

                    std::cout << std::string (pStr, StrLen) 
                              << '/' << pScores [Tag] << '/' << pCounts [Tag];

                    fPrinted = true;
                }

            } else {
                std::cout << "<UNKNOWN>";
            }
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_N2TP == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const float * pProbs;
        const int Count = \
            p_n2tp->Process (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags,
                &pProbs
            );

        if (false == g_no_output) {
            ::FAPrintTagList (std::cout, p_tagset, pTags, Count);
            std::cout << '\t';
            ::FAPrintArray (std::cout, pProbs, Count);
            std::cout << '\n';
        }

    } else if (FAFsmConst::FUNC_U2L == Command) {

        if (g_no_process)
            return;

        const int * pTags;
        const int * pScores;
        const int Count = \
            p_u2l->Process (
                g_ChainBuffer, 
                g_ChainSize, 
                &pTags,
                &pScores
            );

        if (false == g_no_output) {
            ::FAPrintTagList (std::cout, p_tagset, pTags, Count);
            std::cout << '\t';
            ::FAPrintArray (std::cout, pScores, Count);
            std::cout << '\n';
        }

    } // of if (FAFsmConst::FUNC_W2T == Command) ...
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    // process command line
    process_args (argc, argv);

    int LineNum = 0;
    std::string line;        

    try {

        Create ();

        Load ();

        std::istream * pDataIs = &std::cin;
        std::ifstream data_ifs;

        if (NULL != g_pInDataFile) {
            data_ifs.open (g_pInDataFile, std::ios::in);
            FAAssertStream (&data_ifs, g_pInDataFile);
            pDataIs = &data_ifs;
        }

        std::ostream * pErrLogOs = NULL;
        std::ofstream error_log;

        if (NULL != g_pOutErrLog) {
            error_log.open (g_pOutErrLog, std::ios::out);
            pErrLogOs = &error_log;
        }

        DebugLogAssert (pDataIs);

        switch (g_auto_test) {
            case W2B_AUTO_TEST:
                g_pAutoTest = p_test_w2b;
                break;
            case B2W_AUTO_TEST:
                g_pAutoTest = p_test_b2w;
                break;
            case W2S_AUTO_TEST:
                g_pAutoTest = p_test_w2s;
                break;
            case W2T_AUTO_TEST:
                g_pAutoTest = p_test_w2t;
                break;
            case TAG_DICT_AUTO_TEST:
                g_pAutoTest = p_test_tag_dict;
                break;
            case W2H_AUTO_TEST:
            case W2H_ALT_AUTO_TEST:
                g_pAutoTest = p_test_w2h;
                break;
            case B2T_AUTO_TEST:
                g_pAutoTest = p_test_b2t;
                break;
        };
        DebugLogAssert (NO_AUTO_TEST == g_auto_test || NULL != g_pAutoTest);

        p_test_w2t->SetTagSet (p_tagset);
        p_test_w2t->SetFunc (FAFsmConst::FUNC_W2T);
        p_test_b2t->SetTagSet (p_tagset);
        p_test_b2t->SetFunc (FAFsmConst::FUNC_B2T);
        p_test_tag_dict->SetTagSet (p_tagset);
        p_test_tag_dict->SetTagDict (p_tag_dict);
        p_test_w2h->SetW2H (p_w2h);

        // global settings
        if (NULL != g_pAutoTest) {
            g_pAutoTest->SetPRM (p_morph);
            g_pAutoTest->SetOutStream (pErrLogOs);
        }
        // per-function settings
        if (NULL != g_pAutoTest) {

            if (TAG_DICT_AUTO_TEST == g_auto_test) {

                const FADictConfKeeper* pConf = p_morph_ldb->GetTagDictConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (W2S_AUTO_TEST == g_auto_test) {

                const FAW2SConfKeeper * pConf = p_morph_ldb->GetW2SConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (W2B_AUTO_TEST == g_auto_test) {

                const FAWftConfKeeper * pConf = p_morph_ldb->GetW2BConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (B2W_AUTO_TEST == g_auto_test) {

                const FAWftConfKeeper * pConf = p_morph_ldb->GetB2WConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (W2T_AUTO_TEST == g_auto_test) {

                const FAWgConfKeeper * pConf = p_morph_ldb->GetW2TConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (B2T_AUTO_TEST == g_auto_test) {

                const FAWgConfKeeper * pConf = p_morph_ldb->GetB2TConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (W2H_AUTO_TEST == g_auto_test) {

                const FAHyphConfKeeper * pConf = p_morph_ldb->GetW2HConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());

            } else if (W2H_ALT_AUTO_TEST == g_auto_test) {

                const FAHyphConfKeeper * pConf = p_morph_ldb->GetW2HAltConf ();
                DebugLogAssert (pConf);
                g_pAutoTest->SetIgnoreCase (pConf->GetIgnoreCase ());
                p_test_w2h->SetUseAltW2H (true);
            }
        }

        // read the input
        while (!pDataIs->eof ()) {

            if (!std::getline (*pDataIs, line))
                break;

            LineNum++;

            const char * pLineStr = line.c_str ();
            int LineLen = (const int) line.length ();

            // take care of '\r' symbol at the end
            if (0 < LineLen) {
                DebugLogAssert (pLineStr);
                if (0x0D == (unsigned char) pLineStr [LineLen - 1])
                    LineLen--;
            }

            // repeat input, if needed
            if (g_print_input && false == g_no_output) {
                if (0 < LineLen) {
                    std::cout << pLineStr;
                }
                std::cout << '\n';
            }

            // do the processing
            if (0 < LineLen) {

                if (g_pAutoTest) {

                    g_pAutoTest->Test (pLineStr, LineLen);

                } else {

                    Process (pLineStr, LineLen);
                }

            } // of if (0 < StrLen) ...

        } // of while (!pDataIs->eof ())

        if (g_pAutoTest && false == g_no_output) {
            g_pAutoTest->PrintReport (&std::cout);
        }

        // all memory should be freed
        Delete ();

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

        std::cerr << "ERROR: in data at line: " << LineNum << " in \"" \
            << line << "\"\n";

        return 1;
    }

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
