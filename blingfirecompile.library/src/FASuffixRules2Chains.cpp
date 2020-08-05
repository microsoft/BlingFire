/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASuffixRules2Chains.h"
#include "FAUtf8Utils.h"
#include "FATagSet.h"
#include "FAUtils.h"
#include "FAException.h"

#include <string>
#include <iomanip>

namespace BlingFire
{


FASuffixRules2Chains::FASuffixRules2Chains (FAAllocatorA * pAlloc) :
    m_use_utf8 (false),
    m_KeyBase (0),
    m_NumSize (DefNumSize),
    m_ImplicitDelim (false),
    m_pIs (NULL),
    m_pOs (NULL),
    m_pTagSet (NULL),
    m_recode (pAlloc),
    m_pStr (NULL),
    m_StrLen (-1),
    m_TagFrom (-1),
    m_TagTo (-1),
    m_SuffixCut (-1),
    m_PrefixCut (-1)
{
    m_acts.SetAllocator (pAlloc);

    m_act2num.SetAllocator (pAlloc);

    m_suffix_chain.SetAllocator (pAlloc);
    m_suffix_chain.Create ();

    m_prefix_chain.SetAllocator (pAlloc);
    m_prefix_chain.Create ();

    m_left_chain.SetAllocator (pAlloc);
    m_left_chain.Create ();

    m_right_chain.SetAllocator (pAlloc);
    m_right_chain.Create ();

    m_Ow2Freq.SetAllocator (pAlloc);
    m_Ow2Freq.Create ();
}


void FASuffixRules2Chains::Clear ()
{
    m_acts.Clear ();
    m_act2num.Clear ();
    m_Ow2Freq.Clear ();
    m_Ow2Freq.Create ();
}


void FASuffixRules2Chains::SetEncodingName (const char * pInputEnc)
{
    m_recode.SetEncodingName (pInputEnc);
    m_use_utf8 = FAIsUtf8Enc (pInputEnc);
}


void FASuffixRules2Chains::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FASuffixRules2Chains::SetKeyBase (const int KeyBase)
{
    m_KeyBase = KeyBase;
}


void FASuffixRules2Chains::SetNumSize (const int  NumSize)
{
  m_NumSize = NumSize;
}


void FASuffixRules2Chains::SetImplicitDelim (const bool ImplicitDelim)
{
    m_ImplicitDelim = ImplicitDelim;
}


void FASuffixRules2Chains::SetRulesIn (std::istream * pIs)
{
    m_pIs = pIs;
}


void FASuffixRules2Chains::SetChainsOut (std::ostream * pOs)
{
    m_pOs = pOs;
}


const FAMultiMapA * FASuffixRules2Chains::GetActions () const
{
    return & m_acts;
}


const int FASuffixRules2Chains::GetOw2Freq (const int ** pOw2Freq) const
{
    DebugLogAssert (pOw2Freq);

    *pOw2Freq = m_Ow2Freq.begin ();
    const int Count = m_Ow2Freq.size ();

    return Count;
}


inline const int FASuffixRules2Chains::GetDelimPos () const
{
    DebugLogAssert (0 < m_StrLen && m_pStr);

    for (int i = 0; i < m_StrLen - 5; ++i) {

        if (' ' == m_pStr [i + 0] && \
            '-' == m_pStr [i + 1] && \
            '-' == m_pStr [i + 2] && \
            '>' == m_pStr [i + 3] && \
            ' ' == m_pStr [i + 4])
            return i;

    } // for (int i = 0; ...

    return -1;
}


inline void FASuffixRules2Chains::
    Str2Arr (
        const char * pStr, 
        const int StrLen, 
        FAArray_cont_t < int > * pArr
    )
{
    DebugLogAssert (pArr);

    if (0 < StrLen) {

        DebugLogAssert (pStr);

        if (m_use_utf8) {

            pArr->resize (StrLen);
            int * pBegin = pArr->begin ();

            const int Size = \
                FAStrUtf8ToArray (pStr, StrLen, pBegin, StrLen);

            if (0 >= Size) {
                throw FAException (FAMsg::IOError, __FILE__, __LINE__);
            }

            pArr->resize (Size);

        } else {

            pArr->resize (StrLen);
            int * pBegin = pArr->begin ();

#ifndef NDEBUG
            const int Count = 
#endif
                m_recode.Process (pStr, StrLen, pBegin, StrLen);
            DebugLogAssert (StrLen == Count);

        } // of if (m_use_utf8) ...

    } else {

        pArr->resize (0);
    } // of if (0 < StrLen) ...
}


/// parses "suffix_i", see header
void FASuffixRules2Chains::
    ParseLeft (const int LeftPos, const int LeftLen)
{
    DebugLogAssert (0 < m_StrLen && m_pStr);
    DebugLogAssert (0 < LeftLen && LeftLen < m_StrLen);

    // put suffix into m_left_chain as an array of UTF-32 symbols
    Str2Arr (m_pStr + LeftPos, LeftLen, &m_left_chain);

    // see whether left part is not empty
    if (0 == m_left_chain.size ()) {
        FASyntaxError (m_pStr, m_StrLen, LeftPos,
                         "Left part is empty or wrong encoding has been setup.");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }
}

/// parses "-N_i+str_i" chunks
inline void FASuffixRules2Chains::
    ParseAct (
            const char * pRight,
            const int RightLen,
            int * pCut,
            FAArray_cont_t < int > * pArr
        )
{
    DebugLogAssert (pCut && pArr);

    *pCut = 0;
    pArr->resize (0);
    int Pos = 0;

    if (RightLen > Pos && '-' == pRight [Pos]) {
        // skip the minus
        Pos++;
        // get the cut-off length
        *pCut = atoi (pRight + Pos);
        DebugLogAssert (0 <= *pCut);
        // skip digits, upto the end or upto the '+'
        while (RightLen > Pos && '+' != pRight [Pos]) {
            Pos++;
        }
        if (RightLen > Pos) {
            // skip the plus
            DebugLogAssert ('+' == pRight [Pos]);
            Pos++;
            // read-in the new prefix/suffix, it can be empty
            Str2Arr (pRight + Pos, RightLen - Pos, pArr);
        }
    }
}


/// parses "[FromTag][ToTag]-N_i+str_i", see the header
void FASuffixRules2Chains::
    ParseRight (const int RightPos, const int RightLen)
{
    DebugLogAssert (0 < m_StrLen && m_pStr);
    DebugLogAssert (0 < RightPos && 0 < RightLen && RightLen < m_StrLen);

    m_TagFrom = -1;
    m_TagTo = -1;
    m_SuffixCut = 0;
    m_PrefixCut = -1;
    m_suffix_chain.resize (0);
    m_prefix_chain.resize (0);

    const char * pRight = m_pStr + RightPos;
    int Pos = 0;

    // setup tags' positions
    if (m_pTagSet) {

        int TagFromPos = -1;
        int TagFromLen = -1;
        int TagToPos = -1;
        int TagToLen = -1;

        // find tag boundaries
        for (; Pos < RightLen - 1; ++Pos) {

            if ('[' == pRight [Pos] && -1 == TagFromPos) {

                TagFromPos = Pos + 1;

            } else if (']' == pRight [Pos] && -1 == TagFromLen) {

                TagFromLen = Pos - TagFromPos;
                Pos++;

                if ('[' == pRight [Pos])
                    TagToPos = Pos + 1;
                else
                    break;

            } else if (']' == pRight [Pos] && -1 == TagToLen) {
                TagToLen = Pos - TagToPos;
                Pos++;
                break;
            }
        }
        // this tag must exist
        if (-1 != TagFromPos && -1 != TagFromLen) {

            m_TagFrom = m_pTagSet->Str2Tag (pRight + TagFromPos, TagFromLen);

            if (-1 == m_TagFrom) {
                FASyntaxError (m_pStr, m_StrLen, RightPos + TagFromPos, 
                                 "Unknown FromTag was specified.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }

        } else {
            FASyntaxError (m_pStr, m_StrLen, RightPos,
                             "Rule has no conversion tags but tagset was specified.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }
        // this tag can not exist
        if (-1 != TagToPos && -1 != TagToLen) {

            m_TagTo = m_pTagSet->Str2Tag (pRight + TagToPos, TagToLen);

            if (-1 == m_TagTo) {
                FASyntaxError (m_pStr, m_StrLen, RightPos + TagToPos,
                                 "Unknown ToTag was specified.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }
        }
    } // of if (m_pTagSet) ...

    if (RightLen > Pos && '-' == pRight [Pos]) {

        /// parse the suffix
        const char * pSuff = pRight + Pos;
        int SuffLen = 0;
        while (RightLen > Pos + SuffLen && '\t' != pRight [Pos + SuffLen]) {
            SuffLen++;
        }
        Pos += SuffLen;

        ParseAct (pSuff, SuffLen, &m_SuffixCut, &m_suffix_chain);

        /// parse the prefix, if exists
        if (RightLen > Pos) {

            // skip the tab 
            Pos++;

            const char * pPref = pRight + Pos;
            const int PrefLen = RightLen - Pos;

            ParseAct (pPref, PrefLen, &m_PrefixCut, &m_prefix_chain);
        }

    } else {
        FASyntaxError (m_pStr, m_StrLen, RightPos + Pos,
                         "Right part action is missing.");
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }
}


inline const int FASuffixRules2Chains::
    GetKey (const int * pChain, const int Size)
{
    DebugLogAssert (pChain && 0 < Size);

    const int Key = m_act2num.GetIdx (pChain, Size);

    if (-1 == Key) {
        const int NewKey = m_act2num.Add (pChain, Size, 1);
        return NewKey;
    }

    const int Freq = m_act2num.GetValue (Key);
    DebugLogAssert (0 < Freq);

#ifndef NDEBUG
    const int NewKey =
#endif
        m_act2num.Add (pChain, Size, Freq + 1); // does not add, if exists
    DebugLogAssert (NewKey == Key);

    return Key;
}


void FASuffixRules2Chains::PrepareChains ()
{
    int i;

    m_right_chain.resize (0);

    /// copy prefix, if needed
    const int PrefixSize = m_prefix_chain.size ();
    if (0 < PrefixSize || 0 < m_PrefixCut) {
        // prefix action indicator
        const int _flag_ = PrefixSize + 2 + FALimits::MaxWordSize;
        m_right_chain.push_back (_flag_);
        m_right_chain.push_back (m_PrefixCut);
        // copy the prefix itself
        for (i = 0; i < PrefixSize; ++i) {
            const int C = m_prefix_chain [i];
            m_right_chain.push_back (C);
        }
    }
    /// copy suffix
    m_right_chain.push_back (m_SuffixCut);
    const int SuffixSize = m_suffix_chain.size ();
    for (i = 0; i < SuffixSize; ++i) {
        const int C = m_suffix_chain [i];
        m_right_chain.push_back (C);
    }

    /// Map the m_suffix_chain into a unique Key
    const int Key = GetKey (m_right_chain.begin (), m_right_chain.size ());

    /// Reverse the m_left_chain
    int Size = m_left_chain.size ();
    DebugLogAssert (0 < Size);

    for (i = 0; i < (Size / 2); ++i) {
        const int Tmp = m_left_chain [Size - i - 1];
        m_left_chain [Size - i - 1] = m_left_chain [i];
        m_left_chain [i] = Tmp;
    }

    /// See whether '^' is the last symbol
    bool ExplicitDelim = false;
    if ('^' == m_left_chain [Size - 1]) {
        ExplicitDelim = true;
        m_left_chain.pop_back ();
        Size--;
    }

    /// Put tags, if any, into the beginning of the m_left_chain 
    /// and Key into the end
    int Delta = 0;
    if (-1 != m_TagFrom) {
        Delta++;
        if (-1 != m_TagTo) {
            Delta++;
        }
    }
    if (0 < Delta) {

        m_left_chain.resize (Size + Delta);

        for (i = Size - 1; i >= 0; --i) {
            m_left_chain [i + Delta] = m_left_chain [i];
        }
        if (-1 != m_TagFrom) {
            m_left_chain [0] = m_TagFrom;
            if (-1 != m_TagTo) {
                m_left_chain [1] = m_TagTo;
            }
        }
    }
    if (ExplicitDelim || m_ImplicitDelim) {
        m_left_chain.push_back (0);
    }
    m_left_chain.push_back (m_KeyBase + Key);
}


void FASuffixRules2Chains::PrintLeft () const
{
    DebugLogAssert (m_pOs);

    const int * pChain = m_left_chain.begin ();
    const int ChainSize = m_left_chain.size ();
    DebugLogAssert (0 < ChainSize && pChain);

    (*m_pOs) << std::setw (m_NumSize) << std::setfill ('0') << pChain [0];

    for (int i = 1; i < ChainSize; ++i) {
        (*m_pOs) << ' ' << std::setw (m_NumSize) << std::setfill ('0') << pChain [i];
    }

    (*m_pOs) << '\n';
}


void FASuffixRules2Chains::BuildMap ()
{
    const int ChainCount = m_act2num.GetChainCount ();
    DebugLogAssert (0 < ChainCount);

    m_Ow2Freq.resize (ChainCount);

    for (int ChainIdx = 0; ChainIdx < ChainCount; ++ChainIdx) {

        const int * pChain;
        const int ChainSize = m_act2num.GetChain (ChainIdx, &pChain);
        const int Freq = m_act2num.GetValue (ChainIdx);

        m_acts.Set (ChainIdx, pChain, ChainSize);
        m_Ow2Freq [ChainIdx] = Freq;

    } // of for (int ChainIdx = 0; ...
}


void FASuffixRules2Chains::Process ()
{
    DebugLogAssert (m_pIs && m_pOs);

    Clear ();

    std::string line;

    while (!m_pIs->eof ()) {

        if (!std::getline (*m_pIs, line)) {
            break;
        }

        m_pStr = line.c_str ();
        m_StrLen = (int) line.length ();

        if (0 < m_StrLen && 0x0D == m_pStr [m_StrLen - 1]) {
            m_StrLen--;
        }
        if (0 == m_StrLen) {
            continue;
        }
        DebugLogAssert (m_pStr);

        // get delimiter position
        const int DelimPos = GetDelimPos ();

        if (-1 == DelimPos) {
            FASyntaxError (m_pStr, m_StrLen, -1, "Invalid rule.");
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        const int LeftPos = 0;
        const int LeftLen = DelimPos;

        const int RightPos = DelimPos + 5;
        const int RightLen = m_StrLen - RightPos;

        ParseLeft (LeftPos, LeftLen);
        ParseRight (RightPos, RightLen);

        // build left and right chains ready to use
        PrepareChains ();

        // output left part chain
        PrintLeft ();

    } // of while (!m_pIs->eof ()) ...

    // builds action map
    BuildMap ();
}

}
