/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAUtils.h"
#include "FAPrintUtils.h"
#include "FAArray_cont_t.h"
#include "FAAutIOTools.h"
#include "FAMapIOTools.h"
#include "FAStringTokenizer.h"
#include "FARSDfa_ro.h"
#include "FAState2Ow.h"
#include "FAState2Ows.h"
#include "FARSNfa_ro.h"
#include "FARSDfa_pack_triv.h"
#include "FAState2Ow_pack_triv.h"
#include "FAState2Ows_pack_triv.h"
#include "FAPosNfa_pack_triv.h"
#include "FAState2TrBr_pack_triv.h"
#include "FAMultiMap_ar.h"
#include "FAAutInterpretTools_t.h"
#include "FAAutInterpretTools_fnfa_t.h"
#include "FAAutInterpretTools_pos_t.h"
#include "FAAutInterpretTools_trbr_t.h"
#include "FABrResult.h"
#include "FASetImageA.h"
#include "FAImageDump.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace BlingFire;

const char * __PROG__ = "";

const char * g_pInFsmFile = NULL;
const char * g_pInDataFile = NULL;
const char * g_pInPosNfaFile = NULL;
const char * g_pInTrBrFile = NULL;
const char * g_pOutFile = NULL;

int g_format_type = FAFsmConst::FORMAT_TXT;
int g_mode = FAFsmConst::MODE_INT;
int g_fsm_type = FAFsmConst::TYPE_RS_DFA;
int g_int_type = FAFsmConst::INT_TRIV;

bool g_reverse = false;
bool g_no_output = false;
bool g_no_process = false;
bool g_print_input = false;

FAAllocator g_alloc;
FAAutIOTools g_aut_io (&g_alloc);
FAMapIOTools g_map_io (&g_alloc);

FAStringTokenizer g_tokenizer;

FAAutInterpretTools_t < int > g_proc_triv (&g_alloc);
FAAutInterpretTools_fnfa_t < int > g_proc_fnfa (&g_alloc);
FAAutInterpretTools_pos_t < int > g_proc_pos (&g_alloc);
FAAutInterpretTools_trbr_t < int > g_proc_trbr (&g_alloc);

FARSDfa_ro g_fsm_rs (&g_alloc);
FARSDfa_pack_triv g_rs_dump_triv;

FAImageDump g_FsmImage;
FAImageDump g_PosNfaImage;

FAState2Ow g_fsm_moore_ows (&g_alloc);
FAState2Ow_pack_triv g_fsm_moore_ow_dump_triv;

FAState2Ows g_fsm_moore_multi_ows (&g_alloc);
FAState2Ows_pack_triv g_fsm_moore_ows_dump_triv;

// interfaces of data containers
FARSDfaCA * g_pDfa = NULL;
FAState2OwCA * g_pState2Ow = NULL;
FAState2OwsCA * g_pState2Ows = NULL;
// image setup interfaces of selected dump containers
FASetImageA * g_pSetDfaImg = NULL;
FASetImageA * g_pSetState2OwImg = NULL;
FASetImageA * g_pSetState2OwsImg = NULL;

FARSNfa_ro g_follow (&g_alloc);
FAMultiMap_ar g_pos2br_begin;
FAMultiMap_ar g_pos2br_end;
FAPosNfa_pack_triv g_follow_dump;
FAState2TrBr_pack_triv g_pos2br_begin_dump (FAState2TrBr_pack_triv::MapTypeTrBrBegin);
FAState2TrBr_pack_triv g_pos2br_end_dump (FAState2TrBr_pack_triv::MapTypeTrBrBegin);

FARSNfaCA * g_pFollow = NULL;
FAMultiMapCA * g_pPos2BrBeg = NULL;
FAMultiMapCA * g_pPos2BrEnd = NULL;
// image setup interfaces of selected dump containers
FASetImageA * g_pFollowImg = NULL;
FASetImageA * g_pPos2BrBegImg = NULL;
FASetImageA * g_pPos2BrEndImg = NULL;

FABrResult g_br_res (&g_alloc);

const int MaxChainSize = 4096;
int ChainBuffer [MaxChainSize];
int ChainSize;
/// for RS DFA
bool OutRes;
/// for Moore DFA
int OutChainBuffer [MaxChainSize];
/// for Moore Multi DFA
int OutResSizes [MaxChainSize];
const int * OutResPtrs [MaxChainSize];


void usage () {

  std::cout << "\n\
Usage: test_fsm [OPTIONS] [< input.txt] [> output.txt]\n\
\n\
Puts input lines into specified automaton and gets its reaction.\n\
\n\
  --fsm=<input-file> - reads automaton from <input-file>,\n\
    if omited stdin is used\n\
\n\
  --trbr-maps=<input-file> - reads triangular bracket extraction maps,\n\
    if omited reads from stdin\n\
\n\
  --pos-nfa=<input-file> - reads reversed position NFA from <input-file>,\n\
    needed for position reconstruction, if omited reads from stdin\n\
\n\
  --data=<input-text> - reads input text from <input-text> file,\n\
    if omited stdin is used\n\
\n\
  --fsm-type=<type> - specifies input automaton type:\n\
    rs-dfa     - Rabin-Scott DFA, the default value\n\
    moore-dfa  - Moore DFA\n\
    moore-mdfa - Moore DFA with multiple outputs\n\
";
  std::cout << "\n\
  --format=<type> - specifies the IO format\n\
    txt  - textual - container independent, is used by default\n\
    dump - memory dump - container dependent\n\
\n\
  --interpreter=<type> - specifies which rules interpreter to use:\n\
    triv - text matches with rules automaton in ordinary way,\n\
      used by default\n\
    fnfa - implicit factor NFA is constructed from the input text,\n\
      so the rules automaton does not require alphabet loop in the initial\n\
      state\n\
    pos  - determines which regular expression positions match which positions\n\
      in text, can be used with --fsm-type=moore-mdfa only.\n\
    trbr - makes triangular brackets extraction from the matched text,\n\
      can be used with --fsm-type=moore-mdfa only.\n\
\n\
  --mode=<type> - specifies in memory container type:\n\
    int - uses int-based containers for rules and digitizer (the default value)\n\
    triv-dump - trivial memory dump container\n\
\n\
  --rev - processes input in the right to left order\n\
\n\
  --print-input - prints input string to stdout\n\
\n\
  --no-output - does not do any output\n\
\n\
  --no-process - does not do any processing, I/O only\n\
\n\
";
}


void process_args (int& argc, char**& argv)
{
  for (; argc--; ++argv) {

    if (!strcmp ("--help", *argv)) {
        usage ();
        exit (0);
    }
    if (0 == strncmp ("--no-output", *argv, 11)) {
        g_no_output = true;
        continue;
    }
    if (0 == strncmp ("--no-process", *argv, 12)) {
        g_no_process = true;
        continue;
    }
    if (0 == strncmp ("--print-input", *argv, 13)) {
        g_print_input = true;
        continue;
    }
    if (0 == strncmp ("--fsm=", *argv, 6)) {
        g_pInFsmFile = &((*argv) [6]);
        continue;
    }
    if (0 == strncmp ("--trbr-maps=", *argv, 12)) {
        g_pInTrBrFile = &((*argv) [12]);
        continue;
    }
    if (0 == strncmp ("--pos-nfa=", *argv, 10)) {
        g_pInPosNfaFile = &((*argv) [10]);
        continue;
    }
    if (0 == strncmp ("--data=", *argv, 7)) {
        g_pInDataFile = &((*argv) [7]);
        continue;
    }
    if (0 == strcmp ("--fsm-type=rs-dfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_RS_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=moore-dfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MOORE_DFA;
        continue;
    }
    if (0 == strcmp ("--fsm-type=moore-mdfa", *argv)) {
        g_fsm_type = FAFsmConst::TYPE_MOORE_MULTI_DFA;
        continue;
    }
    if (0 == strcmp ("--interpreter=triv", *argv)) {
        g_int_type = FAFsmConst::INT_TRIV;
        continue;
    }
    if (0 == strcmp ("--interpreter=fnfa", *argv)) {
        g_int_type = FAFsmConst::INT_FNFA;
        continue;
    }
    if (0 == strcmp ("--interpreter=pos", *argv)) {
        g_int_type = FAFsmConst::INT_POS;
        continue;
    }
    if (0 == strcmp ("--interpreter=trbr", *argv)) {
        g_int_type = FAFsmConst::INT_TRBR;
        continue;
    }
    if (0 == strcmp ("--mode=int", *argv)) {
        g_mode = FAFsmConst::MODE_INT;
        continue;
    }
    if (0 == strcmp ("--mode=triv-dump", *argv)) {
        g_mode = FAFsmConst::MODE_PACK_TRIV;
        continue;
    }
    if (0 == strcmp ("--format=txt", *argv)) {
        g_format_type = FAFsmConst::FORMAT_TXT;
        continue;
    }
    if (0 == strcmp ("--format=dump", *argv)) {
        g_format_type = FAFsmConst::FORMAT_DUMP;
        continue;
    }
    if (0 == strncmp ("--rev", *argv, 5)) {
      g_reverse = true;
      continue;
    }
  }
}


void SetupPtrs ()
{
    if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

        g_pDfa = &g_fsm_rs;

        if (FAFsmConst::MODE_PACK_TRIV == g_mode) {

            g_pDfa = &g_rs_dump_triv;
            g_pSetDfaImg = &g_rs_dump_triv;
        }

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        g_pDfa = &g_fsm_rs;
        g_pState2Ow = &g_fsm_moore_ows;

        if (FAFsmConst::MODE_PACK_TRIV == g_mode) {

            g_pDfa = &g_rs_dump_triv;
            g_pSetDfaImg = &g_rs_dump_triv;
            g_pState2Ow = &g_fsm_moore_ow_dump_triv;
            g_pSetState2OwImg = &g_fsm_moore_ow_dump_triv;
        }

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        g_pDfa = &g_fsm_rs;
        g_pState2Ows = &g_fsm_moore_multi_ows;

        if (FAFsmConst::MODE_PACK_TRIV == g_mode) {

            g_pDfa = &g_rs_dump_triv;
            g_pSetDfaImg = &g_rs_dump_triv;
            g_pState2Ows = &g_fsm_moore_ows_dump_triv;
            g_pSetState2OwsImg = &g_fsm_moore_ows_dump_triv;
        }
    }

    if (FAFsmConst::FORMAT_DUMP != g_format_type) {

        g_pFollow = &g_follow;
        g_pPos2BrBeg = &g_pos2br_begin;
        g_pPos2BrEnd = &g_pos2br_end;

    } else {

        g_pFollow = &g_follow_dump;
        g_pFollowImg = &g_follow_dump;
        g_pPos2BrBeg = &g_pos2br_begin_dump;
        g_pPos2BrBegImg = &g_pos2br_begin_dump;
        g_pPos2BrEnd = &g_pos2br_end_dump;
        g_pPos2BrEndImg = &g_pos2br_end_dump;
    }
}


// loads automata and maps in textual format
void Load_txt ()
{
    std::istream * pFsmIs = &std::cin;
    std::ifstream fsm_ifs;

    std::istream * pTrBrIs = &std::cin;
    std::ifstream trbr_ifs;

    std::istream * pPosNfaIs = &std::cin;
    std::ifstream pos_nfa_ifs;

    if (NULL != g_pInFsmFile) {
        fsm_ifs.open (g_pInFsmFile, std::ios::in);
        FAAssertStream (&fsm_ifs, g_pInFsmFile);
        pFsmIs = &fsm_ifs;
    }
    if (NULL != g_pInPosNfaFile) {
        pos_nfa_ifs.open (g_pInPosNfaFile, std::ios::in);
        FAAssertStream (&pos_nfa_ifs, g_pInPosNfaFile);
        pPosNfaIs = &pos_nfa_ifs;
    }
    if (NULL != g_pInTrBrFile) {
        trbr_ifs.open (g_pInTrBrFile, std::ios::in);
        FAAssertStream (&trbr_ifs, g_pInTrBrFile);
        pTrBrIs = &trbr_ifs;
    }

    DebugLogAssert (pFsmIs);
    DebugLogAssert (pTrBrIs);
    DebugLogAssert (pPosNfaIs);

    if (FAFsmConst::TYPE_RS_DFA == g_fsm_type)
        g_aut_io.Read (*pFsmIs, &g_fsm_rs);
    else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type)
        g_aut_io.Read (*pFsmIs, &g_fsm_rs, &g_fsm_moore_ows);
    else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type)
        g_aut_io.Read (*pFsmIs, &g_fsm_rs, &g_fsm_moore_multi_ows);

    if (FAFsmConst::INT_POS == g_int_type || 
        FAFsmConst::INT_TRBR == g_int_type) {

        g_aut_io.Read (*pPosNfaIs, &g_follow);
        g_map_io.Read (*pTrBrIs, &g_pos2br_begin);
        g_map_io.Read (*pTrBrIs, &g_pos2br_end);
    }
}


// loads automata and maps in binary format
void Load_dump ()
{
    DebugLogAssert (g_pInFsmFile);

    // load automaton (RS, Moore or Multi Moore)
    g_FsmImage.Load (g_pInFsmFile);
    const unsigned char * pImg = g_FsmImage.GetImageDump ();
    DebugLogAssert (pImg);

    if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

        g_pSetDfaImg->SetImage (pImg);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        g_pSetDfaImg->SetImage (pImg);
        g_pSetState2OwImg->SetImage (pImg);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        g_pSetDfaImg->SetImage (pImg);
        g_pSetState2OwsImg->SetImage (pImg);
    }

    if (FAFsmConst::INT_POS == g_int_type || 
        FAFsmConst::INT_TRBR == g_int_type) {

        // load PosNfa and TrBr maps
        g_PosNfaImage.Load (g_pInPosNfaFile);
        pImg = g_PosNfaImage.GetImageDump ();
        DebugLogAssert (pImg);

        g_pFollowImg->SetImage (pImg);
        g_pPos2BrBegImg->SetImage (pImg);
        g_pPos2BrEndImg->SetImage (pImg);
    }
}


// can be done only after Dfa, Nfa and Maps are loaded
void Prepare ()
{
    if (FAFsmConst::INT_TRIV == g_int_type) {

        g_proc_triv.SetRsDfa (g_pDfa);
        g_proc_triv.SetState2Ow (g_pState2Ow);
        g_proc_triv.SetState2Ows (g_pState2Ows);

    } else if (FAFsmConst::INT_FNFA == g_int_type) {

        g_proc_fnfa.SetRsDfa (g_pDfa);
        g_proc_fnfa.SetState2Ow (g_pState2Ow);
        g_proc_fnfa.SetState2Ows (g_pState2Ows);

    } else if (FAFsmConst::INT_POS == g_int_type) {

        g_proc_pos.SetRsDfa (g_pDfa);
        g_proc_pos.SetState2Ows (g_pState2Ows);
        g_proc_pos.SetFollow (g_pFollow);

    } else if (FAFsmConst::INT_TRBR == g_int_type) {

        g_proc_trbr.SetRsDfa (g_pDfa);
        g_proc_trbr.SetState2Ows (g_pState2Ows);
        g_proc_trbr.SetPos2BrBegin (g_pPos2BrBeg);
        g_proc_trbr.SetPos2BrEnd (g_pPos2BrEnd);
        g_proc_trbr.SetFollow (g_pFollow);
    }
}


void String2Chain (const char * pStr, const int StrLen)
{
    DebugLogAssert (pStr && 0 < StrLen);

    ChainSize = 0;
    g_tokenizer.SetString (pStr, StrLen);

    int TmpInt;
    while (g_tokenizer.GetNextInt (&TmpInt)) {

        DebugLogAssert (ChainSize < MaxChainSize);
        ChainBuffer [ChainSize] = TmpInt;
        ChainSize++;
    }

    if (g_reverse) {

        for (int i = 0; i < ChainSize/2; ++i) {
            const int TmpSymbol = ChainBuffer [i];
            ChainBuffer [i] = ChainBuffer [ChainSize - i - 1];
            ChainBuffer [ChainSize - i - 1] = TmpSymbol;
        }
    }
}


void Print_rs (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (OutRes)
        (*pOs) << "Accepted\n";
    else
        (*pOs) << "Rejected\n";
}

void Print_moore (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (false == g_reverse)
        ::FAPrintArray (*pOs, OutChainBuffer, ChainSize);
    else
        ::FAPrintArray_rev (*pOs, OutChainBuffer, ChainSize);

    (*pOs) << '\n';
}

void Print_pos (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (OutRes) {

        if (false == g_reverse)
            ::FAPrintArray (*pOs, OutChainBuffer, ChainSize);
        else
            ::FAPrintArray_rev (*pOs, OutChainBuffer, ChainSize);

        (*pOs) << '\n';

    } else {

        (*pOs) << "Rejected\n";
    }
}

void Print_trbr (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (OutRes) {

        const int * pFromTo;
        int BrId = -1;
        int Count = g_br_res.GetNextRes (&BrId, &pFromTo);

        while (-1 != Count) {

            DebugLogAssert (0 == Count % 2);

            for (int i = 0; i < Count; ++i) {

                const int From = pFromTo [i++];
                const int To = pFromTo [i];

                if (false == g_reverse) {
                    (*pOs) << BrId << ' ' << From << ' ' << To << '\n';
                } else {
                    const int NewFrom = ChainSize - To - 1;
                    const int NewTo = ChainSize - From - 1;
                    (*pOs) << BrId << ' ' << NewFrom << ' ' << NewTo << '\n';
                }
            }

            Count = g_br_res.GetNextRes (&BrId, &pFromTo);

        } // of while ...

        (*pOs) << '\n';

    } else {

        (*pOs) << "Rejected\n";
    }
}

void Print_multi_more (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (false == g_reverse) {

        for (int i = 0; i < ChainSize; ++i) {

            const int Size = OutResSizes [i];

            if (0 < Size) {

                const int * pRuleNums = OutResPtrs [i];

                (*pOs) << i << " : ";
                ::FAPrintArray (*pOs, pRuleNums, Size);
                (*pOs) << ' ';
            }
        }

    } else {

        for (int i = ChainSize - 1; i >=0; --i) {

            const int Size = OutResSizes [i];

            if (0 < Size) {

                const int * pRuleNums = OutResPtrs [i];

                (*pOs) << i << " : ";
                ::FAPrintArray (*pOs, pRuleNums, Size);
                (*pOs) << ' ';
            }
        }
    }
    (*pOs) << '\n';
}

void Print (std::ostream * pOs)
{
    DebugLogAssert (pOs);

    if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

        Print_rs (pOs);

    } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

        Print_moore (pOs);

    } else if (FAFsmConst::INT_POS == g_int_type) {

        Print_pos (pOs);

    } else if (FAFsmConst::INT_TRBR == g_int_type) {

        Print_trbr (pOs);

    } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

        Print_multi_more (pOs);
    }
}


void Process (const int * pChain, const int Size)
{
    if (FAFsmConst::INT_TRIV == g_int_type) {

        if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

            OutRes = g_proc_triv.Chain2Bool (pChain, Size);

        } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

            g_proc_triv.Chain2OwChain (pChain, OutChainBuffer, Size);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

            g_proc_triv.Chain2OwSetChain (pChain, OutResPtrs, OutResSizes, Size);

        }

    } else if (FAFsmConst::INT_FNFA == g_int_type) {

        if (FAFsmConst::TYPE_RS_DFA == g_fsm_type) {

            OutRes = g_proc_fnfa.Chain2Bool (pChain, Size);

        } else if (FAFsmConst::TYPE_MOORE_DFA == g_fsm_type) {

            /// not implemented
            DebugLogAssert (0);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type) {

            g_proc_fnfa.Chain2OwSetChain (pChain, OutResPtrs, OutResSizes, Size);

        }

    } else if (FAFsmConst::INT_POS == g_int_type) {

        DebugLogAssert (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type);

        OutRes = g_proc_pos.Chain2PosChain (pChain, OutChainBuffer, Size);

    } else if (FAFsmConst::INT_TRBR == g_int_type) {

        DebugLogAssert (FAFsmConst::TYPE_MOORE_MULTI_DFA == g_fsm_type);

        g_br_res.Clear ();

        OutRes = g_proc_trbr.Chain2BrRes (pChain, Size, &g_br_res);
    }
}


int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    // parse a command line
    process_args (argc, argv);

    // setup allocator where needed
    g_pos2br_begin.SetAllocator (&g_alloc);
    g_pos2br_end.SetAllocator (&g_alloc);

    /// adjust the pointers
    SetupPtrs ();

    std::istream * pDataIs = &std::cin;
    std::ifstream data_ifs;

    std::ostream * pOs = &std::cout;
    std::ofstream ofs;

    if (NULL != g_pInDataFile) {
        data_ifs.open (g_pInDataFile, std::ios::in);
        FAAssertStream (&data_ifs, g_pInDataFile);
        pDataIs = &data_ifs;
    }
    if (NULL != g_pOutFile) {
        ofs.open (g_pOutFile, std::ios::out);
        pOs = &ofs;
    }

    DebugLogAssert (pDataIs);
    DebugLogAssert (pOs);

    try {

        // load automata and maps
        if (FAFsmConst::FORMAT_TXT == g_format_type) {

            Load_txt ();

        } else {

            DebugLogAssert (FAFsmConst::FORMAT_DUMP == g_format_type);
            Load_dump ();
        }

        // make processors ready
        Prepare ();

        std::string line;

        // read the input
        while (!pDataIs->eof ()) {

            if (!std::getline (*pDataIs, line))
                break;

            if (!line.empty ()) {

                if (g_print_input) {
                    (*pOs) << line << '\n';
                }

                const char * pChainStr = line.c_str ();
                const int StrLen = (const int) line.length ();
                DebugLogAssert (pChainStr && 0 < StrLen);

                String2Chain (pChainStr, StrLen);

                // process the input chain
                if (false == g_no_process) {

                    Process (ChainBuffer, ChainSize);

                    if (false == g_no_output)
                        Print (pOs);

                } // of if (false == g_no_process) ...

            } // of if if (!line.empty ()) ...

        } // of while (!pDataIs->eof ()) ...

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
