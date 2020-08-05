/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWRETokens2Digitizers.h"
#include "FAException.h"
#include "FAUtils.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace BlingFire
{


FAWRETokens2Digitizers::FAWRETokens2Digitizers (FAAllocatorA * pAlloc) :
    FAWRETokens2Dicts (pAlloc),
    m_NoTxt (true),
    m_NoTag (true),
    m_NoDct (true),
    m_pDictRoot (NULL),
    m_DynType (1),
    m_recode (pAlloc),
    m_re2dfa (pAlloc),
    m_wl2dfa (pAlloc),
    m_tynfa (pAlloc),
    m_dot_exp (pAlloc),
    m_nfa (pAlloc),
    m_nfa2dfa (pAlloc),
    m_dfa (pAlloc),
    m_dfa2mindfa (pAlloc),
    m_min_dfa (pAlloc),
    m_tyrs2tymoore (pAlloc),
    m_moore_dfa (pAlloc),
    m_moore_dfa_ows (pAlloc),
    m_pDictTags (NULL),
    m_DictTagCount (0),
    m_pTagDict (NULL),
    m_IdCount (0),
    m_classify (pAlloc),
    m_IniOw (DefOw),
    m_pPOSTags (NULL),
    m_POSTagCount (0)
{
    m_wl.SetAllocator (pAlloc);
    m_wl.Create ();

    m_tynfa.SetEpsilonIw (FAFsmConst::IW_EPSILON);
    m_tynfa.SetAnyIw (FAFsmConst::IW_ANY);
    m_tynfa.SetInitialTypeIw (DefInitTyIw);

    m_dot_exp.SetAnyIw (FAFsmConst::IW_ANY);
    m_dot_exp.SetOutNfa (&m_nfa);
    m_dot_exp.SetIwBase (FAFsmConst::IW_EPSILON + 1);
    m_dot_exp.SetIwMax (DefInitTyIw - 1);

    m_nfa2dfa.SetNFA (&m_nfa);
    m_nfa2dfa.SetDFA (&m_dfa);

    m_dfa2mindfa.SetInDfa (&m_dfa);
    m_dfa2mindfa.SetOutDfa (&m_min_dfa);

    m_type2ows.SetAllocator (pAlloc);

    m_tyrs2tymoore.SetInitialOw (DefOw);
    m_tyrs2tymoore.SetInitialTypeIw (DefInitTyIw);
    m_tyrs2tymoore.SetInDfa (&m_min_dfa);
    m_tyrs2tymoore.SetOutMooreDfa (&m_moore_dfa, &m_moore_dfa_ows);
    m_tyrs2tymoore.SetType2OwsMap (&m_type2ows);

    m_tag2ids.SetAllocator (pAlloc);
    m_id2ow.SetAllocator (pAlloc);
    m_id2ow.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_tmp2.SetAllocator (pAlloc);
    m_tmp2.Create ();
}


FAWRETokens2Digitizers::~FAWRETokens2Digitizers ()
{
    FAWRETokens2Digitizers::Clear ();
}


void FAWRETokens2Digitizers::SetDictRoot (const char * pDictRoot)
{
    m_pDictRoot = pDictRoot;
}


void FAWRETokens2Digitizers::SetEncodingName (const char * pEncName)
{
    m_recode.SetEncodingName (pEncName);
    m_re2dfa.SetEncodingName (pEncName);
    m_wl2dfa.SetEncodingName (pEncName);
}


void FAWRETokens2Digitizers::SetInitialTypeIw (const int InitialTypeIw)
{
    DebugLogAssert (0 < InitialTypeIw);

    m_tynfa.SetInitialTypeIw (InitialTypeIw);
    m_tyrs2tymoore.SetInitialTypeIw (InitialTypeIw);
    m_dot_exp.SetIwMax (InitialTypeIw - 1);
}


void FAWRETokens2Digitizers::SetInitialOw (const int Ow)
{
    m_IniOw = Ow;
    m_tyrs2tymoore.SetInitialOw (Ow);
}


void FAWRETokens2Digitizers::
    SetTagDict (const FADictInterpreter_t < int > * pTagDict)
{
    m_pTagDict = pTagDict;
}


void FAWRETokens2Digitizers::Clear ()
{
    FAWRETokens2Dicts::Clear ();

    m_tyrs2tymoore.Clear ();
    m_moore_dfa.Clear ();
    m_moore_dfa_ows.Clear ();
    m_wl.Clear ();
    m_wl.Create ();
    m_DynType = 1;

    m_pDictTags = NULL;
    m_DictTagCount = 0;
    m_IdCount = 0;
    m_tag2ids.Clear ();
    m_id2ow.Clear ();
    m_id2ow.Create ();

    m_pPOSTags = NULL;
    m_POSTagCount = 0;

    m_NoTxt = true;
    m_NoTag = true;
    m_NoDct = true;

    m_type2ows.Clear ();
}


const int FAWRETokens2Digitizers::
    GetLength (const char * pBuff, const int MaxBuffSize)
{
    DebugLogAssert (pBuff);

    for (int i = 0; i < MaxBuffSize; ++i) {
        if (0 == pBuff [i]) {
            return i;
        }
    }

    return MaxBuffSize;
}


// 0-separated word-list
void FAWRETokens2Digitizers::
    PutTxtWords (const char * pBegin, const int Length)
{
    DebugLogAssert (pBegin && 0 < Length);

    m_wl2dfa.SetStrList (pBegin, Length);
    m_wl2dfa.Process ();

    const FARSDfaA * pDfa = m_wl2dfa.GetRsDfa ();

    std::ostringstream TypeOs;
    TypeOs << "wordlist" << m_DynType++ << char (0);
    const std::string & TypeStr = TypeOs.str ();

    m_tynfa.AddTyDfa (TypeStr.c_str (), pDfa);
    m_NoTxt = false;
}


// regexp
void FAWRETokens2Digitizers::
    PutTxtRegexp (const char * pBegin, const int Length2)
{
    DebugLogAssert (pBegin && 0 < Length2);

    const int Length = GetLength (pBegin, Length2);

    m_re2dfa.SetRegexp (pBegin, Length);
    m_re2dfa.Process ();

    const FARSDfaA * pDfa = m_re2dfa.GetRsDfa ();

    std::ostringstream TypeOs;
    TypeOs << "regexp" << m_DynType++ << char (0);
    const std::string & TypeStr = TypeOs.str ();

    m_tynfa.AddTyDfa (TypeStr.c_str (), pDfa);
    m_NoTxt = false;
}


const bool FAWRETokens2Digitizers::ReadTxtDict (const char * pFileName)
{
    DebugLogAssert (pFileName);

    bool Res = false;
    m_wl.resize (0);

    std::string line;
    std::ifstream ifs;
    ifs.open (pFileName, std::ios::in);

    if (!ifs.good ()) {
        return Res;
    }

    while (!ifs.eof ()) {

        if (!std::getline (ifs, line))
            break;

        const char * pLineStr = line.c_str ();
        int LineLen = (const int) line.length ();

        if (0 < LineLen) {
            DebugLogAssert (pLineStr);
            if (0x0D == (unsigned char) pLineStr [LineLen - 1])
                LineLen--;
        }
        if (0 < LineLen) {

            if ('#' == *pLineStr)
                continue;

            const int OldSize = m_wl.size ();
            m_wl.resize (OldSize + LineLen + 1);

            char * pOut = m_wl.begin () + OldSize;
            memcpy (pOut, pLineStr, LineLen);
            pOut [LineLen] = 0;

            Res = true;
        }
    } // of while ...

    return Res;
}


const bool FAWRETokens2Digitizers::ReadRgxDict (const char * pFileName)
{ 
   DebugLogAssert (pFileName);

    bool Res = false;
    m_wl.resize (0);

    std::string line;
    std::ifstream ifs;
    ifs.open (pFileName, std::ios::in);

    if (!ifs.good ()) {
        return Res;
    }

    while (!ifs.eof ()) {

        if (!std::getline (ifs, line))
            break;

        const char * pLineStr = line.c_str ();
        int LineLen = (const int) line.length ();

        if (0 < LineLen) {
            DebugLogAssert (pLineStr);
            if (0x0D == (unsigned char) pLineStr [LineLen - 1])
                LineLen--;
        }
        if (0 < LineLen) {

            if ('#' == *pLineStr)
                continue;

            const int OldSize = m_wl.size ();
            m_wl.resize (OldSize + LineLen + 3);

            char * pOut = m_wl.begin () + OldSize;
            *pOut++ = '(';
            memcpy (pOut, pLineStr, LineLen);
            pOut += LineLen;
            *pOut++ = ')';
            *pOut = '|';

            Res = true;
        }
    } // of while ...

    // remove the last disjunction, if any
    if (0 < m_wl.size ()) {
        m_wl.pop_back ();
        DebugLogAssert (0 < m_wl.size ());
    }

    return Res;
}


// dict name: s.t. "$dicts_root/dict_$dict_name.txt" or 
// "$dicts_root/dict_$dict_name.rgx" should exist
void FAWRETokens2Digitizers::
    PutTxtDictName (const char * pBegin, const int Length2)
{
    DebugLogAssert (pBegin && 0 < Length2);

    const int Length = GetLength (pBegin, Length2);

    bool NotFound = true;
    const std::string TypeStr (pBegin, Length);

    std::string BaseName = std::string ("dict_") + TypeStr;
    if (NULL != m_pDictRoot) {
        BaseName = std::string (m_pDictRoot) + std::string ("/") + BaseName;
    }

    std::string TxtName = BaseName + std::string (".txt");
    std::string RgxName = BaseName + std::string (".rgx");

    if (ReadTxtDict (TxtName.c_str ())) {

        m_wl2dfa.SetStrList (m_wl.begin (), m_wl.size ());
        m_wl2dfa.Process ();
        const FARSDfaA * pDfa = m_wl2dfa.GetRsDfa ();
        m_tynfa.AddTyDfa (TypeStr.c_str (), pDfa);

        NotFound = false;
    }
    if (ReadRgxDict (RgxName.c_str ())) {

        m_re2dfa.SetRegexp (m_wl.begin (), m_wl.size ());
        m_re2dfa.Process ();
        const FARSDfaA * pDfa = m_re2dfa.GetRsDfa ();
        m_tynfa.AddTyDfa (TypeStr.c_str (), pDfa);

        NotFound = false;
    }
    if (NotFound) {
        const std::string msg = \
            std::string ("Can't open the dictionary '") + \
            TxtName + std::string ("' or '") + RgxName;
        FASyntaxError (NULL, 0, -1, msg.c_str ());
        throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
    }
    m_NoTxt = false;
}


void FAWRETokens2Digitizers::PutPosTags (const int * pTags, const int Count)
{
    m_pPOSTags = pTags;
    m_POSTagCount = Count;

    m_NoTag = (0 >= m_POSTagCount);
}


void FAWRETokens2Digitizers::PutDictTags (const int * pTags, const int Count)
{
    m_pDictTags = pTags;
    m_DictTagCount = Count;

    m_NoDct = (0 >= m_DictTagCount);
}


void FAWRETokens2Digitizers::BuildTxtDigitizer ()
{
    // 1. build common TyNfa
    m_tynfa.Process ();

    const FARSNfaA * pNfa = m_tynfa.GetNfa ();
    DebugLogAssert (pNfa);

    // 2. make global expansion for the '.'-symbol
    m_dot_exp.SetInNfa (pNfa);
    m_dot_exp.Process ();
    /// m_dot_exp.Clear ();  // TODO: add Clear to FAAny2AnyOther_global_t
    m_tynfa.Clear ();

    // 3. build DFA
    m_nfa2dfa.Process ();
    m_nfa.Clear ();
    m_nfa2dfa.Clear ();

    // add one more state, to be used as a dead
    const int MaxState = m_dfa.GetMaxState ();
    m_dfa.SetMaxState (MaxState + 1);

    // 4. build Min DFA
    m_dfa2mindfa.Process ();
    m_dfa.Clear ();
    m_dfa2mindfa.Clear ();

    // 5. build Moore digitizer
    m_tyrs2tymoore.Process ();
    m_min_dfa.Clear ();
}


void FAWRETokens2Digitizers::BuildTagDigitizer ()
{
    if (!m_pPOSTags || !m_pTagSet || 0 >= m_POSTagCount) {
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    /// all we do is to update < Dgt, TypeNum > -> { Ows } map
    for (int i = 0; i < m_POSTagCount; ++i) {

        const int Tag = m_pPOSTags [i];
        const int Type = Tag + 1;
        const int Ow = Tag + m_IniOw;

        m_type2ows.Set ((FAFsmConst::DIGITIZER_TAGS << 24) | Type, &Ow, 1);
    }
}


inline void FAWRETokens2Digitizers::BuildTag2Ids ()
{
    DebugLogAssert (false == m_NoDct);

    int i;
    int TagCount;
    m_IdCount = 0;
    m_tag2ids.Clear ();

    const int MaxTags = m_pTagDict->GetMaxInfoSize ();
    DebugLogAssert (0 < MaxTags);

    m_tmp.resize (MaxTags);
    int * pTags = m_tmp.begin (); // use m_tmp as Tag storage

    while (-1 != (TagCount = m_pTagDict->GetInfo (m_IdCount, pTags, MaxTags))){

        DebugLogAssert (TagCount <= MaxTags);
        DebugLogAssert (FAIsSortUniqed (pTags, TagCount));

        for (i = 0; i < m_DictTagCount; ++i) {

            DebugLogAssert (m_pDictTags);
            const int Tag = m_pDictTags [i];

            if (-1 != FAFind_log (pTags, TagCount, Tag)) {
                m_tag2ids.Add (Tag + 1, m_IdCount);
            }
        }
        m_IdCount++;
    }
    if (0 >= m_IdCount) {
        // the dictionary is empty
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }
}


inline void FAWRETokens2Digitizers::ClassifyIds ()
{
    DebugLogAssert (false == m_NoDct);
    DebugLogAssert (0 < m_IdCount);

    m_classify.Prepare (m_IdCount);
    m_tmp.resize (m_IdCount);
    int * pI = m_tmp.begin ();  // use m_tmp as pI storage

    for (int i = 0; i < m_DictTagCount; ++i) {

        DebugLogAssert (m_pDictTags);
        const int Tag = m_pDictTags [i];

        memset (pI, 0, sizeof (int) * m_IdCount);

        const int * pIds;
        const int Count = m_tag2ids.Get (Tag + 1, &pIds);
        DebugLogAssert (0 < Count && pIds);

        for (int j = 0; j < Count; ++j) {

            const int Id = pIds [j];
            DebugLogAssert (0 <= Id && Id < m_IdCount);

            pI [Id] = 1;
        }

        m_classify.AddInfo (pI, m_IdCount);
    }
    m_classify.Process ();

    const int * pI2C = NULL;
#ifndef NDEBUG
    const int I2CSize = 
#endif
        m_classify.GetClasses (&pI2C);
    DebugLogAssert (I2CSize == m_IdCount && pI2C);

    // copy Id2Ow map into m_id2ow
    m_id2ow.resize (m_IdCount);
    int * pId2Ow = m_id2ow.begin ();
    for (int j = 0; j < m_IdCount; ++j) {
        const int C = pI2C [j];
        pId2Ow [j] = C + m_IniOw;
    }

    m_classify.Clear ();
}


inline void FAWRETokens2Digitizers::BuildTag2Ows ()
{
    DebugLogAssert (false == m_NoDct);
    DebugLogAssert (0 < m_IdCount);

    // mark all Ids as unused (not being referenced by any Tag)
    m_tmp2.resize (m_IdCount);
    m_tmp2.set_bits (0, m_IdCount - 1, true);

    for (int i = 0; i < m_DictTagCount; ++i) {

        DebugLogAssert (m_pDictTags);
        const int Tag = m_pDictTags [i];

        // get Ids where Tag is mentioned
        const int * pIds;
        const int Count = m_tag2ids.Get (Tag + 1, &pIds);

        if (0 > Count) {
            // there is no data in the dictionary for the Tag
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }

        // remap each Id into corresponding Ow
        m_tmp.resize (Count);
        for (int j = 0; j < Count; ++j) {

            DebugLogAssert (pIds);
            const int Id = pIds [j];
            DebugLogAssert (0 <= Id && Id < m_IdCount);

            // mark the Id as referenced
            m_tmp2.set_bit (Id, false);

            const int Ow = m_id2ow [Id];
            m_tmp [j] = Ow;
        }

        // add dict-digitizer entries
        const int TyNum = (FAFsmConst::DIGITIZER_DCTS << 24) | (Tag + 1);
        m_type2ows.Set (TyNum, m_tmp.begin (), Count);
    }
    // make iteration thru the unreferenced Ids
    for (int j = 0; j < m_IdCount; ++j) {
        if (m_tmp2.get_bit (j)) {
            m_id2ow [j] = FAFsmConst::IW_ANY;
        }
    }
    // no longer needed
    m_tag2ids.Clear ();
}


void FAWRETokens2Digitizers::BuildDctDigitizer ()
{
    DebugLogAssert (FAIsSortUniqed (m_pDictTags, m_DictTagCount));

    if (!m_pTagDict || !m_pTagSet2 || 0 >= m_DictTagCount) {
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    /// 1. build tag2ids map
    BuildTag2Ids ();

    /// 2. classify ids
    ClassifyIds ();

    /// 3. build tag2ows map
    BuildTag2Ows ();

    /// 4. local clean
    m_tmp.Clear ();
    m_tmp.Create ();
    m_tmp2.Clear ();
    m_tmp2.Create ();
}


void FAWRETokens2Digitizers::PutDone ()
{
    if (!m_NoTxt) {
        BuildTxtDigitizer ();
    }
    if (!m_NoTag) {
        BuildTagDigitizer ();
    }
    if (!m_NoDct) {
        BuildDctDigitizer ();
    }
    m_type2ows.SortUniq ();
}


const FAMultiMapA * FAWRETokens2Digitizers::GetType2Ows () const
{
    return & m_type2ows;
}


const bool FAWRETokens2Digitizers::
    GetTxtDigititizer (
        const FARSDfaA ** ppDfa, 
        const FAState2OwA ** pState2Ow
    ) const
{
    DebugLogAssert (ppDfa && pState2Ow);

    *ppDfa = &m_moore_dfa;
    *pState2Ow = &m_moore_dfa_ows;

    return !m_NoTxt;
}


const bool FAWRETokens2Digitizers::GetTagDigitizer (const int **, int *) const
{
    DebugLogAssert (0);
    return !m_NoTag;
}


const bool FAWRETokens2Digitizers::
    GetDictDigitizer (const int ** ppId2Ow, int * pId2OwSize) const
{
    DebugLogAssert (ppId2Ow && pId2OwSize);

    *ppId2Ow = m_id2ow.begin ();
    *pId2OwSize = m_id2ow.size ();

    return !m_NoDct;
}

}
