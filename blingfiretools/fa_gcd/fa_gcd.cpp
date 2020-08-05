/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAImageDump.h"
#include "FATaggedText.h"
#include "FAMapIOTools.h"
#include "FACorpusIOTools_utf8.h"
#include "FAPrintUtils.h"
#include "FAException.h"
#include "FATagSet.h"
#include "FAGcLDB.h"
#include "FAMorphLDB_t_packaged.h"
#include "FAGcInterpreter_t.h"
#include "FABrResultCA.h"
#include "FAActionsA.h"
#include "FAResolveMatchA.h"
#include "FAResolveMatch_nest.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";


FAAllocator g_alloc;

const char * g_pLdbFile = "gc.ldb.dump";
const char * g_pInPrmLdbFile = NULL;
const char * g_pTagsetFile = "tagset.txt";
const char * g_pInFile = NULL;
const char * g_pOutFile = NULL;

std::istream * g_pIs = &std::cin;
std::ifstream g_ifs;

std::ostream * g_pOs = &std::cout;
std::ofstream g_ofs;

bool g_print_input = false;
bool g_ignore_case = false;
bool g_no_pos_tags = false;
bool g_no_output = false;
bool g_no_process = false;
bool g_verbose = false;
int g_resolver_type = FAFsmConst::RESOLVE_MATCH_ALL;


void usage () {

  std::cout << "\n\
Usage: fa_gcd [OPTIONS]\n\
\n\
This program executes GC rules over the input tagged sentences. Each sentence\n\
is read from the separate line in FACorpusIOTools_utf8 format.\n\
\n\
  --ldb=<ldb> - reads comiled rules from the <ldb> file,\n\
    gc.ldb.dump is used by default\n\
\n\
  --prm-ldb=<ldb> - reads PRM LDB dump from file <input>,\n\
    no used by default\n\
\n\
  --tagset=<tagset> - reads input tagset from the <tagset> file,\n\
    tagset.txt is used by default\n\
\n\
  --in=<input> - reads input text from the <input> file,\n\
    if omited stdin is used\n\
\n\
  --out=<output> - writes output to the <output> file,\n\
    if omited stdout is used\n\
\n\
  --ignore-case - converts input text to the lower case,\n\
    uses simple case folding algorithm due to Unicode 4.1.0\n\
\n\
  --resolve=<type> - specifies the type of match results resolver,\n\
    all - takes all results (is used by default)\n\
    nest - removes overlappings and same rule nestings\n\
\n\
  --no-pos-tags - the input text is word-broken but words have no POS tags\n\
\n\
  --print-input - prints input string to stdout\n\
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
    if (0 == strncmp ("--ldb=", *argv, 6)) {
        g_pLdbFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--prm-ldb=", *argv, 10)) {
        g_pInPrmLdbFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        g_pTagsetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strcmp ("--resolve=all", *argv)) {
        g_resolver_type = FAFsmConst::RESOLVE_MATCH_ALL;
        continue;
    }
    if (0 == strcmp ("--resolve=nest", *argv)) {
        g_resolver_type = FAFsmConst::RESOLVE_MATCH_NEST;
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
    if (0 == strcmp ("--verbose", *argv)) {
        g_verbose = true;
        continue;
    }
  }
}


class FADebugActions : public FAActionsA {

public:
    FADebugActions ();

public:
    void SetOs (std::ostream * pOs);
    void SetTagSet (const FATagSet * pTagSet);
    void SetTaggedText (const FATaggedTextCA * pText);

    void Process (
        const int ActNum, 
        const int From, 
        const int To, 
        const FABrResultCA * pRes,
        void * pContext
    ) const;

private:
    inline void PrintText (
            std::ostream& os,
            const FATagSet * pTagSet, 
            const FATaggedTextCA * pText, 
            const int From, 
            const int To
        ) const;

private:
    std::ostream * m_pOs;
    const FATagSet * m_pTagSet;
    const FATaggedTextCA * m_pText;
};

FADebugActions::FADebugActions () :
    m_pOs (NULL),
    m_pTagSet (NULL),
    m_pText (NULL)
{}

void FADebugActions::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}

void FADebugActions::SetOs (std::ostream * pOs)
{
    m_pOs = pOs;
}

void FADebugActions::SetTaggedText (const FATaggedTextCA * pText)
{
    m_pText = pText;
}

inline void FADebugActions::
    PrintText (
        std::ostream& os,
        const FATagSet * pTagSet, 
        const FATaggedTextCA * pText, 
        const int From, 
        const int To
    ) const
{
    DebugLogAssert (pText && pTagSet);

    const int Count = pText->GetWordCount ();

    for (int i = From; i <= To; ++i) {

        if (From != i)
            os << char (FAFsmConst::CHAR_WORD_DELIM);

        if (0 <= i && i < Count) {

            const int * pWord;
            const int Len = pText->GetWord (i, &pWord);
            const int Tag = pText->GetTag (i);

            ::FAPrintTaggedWord (os, pTagSet, pWord, Len, Tag);

        } else {

            os << "???";
        }
    }
}

void FADebugActions::
    Process (
        const int ActNum, 
        const int From, 
        const int To, 
        const FABrResultCA * pRes,
        void * /*pContext*/
    ) const
{
    DebugLogAssert (pRes);

    if (m_pOs) {

        (*m_pOs) << "rule " << ActNum << ' ' << From << ' ' << To << " : ";
        PrintText ((*m_pOs), m_pTagSet, m_pText, From, To);
        (*m_pOs) << '\n';

        const int * pFromTo;
        int BrId = -1;
        int Count = pRes->GetNextRes (&BrId, &pFromTo);

        while (-1 != Count) {

            DebugLogAssert (0 == Count % 2);

            for (int i = 0; i < Count; ++i) {

                const int BrFrom = pFromTo [i++];
                const int BrTo = pFromTo [i];

                (*m_pOs) << "  br " << BrId << ' ' << BrFrom << ' ' << BrTo << " : ";
                PrintText ((*m_pOs), m_pTagSet, m_pText, BrFrom, BrTo);
                (*m_pOs) << '\n';
            }

            Count = pRes->GetNextRes (&BrId, &pFromTo);

        } // of while ...

        (*m_pOs) << '\n';
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    process_args (argc, argv);

    try {

        ///
        /// create objects
        ///

        // GC-LDB
        FAGcLDB g_gc_ldb (&g_alloc);
        FAImageDump g_image;
        // PRM LDB
        FAMorphLDB_t < int > g_morph_ldb;
        FADictInterpreter_t < int > g_tag_dict;
        FAImageDump g_image2;
        // gc interpreter
        FAGcInterpreter_t < int > g_proc (&g_alloc);
        // resolver
        FAResolveMatchA * g_pResolver = NULL;
        FAResolveMatch_nest g_resolve_nest (&g_alloc);
        // tagset
        FATagSet g_tagset (&g_alloc);
        // IO
        FAMapIOTools g_map_io (&g_alloc);
        FACorpusIOTools_utf8 g_txt_io (&g_alloc);
        // input tagged text
        FATaggedText g_text (&g_alloc);
        // debug actions
        FADebugActions g_acts;

        ///
        /// initialize
        ///
        g_txt_io.SetTagSet (&g_tagset);
        g_txt_io.SetNoPosTags (g_no_pos_tags);

        if (g_pInFile) {
            g_ifs.open (g_pInFile, std::ios::in);
            FAAssertStream (&g_ifs, g_pInFile);
            g_pIs = &g_ifs;
        }
        if (g_pOutFile) {
            g_ofs.open (g_pOutFile, std::ios::out);
            g_pOs = &g_ofs;
        }
        if (g_pTagsetFile) {
            std::ifstream tagset_ifs (g_pTagsetFile, std::ios::in);
            FAAssertStream (&tagset_ifs, g_pTagsetFile);
            g_map_io.Read (tagset_ifs, &g_tagset);
        }

        g_acts.SetTagSet (&g_tagset);
        g_acts.SetTaggedText (&g_text);

        if (false == g_no_output) {
            g_acts.SetOs (g_pOs);
        }

        g_image.Load (g_pLdbFile);
        g_gc_ldb.SetImage (g_image.GetImageDump ());

        if (FAFsmConst::RESOLVE_MATCH_NEST == g_resolver_type) {
            g_pResolver = &g_resolve_nest;
        }

        if (g_pInPrmLdbFile) {
            g_image2.Load (g_pInPrmLdbFile);
            g_morph_ldb.SetImage (g_image2.GetImageDump ());
            g_tag_dict.SetConf (
                    g_morph_ldb.GetTagDictConf (), 
                    g_morph_ldb.GetInTr ()
                );
            g_proc.SetTagDict (&g_tag_dict);
        }

        g_proc.SetIgnoreCase (g_ignore_case);
        g_proc.SetLDB (&g_gc_ldb);
        g_proc.SetActions (&g_acts);
        g_proc.SetResolver (g_pResolver);

        ///
        /// process data
        ///

        while (!g_pIs->eof ()) {

            g_txt_io.Read (*g_pIs, &g_text);

            if (g_print_input && !g_no_output) {
                DebugLogAssert (g_pOs);
                (*g_pOs) << "Input : ";
                g_txt_io.Print (*g_pOs, &g_text);
            }

            const int WordCount = g_text.GetWordCount ();

            if (!g_no_process && 0 < WordCount) {

                for (int i = 0; i < WordCount; ++i) {

                    const int * pWord;
                    const int WordLen = g_text.GetWord (i, &pWord);
                    DebugLogAssert (0 < WordLen && pWord);

                    const int Tag = g_text.GetTag (i);
                    DebugLogAssert (0 < Tag);

                    g_proc.AddWord (pWord, WordLen, Tag);
                }

                g_proc.Process ();
            }

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
