/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FASetUtils.h"
#include "FAMapIOTools.h"
#include "FAParser2WRE.h"
#include "FATagSet.h"
#include "FAAct2Arr_css.h"
#include "FAAct2Arr_score_tag.h"
#include "FAAct2Arr_data.h"
#include "FAMultiMap_judy.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

FAMapIOTools g_map_io (&g_alloc);
FATagSet g_tagset (&g_alloc);
FAAct2Arr_css g_css_rules (&g_alloc);
FAAct2Arr_score_tag g_score_tag_rules (&g_alloc);
FAAct2Arr_data g_data_rules (&g_alloc);

FAMultiMap_judy g_out_data_map;

const char * pInLeftFile = NULL;
const char * pInRightFile = NULL;
const char * pInTagSetFile = NULL;
const char * pOutLeftFile = NULL;
const char * pOutRightFile = NULL;
const char * pOutTagSetFile = NULL;
const char * pOutDataMapFile = NULL;

int g_LabelType = FAFsmConst::LABEL_WRE;
const char * pInputEnc = NULL;

std::ifstream ifs_left;
std::ifstream ifs_right;
std::ifstream ifs_tagset;
std::ofstream ofs_left;
std::ofstream ofs_right;
std::ofstream ofs_tagset;
std::ofstream ofs_data_map;

std::istream * g_pLeftIs = &std::cin;
std::istream * g_pRightIs = &std::cin;
std::istream * g_pTagSetIs = NULL;
std::ostream * g_pLeftOs = &std::cout;
std::ostream * g_pRightOs = &std::cout;
std::ostream * g_pTagSetOs = NULL;
std::ostream * g_pDataMapOs = NULL;

int g_LcCut = 0;

enum {
    ACT_FORMAT_DEFAULT = 0,
    ACT_FORMAT_CSS,
    ACT_FORMAT_SCORE_TAG,
    ACT_FORMAT_DATA_ARRAY,
};

int g_ActFormat = ACT_FORMAT_DEFAULT;

bool g_no_output = false;


void usage () {

  std::cout << "\n\
Usage: fa_pr2wre [OPTIONS]\n\
\n\
This program converts parser rules into a list of WRE rules and builds\n\
extended tagset (it should be used at compilation time only).\n\
\n\
  --in=<input-file> - reads left parts of parser rules,\n\
    if omited stdin is used\n\
\n\
  --in-actions=<input-file> - reads right parts of parser rules,\n\
    if omited stdin is used\n\
\n\
  --tagset=<input-file> - reads input tagset from the <input-file>,\n\
    does not read tagset by default\n\
\n\
  --out=<output-file> - writes list of WRE rules,\n\
    if omited stdout is used\n\
\n\
  --out-actions=<output-file> - writes right parts of WRE rules,\n\
    if omited stdout is used\n\
\n\
  --out-tagset=<output-file> - writes extended tagset to <output-file>,\n\
    does not print by default\n\
\n\
  --out-actions-data=<output-file> - writes serialized multi map with,\n\
    action data, can be only used with --act-format=tag-num-array\n\
\n\
  --label=<label-type> - the following types are available:\n\
\n\
    digit - assumes unsigned ints as a regular expression symbols,\n\
      the default value\n\
    char  - assumes chracters of 8-bit encoding or UTF-8 as input symbols\n\
    wre   - assumes WRE tokens as input symbols\n\
\n\
  --input-enc=<encoding> - input encoding name (\"UTF-8\", \"CP1251\", ...)\n\
    can be used with --label=char only, if not specified then\n\
    byte representation of characters is used for automaton input weights\n\
\n\
  --cut-left-context=N - specifies how many characters/words should be removed\n\
    from the left context, this allows to have extracting rules prefixed with\n\
    some fixed length expressions. 0 is used by default\n\
\n\
  --act-format=<format> - sets up action format, if not specified the default\n\
    format of actions is used. Possible values:\n\
      tag-num-array - the data are a sequence of tags and/or integer numbers\n\
      css-rules - sets up CSS rules parser format\n\
      score-tag - sets up score tag format\n\
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
    if (0 == strncmp ("--in=", *argv, 5)) {
        pInLeftFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--in-actions=", *argv, 13)) {
        pInRightFile = &((*argv) [13]);
        continue;
    }
    if (0 == strncmp ("--tagset=", *argv, 9)) {
        pInTagSetFile = &((*argv) [9]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        pOutLeftFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--out-actions=", *argv, 14)) {
        pOutRightFile = &((*argv) [14]);
        continue;
    }
    if (0 == strncmp ("--out-actions-data=", *argv, 19)) {
        pOutDataMapFile = &((*argv) [19]);
        continue;
    }
    if (0 == strncmp ("--out-tagset=", *argv, 13)) {
        pOutTagSetFile = &((*argv) [13]);
        continue;
    }
    if (0 == strcmp ("--label=digit", *argv)) {
      g_LabelType = FAFsmConst::LABEL_DIGIT;
      continue;
    }
    if (0 == strcmp ("--label=char", *argv)) {
      g_LabelType = FAFsmConst::LABEL_CHAR;
      continue;
    }
    if (0 == strcmp ("--label=wre", *argv)) {
      g_LabelType = FAFsmConst::LABEL_WRE;
      continue;
    }
    if (0 == strncmp ("--input-enc=", *argv, 12)) {
      pInputEnc = &((*argv) [12]);
      continue;
    }
    if (0 == strncmp ("--cut-left-context=", *argv, 19)) {
        g_LcCut = atoi (&((*argv) [19]));
        continue;
    }
    if (0 == strncmp ("--act-format=css-rules", *argv, 22)) {
        g_ActFormat = ACT_FORMAT_CSS;
        continue;
    }
    if (0 == strncmp ("--act-format=tag-num-array", *argv, 26)) {
        g_ActFormat = ACT_FORMAT_DATA_ARRAY;
        continue;
    }
    if (0 == strncmp ("--act-format=score-tag", *argv, 22)) {
        g_ActFormat = ACT_FORMAT_SCORE_TAG;
        continue;
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
        g_no_output = true;
        continue;
    }
  }
}


void AdjustIOPtrs ()
{
    if (pInLeftFile) {
        ifs_left.open (pInLeftFile, std::ios::in);
        FAAssertStream (&ifs_left, pInLeftFile);
        g_pLeftIs = &ifs_left;
    }
    if (pInRightFile) {
        ifs_right.open (pInRightFile, std::ios::in);
        FAAssertStream (&ifs_right, pInRightFile);
        g_pRightIs = &ifs_right;
    }
    if (pInTagSetFile) {
        ifs_tagset.open (pInTagSetFile, std::ios::in);
        FAAssertStream (&ifs_tagset, pInTagSetFile);
        g_pTagSetIs = &ifs_tagset;
    }

    if (false == g_no_output) {

        if (pOutLeftFile) {
            ofs_left.open (pOutLeftFile, std::ios::out);
            g_pLeftOs = &ofs_left;
        }
        if (pOutRightFile) {
            ofs_right.open (pOutRightFile, std::ios::out);
            g_pRightOs = &ofs_right;
        }
        if (pOutTagSetFile) {
            ofs_tagset.open (pOutTagSetFile, std::ios::out);
            g_pTagSetOs = &ofs_tagset;
        }
        if (pOutDataMapFile) {
            LogAssert (ACT_FORMAT_DATA_ARRAY == g_ActFormat);
            ofs_data_map.open (pOutDataMapFile, std::ios::out);
            g_pDataMapOs = &ofs_data_map;
        }

    } else {

        g_pLeftOs = NULL;
        g_pRightOs = NULL;
        g_pTagSetOs = NULL;
        g_pDataMapOs = NULL;
    }
}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        AdjustIOPtrs ();

        g_out_data_map.SetAllocator (&g_alloc);

        FAParser2WRE pr2wre (&g_alloc);

        // load tagset and setup input
        if (g_pTagSetIs) {
            g_map_io.Read (*g_pTagSetIs, &g_tagset);
            pr2wre.SetInTagSet (&g_tagset);
        }

        // see if a custom CSS rules actions parser is needed
        if (ACT_FORMAT_CSS == g_ActFormat) {
            g_css_rules.SetTagSet (&g_tagset);
            pr2wre.SetActionParser (&g_css_rules);
        } else if (ACT_FORMAT_SCORE_TAG == g_ActFormat) {
            g_score_tag_rules.SetTagSet (&g_tagset);
            pr2wre.SetActionParser (&g_score_tag_rules);
        } else if (ACT_FORMAT_DATA_ARRAY == g_ActFormat) {
            g_data_rules.SetTagSet (&g_tagset);
            pr2wre.SetActionParser (&g_data_rules);
        }

        pr2wre.SetLabelType (g_LabelType);
        pr2wre.SetUseUtf8 (::FAIsUtf8Enc (pInputEnc));
        pr2wre.SetLeftIn (g_pLeftIs);
        pr2wre.SetRightIn (g_pRightIs);
        pr2wre.SetLeftOut (g_pLeftOs);
        pr2wre.SetLcCut (g_LcCut);

        pr2wre.Process ();

        // print out actions map
        if (g_pRightOs) {
            const FAMultiMapA * pActMap = pr2wre.GetActMap ();
            DebugLogAssert (pActMap);
            g_map_io.Print (*g_pRightOs, pActMap);
        }
        // print out extended tagset, if asked
        if (g_pTagSetOs) {
            const FATagSet * pOutTagSet = pr2wre.GetOutTagSet ();
            DebugLogAssert (pOutTagSet);
            g_map_io.Print (*g_pTagSetOs, pOutTagSet);
        }
        // print the action data
        if (g_pDataMapOs) {
            // only ACT_FORMAT_DATA_ARRAY has this option
            LogAssert (ACT_FORMAT_DATA_ARRAY == g_ActFormat);
            // copy the data into the multi map
            const int Count = g_data_rules.GetDataCount ();
            for (int i = 0; i < Count; ++i) {
                const int * pData;
                const int Size = g_data_rules.GetData (i, &pData);
                // add to the multi map
                g_out_data_map.Set (i + 1, pData, Size);
            }
            // serialize the multi map into a file
            g_map_io.Print (*g_pDataMapOs, &g_out_data_map);
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
