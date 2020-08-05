/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAException.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAImageDump.h"
#include "FAChain2Num_hash.h"
#include "FATrWordIOTools_utf8.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAUtf8Utils.h"
#include "FARSDfa_pack_triv.h"
#include "FAMealyDfa_pack_triv.h"
#include "FAArray_pack.h"
#include "FAMultiMap_pack.h"
#include "FAOw2Iw_pack_triv.h"
#include "FARSDfa_ro.h"
#include "FAMealyDfa_ro.h"
#include "FAArray_p2ca.h"
#include "FAMultiMap_ar.h"
#include "FAMultiMap_pack_fixed.h"
#include "FASelectTrPatterns.h"
#include "FAMphInterpretTools_t.h"

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

// input data file
const char * pInFile = NULL;
// all input patterns storage
const char * pFsmFile = NULL;
const char * pK2IFile = NULL;
const char * pI2InfoFile = NULL;
// output patterns file
const char * pOutFile = NULL;
// output unsolved entries file
const char * pUnOutFile = NULL;
// automata format
int g_format_type = FAFsmConst::FORMAT_DUMP;

int g_spec_l_anchor = FAFsmConst::IW_L_ANCHOR;
int g_spec_r_anchor = FAFsmConst::IW_R_ANCHOR;

int g_Iws [FALimits::MaxWordLen];
int g_Ows [FALimits::MaxWordLen];
int g_Count;

bool g_ignore_case = false;

const char * g_pCharMapFile = NULL;
FAImageDump g_charmap_image;
FAMultiMap_pack_fixed g_charmap;
const FAMultiMapCA * g_pMap = NULL;

const FARSDfaCA * g_pDfa = NULL;
const FAMealyDfaCA * g_pMealy = NULL;
const FAOw2IwCA * g_pOw2Iw = NULL;
const FAArrayCA * g_pK2I = NULL;
const FAMultiMapCA * g_pI2Info = NULL;

bool g_NoEmpty = false;
bool g_TakeAll = false;


void usage () {

  std::cout << "\n\
Usage: fa_pats_select [OPTIONS]\n\
\n\
This program selects \"the best\" sufficient subset of word convertion\n\
patterns from the set of all existing patterns.\n\
\n\
  --in=<input-file> - reads the source training dictionary from \n\
    the <input-file>, if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-unsolved=<unsolved> - writes unsolved entries to the <unsolved> file,\n\
    if omited does not print them\n\
\n\
  --fsm=<fsm> - reads MPH automaton from <fsm> file,\n\
    stdin is used by default\n\
\n\
  --k2i=<k2i> - reads K2I array from <k2i> file,\n\
    stdin is used by default\n\
\n\
  --i2info=<i2info> - reads I2INFO map from the <i2info>,\n\
    stdin is used by default\n\
\n\
  --spec-l-anchor=N - specifies weight for the beginning of the sequence,\n\
    the default is 1\n\
\n\
  --spec-r-anchor=N - specifies weight for the end of the sequence,\n\
    the default is 2\n\
\n\
  --format=txt|dump - specifies the IO format for the input automata\n\
    dump - is used by default\n\
\n\
  --no-empty - will not consider non-hyphenating points\n\
\n\
  --ignore-case - converts input symbols to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --charmap=<mmap-dump> - applies a custom character normalization procedure\n\
    according to the <mmap-dump>, the dump should be in \"fixed\" format\n\
\n\
  --take-all-pats - takes all patterns, skips the stage of calculating a subset\n\
    this key should be used to improve recall on unknown words\n\
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
            pInFile = &((*argv) [5]);
            continue;
        }
        if (0 == strncmp ("--fsm=", *argv, 6)) {
            pFsmFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--k2i=", *argv, 6)) {
            pK2IFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--i2info=", *argv, 9)) {
            pI2InfoFile = &((*argv) [9]);
            continue;
        }
        if (0 == strncmp ("--out=", *argv, 6)) {
            pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--out-unsolved=", *argv, 15)) {
            pUnOutFile = &((*argv) [15]);
            continue;
        }
        if (0 == strncmp ("--spec-l-anchor=", *argv, 16)) {
            g_spec_l_anchor = atoi (&((*argv) [16]));
            continue;
        }
        if (0 == strncmp ("--spec-r-anchor=", *argv, 16)) {
            g_spec_r_anchor = atoi (&((*argv) [16]));
            continue;
        }
        if (0 == strcmp ("--format=dump", *argv)) {
            g_format_type = FAFsmConst::FORMAT_DUMP;
            continue;
        }
        if (0 == strcmp ("--format=txt", *argv)) {
            g_format_type = FAFsmConst::FORMAT_TXT;
            continue;
        }
        if (!strcmp ("--no-empty", *argv)) {
            g_NoEmpty = true;
            continue;
        }
        if (0 == strncmp ("--charmap=", *argv, 10)) {
            g_pCharMapFile = &((*argv) [10]);
            g_pMap = &g_charmap;
            continue;
        }
        if (0 == strcmp ("--ignore-case", *argv)) {
            g_ignore_case = true;
            continue;
        }
        if (0 == strcmp ("--take-all-pats", *argv)) {
            g_TakeAll = true;
            continue;
        }
    }
}


///
/// Prints out patterns to pPatOs and unsolved points to pUnOs
///

class FASelectTrPatterns_print : public FASelectTrPatterns {

public:
    FASelectTrPatterns_print (
            FAAllocatorA * pAlloc,
            std::ostream * pPatOs,
            std::ostream * pUnOs
        );

public:
    void PutPattern (
            const int * pIws,
            const int * pOws,
            const int Count
        );
    void PutUnsolved (
            const int * pIws,
            const int * pOws,
            const int Count
        );
    void PutConflict (
            const int * pIws, 
            const int Count,
            const int Pos
        );

private:
    std::ostream * m_pPatOs;
    std::ostream * m_pUnOs;
};


FASelectTrPatterns_print::
    FASelectTrPatterns_print (
            FAAllocatorA * pAlloc,
            std::ostream * pPatOs,
            std::ostream * pUnOs
        ) :
    FASelectTrPatterns (pAlloc),
    m_pPatOs (pPatOs),
    m_pUnOs (pUnOs)
{}

void FASelectTrPatterns_print::
    PutPattern (const int * pIws, const int * pOws, const int Count)
{
    DebugLogAssert (m_pPatOs);

    const int MaxLen = FAUtf8Const::MAX_CHAR_SIZE * FALimits::MaxWordSize + 3;
    char Str [MaxLen];

    const int StrLen = ::FAArrayToStrUtf8 (pIws, Count, Str, MaxLen - 1);
    FAAssert (0 < StrLen && StrLen < MaxLen, FAMsg::InternalError);

    Str [StrLen] = 0;
    (*m_pPatOs) << Str;

    for (int i = 0; i < Count; ++i) {
        const int Ow = pOws [i];
        (*m_pPatOs) << '\t' << Ow;
    }

    (*m_pPatOs) << '\n';
}

void FASelectTrPatterns_print::
    PutUnsolved (const int * pIws, const int * pOws, const int Count)
{
    if (!m_pUnOs)
        return;

    const int MaxLen = FAUtf8Const::MAX_CHAR_SIZE * FALimits::MaxWordSize + 1;
    char Str [MaxLen];

    const int StrLen = ::FAArrayToStrUtf8 (pIws, Count, Str, MaxLen - 1);
    FAAssert (0 < StrLen && StrLen < MaxLen, FAMsg::InternalError);

    // print the word
    Str [StrLen] = 0;
    (*m_pUnOs) << Str;

    // print missing hyphenation points: [Where,What]
    for (int i = 0; i < Count; ++i) {
        const int Ow = pOws [i];
        if (-1 > Ow) {
            const int Ow2 = - (Ow + 1);
            (*m_pUnOs) << '\t' << i << '\t' << Ow2;
        }
    }

    (*m_pUnOs) << '\n';
}

void  FASelectTrPatterns_print::
    PutConflict (const int * pIws, const int Count, const int Pos)
{
    if (!m_pUnOs)
        return;

    const int MaxLen = FAUtf8Const::MAX_CHAR_SIZE * FALimits::MaxWordSize + 1;
    char Str [MaxLen];

    const int StrLen = ::FAArrayToStrUtf8 (pIws, Count, Str, MaxLen - 1);
    FAAssert (0 < StrLen && StrLen < MaxLen, FAMsg::InternalError);

    Str [StrLen] = 0;
    (*m_pUnOs) << Str << "\tconflicts at " << Pos << '\n';
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        FAAssert (pFsmFile && pK2IFile && pI2InfoFile, FAMsg::IOError);

        FAAutIOTools aut_io (&g_alloc);
        FAMapIOTools map_io (&g_alloc);

        // patterns storage
        FAImageDump FsmImg;
        FAImageDump K2IImg;
        FAImageDump I2InfoImg;

        FARSDfa_pack_triv PatRsDfa;
        FAMealyDfa_pack_triv PatMealyDfa;
        FAOw2Iw_pack_triv PatRevMealy;
        FAArray_pack K2I;
        FAMultiMap_pack OwsId2Ows;

        FARSDfa_ro PatRsDfa_txt (&g_alloc);
        FAMealyDfa_ro PatMealyDfa_txt (&g_alloc);
        FAArray_p2ca K2I_txt;
        FAMultiMap_ar OwsId2Ows_txt;
        OwsId2Ows_txt.SetAllocator (&g_alloc);
        PatMealyDfa_txt.SetRsDfa (&PatRsDfa_txt);

        // load automata
        if (FAFsmConst::FORMAT_DUMP == g_format_type) {

            FsmImg.Load (pFsmFile);
            K2IImg.Load (pK2IFile);
            I2InfoImg.Load (pI2InfoFile);

            PatRsDfa.SetImage (FsmImg.GetImageDump ());
            PatMealyDfa.SetImage (FsmImg.GetImageDump ());
            PatRevMealy.SetImage (FsmImg.GetImageDump ());
            K2I.SetImage (K2IImg.GetImageDump ());
            OwsId2Ows.SetImage (I2InfoImg.GetImageDump ());

            g_pDfa = &PatRsDfa;
            g_pMealy = &PatMealyDfa;
            g_pOw2Iw = &PatRevMealy;
            g_pK2I = &K2I;
            g_pI2Info = &OwsId2Ows;

        } else {
            DebugLogAssert (FAFsmConst::FORMAT_TXT == g_format_type);

            std::ifstream fsm_ifs (pFsmFile, std::ios::in);
            FAAssertStream (&fsm_ifs, pFsmFile);
            aut_io.Read (fsm_ifs, &PatRsDfa_txt, &PatMealyDfa_txt);

            std::ifstream ows_ifs (pI2InfoFile, std::ios::in);
            FAAssertStream (&ows_ifs, pI2InfoFile);
            map_io.Read (ows_ifs, &OwsId2Ows_txt);

            const int * pA;
            int Count;
            std::ifstream k2i_ifs (pK2IFile, std::ios::in);
            FAAssertStream (&k2i_ifs, pK2IFile);
            map_io.Read (k2i_ifs, &pA, &Count);
            K2I_txt.SetArray (pA, Count);

            g_pDfa = &PatRsDfa_txt;
            g_pMealy = &PatMealyDfa_txt;
            g_pOw2Iw = &PatMealyDfa_txt;
            g_pK2I = &K2I_txt;
            g_pI2Info = &OwsId2Ows_txt;
        }
        // load normalization map, if needed
        if (g_pCharMapFile) {
            g_charmap_image.Load (g_pCharMapFile);
            const unsigned char * pImg = g_charmap_image.GetImageDump ();
            DebugLogAssert (pImg);
            g_charmap.SetImage (pImg);
        }

        // adjust input/output

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        std::ostream * pUnOs = NULL;
        std::ofstream un_ofs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (NULL != pUnOutFile) {
            un_ofs.open (pUnOutFile, std::ios::out);
            pUnOs = &un_ofs;
        }
        FAAssert (pIs && pOs, FAMsg::IOError);

        FASelectTrPatterns_print Pat2Pat (&g_alloc, pOs, pUnOs);

        // see if a subset should be calculated
        if (false == g_TakeAll) {

            Pat2Pat.SetNoEmpty (g_NoEmpty);
            Pat2Pat.SetFsm (g_pDfa, g_pMealy, g_pOw2Iw);
            Pat2Pat.SetK2I (g_pK2I);
            Pat2Pat.SetI2Info (g_pI2Info);

            /// add left anchor
            g_Iws [0] = g_spec_l_anchor;
            g_Ows [0] = FAFsmConst::HYPH_NO_HYPH;

            // process the input
            std::string line;

            while (!(pIs->eof ())) {

                if (!std::getline (*pIs, line))
                    break;

                const char * pLine = line.c_str ();
                int LineLen = (const int) line.length ();

                if (0 < LineLen) {
                    DebugLogAssert (pLine);
                    if (0x0D == (unsigned char) pLine [LineLen - 1])
                        LineLen--;
                }
                if (0 < LineLen) {

                    /// str -> chains
                    g_Count = FATrWordIOTools_utf8::Str2IwOw (pLine, LineLen, \
                        g_Iws + 1, g_Ows + 1, FALimits::MaxWordLen - 2,
                        g_ignore_case, g_pMap);

                    /// add right anchor
                    if (g_Count + 1 < FALimits::MaxWordLen) {
                        g_Iws [g_Count + 1] = g_spec_r_anchor;
                        g_Ows [g_Count + 1] = FAFsmConst::HYPH_NO_HYPH;
                    }

                    g_Count += 2;

                    Pat2Pat.AddIwsOws (g_Iws, g_Ows, g_Count);
                }

            } // of while (!(pIs->eof ())) ...

            Pat2Pat.Process ();

        } else { // if (g_TakeAll)

            FAMphInterpretTools_t < int > mph;

            mph.SetMealy (g_pMealy);
            mph.SetRsDfa (g_pDfa);
            mph.SetOw2Iw (g_pOw2Iw);

            const int Count = g_pK2I->GetCount ();

            for (int K = 0; K < Count; ++K) {

                // get left side of a pattern
                const int LeftSize = \
                    mph.GetChain (K, g_Iws, FALimits::MaxWordLen);
                FAAssert (0 < LeftSize && LeftSize <= FALimits::MaxWordLen, 
                    FAMsg::InternalError);

                // get right side of a pattern
                const int I = g_pK2I->GetAt (K);
                const int RightSize = \
                    g_pI2Info->Get (I, g_Ows, FALimits::MaxWordLen);
                FAAssert (RightSize - 1 == LeftSize, FAMsg::InternalError);

                // return the pattern without the frequency
                Pat2Pat.PutPattern (g_Iws, g_Ows + 1, LeftSize);
            }
        } //  of if (false == g_TakeAll) ... 

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
