/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FALimits.h"
#include "FAParser2WRE.h"
#include "FARegexpTree.h"
#include "FARegexpTree2Funcs.h"
#include "FAStringTokenizer.h"
#include "FAUtils.h"
#include "FAException.h"
#include "FAFsmConst.h"
#include "FAAct2ArrA.h"

#include <sstream>
#include <iomanip>

namespace BlingFire
{


FAParser2WRE::FAParser2WRE (FAAllocatorA * pAlloc) :
    m_pLeftIs (NULL),
    m_pRightIs (NULL),
    m_pInTagSet (NULL),
    m_pLeftOs (NULL),
    m_OutTagSet (pAlloc),
    m_ExtTagBase (1),
    m_ExtTagCount (0),
    m_LabelType (FAFsmConst::LABEL_WRE),
    m_pRuleStr (NULL),
    m_RuleStrLen (0),
    m_RuleNum (0),
    m_tmp_nfa (pAlloc),
    m_regexp2nfa (pAlloc),
    m_pFuncs (NULL),
    m_tree2str (pAlloc),
    m_lcs (0),
    m_rcs (0),
    m_LcCut (0),
    m_pAct2Arr (NULL)
{
    m_tmp_nfa.SetAnyIw (FAFsmConst::IW_ANY);
    m_tmp_nfa.SetAnchorLIw (FAFsmConst::IW_L_ANCHOR);
    m_tmp_nfa.SetAnchorRIw (FAFsmConst::IW_R_ANCHOR);

    m_regexp2nfa.SetLabelType (m_LabelType);
    m_regexp2nfa.SetNfa (&m_tmp_nfa);

    m_acts.SetAllocator (pAlloc);
    m_acts.Create ();

    m_i2a.SetAllocator (pAlloc);
    m_i2a.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_tmp2.SetAllocator (pAlloc);
    m_tmp2.Create ();

    m_size2pos_lc.SetAllocator (pAlloc);
    m_size2pos_rc.SetAllocator (pAlloc);
    m_out_i2a.SetAllocator (pAlloc);
}


void FAParser2WRE::Clear ()
{
    m_acts.resize (0);
    m_i2a.resize (0);
    m_right2left.clear ();
}


void FAParser2WRE::SetActionParser (FAAct2ArrA * pAct2Arr)
{
    m_pAct2Arr = pAct2Arr;

    if (NULL != m_pAct2Arr) {
        // let the parser use the output tagset, so it knows function ids
        m_pAct2Arr->SetTagSet (&m_OutTagSet);
    }
}


void FAParser2WRE::SetLcCut (const int LcCut)
{
    m_LcCut = LcCut;
}


void FAParser2WRE::SetLabelType (const int LabelType)
{
    DebugLogAssert (FAFsmConst::LABEL_DIGIT == LabelType || \
            FAFsmConst::LABEL_CHAR == LabelType || \
            FAFsmConst::LABEL_WRE == LabelType);

    m_LabelType = LabelType;

    m_regexp2nfa.SetLabelType (LabelType);
}


void FAParser2WRE::SetUseUtf8 (const bool UseUtf8)
{
    m_regexp2nfa.SetUseUtf8 (UseUtf8);
}


void FAParser2WRE::SetLeftIn (std::istream * pLeftIs)
{
    m_pLeftIs = pLeftIs;
}


void FAParser2WRE::SetRightIn (std::istream * pRightIs)
{
    m_pRightIs = pRightIs;
}


void FAParser2WRE::SetInTagSet (const FATagSet * pInTagSet)
{
    m_pInTagSet = pInTagSet;
}


void FAParser2WRE::SetLeftOut (std::ostream * pLeftOs)
{
    m_pLeftOs = pLeftOs;
}


const FATagSet * FAParser2WRE::GetOutTagSet () const
{
    return & m_OutTagSet;
}


const FAMultiMapA * FAParser2WRE::GetActMap () const
{
    return & m_out_i2a;
}


inline const int FAParser2WRE::GetRootNode () const
{
    const FARegexpTree * pTree = m_regexp2nfa.GetRegexpTree ();
    DebugLogAssert (pTree);

    const int RootId = pTree->GetRoot ();

    if (-1 == RootId) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "fatal error");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    return RootId;
}


inline const int FAParser2WRE::GetTrBrNode () const
{
    const FARegexpTree * pTree = m_regexp2nfa.GetRegexpTree ();
    DebugLogAssert (pTree);

    int TrBrNode = -1;

    const int MaxNodeId = pTree->GetMaxNodeId ();

    for (int i = 0; i <= MaxNodeId; ++i) {

        const int TrBr = pTree->GetTrBr (i);

        if (-1 != TrBr) {
            if (-1 == TrBrNode) {
                TrBrNode = i;
            } else {
                FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, \
                    "Multiple triangular brackets are not allowed in parser rules.");
                throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
            }
        }
    } // of for (int i = 0; ...

    if (-1 == TrBrNode) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, \
            "No triangular bracket was found.");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    return TrBrNode;
}


void FAParser2WRE::CalcContexts ()
{
    m_pFuncs = m_regexp2nfa.GetRegexpFuncs ();
    DebugLogAssert (m_pFuncs);

    // get root's first and last sets
    const int RootNode = GetRootNode ();

    const int * pFirst;
    const int FirstSize = m_pFuncs->GetFirstPos (RootNode, &pFirst);

    if (0 >= FirstSize || false == FAIsSortUniqed (pFirst, FirstSize)) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "fatal error");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    const int * pLast;
    const int LastSize = m_pFuncs->GetLastPos (RootNode, &pLast);

    if (0 >= LastSize || false == FAIsSortUniqed (pLast, LastSize)) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "fatal error");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    // get trbr's first and last sets
    const int TrBrNode = GetTrBrNode ();

    const int * pTrBrFirst;
    const int TrBrFirstSize = m_pFuncs->GetFirstPos (TrBrNode, &pTrBrFirst);

    if (0 >= TrBrFirstSize || \
        false == FAIsSortUniqed (pTrBrFirst, TrBrFirstSize)) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "fatal error");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    const int * pTrBrLast;
    const int TrBrLastSize = m_pFuncs->GetLastPos (TrBrNode, &pTrBrLast);

    if (0 >= TrBrLastSize || \
        false == FAIsSortUniqed (pTrBrLast, TrBrLastSize)) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "fatal error");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    // return context structures into the initial state
    m_size2pos_lc.Clear ();
    m_size2pos_rc.Clear ();
    m_tmp.resize (0);
    m_tmp2.resize (0);

    // get positions which textually follow TrBrNode
    const int TrBrMaxPos = pTrBrLast [TrBrLastSize - 1];

    for (int i = 0; i < TrBrLastSize; ++i) {

        const int Pos = pTrBrLast [i];

        const int * pNext;
        const int NextSize = m_pFuncs->GetFollowPos (Pos, &pNext);
        DebugLogAssert (0 < NextSize && pNext);

        for (int j = 0; j < NextSize; ++j) {

            const int NextPos = pNext [j];

            if (NextPos > TrBrMaxPos) {
                m_tmp2.push_back (NextPos);
            }
        }
    }

    const int NewSize = FASortUniq (m_tmp2.begin (), m_tmp2.end ());
    m_tmp2.resize (NewSize);
    DebugLogAssert (0 < NewSize);

    // calc left context
    CalcContext_rec (pFirst, FirstSize, pTrBrFirst, TrBrFirstSize, &m_size2pos_lc);

    // calc right context
    DebugLogAssert (0 == m_tmp.size ());
    CalcContext_rec (m_tmp2.begin (), NewSize, pLast, LastSize, &m_size2pos_rc);
}


inline void FAParser2WRE::UpdateMap (
        FAMultiMapA * pMap,
        const int Key,
        const int * pVals,
        const int Count
    )
{
    DebugLogAssert (pMap);
    DebugLogAssert (0 <= Key);
    DebugLogAssert ((0 == Key && 0 == Count) || (0 < Count && pVals));

    // make sure there is only one mapping for Key == 0
    if (0 == Key) {

        const int * pRes;
        const int ResCount = pMap->Get (Key, &pRes);

        if (0 < ResCount) {
            return;
        }
    }

    for (int i = 0; i < Count; ++i) {
        const int Val = pVals [i];
        pMap->Add (Key, Val);
    }

    pMap->Add (Key, -1);
}


void FAParser2WRE::Validate (
        const int Pos,
        const int * pFollow,
        const int Count
    ) const
{
    DebugLogAssert (m_pFuncs);
    DebugLogAssert (0 <= Pos && m_pFuncs->GetMaxPos () > Pos);
    DebugLogAssert (0 < Count && pFollow);

    if (0 >= Count || false == FAIsSortUniqed (pFollow, Count)) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, \
            "Empty expressions are not allowed within the brackets.");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    for (int i = 0; i < Count; ++i) {

        const int Follow = pFollow [i];
        DebugLogAssert (0 <= Follow && m_pFuncs->GetMaxPos () >= Follow);

        if (Follow <= Pos) {
            FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, \
                "Context contains cycles.");
            throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
        }
    }
}


void FAParser2WRE::CalcContext_rec (
        const int * pFrom,
        const int FromSize,
        const int * pTo,
        const int ToSize,
        FAMultiMapA * pMap)
{
    DebugLogAssert (m_pFuncs);
    DebugLogAssert (pMap);
    DebugLogAssert (0 < FromSize && pFrom);
    DebugLogAssert (0 < ToSize && pTo);

    if (DefMaxContext < m_tmp.size ()) {
        FASyntaxError (m_pRuleStr, m_RuleStrLen, -1, "Context is too long.");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    for (int i = 0; i < FromSize; ++i) {

        const int Pos = pFrom [i];
        DebugLogAssert (0 <= Pos && m_pFuncs->GetMaxPos () >= Pos);

        if (-1 != FAFind_log (pTo, ToSize, Pos)) {

            const int Size = m_tmp.size ();
            const int * pTmp = m_tmp.begin ();

            // Size --> [ TmpContent ], mapping
            UpdateMap (pMap, Size, pTmp, Size);

        } else {

            const int * pFollow;
            const int FollowSize = m_pFuncs->GetFollowPos (Pos, &pFollow);

            Validate (Pos, pFollow, FollowSize);

            m_tmp.push_back (Pos);
            CalcContext_rec (pFollow, FollowSize, pTo, ToSize, pMap);
            m_tmp.pop_back ();
        }

    } // of for (int i = 0; ...
}


inline void FAParser2WRE::AddRule ()
{
    DebugLogAssert (0 < m_B.length ());
    DebugLogAssert ((unsigned int) m_RuleNum < m_i2a.size ());

    const std::string LeftBr ("(");
    const std::string RightBr (")");
    const std::string Or ("|");

    const char * pActStr = m_acts.begin () + m_i2a [m_RuleNum];
    DebugLogAssert (pActStr < m_acts.end ());

    const int ActStrLen = (int) strlen (pActStr);
    DebugLogAssert (0 < ActStrLen);

    // the adjust values for the braket context
    int LeftCxAdjust = 0;
    int RightCxAdjust = 0;

    /// build "right" side of the rule: action array without context
    std::ostringstream RightActOs;

    if (NULL == m_pAct2Arr) {

        FAStringTokenizer tokenizer;
        tokenizer.SetString (pActStr, ActStrLen);

        const char * pTmpStr;
        int TmpStrLen;
        bool CallsFunction = false;

        /// iterate thru the tokens of the action
        while (tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {
            // skip the "_call" keyword
            if (0 == strncmp ("_call", pTmpStr, 5)) {
                // print a delimiter
                RightActOs << "\t" << std::setw (5) << 0 ;
                CallsFunction = true;
            // check for _main "function" call
            } else if (0 == strncmp ("_main", pTmpStr, 5)) {
                // print a delimiter
                RightActOs << "\t" << std::setw (5) << 0 ;
                CallsFunction = false;
            // check for left context bracket adjustment: e.g. <-3 <+100
            } else if ( 3 <= TmpStrLen && '<' == pTmpStr [0] &&
                       ('+' == pTmpStr [1] || '-' == pTmpStr [1]) &&
                       ('0' <= pTmpStr [2] && '9' >= pTmpStr [2])) {
                // for the run-time bigger values mean narrower bracket context
                LeftCxAdjust = - atoi (pTmpStr+1);
            // check for right context bracket adjustment: e.g. >-3 >+100
            } else if ( 3 <= TmpStrLen && '>' == pTmpStr [0] &&
                       ('+' == pTmpStr [1] || '-' == pTmpStr [1]) &&
                       ('0' <= pTmpStr [2] && '9' >= pTmpStr [2])) {
                // for the run-time bigger values mean narrower bracket context
                RightCxAdjust = - atoi (pTmpStr+1);
            } else {
                // print a tag
                const int Tag = m_OutTagSet.Str2Tag (pTmpStr, TmpStrLen);
                FAAssert (-1 != Tag, FAMsg::InternalError);
                RightActOs << '\t' << std::setw (5) << Tag;
                CallsFunction = false;
            }
        } // of while (tokenizer.GetNextStr ...

        // if there was a _call, there needs to be a function after it
        FAAssert (!CallsFunction, FAMsg::InternalError);

    } else {
        // make custom parsing
        const int * pArr = NULL;
        const int ArrCount = m_pAct2Arr->Process (pActStr, ActStrLen, &pArr, 
            &LeftCxAdjust, &RightCxAdjust);
        FAAssert (0 < ArrCount, FAMsg::IOError);
        // store the resulting array from the custom parser
        for (int i = 0; i < ArrCount; ++i) {
            RightActOs << '\t' << std::setw (5) << pArr [i];
        }
    }

    // get a string
    const std::string & RightActStr = RightActOs.str ();

    /// build full "right" side of the rule: append context and action array
    std::ostringstream RightOs;
    RightOs << std::setfill ('0') 
        << std::setw (3) << (m_lcs + LeftCxAdjust) 
        << '\t' << std::setw (3) << (m_rcs + RightCxAdjust)
        << RightActStr;

    const std::string & Right = RightOs.str ();

    /// build "left" side of the rule
    std::string Left = LeftBr + m_B + RightBr;
    if (0 < m_Lc.length ()) {
        Left = LeftBr + m_Lc + RightBr + Left;
    }
    if (0 < m_Rc.length ()) {
        Left = Left + LeftBr + m_Rc + RightBr;
    }
    if (0 < m_Fn.length ()) {
        Left = LeftBr + m_Fn + RightBr + LeftBr + Left + RightBr;
    }
    if (0 < m_Lc.length () || 0 < m_Rc.length () || 0 < m_Fn.length ()) {
        Left = LeftBr + Left + RightBr;
    }

    std::map < std::string, std::string >::iterator I = \
        m_right2left.find (Right);

    if (I != m_right2left.end ()) {
        I->second = I->second + Or + Left;
    } else {
        m_right2left.insert (std::make_pair (Right, Left));
    }
}


inline void FAParser2WRE::BuildBody ()
{
    const FARegexpTree * pTree = m_regexp2nfa.GetRegexpTree ();
    DebugLogAssert (pTree);

    m_tree2str.SetRegexp (m_pRuleStr, m_RuleStrLen);
    m_tree2str.SetRegexpTree (pTree);

    const int TrBrNode = GetTrBrNode ();

    const char * pBodyText = m_tree2str.Process (TrBrNode);
    DebugLogAssert (pBodyText);

    m_B = std::string (pBodyText);
}


inline void FAParser2WRE::BuildContext (
        const int * pPos,
        const int Count,
        std::string * pContext
    )
{
    DebugLogAssert (m_pFuncs);
    DebugLogAssert (0 < Count && pPos);
    DebugLogAssert (pContext);

    (*pContext) = "";

    if (1 < Count) {

        m_disj_set.clear ();

        const FARegexpTree * pTree = m_regexp2nfa.GetRegexpTree ();
        DebugLogAssert (pTree);

        const std::string LeftBr ("(");
        const std::string RightBr (")");
        const std::string Or ("|");

        std::string disj;

        for (int i = 0; i < Count; ++i) {

            const int Pos = pPos [i];

            if (-1 != Pos) {

                const int NodeId = m_pFuncs->GetNodeId (Pos);
                DebugLogAssert (-1 != NodeId);

                const int Offset = pTree->GetOffset (NodeId);
                DebugLogAssert (0 <= Offset && Offset < m_RuleStrLen);
                const int Length = pTree->GetLength (NodeId);
                // 0 < Length, as Pos refers to the valid symbols
                DebugLogAssert (0 < Length && Offset + Length <= m_RuleStrLen);

                std::string Symbol = std::string (m_pRuleStr + Offset, Length);
                disj = disj + (LeftBr + Symbol + RightBr);

            } else {

                m_disj_set.insert (disj);
                disj = "";
            }
        } // of for (int i = 0; i < Count - 1; ...

        std::set < std::string >::const_iterator I = m_disj_set.begin ();
        std::set < std::string >::const_iterator End = m_disj_set.end ();

        if (I != End) {
            (*pContext) = (*pContext) + (LeftBr + *I + RightBr);
            ++I;
        }
        for (; I != End; ++I) {
            (*pContext) = (*pContext) + (Or + LeftBr + *I + RightBr);
        }

    } // of if (1 < Count) ...
}


void FAParser2WRE::AddRules ()
{
    // build m_B
    BuildBody ();

    // make iteration thru the m_size2pos_lc and m_size2pos_rc
    int LeftKey = -1;
    const int * pLeftValues;
    int LeftSize = m_size2pos_lc.Prev (&LeftKey, &pLeftValues);

    while (-1 != LeftSize) {

        m_lcs = LeftKey;

        BuildContext (pLeftValues, LeftSize, &m_Lc);

        int RightKey = -1;
        const int * pRightValues;
        int RightSize = m_size2pos_rc.Prev (&RightKey, &pRightValues);

        while (-1 != RightSize) {

            m_rcs = RightKey;

            BuildContext (pRightValues, RightSize, &m_Rc);

            AddRule ();

            RightSize = m_size2pos_rc.Prev (&RightKey, &pRightValues);
        }

        LeftSize = m_size2pos_lc.Prev (&LeftKey, &pLeftValues);
    }
}


void FAParser2WRE::ProcessLeft ()
{
    DebugLogAssert (m_pLeftIs);

    m_Fn = std::string ("");

    std::string line;
    m_RuleNum = -1;

    while (!(m_pLeftIs->eof ())) {

        std::string Re;

        // read until the empty line is met
        if (!std::getline (*m_pLeftIs, line)) {
            break;
        }

        int LineLen = (const int) line.length ();
        const char * pLine = line.c_str ();

        if (0 < LineLen && \
            0x0D == (unsigned char) pLine [LineLen - 1]) {
            LineLen--;
        }

        // see if this is a _function keyword
        if (0 == strncmp ("_function ", pLine, 10)) {

            // check whether function name is a known tag
            const char * pName = pLine + 10;
            const int NameLen = LineLen - 10;
            int Tag = m_OutTagSet.Str2Tag (pName, NameLen);

            if (-1 == Tag) {
                Tag = m_ExtTagBase + m_ExtTagCount;
                m_OutTagSet.Add (pName, NameLen, Tag);
                m_ExtTagCount++;
            }

            // use left anchor as a function regexp prefix
            const std::string FnPref = "$ " ;

            if (FAFsmConst::LABEL_WRE == m_LabelType) {
                m_Fn = FnPref + std::string (pName, NameLen);
            } else if (FAFsmConst::LABEL_CHAR == m_LabelType) {
                std::ostringstream TagValueOs;
                TagValueOs << FnPref << "\\x" << std::setfill ('0') << \
                    std::hex << std::setw (5) << Tag;
                m_Fn = TagValueOs.str ();
            } else if (FAFsmConst::LABEL_DIGIT == m_LabelType) {
                std::ostringstream TagValueOs;
                TagValueOs << FnPref << Tag;
                m_Fn = TagValueOs.str ();
            }
            continue;
        }
        // see if this is _end of function
        if (0 == strncmp ("_end", pLine, 4)) {
            // reset the m_Fn value
            m_Fn = std::string ("");
            continue;
        }

        while (0 < LineLen) {

            Re = Re + std::string (pLine, LineLen) + " ";

            if (m_pLeftIs->eof ()) {
                break;
            }
            if (!std::getline (*m_pLeftIs, line)) {
                break;
            }

            LineLen = (const int) line.length ();
            pLine = line.c_str ();

            if (0 < LineLen && \
                0x0D == (unsigned char) pLine [LineLen - 1]) {
                LineLen--;
            }
        }

        m_pRuleStr = Re.c_str ();
        m_RuleStrLen = (const int) Re.length ();

        if (0 < m_RuleStrLen) {

            DebugLogAssert (m_pRuleStr);

            m_RuleNum++;

            if ((unsigned int) m_RuleNum >= m_i2a.size ()) {
                FASyntaxError (NULL, 0, -1, \
                    "Number of rules does not match the number of actions.");
                throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
            }

            // make regexp analysis
            m_tmp_nfa.Clear ();
            m_tmp_nfa.SetRegexp (m_pRuleStr, m_RuleStrLen);
            m_regexp2nfa.SetRegexp (m_pRuleStr, m_RuleStrLen);
            m_regexp2nfa.Process ();
            m_tmp_nfa.Prepare ();

            // calc left/right context size
            CalcContexts ();

            // add one by one fixed context rules
            AddRules ();

        } else {

            break;
        }
    } // of while

    // final check
    if ((unsigned int) (m_RuleNum + 1) != m_i2a.size ()) {
        FASyntaxError (NULL, 0, -1, \
            "Number of rules does not match the number of actions.");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }
}


void FAParser2WRE::WriteOutput ()
{
    int Action [DefMaxActSize];
    int RuleNum = 0;

    FAStringTokenizer tokenizer;
    tokenizer.SetSpaces ("\t");

    std::map <std::string, std::string>::const_iterator I =
        m_right2left.begin ();

    for (; m_right2left.end () != I; ++I) {

        // put Left part into the output stream
        const char * pLeft = I->second.c_str ();
        DebugLogAssert (pLeft);

        if (m_pLeftOs) {
            (*m_pLeftOs) << pLeft << "\n\n";
        }

        // put corresponding Right part into the action map
        const char * pRight = I->first.c_str ();
        const int RightLen = (int) I->first.length ();
        DebugLogAssert (pRight && 0 < RightLen);

        tokenizer.SetString (pRight, RightLen);
        const int ActSize = tokenizer.GetArray2 (Action, DefMaxActSize);

        // reduce left context, if needed
        Action [0] -= m_LcCut;

        /// a few validations before the data are stored

        // actions size should be in between [3..100]
        FAAssert (3 <= ActSize && ActSize <= DefMaxActSize, FAMsg::InternalError);
        // left context can be negative or positive
        FAAssert (-FALimits::MaxArrSize <= Action [0] && FALimits::MaxArrSize >= Action [0],
            FAMsg::InternalError);
        // left context can be negative or positive
        FAAssert (-FALimits::MaxArrSize <= Action [1] && FALimits::MaxArrSize >= Action [1],
            FAMsg::InternalError);
        // tag should be either 0 (no tag) or positive
        FAAssert (0 <= Action [2], FAMsg::InternalError);

        // add action into the action map
        m_out_i2a.Set (RuleNum++, Action, ActSize);
    }
}


void FAParser2WRE::ProcessRight ()
{
    DebugLogAssert (m_pRightIs);

    FAStringTokenizer tokenizer;

    m_ExtTagCount = 0;

    std::string line;

    while (!(m_pRightIs->eof ())) {

        if (!std::getline (*m_pRightIs, line)) {
            break;
        }

        const char * pLine = line.c_str ();
        int LineLen = (const int) line.length ();

        if (0 < LineLen && \
            0x0D == (unsigned char) pLine [LineLen - 1]) {
            LineLen--;
        }

        if (0 < LineLen) {

            DebugLogAssert (pLine);

            /// store the action
            const int Pos = m_acts.size ();
            m_i2a.push_back (Pos);

            const int OldActsSize = m_acts.size ();
            m_acts.resize (OldActsSize + LineLen + 1);
            memcpy (m_acts.begin () + OldActsSize, pLine, LineLen);
            m_acts [OldActsSize + LineLen] = 0;

            /// process tag/function names
            tokenizer.SetString (pLine, LineLen);

            const char * pTmpStr;
            int TmpStrLen;

            /// iterate thru the tokens of the action
            while (tokenizer.GetNextStr (&pTmpStr, &TmpStrLen)) {

                // skip the "_call" keyword
                if (0 == strncmp ("_call", pTmpStr, 5)) {
                    continue;
                }
                // see if name is known already
                int Tag = m_OutTagSet.Str2Tag (pTmpStr, TmpStrLen);
                // add tag
                if (-1 == Tag) {
                    Tag = m_ExtTagBase + m_ExtTagCount++;
                    m_OutTagSet.Add (pTmpStr, TmpStrLen, Tag);
                }

            } // of while (res)  ...

        } // of if (0 < LineLen) ...

    } // of while (!(m_pRightIs->eof ())) ...
}


void FAParser2WRE::Prepare ()
{
    m_out_i2a.Clear ();
    m_OutTagSet.Clear ();
    m_ExtTagBase = 1;
    m_ExtTagCount = 0;

    if (m_pInTagSet) {

        const int Count = m_pInTagSet->GetStrCount ();

        for (int i = 0; i < Count; ++i) {

            // get next TagStr -> Tag pair
            const char * pTagStr;
            const int TagStrLen = m_pInTagSet->GetStr (i, &pTagStr);
            DebugLogAssert (0 < TagStrLen && pTagStr);

            const int Tag = m_pInTagSet->GetValue (i);
            DebugLogAssert (0 <= Tag);

            if (Tag + 1 > m_ExtTagBase) {
                m_ExtTagBase = Tag + 1;
            }

            // copy it into output tagset
            m_OutTagSet.Add (pTagStr, TagStrLen, Tag);
        }
    }
}


void FAParser2WRE::Process ()
{
    if (NULL == m_pLeftIs || NULL == m_pRightIs) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    Prepare ();

    ProcessRight ();
    ProcessLeft ();
    WriteOutput ();

    Clear ();
}

}
