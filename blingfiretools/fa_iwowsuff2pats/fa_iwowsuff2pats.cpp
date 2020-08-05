/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAAllocator.h"
#include "FAPrintUtils.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAIwOwSuffArr2Patterns.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

FAAllocator g_alloc;

int g_Chain [2 * FALimits::MaxWordLen];
int g_Chain2 [2 * FALimits::MaxWordLen];
int g_ChainSize = 0;

int g_MinPatLen = 3;
float g_MinPatPrec = 100.0f;
int g_MinPatFreq = 1;
bool g_NoEmpty = false;
bool g_DontCarePats = false;
int g_MaxLeftCx = 4;

const char * pInFile = NULL;
const char * pOutFile = NULL;

void usage () {

  std::cout << "\n\
Usage: fa_iwowsuff2pats [OPTIONS]\n\
\n\
From the stream of sorted suffixes of Iw/Ow pairs this program builds a list\n\
of patterns.\n\
\n\
  --in=<input-file> - reads input from the <input-file>,\n\
    if omited stdin is used\n\
\n\
  --out=<output-file> - writes output to the <output-file>,\n\
    if omited stdout is used\n\
\n\
  --min-length=N - sets up a mimimal pattern length, 3 is used by default\n\
\n\
  --min-prec=N - sets up a mimimal pattern precision, 100.0 is used by default\n\
\n\
  --min-freq=N - sets up a mimimal pattern frequency, 1 is used by default\n\
\n\
  --no-empty - will not produce patterns without hypenation points\n\
\n\
  --dont-care-pats - generates patterns with \"don't care\" output symbols\n\
\n\
  --max-context=N - specifies the maximum left context for the \"don't care\"\n\
    patterns, 4 is used by default\n\
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
        if (0 == strncmp ("--out=", *argv, 6)) {
            pOutFile = &((*argv) [6]);
            continue;
        }
        if (0 == strncmp ("--min-length=", *argv, 13)) {
            g_MinPatLen = atoi (&((*argv) [13]));
            continue;
        }
        if (0 == strncmp ("--min-prec=", *argv, 11)) {
            g_MinPatPrec = float (atof (&((*argv) [11])));
            continue;
        }
        if (0 == strncmp ("--min-freq=", *argv, 11)) {
            g_MinPatFreq = atoi (&((*argv) [11]));
            continue;
        }
        if (!strcmp ("--no-empty", *argv)) {
            g_NoEmpty = true;
            continue;
        }
        if (!strcmp ("--dont-care-pats", *argv)) {
            g_DontCarePats = true;
            continue;
        }
        if (0 == strncmp ("--max-context=", *argv, 14)) {
            g_MaxLeftCx = atoi (&((*argv) [14]));
            FAAssert (0 <= g_MaxLeftCx && 10 > g_MaxLeftCx, \
                FAMsg::InvalidParameters);
            continue;
        }
    }
}


///
/// class with overriden output
///

class FATmpSuff2Pats : public FAIwOwSuffArr2Patterns {

public:
    FATmpSuff2Pats (FAAllocatorA * pAlloc);

public:
    void SetOutStream (std::ostream * pOs);
    void PutPattern (const int * pPat, const int Size, const int Freq);

private:
    std::ostream * m_pOs;
};

FATmpSuff2Pats::FATmpSuff2Pats (FAAllocatorA * pAlloc) :
    FAIwOwSuffArr2Patterns (pAlloc),
    m_pOs (NULL)
{}

void FATmpSuff2Pats::SetOutStream (std::ostream * pOs)
{
    m_pOs = pOs;
}

void FATmpSuff2Pats::
    PutPattern (const int * pPat, const int Size, const int Freq)
{
    DebugLogAssert (m_pOs);
    DebugLogAssert (0 == (Size % 2));

    char Utf8Char [FAUtf8Const::MAX_CHAR_SIZE + 1];
    int i;

    /// check the frequency limit
    if (g_MinPatFreq > Freq) {
        return;
    }
    /// see if pattern consists of DontCare symbols only
    if (g_DontCarePats) {
        for (i = 1; i < Size; i += 2) {
            const int Ow = pPat [i];
            if (FAFsmConst::HYPH_DONT_CARE != Ow)
                break;
        }
        if (i >= Size) {
            return;
        }
    }
    /// see whether pattern is not empty
    if (g_NoEmpty) {
        for (i = 1; i < Size; i += 2) {
            const int Ow = pPat [i];
            if (FAFsmConst::HYPH_NO_HYPH != Ow && \
                FAFsmConst::HYPH_DONT_CARE != Ow) {
                break;
            }
        }
        if (i >= Size) {
            return;
        }
    }

    /// print the Key string
    for (i = 0; i < Size; i += 2) {

        const int Iw = pPat [i];

        DebugLogAssert (0 < ::FAUtf8Size (Iw) && \
            FAUtf8Const::MAX_CHAR_SIZE >= ::FAUtf8Size (Iw));

        char * pEnd = ::FAIntToUtf8 (Iw, Utf8Char, FAUtf8Const::MAX_CHAR_SIZE);
        DebugLogAssert (pEnd);
        *pEnd = 0;

        (*m_pOs) << Utf8Char;
    }

    /// print the tab-separated action with frequency
    (*m_pOs) << '\t' << Freq;

    for (i = 1; i < Size; i += 2) {
        const int Ow = pPat [i];
        (*m_pOs) << '\t' << Ow;
    }

    (*m_pOs) << '\n';
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    try {

        // adjust input/output

        std::istream * pIs = &std::cin;
        std::ifstream ifs;

        std::ostream * pOs = &std::cout;
        std::ofstream ofs;

        if (NULL != pInFile) {
            ifs.open (pInFile, std::ios::in);
            FAAssertStream (&ifs, pInFile);
            pIs = &ifs;
        }
        if (NULL != pOutFile) {
            ofs.open (pOutFile, std::ios::out);
            pOs = &ofs;
        }

        DebugLogAssert (pIs);
        DebugLogAssert (pOs);

        FATmpSuff2Pats suff2pat (&g_alloc);

        suff2pat.SetMinPatLen (g_MinPatLen);
        suff2pat.SetMinPatPrec (g_MinPatPrec);
        suff2pat.SetOutStream (pOs);

        FATmpSuff2Pats ** ppSuff2Pats = NULL;

        if (g_DontCarePats) {

            ppSuff2Pats = NEW FATmpSuff2Pats* [g_MaxLeftCx];
            FAAssert (ppSuff2Pats, FAMsg::OutOfMemory);

            for (int i = 0; i < g_MaxLeftCx; ++i) {

                FATmpSuff2Pats * pSuff2Pats = NEW FATmpSuff2Pats (&g_alloc);
                FAAssert (pSuff2Pats, FAMsg::OutOfMemory);
                ppSuff2Pats [i] = pSuff2Pats;

                if (i + 1 > g_MinPatLen) {
                    pSuff2Pats->SetMinPatLen (i + 1);
                } else {
                    pSuff2Pats->SetMinPatLen (g_MinPatLen);
                }

                pSuff2Pats->SetMinPatPrec (g_MinPatPrec);
                pSuff2Pats->SetOutStream (pOs);
            }
        }

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

                const int Padding1 = (int) strspn (pLine, " \t");
                pLine += Padding1;
                LineLen -= Padding1;
                FAAssert (0 <= Padding1 && 0 < LineLen, FAMsg::IOError);

                // read the frequency
                const int Freq = strtol (pLine, NULL, 10);

                const int FreqStrLen = (int) strcspn (pLine, " \t");
                pLine += FreqStrLen;
                LineLen -= FreqStrLen;
                FAAssert (0 < FreqStrLen && 0 < LineLen, FAMsg::IOError);

                const int Padding2 = (int) strspn (pLine, " \t");
                pLine += Padding2;
                LineLen -= Padding2;
                FAAssert (0 <= Padding2 && 0 < LineLen, FAMsg::IOError);

                const char * pChainStr = pLine;
                const char * pChainStrEnd = pLine + LineLen;

                g_ChainSize = 0;

                while (pChainStr < pChainStrEnd) {

                    FAAssert (g_ChainSize < 2 * FALimits::MaxWordLen, \
                        FAMsg::IOError);

                    g_Chain [g_ChainSize] = strtol (pChainStr, NULL, 16);
                    g_ChainSize++;

                    pChainStr = strchr (pChainStr, ' ');

                    if (NULL == pChainStr)
                        break;

                    pChainStr++;
                }

                if (g_DontCarePats) {

                    const int ChainSize_2 = g_ChainSize / 2;

                    for (int i = 0; i < ChainSize_2; ++i) {

                        const int Iw = g_Chain [i];

                        g_Chain2 [i << 1] = Iw;
                        g_Chain2 [(i << 1) + 1] = FAFsmConst::HYPH_DONT_CARE;
                    }

                    for (int Pos = 0; Pos < g_MaxLeftCx && Pos < ChainSize_2; ++Pos) {

                        FATmpSuff2Pats * pSuff2Pats = ppSuff2Pats [Pos];
                        DebugLogAssert (pSuff2Pats);

                        // store the actual Ow for the given position
                        const int Ow = g_Chain [Pos + ChainSize_2];
                        g_Chain2 [(Pos << 1) + 1] = Ow;

                        pSuff2Pats->AddChain (g_Chain2, g_ChainSize, Freq);

                        // restore the Ow value back
                        g_Chain2 [(Pos << 1) + 1] = FAFsmConst::HYPH_DONT_CARE;
                    }
 
                } else {

                    const int ChainSize_2 = g_ChainSize / 2;

                    for (int i = 0; i < ChainSize_2; ++i) {

                        const int Iw = g_Chain [i];
                        const int Ow = g_Chain [i + ChainSize_2];

                        g_Chain2 [i << 1] = Iw;
                        g_Chain2 [(i << 1) + 1] = Ow;
                    }

                    suff2pat.AddChain (g_Chain2, g_ChainSize, Freq);

                } // of if (if (m_DontCarePats)) ...
            } // of if (0 < LineLen) ...
        } // of while ...

        // finish up the processing
        if (g_DontCarePats) {

            for (int i = 0; i < g_MaxLeftCx; ++i) {
                FATmpSuff2Pats * pSuff2Pats = ppSuff2Pats [i];
                pSuff2Pats->Process ();
                delete pSuff2Pats ;
            }
            delete[] ppSuff2Pats;

        } else {

            suff2pat.Process ();
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
