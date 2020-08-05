/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAException.h"
#include "FAMapIOTools.h"
#include "FADictSplit.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;


const char * __PROG__ = "";

FAAllocator g_alloc;
FAMapIOTools g_map_io (&g_alloc);

const char * pInFile = NULL;
const char * pOutFile = NULL;
const char * pOutK2IFile = NULL;
const char * pOutI2InfoFile = NULL;

int g_base = 10;
int g_num_size = -1;
int g_info_base = 0;
int g_mode = FAFsmConst::DM_TAGS;
bool g_no_k2i = false;

const int MaxChainSize = 4096;

int g_ChainSize = 0;
int ChainBuffer [MaxChainSize];

FADictSplit g_dict_split (&g_alloc);


void usage ()
{
    std::cerr << "\
Usage: fa_dict_split [OPTION]\n\
\n\
This tool reads digitized dictionary stream of sorted 0-separated Key/Info\n\
pairs and splits it into stream of Keys, KeyNum -> InfoId and InfoId -> Info\n\
maps, where KeyNum is a unique Key index in the input stream.\n\
\n\
  --in=<input-file>  - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes Keys to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-i2info=<output-file> - writes InfoId -> Info map to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --out-k2i=<output-file> - writes KeyNum -> InfoId map to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --no-k2i - does not create KeyNum -> InfoId map, the InfoId-s are added\n\
    to the corresponding Key directly\n\
\n\
  --info-base=Base - this value will be added to InfoIds in --no-k2i mode,\n\
    0 is used by default\n\
\n\
  --raw - store info data as-is, in this mode the input data should not\n\
    contain duplicate keys (not used by default)\n\
\n\
  --tag-prob - Tag Prob mode, all words should be sorted, every input line\n\
    should contain KEY TAG PROB values (not used by default)\n\
\n\
  --hyph - Hyph mode, all words should be sorted, every input line\n\
    should contain KEY FREQ OWS values (not used by default)\n\
\n\
  --base=10 - uses decimal base for the output numbers,\n\
    (is used by default)\n\
\n\
  --base=16 - uses hexadecimal base for the output numbers\n\
\n\
  --num-size=N - number of digitis per value/key,\n\
    4 for --base=16 and 5 for --base=10 is used by default\n\
\n\
";

}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (0 == strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--in=", *argv, 5)) {
        pInFile = &((*argv) [5]);
        continue;
    }
    if (0 == strncmp ("--out=", *argv, 6)) {
        pOutFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--out-k2i=", *argv, 10)) {
        pOutK2IFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--out-i2info=", *argv, 13)) {
        pOutI2InfoFile = &((*argv) [13]);
        continue;
    }
    if (0 == strcmp ("--base=16", *argv)) {
        g_base = 16;
        continue;
    }
    if (0 == strcmp ("--base=10", *argv)) {
        g_base = 10;
        continue;
    }
    if (0 == strcmp ("--raw", *argv)) {
        g_mode = FAFsmConst::DM_RAW;
        continue;
    }
    if (0 == strcmp ("--tag-prob", *argv)) {
        g_mode = FAFsmConst::DM_TAG_PROB;
        continue;
    }
    if (0 == strcmp ("--hyph", *argv)) {
        g_mode = FAFsmConst::DM_HYPH;
        continue;
    }
    if (0 == strcmp ("--no-k2i", *argv)) {
        g_no_k2i = true;
        continue;
    }
    if (0 == strncmp ("--num-size=", *argv, 11)) {
        g_num_size = atoi (&((*argv) [11]));
        continue;
    }
    if (0 == strncmp ("--info-base=", *argv, 12)) {
        g_info_base = atoi (&((*argv) [12]));
        continue;
    }
  }
}


void ReadChain (const char * pChainStr, const int ChainStrLen)
{
    DebugLogAssert (pChainStr && 0 < ChainStrLen);

    const char * pChainStrEnd = pChainStr + ChainStrLen;
    g_ChainSize = 0;

    while (pChainStr < pChainStrEnd) {

        if (g_ChainSize >= MaxChainSize) {
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        ChainBuffer [g_ChainSize++] = strtol (pChainStr, NULL, g_base);

        pChainStr = strchr (pChainStr, ' ');

        if (NULL == pChainStr)
            break;

        pChainStr++;
    }

    if (0 == g_ChainSize) {
        throw FAException (FAMsg::IOError, __FILE__, __LINE__);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    if (-1 == g_num_size) {
        if (16 == g_base)
            g_num_size = 4;
        else
            g_num_size = 5;
    }

    try {
        // select in/out streams
        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        std::ostream * pOsK2I = &std::cout;
        std::ofstream ofs_k2i;

        std::ostream * pOsI2Info = &std::cout;
        std::ofstream ofs_i2info;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }
        if (NULL != pOutK2IFile) {
            ofs_k2i.open (pOutK2IFile, std::ios::out);
            pOsK2I = &ofs_k2i;
        }
        if (NULL != pOutI2InfoFile) {
            ofs_i2info.open (pOutI2InfoFile, std::ios::out);
            pOsI2Info = &ofs_i2info;
        }

        g_dict_split.SetBase (g_base);
        g_dict_split.SetNumSize (g_num_size);
        g_dict_split.SetKeyOs (pOs);
        g_dict_split.SetMode (g_mode);
        g_dict_split.SetNoK2I (g_no_k2i);
        g_dict_split.SetInfoIdBase (g_info_base);

        // process the input

        std::string line;

        while (!std::cin.eof ()) {

            if (!std::getline (std::cin, line))
                break;

            const char * pLine = line.c_str ();
            int LineLen = (const int) line.length ();

            if (0 < LineLen) {
                DebugLogAssert (pLine);
                if (0x0D == (unsigned char) pLine [LineLen - 1])
                    LineLen--;
            }

            if (0 < LineLen) {
                // read in the chain
                ReadChain (pLine, LineLen);
                // process it
                g_dict_split.AddChain (ChainBuffer, g_ChainSize);
            }

        } // of while (!std::cin.eof ()) ...

        // finish processing
        g_dict_split.Process ();

        // print out maps
        if (false == g_no_k2i) {

            const int * pKeyNum2SetId = NULL;
            const int KeyNum2SetIdCount = g_dict_split.GetK2I (&pKeyNum2SetId);
            DebugLogAssert (pKeyNum2SetId && 0 < KeyNum2SetIdCount);

            g_map_io.Print (*pOsK2I, pKeyNum2SetId, KeyNum2SetIdCount);
        }

        const FAMultiMapA * pMMap = g_dict_split.GetI2Info ();
        DebugLogAssert (pMMap);
        g_map_io.Print (*pOsI2Info, pMMap);

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
