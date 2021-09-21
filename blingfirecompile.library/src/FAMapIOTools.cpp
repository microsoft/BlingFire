/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMapIOTools.h"
#include "FAAllocatorA.h"
#include "FAMapA.h"
#include "FAMultiMapA.h"
#include "FAChain2NumA.h"
#include "FAStr2IntA.h"
#include "FAArray_cont_t.h"
#include "FAStringTokenizer.h"
#include "FAException.h"
#include "FAUtils.h"

#include <string>

namespace BlingFire
{


FAMapIOTools::FAMapIOTools (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc)
{
    m_tmp_arr.Clear ();
    m_tmp_arr.SetAllocator (pAlloc);
    m_tmp_arr.Create ();

    m_tmp_farr.Clear ();
    m_tmp_farr.SetAllocator (pAlloc);
    m_tmp_farr.Create ();
}


void FAMapIOTools::Read (std::istream& is, FAMapA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int Key = -1;
    int Value = -1;

    FAStringTokenizer tokenizer;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        std::string::size_type EndOfLine = line.find_last_not_of("\r\n");
        if (EndOfLine != std::string::npos) {
            line.erase(EndOfLine + 1);
        }

        const char * pLine = line.c_str ();
        const int LineLen = (const int) line.length ();

        tokenizer.SetString (pLine, LineLen);

        bool res = tokenizer.GetNextInt (&Key);
        res = res && tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&Value);
        FAAssert (res, FAMsg::IOError);
        FAAssert (0 == strncmp ("->", pTmpStr, TmpStrLen), FAMsg::IOError);

        pMap->Set (Key, Value);
    }
}


void FAMapIOTools::Print (std::ostream& os, const FAMapA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    int Key = -1;
    const int * pValue = pMap->Prev (&Key);

    while (NULL != pValue) {

        os << Key << " -> " << *pValue << '\n';

        pValue = pMap->Prev (&Key);
    }

    // print final empty line
    os << '\n'; 
}


void FAMapIOTools::Read (std::istream& is, FAMultiMapA * pMMap)
{
    FAAssert (pMMap, FAMsg::IOError);

    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int Key = -1;
    int Size = -1;

    FAStringTokenizer tokenizer;

    FAArray_cont_t < int > vals;
    vals.SetAllocator (m_pAlloc);
    vals.Create ();

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLine = line.c_str ();
        const int LineLen = (const int) line.length ();

        tokenizer.SetString (pLine, LineLen);

        bool res = tokenizer.GetNextInt (&Key);
        res = res && tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&Size);
        FAAssert (res, FAMsg::IOError);

        FAAssert (0 == strncmp ("->", pTmpStr, TmpStrLen), FAMsg::IOError);
        FAAssert (0 <= Size, FAMsg::IOError);

        vals.resize (Size);

        res = tokenizer.GetArray (vals.begin (), vals.size ());
        FAAssert (res, FAMsg::IOError);

        pMMap->Set (Key, vals.begin (), Size);
    }
}


void FAMapIOTools::Print (std::ostream& os, const FAMultiMapA * pMMap)
{
    FAAssert (pMMap, FAMsg::IOError);

    const int * pValues;
    int Key = -1;
    int Size = pMMap->Prev (&Key, &pValues);

    while (-1 != Size) {

        os << Key << " -> " << Size ;

        for (int i = 0; i < Size; ++i) {

            FAAssert (pValues, FAMsg::IOError);
            const int Val = pValues [i];

            os << ' ' << Val;
        }

        os << '\n';

        Size = pMMap->Prev (&Key, &pValues);
    }
    // print final empty line
    os << '\n';
}


void FAMapIOTools::Read (std::istream& is, FAChain2NumA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    int Val, Size;
    std::string line;
    const char * pTmpStr;
    int TmpStrLen;

    FAStringTokenizer tokenizer;

    FAArray_cont_t < int > chain;
    chain.SetAllocator (m_pAlloc);
    chain.Create ();

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLine = line.c_str ();
        const int LineLen = (const int) line.length ();

        tokenizer.SetString (pLine, LineLen);

        // read the chain size
        bool res = tokenizer.GetNextInt (&Size);
        FAAssert (res, FAMsg::IOError);
        FAAssert (0 < Size, FAMsg::IOError);

        chain.resize (Size);

        // read the chain
        res = tokenizer.GetArray (chain.begin (), chain.size ());
        FAAssert (res, FAMsg::IOError);

        // read string delimiter
        res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        FAAssert (res, FAMsg::IOError);
        FAAssert (0 == strncmp ("->", pTmpStr, TmpStrLen), FAMsg::IOError);

        // read value
        res = tokenizer.GetNextInt (&Val);
        FAAssert (res, FAMsg::IOError);

        pMap->Add (chain.begin (), Size, Val);
    }
}


void FAMapIOTools::Print (std::ostream& os, const FAChain2NumA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    const int ChainCount = pMap->GetChainCount ();

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int * pChain;
        const int ChainSize = pMap->GetChain (ChainIdx, &pChain);
        const int Value = pMap->GetValue (ChainIdx);

        FAAssert (pChain, FAMsg::IOError);
        FAAssert (0 < ChainSize, FAMsg::IOError);

        os << ChainSize << ' ';
        for (int i = 0; i < ChainSize; ++i) {
            os << pChain [i] << ' ';
        }
        os << "-> " << Value << '\n';
    }
    // print final empty line
    os << '\n';
}


void FAMapIOTools::Read (std::istream& is, FAStr2IntA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    int Val, i;
    std::string line;
    std::string str;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLineStr = line.c_str ();
        FAAssert (pLineStr, FAMsg::IOError);

        // find the last space
        for (i = (const int) line.length () - 1; i >= 0; --i) {
            if (' ' == pLineStr [i])
                break;
        }

        if (0 < i) {

            str.assign (pLineStr, 0, i);
            Val = atoi (pLineStr + i + 1);

            const char * pStr = str.c_str ();
            const int StrLen = (const int) str.length ();

            pMap->Add (pStr, StrLen, Val);
        }
    }
}


void FAMapIOTools::Print (std::ostream& os, const FAStr2IntA * pMap)
{
    FAAssert (pMap, FAMsg::IOError);

    std::string str;

    const int StrCount = pMap->GetStrCount ();

    for (int i = 0; i < StrCount; ++i) {

        const char * pStr;
        const int Size = pMap->GetStr (i, &pStr);
        FAAssert (pStr && 0 < Size, FAMsg::IOError);

        str.assign (pStr, 0, Size);

        const int Value = pMap->GetValue (i);

        os << str << ' ' << Value << '\n';
    }
    os << '\n';
}


void FAMapIOTools::Read (std::istream& is, const int ** ppArr, int * pCount)
{
    FAAssert (ppArr, FAMsg::IOError);
    FAAssert (pCount, FAMsg::IOError);

    m_tmp_arr.resize (0);
    std::string line;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLineStr = line.c_str ();
        FAAssert (pLineStr, FAMsg::IOError);

        const int Value = atoi (pLineStr);
        m_tmp_arr.push_back (Value);
    }

    // copy pointers
    *ppArr = m_tmp_arr.begin ();
    *pCount = m_tmp_arr.size ();
}


void FAMapIOTools::Print (std::ostream& os, const int * pArr, const int Count)
{
    FAAssert (pArr, FAMsg::IOError);
    FAAssert (0 < Count, FAMsg::IOError);

    for (int i = 0; i < Count; ++i) {
        const int Value = pArr [i];
        os << Value << '\n';
    }

    os << '\n';
}


void FAMapIOTools::Read (std::istream& is, const float ** ppArr, int * pCount)
{
    FAAssert (ppArr, FAMsg::IOError);
    FAAssert (pCount, FAMsg::IOError);

    m_tmp_farr.resize (0);
    std::string line;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLineStr = line.c_str ();
        FAAssert (pLineStr, FAMsg::IOError);

        const float Value = (float) atof (pLineStr);
        m_tmp_farr.push_back (Value);
    }

    // copy pointers
    *ppArr = m_tmp_farr.begin ();
    *pCount = m_tmp_farr.size ();
}


void FAMapIOTools::Print (std::ostream& os, const float * pArr, const int Count)
{
    FAAssert (pArr, FAMsg::IOError);
    FAAssert (0 < Count, FAMsg::IOError);

    for (int i = 0; i < Count; ++i) {
        const float Value = pArr [i];
        os << Value << '\n';
    }

    os << '\n';
}


void FAMapIOTools::Read (std::istream& is, FAArray_cont_t < unsigned char > * pBuff, FAArray_cont_t < int > * pOffsets, bool fStringFormat)
{
    FAAssert (pBuff, FAMsg::IOError);
    FAAssert (pOffsets, FAMsg::IOError);

    pBuff->clear();
    pOffsets->clear();

    std::string line;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // interpret the data as a string or as a sequence of numbers
        if (fStringFormat) {
            
            const int Offset = pBuff->size();
            pOffsets->push_back(Offset);
            if (0 < line.length()) {
                pBuff->resize(pBuff->size() + line.length());
                memcpy(pBuff->begin () + Offset, line.c_str(), line.length());
            }

        } else {

            const int MaxTmpSize = 4096;
            unsigned char Tmp [MaxTmpSize];
            const int Count2 = FAReadIntegerChain <unsigned char> (line.c_str(), line.length(), 10, Tmp, MaxTmpSize);
            FAAssert(0 <= Count2 && MaxTmpSize > Count2, FAMsg::IOError);

            const int Offset = pBuff->size();
            pOffsets->push_back(Offset);
            if (0 < Count2) {
                pBuff->resize(pBuff->size() + Count2);
                memcpy(pBuff->begin () + Offset, Tmp, Count2);
            }
        }
    }
}


void FAMapIOTools::Print (std::ostream& os, const FAArray_cont_t < unsigned char > * pBuff, const FAArray_cont_t < int > * pOffsets, bool fStringFormat)
{
    FAAssert (pBuff, FAMsg::IOError);
    FAAssert (pOffsets, FAMsg::IOError);

    const unsigned char * pBuffData = pBuff->begin();

    for(int i = 0; i < pOffsets->size(); ++i) {

        const unsigned char * pBegin = pBuffData + (*pOffsets)[i];
        const unsigned char * pEnd = i + 1 == pOffsets->size() ? pBuff->end() : pBuffData + (*pOffsets)[i + 1];

        if (fStringFormat) {
            os << std::string (pBegin, pEnd) << '\n';
        } else {
            const size_t Count = pEnd - pBegin;
            for (int i = 0; i < Count; ++i) {
                if (0 != i) {
                    os << ' ';
                }
                os << (long) pBegin [i] << '\n';
            }
        }
    }
}


}
