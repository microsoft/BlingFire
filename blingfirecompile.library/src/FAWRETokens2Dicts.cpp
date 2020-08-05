/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWRETokens2Dicts.h"
#include "FAAllocatorA.h"
#include "FAChain2NumA.h"
#include "FAMultiMapA.h"
#include "FATagSet.h"
#include "FAException.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FAWRETokens2Dicts::FAWRETokens2Dicts (FAAllocatorA * pAlloc) :
    m_pToken2Num (NULL),
    m_pTokenNum2CNF (NULL),
    m_pDictOs (NULL),
    m_token_parser (pAlloc),
    m_curr_token (pAlloc),
    m_pAlloc (pAlloc),
    m_min_type (1),
    m_pTagSet (NULL),
    m_pTagSet2 (NULL)
{
    m_token2type.SetAllocator (pAlloc);
    m_token2type.SetCopyChains (true);

    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();

    m_UnEsc.SetAllocator (pAlloc);
    m_UnEsc.Create ();

    m_dict_tags.SetAllocator (pAlloc);
    m_dict_tags.Create ();

    m_pos_tags.SetAllocator (pAlloc);
    m_pos_tags.Create ();
}


FAWRETokens2Dicts::~FAWRETokens2Dicts ()
{
    FAWRETokens2Dicts::Clear ();
}


void FAWRETokens2Dicts::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FAWRETokens2Dicts::SetTagSet2 (const FATagSet * pTagSet2)
{
    m_pTagSet2 = pTagSet2;
}


void FAWRETokens2Dicts::SetTokens (const FAChain2NumA * pToken2Num)
{
    m_pToken2Num = pToken2Num;
}


void FAWRETokens2Dicts::SetToken2CNF (FAMultiMapA* pTokenNum2CNF)
{
    m_pTokenNum2CNF = pTokenNum2CNF;
}


void FAWRETokens2Dicts::SetDictOs (std::ostream * pDictOs)
{
    m_pDictOs = pDictOs;
}


void FAWRETokens2Dicts::Clear ()
{
    m_token2type.Clear ();
    m_dict_tags.resize (0);
    m_pos_tags.resize (0);

    // TypeNums are 1-based
    m_min_type = 1;
}


void FAWRETokens2Dicts::Process ()
{
    DebugLogAssert (m_pToken2Num);

    Clear ();

    // parse all tokens
    ParseTokens ();

    // put data to build txt-digitizer
    PutTxtTypes ();

    // put data to build tag-digitizer
    if (m_pTagSet) {

        const int Count = m_pTagSet->GetStrCount ();
        m_pos_tags.resize (Count);

        for (int i = 0; i < Count; ++i) {
            const int Tag = m_pTagSet->GetValue (i);
            m_pos_tags [i] = Tag;
        }

        const int NewSize = \
            FASortUniq (m_pos_tags.begin (), m_pos_tags.end ());
        FAAssert (NewSize == Count, FAMsg::InternalError);

        if (0 < Count) {
            PutPosTags (m_pos_tags.begin (), Count);
        }
    }

    // put data to build dict-digitizer
    if (m_pTagSet2) {

        const int NewSize = \
            FASortUniq (m_dict_tags.begin (), m_dict_tags.end ());
        m_dict_tags.resize (NewSize);

        if (0 < NewSize) {
            PutDictTags (m_dict_tags.begin (), NewSize);
        }
    }

    // finilize
    PutDone ();
}


const int FAWRETokens2Dicts::
    UnEscape (const char * pStr, const int StrLen, const char ** ppOutStr)
{
    DebugLogAssert (ppOutStr);
    DebugLogAssert (0 < StrLen && pStr);

    m_UnEsc.resize (0);

    for (int i = 0; i < StrLen; ++i) {

        const char C = pStr [i];

        if ('\\' != C) {

            m_UnEsc.push_back (C);

        } else if (0 < i && '\\' == pStr [i - 1]) {

            DebugLogAssert ('\\' == C);
            m_UnEsc.push_back (C);
        }
    }

    *ppOutStr = m_UnEsc.begin ();
    return m_UnEsc.size ();
}


void FAWRETokens2Dicts::PutTxtTypes ()
{
    /// make iteration in the order the keys were added into m_token2type
    const int TypeCount = m_token2type.GetChainCount ();

    for (int i = 0; i < TypeCount; ++i) {

        const int * pChain = NULL;
        const int Size = m_token2type.GetChain (i, &pChain);
        DebugLogAssert (0 < Size);
        DebugLogAssert (pChain);

        /// interpret it as a sequence of bytes
        const char * pBegin = (const char *) pChain;
        const int KeyType = *pBegin;
        const int Length = (Size * sizeof(int)) - 1;
        pBegin++;

        if (KEY_WORDLIST == KeyType) {

            PutTxtWords (pBegin, Length);

        } else if (KEY_REGEXP == KeyType) {

            PutTxtRegexp (pBegin, Length);

        } else if (KEY_DICT_NAME == KeyType) {

            PutTxtDictName (pBegin, Length);

        } else {

            /// problems with m_token2type
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }

        if (m_pDictOs)
            (*m_pDictOs) << '\n';

    } // of for
}


void FAWRETokens2Dicts::ParseTokens ()
{
    DebugLogAssert (m_pToken2Num);

    const int ChainCount = m_pToken2Num->GetChainCount ();

    for (int TokenNum = 0; TokenNum < ChainCount; ++TokenNum) {

        // get next chain
        const int * pChain;
        const int ChainSize = m_pToken2Num->GetChain (TokenNum, &pChain);

        DebugLogAssert (pChain);
        DebugLogAssert (0 < ChainSize);

        // get token string
        const char * pTokenStr = (const char *) pChain;
        DebugLogAssert (pTokenStr);

        const int Len = ChainSize * sizeof (int);

        // parse next WRE token
        m_curr_token.Clear ();
        m_token_parser.SetTokenStr (pTokenStr, Len);
        m_token_parser.SetToken (&m_curr_token);
        m_token_parser.Process ();

        // process current token
        ProcessCurrToken (TokenNum);
    }
}


void FAWRETokens2Dicts::ProcessCurrToken (const int TokenNum)
{
    // produces output for Text digitizer

    // TODO : add explicit digitizer type setup
    DebugLogAssert (0 == FAFsmConst::DIGITIZER_TEXT);

    if (0 < m_curr_token.GetWordCount ()) {

        ProcessCurrWords (TokenNum);
    }
    if (0 < m_curr_token.GetRegexpCount ()) {

        ProcessCurrRegexps (TokenNum);
    }
    if (0 < m_curr_token.GetDictCount ()) {

        ProcessCurrDicts (TokenNum);
    }

    // produces output for Tag digitizer

    if (0 < m_curr_token.GetTagCount ()) {

        ProcessCurrTags (TokenNum);
    }
}


void FAWRETokens2Dicts::words2arr ()
{
    int i;

    const char * pTokenText = m_curr_token.GetStr ();
    DebugLogAssert (pTokenText);

    /// calc the summ length for all words plsu one byte for the key type
    int Length = 1;
    const int WordCount = m_curr_token.GetWordCount ();

    for (i = 0; i < WordCount; ++i) {

        /// get next word token
        const FAToken * pSimpleToken = m_curr_token.GetWordToken (i);
        DebugLogAssert (pSimpleToken);

        const int CurrLen = pSimpleToken->GetLength ();

        /// add word length plus 0-delimiter
        Length += (CurrLen + 1);
    }

    /// adjust chain's size
    int ChainLen = Length / sizeof (int);
    m_arr.resize (ChainLen);

    if (Length % sizeof (int)) {

        ChainLen++;
        m_arr.push_back (0);
    }

    // copy words into m_arr

    char * pBegin = (char *) m_arr.begin ();
    DebugLogAssert (pBegin);

    *pBegin = KEY_WORDLIST;
    pBegin++;

    for (i = 0; i < WordCount; ++i) {

        /// get next word token
        const FAToken * pSimpleToken = m_curr_token.GetWordToken (i);
        DebugLogAssert (pSimpleToken);

        const int Offset = pSimpleToken->GetOffset ();
        const int CurrLen = pSimpleToken->GetLength ();
        const char * pWordStr = pTokenText + Offset;

        /// copy word text
        memcpy (pBegin, pWordStr, CurrLen);
        /// copy 0-delimiter
        pBegin [CurrLen] = 0;

        pBegin += (CurrLen + 1);        
    }
}


void FAWRETokens2Dicts::regexp2arr (const int i)
{
    DebugLogAssert (m_curr_token.GetRegexpCount () > i);

    const FAToken * pSimpleToken = m_curr_token.GetRegexpToken (i);
    DebugLogAssert (pSimpleToken);

    const char * pTokenText = m_curr_token.GetStr ();
    DebugLogAssert (pTokenText);

    /// 1-byte will be added to make regexp '.' and word "." 
    /// having different representation
    const int Length = 1 + pSimpleToken->GetLength ();
    const int Offset = pSimpleToken->GetOffset ();
    const char * pRegexpStr = pTokenText + Offset;

    /// adjust chain's size
    int ChainLen = Length / sizeof (int);
    m_arr.resize (ChainLen);

    if (Length % sizeof (int)) {

        ChainLen++;
        m_arr.push_back (0);
    }

    /// copy regexp into the m_arr

    char * pBegin = (char *) m_arr.begin ();
    DebugLogAssert (pBegin);

    // copy the special byte
    *pBegin = KEY_REGEXP;
    pBegin++;

    // copy the rest
    memcpy (pBegin, pRegexpStr, Length - 1);
}


void FAWRETokens2Dicts::dict2arr (const int i)
{
    DebugLogAssert (m_curr_token.GetDictCount () > i);

    const FAToken * pSimpleToken = m_curr_token.GetDictToken (i);
    DebugLogAssert (pSimpleToken);

    const char * pTokenText = m_curr_token.GetStr ();
    DebugLogAssert (pTokenText);

    /// 1-byte will be added to make dictionary fruits and word "fruits" 
    /// having different representation
    const int Length = 1 + pSimpleToken->GetLength ();
    const int Offset = pSimpleToken->GetOffset ();
    const char * pDictStr = pTokenText + Offset;

    /// adjust chain's size
    int ChainLen = Length / sizeof (int);
    m_arr.resize (ChainLen);

    if (Length % sizeof (int)) {

        ChainLen++;
        m_arr.push_back (0);
    }

    /// copy regexp into the m_arr

    char * pBegin = (char *) m_arr.begin ();
    DebugLogAssert (pBegin);

    // copy the special byte
    *pBegin = KEY_DICT_NAME;
    pBegin++;

    // copy the rest
    memcpy (pBegin, pDictStr, Length - 1);
}


const int FAWRETokens2Dicts::arr2typenum ()
{
    const int * pBegin = m_arr.begin ();
    const int Size = m_arr.size ();

    int TypeNum = m_min_type;
    const int * pTypeNum = m_token2type.Get (pBegin, Size);

    if (NULL == pTypeNum) {

        m_token2type.Add (pBegin, Size, TypeNum);
        m_min_type++;

    } else {

        TypeNum = *pTypeNum;
    }

    return TypeNum;
}


void FAWRETokens2Dicts::StartDisjunct (const int TokenNum)
{
    DebugLogAssert (m_pTokenNum2CNF);

    /// get the number of values associated with the TokenNum
    const int * pValues;
    const int CNFSize = m_pTokenNum2CNF->Get (TokenNum, &pValues);

    /// see whether CNF is not empty
    if (-1 != CNFSize) {
        /// add disjunct delimiter (AND operator)
        m_pTokenNum2CNF->Add (TokenNum, 0);
    }
}


void FAWRETokens2Dicts::AddToDisjunct (const int TokenNum, 
                                       const int TypeNum, 
                                       const bool IsNegative)
{
    DebugLogAssert (m_pTokenNum2CNF);

    /// add TypeNum into the last disjunct
    if (IsNegative)
        m_pTokenNum2CNF->Add (TokenNum, -TypeNum);
    else
        m_pTokenNum2CNF->Add (TokenNum, TypeNum);
}


void FAWRETokens2Dicts::ProcessCurrWords (const int TokenNum)
{
    /// copy all words 0-delimited into m_arr
    words2arr ();

    /// generate new type id or get a ald one
    const int TypeNum = arr2typenum ();

    /// see whether word list is negative
    DebugLogAssert (0 < m_curr_token.GetWordCount ());

    const FAToken * pSimpleToken = m_curr_token.GetWordToken (0);
    DebugLogAssert (pSimpleToken);

    const bool IsNegative = \
      FAWREToken::TT_NEGATIVE_WORD == pSimpleToken->GetType ();

    // associate disjunct with the TokenNum
    StartDisjunct (TokenNum);
    AddToDisjunct (TokenNum, TypeNum, IsNegative);
}


void FAWRETokens2Dicts::ProcessCurrRegexps (const int TokenNum)
{
    /// see whether regexp list is disjunctive
    const int IsDisjuctive = m_curr_token.GetRegexpsDisj ();

    /// for ease of compilation each regexp is treated as separate type
    const int RegexpCount = m_curr_token.GetRegexpCount ();

    for (int i = 0; i < RegexpCount; ++i) {

        /// put it into array of ints
        regexp2arr (i);

        /// generate new type id or get a ald one
        const int TypeNum = arr2typenum ();

        /// see whether regexp is negative
        const FAToken * pSimpleToken = m_curr_token.GetRegexpToken (i);
        DebugLogAssert (pSimpleToken);

        const bool IsNegative = \
          FAWREToken::TT_NEGATIVE_REGEXP == pSimpleToken->GetType ();

        if (false == IsDisjuctive) {

            StartDisjunct (TokenNum);
            AddToDisjunct (TokenNum, TypeNum, IsNegative);

        } else {

            if (0 == i)
                StartDisjunct (TokenNum);

            AddToDisjunct (TokenNum, TypeNum, IsNegative);

        } // of if (false == IsDisjuctive) ... 
    } // of for
}


void FAWRETokens2Dicts::ProcessCurrDicts (const int TokenNum)
{
    // if tagset2 is specified then dictionary digitizer will be used
    // otherwise all dictionaries will be built into the text digitizer

    int DigTokenNum = TokenNum;

    if (m_pTagSet2) {
        DigTokenNum |= (FAFsmConst::DIGITIZER_DCTS << 24);
    }

    const char * pText = m_curr_token.GetStr ();
    DebugLogAssert (pText);

    /// see whether dict list is disjunctive
    const int IsDisjuctive = m_curr_token.GetDictsDisj ();

    const int DictCount = m_curr_token.GetDictCount ();

    for (int i = 0; i < DictCount; ++i) {

        int TypeNum;

        const FAToken * pSimpleToken = m_curr_token.GetDictToken (i);
        DebugLogAssert (pSimpleToken);

        const bool IsNegative = \
          FAWREToken::TT_NEGATIVE_DICT == pSimpleToken->GetType ();

        if (!m_pTagSet2) {

            /// generate a new type id or get a ald one
            dict2arr (i);
            TypeNum = arr2typenum ();

        } else {

            const int Offset = pSimpleToken->GetOffset ();
            const int Length = pSimpleToken->GetLength ();

            const char * pTagText;
            const int TagTextLen = UnEscape (pText + Offset, Length, &pTagText);
            DebugLogAssert (0 < TagTextLen && pTagText);

            const int Tag = m_pTagSet2->Str2Tag (pTagText, TagTextLen);

            if (0 > Tag) {
                FASyntaxError (pTagText, TagTextLen, 0, "Unknown dictionary tag.");
                throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
            }

            m_dict_tags.push_back (Tag);
            TypeNum = Tag + 1;
        }

        if (false == IsDisjuctive) {

            StartDisjunct (DigTokenNum);
            AddToDisjunct (DigTokenNum, TypeNum, IsNegative);

        } else {

            if (0 == i) {
                StartDisjunct (DigTokenNum);
            }

            AddToDisjunct (DigTokenNum, TypeNum, IsNegative);

        } // of if (false == IsDisjuctive) ... 
    } // of for
}


void FAWRETokens2Dicts::ProcessCurrTags  (const int TokenNum)
{
    if (!m_pTagSet) {
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    const int DigTokenNum = (FAFsmConst::DIGITIZER_TAGS << 24) | TokenNum;

    const char * pText = m_curr_token.GetStr ();
    DebugLogAssert (pText);
    const int IsDisjuctive = m_curr_token.GetTagsDisj ();
    const int TagCount = m_curr_token.GetTagCount ();

    for (int i = 0; i < TagCount; ++i) {

        const FAToken * pSimpleToken = m_curr_token.GetTagToken (i);
        DebugLogAssert (pSimpleToken);

        const bool IsNegative = \
          FAWREToken::TT_NEGATIVE_TAG == pSimpleToken->GetType ();

        const int Offset = pSimpleToken->GetOffset ();
        const int Length = pSimpleToken->GetLength ();

        const char * pTagText;
        const int TagTextLen = UnEscape (pText + Offset, Length, &pTagText);
        DebugLogAssert (0 < TagTextLen && pTagText);

        const int Tag = m_pTagSet->Str2Tag (pTagText, TagTextLen);

        if (0 > Tag) {
            FASyntaxError (pTagText, TagTextLen, 0, "Unknown POS tag.");
            throw FAException (FAMsg::SyntaxError, __FILE__, __LINE__);
        }

        const int TypeNum = Tag + 1;

        if (false == IsDisjuctive) {

            StartDisjunct (DigTokenNum);
            AddToDisjunct (DigTokenNum, TypeNum, IsNegative);

        } else {

            if (0 == i) {
                StartDisjunct (DigTokenNum);
            }

            AddToDisjunct (DigTokenNum, TypeNum, IsNegative);

        } // of if (false == IsDisjuctive) ... 
    } // of for
}



void FAWRETokens2Dicts::
    PutTxtWords (const char * pBegin, const int Length)
{
    DebugLogAssert (pBegin);
    DebugLogAssert (0 < Length);

    if (NULL == m_pDictOs)
        return;

    (*m_pDictOs) << "wordlist\n";

    for (int i = 0; i < Length; ++i) {

        const char C = pBegin [i];

        if (0 != C) {

            (*m_pDictOs) << C;

        } else {

            /// check for padding 0
            if (i + 1 < Length) {
                if (0 != pBegin [i + 1]) {
                    (*m_pDictOs) << '\n';
                }
            }

        } // of if (0 != C)

    } // of for

    (*m_pDictOs) << '\n';
}


void FAWRETokens2Dicts::
    PutTxtRegexp (const char * pBegin, const int Length)
{
    DebugLogAssert (pBegin);
    DebugLogAssert (0 < Length);

    if (NULL == m_pDictOs)
        return;

    (*m_pDictOs) << "regexp\n";

    for (int i = 0; i < Length; ++i) {

        const char C = pBegin [i];

        if (0 != C)
            (*m_pDictOs) << C;
        else
            break;
    }

    (*m_pDictOs) << '\n';
}


void FAWRETokens2Dicts::
    PutTxtDictName (const char * pBegin, const int Length)
{
    DebugLogAssert (pBegin);
    DebugLogAssert (0 < Length);

    if (NULL == m_pDictOs)
        return;

    (*m_pDictOs) << "dict\n";

    for (int i = 0; i < Length; ++i) {

        const char C = pBegin [i];

        if (0 != C)
            (*m_pDictOs) << C;
        else
            break;
    }

    (*m_pDictOs) << '\n';
}


void FAWRETokens2Dicts::
    PutPosTags (const int * /*pTags*/, const int /*Count*/)
{}


void FAWRETokens2Dicts::
    PutDictTags (const int * /*pTags*/, const int /*Count*/)
{}


void FAWRETokens2Dicts::PutDone ()
{}

}
