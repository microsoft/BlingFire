/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAUtils.h"
#include "FAAllocator.h"
#include "FAMergeDumps.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;
const char * g_pOutFile = NULL;


void usage () {

  std::cout << "\n\
Usage: fa_merge_dumps [OPTIONS] FILE [FILE [...]]\n\
\n\
This program merges binary files together in a specified order and adds\n\
the header array of entry points. All array values are ints. The first value\n\
indicates the number of merged files, each subsequent value is an offset for\n\
the given entry point.\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
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
        if (0 == strncmp ("--out=", *argv, 6)) {
            g_pOutFile = &((*argv) [6]);
            continue;
        }
        break;
    }
}


int __cdecl main (int argc, char** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    // see whether file names left
    if (0 > argc) {
        std::cerr << "ERROR: \"At least one file should be specified\""
                  << " in program " << __PROG__ << '\n';
        return 2;
    }

    // do merging
    try {

        FAMergeDumps merger (&g_alloc);

        while (0 <= argc) {

            const char * pFileName = *argv;
            merger.AddDumpFile (pFileName);

            argc--; 
            argv++;
        }

        // create an empty file, if upto this point there were no errors
        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != g_pOutFile) {
            ofs.open (g_pOutFile, std::ios::out);
            pOs = &ofs;
        }

        // write the merged file down
        merger.Save (pOs);

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
