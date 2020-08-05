/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAFsmRenum.h"
#include "FAAllocatorA.h"
#include "FAStringTokenizer.h"
#include "FAFsmConst.h"

#include <string>
#include <algorithm>

namespace BlingFire
{


FAFsmRenum::FAFsmRenum (FAAllocatorA * pAlloc) :
    m_DeadState (FAFsmConst::NFA_DEAD_STATE),
    m_OldMaxState (-1),
    m_NewMaxState (-1),
    m_MaxIw (-1),
    m_ICount (0),
    m_FCount (0),
    m_QCount (0),
    m_fsm_type (FAFsmConst::TYPE_RS_NFA),
    m_state2ow (pAlloc),
    m_state2ows (pAlloc),
    m_in_sigma (pAlloc),
    m_out_sigma (pAlloc)
{
    m_trs_storage.SetAllocator (pAlloc);
    m_trs_storage.Create ();

    m_trs.SetAllocator (pAlloc);
    m_trs.Create ();

    m_ows.SetAllocator (pAlloc);
    m_ows.Create ();
}


void FAFsmRenum::Clear ()
{
    m_OldMaxState = -1;
    m_NewMaxState = -1;

    m_MaxIw = -1;

    m_ICount = 0;
    m_I.Clear ();

    m_FCount = 0;
    m_F.Clear ();

    m_QCount = 0;
    m_Q.Clear ();

    m_trs_storage.resize (0);
    m_trs.resize (0);

    m_state2ow.Clear ();
    m_state2ows.Clear ();
    m_in_sigma.Clear ();
    m_out_sigma.Clear ();
}


void FAFsmRenum::SetFsmType (const int fsm_type)
{
    DebugLogAssert (0 <= fsm_type && FAFsmConst::TYPE_COUNT > fsm_type);
    m_fsm_type = fsm_type;

    if (FAFsmConst::TYPE_RS_NFA == m_fsm_type || \
        FAFsmConst::TYPE_POS_RS_NFA == m_fsm_type || \
        FAFsmConst::TYPE_MEALY_NFA == m_fsm_type) {

        m_DeadState = FAFsmConst::NFA_DEAD_STATE;

    } else {

        m_DeadState = FAFsmConst::DFA_DEAD_STATE;
    }
}


void FAFsmRenum::AddTranition (const int * pTr, const int TrSize)
{
    DebugLogAssert (pTr && 2 < TrSize);

    const int TrIdx = m_trs_storage.size ();

    for (int i = 0; i < TrSize; ++i) {

        const int Val = pTr [i];
        m_trs_storage.push_back (Val);
    }

    m_trs.push_back (TrIdx);
}


void FAFsmRenum::ReadFsm (std::istream * pIs)
{
    DebugLogAssert (pIs);

    const int MaxTrSize = 4;
    int TmpTr [MaxTrSize];

    const char * pTmpStr;
    int TmpStrLen;
    int State, DstState;
    bool res;
    std::string line;

    while (!pIs->eof ()) {

        if (!std::getline (*pIs, line))
            break;

        // break reading on empty line
        if (line.empty ())
            break;

        const char * pStr = line.c_str ();
        DebugLogAssert (pStr);
        const int StrLen = (const int) line.length ();

        m_tokenizer.SetString (pStr, StrLen);
        res = m_tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        DebugLogAssert (res);

        // see whether we have an initial state
        if (0 == strncmp ("initial:", pTmpStr, TmpStrLen)) {

            res = m_tokenizer.GetNextInt (&State);
            DebugLogAssert (res);

            if (NULL == m_I.Get (State)) {
                m_I.Set (State, m_ICount);
                m_ICount++;
            }
            if (NULL == m_Q.Get (State)) {
                m_Q.Set (State, m_QCount);
                m_QCount++;
            }
            if (m_OldMaxState < State)
                m_OldMaxState = State;

            continue;
        }
        // see whether we have a final state
        if (0 == strncmp ("final:", pTmpStr, TmpStrLen)) {

            res = m_tokenizer.GetNextInt (&State);
            DebugLogAssert (res);

            if (NULL == m_F.Get (State)) {
                m_F.Set (State, m_FCount);
                m_FCount++;
            }
            if (NULL == m_Q.Get (State)) {
                m_Q.Set (State, m_QCount);
                m_QCount++;
            }
            if (m_OldMaxState < State)
                m_OldMaxState = State;

            continue;
        }
        // reinterpret input string as an array of ints
        m_tokenizer.SetString (pStr, StrLen);
        const int TrSize = m_tokenizer.GetArray2 (TmpTr, MaxTrSize);

        // see whether we have a transition
        if (3 <= TrSize) {

            State = TmpTr [0];
            if (NULL == m_Q.Get (State)) {
                m_Q.Set (State, m_QCount);
                m_QCount++;
            }
            if (m_OldMaxState < State)
                m_OldMaxState = State;

            DstState = TmpTr [1];
            if (NULL == m_Q.Get (DstState)) {
                m_Q.Set (DstState, m_QCount);
                m_QCount++;
            }
            if (m_OldMaxState < DstState)
                m_OldMaxState = DstState;

            const int Iw = TmpTr [2];
            if (m_MaxIw < Iw)
                m_MaxIw = Iw;

            // add this transition
            AddTranition (TmpTr, 3);

            if (FAFsmConst::TYPE_MEALY_NFA == m_fsm_type || \
                FAFsmConst::TYPE_MEALY_DFA == m_fsm_type) {

                DebugLogAssert (4 == TrSize);

                const int Ow = TmpTr [3];
                if (-1 != Ow) {
                    m_in_sigma.SetOw (State, Iw, DstState, Ow);
                }
            }

        } // of if (3 <= TrSize)

    } // of while (!pIs->eof ()) ...

    m_NewMaxState = m_QCount;
}


void FAFsmRenum::ReadOwMap (std::istream * pIs)
{
    DebugLogAssert (pIs);

    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int State = -1;
    int Ow = -1;

    while (!pIs->eof ()) {

        if (!std::getline (*pIs, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLine = line.c_str ();
        const int LineLen = (const int) line.length ();

        m_tokenizer.SetString (pLine, LineLen);

        bool res = m_tokenizer.GetNextInt (&State);
        res = res && m_tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && m_tokenizer.GetNextInt (&Ow);
        DebugLogAssert (res);
        DebugLogAssert (0 == strncmp ("->", pTmpStr, TmpStrLen));

        const int NewState = Old2New (State);
        m_state2ow.SetOw (NewState, Ow);
    }
}


void FAFsmRenum::ReadOwsMap (std::istream * pIs)
{
    DebugLogAssert (pIs);

    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int State = -1;
    int Size = -1;

    while (!pIs->eof ()) {

        if (!std::getline (*pIs, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        const char * pLine = line.c_str ();
        const int LineLen = (const int) line.length ();

        m_tokenizer.SetString (pLine, LineLen);

        bool res = m_tokenizer.GetNextInt (&State);
        res = res && m_tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && m_tokenizer.GetNextInt (&Size);
        DebugLogAssert (res);
        DebugLogAssert (0 == strncmp ("->", pTmpStr, TmpStrLen));
        DebugLogAssert (0 <= Size);

        m_ows.resize (Size);

        res = m_tokenizer.GetArray (m_ows.begin (), m_ows.size ());
        DebugLogAssert (res);

        const int NewState = Old2New (State);
        m_state2ows.SetOws (NewState, m_ows.begin (), Size);
    }
}


const int FAFsmRenum::Old2New (const int State) const
{
    if (State == m_DeadState)
        return m_DeadState;

    const int * pFNum = m_F.Get (State);
    if (pFNum) {
        const int NewState = m_ICount + m_QCount + *pFNum;
        return NewState;
    }

    const int * pINum = m_I.Get (State);
    if (pINum) {
        const int NewState = *pINum;
        return NewState;
    }

    const int * pQNum = m_Q.Get (State);
    DebugLogAssert (pQNum);

    const int NewState = m_ICount + *pQNum;
    return NewState;
}


void FAFsmRenum::Remap ()
{
    int State, NewState;
    const int * pNum;

    // F and I from Q

    State = -1;
    pNum = m_F.Prev (&State);

    while (NULL != pNum) {
        if (NULL != m_Q.Get (State)) {
            m_Q.Remove (State);
            m_QCount--;
        }
        pNum = m_F.Prev (&State);
    }

    State = -1;
    pNum = m_I.Prev (&State);

    while (NULL != pNum) {
        if (NULL != m_Q.Get (State)) {
            m_Q.Remove (State);
            m_QCount--;
        }
        pNum = m_I.Prev (&State);
    }

    // renum Q
    int Count = 0;
    for (State = 0; State <= m_OldMaxState; ++State) {

        if (NULL != m_Q.Get (State)) {
            m_Q.Set (State, Count);
            Count++;
        }
    }

    // renum transitions
    const int TrCount = m_trs.size ();
    for (int i = 0; i < TrCount; ++i) {

        const int TrIdx = m_trs [i];

        State = m_trs_storage [TrIdx];
        NewState = Old2New (State);
        m_trs_storage [TrIdx] = NewState;

        const int DstState = m_trs_storage [TrIdx + 1];
        const int NewDstState = Old2New (DstState);
        m_trs_storage [TrIdx + 1] = NewDstState;

        if (FAFsmConst::TYPE_MEALY_NFA == m_fsm_type || \
            FAFsmConst::TYPE_MEALY_DFA == m_fsm_type) {

            const int Iw = m_trs_storage [TrIdx + 2];
            const int Ow = m_in_sigma.GetOw (State, Iw, DstState);

            m_out_sigma.SetOw (NewState, Iw, NewDstState, Ow);
        }
    }
}


FAFsmRenum::TrCmp::TrCmp (const FAArray_t < int > * pTrs) :
    m_pTrs (pTrs)
{}


const bool FAFsmRenum::TrCmp::operator() (const int TrIdx1, const int TrIdx2) const
{
    DebugLogAssert (m_pTrs);

    const int SrcState1 = (*m_pTrs) [TrIdx1];
    const int SrcState2 = (*m_pTrs) [TrIdx2];

    if (SrcState1 < SrcState2)
        return true;
    
    const int Iw1 = (*m_pTrs) [TrIdx1 + 2];
    const int Iw2 = (*m_pTrs) [TrIdx2 + 2];

    if (SrcState1 == SrcState2 && Iw1 < Iw2)
        return true;

    const int DstState1 = (*m_pTrs) [TrIdx1 + 1];
    const int DstState2 = (*m_pTrs) [TrIdx2 + 1];

    if (SrcState1 == SrcState2 && Iw1 == Iw2 &&  DstState1 < DstState2)
        return true;

    return false;
}


void FAFsmRenum::Sort ()
{
    std::sort (m_trs.begin (), m_trs.end (), TrCmp (&m_trs_storage));
}


void FAFsmRenum::PrintFsm (std::ostream * pOs) const
{
    DebugLogAssert (pOs);

    const int * pNum;
    int State;

    // print header
    (*pOs) << "MaxState: " << m_NewMaxState << '\n';
    (*pOs) << "MaxIw: " << m_MaxIw << '\n';

    // print initial states
    State = -1;
    pNum = m_I.Prev (&State);
    while (NULL != pNum) {

        const int NewState = Old2New (State);
        (*pOs) << "initial: " << NewState << '\n';
        pNum = m_I.Prev (&State);
    }

    // print final states
    State = -1;
    pNum = m_F.Prev (&State);
    while (NULL != pNum) {

        const int NewState = Old2New (State);
        (*pOs) << "final: " << NewState << '\n';
        pNum = m_F.Prev (&State);
    }

    // print transitions
    const int TrCount = m_trs.size ();
    for (int i = 0; i < TrCount; ++i) {

        const int TrIdx = m_trs [i];

        const int From = m_trs_storage [TrIdx];
        const int To = m_trs_storage [TrIdx + 1];
        const int Iw = m_trs_storage [TrIdx + 2];

        (*pOs) << From << ' ' << To << ' ' << Iw;

        if (FAFsmConst::TYPE_MEALY_NFA == m_fsm_type || \
            FAFsmConst::TYPE_MEALY_DFA == m_fsm_type) {

            const int Ow = m_out_sigma.GetOw (From, Iw, To);
            (*pOs) << ' ' << Ow;
        }

        (*pOs) << '\n';
    }
    (*pOs) << '\n';
}


void FAFsmRenum::PrintOwMap (std::ostream * pOs) const
{
    DebugLogAssert (pOs);

    for (int State = 0; State <= m_NewMaxState; ++State) {

        const int Ow = m_state2ow.GetOw (State);

        if (-1 != Ow) {
            (*pOs) << State << " -> " << Ow << '\n';
        }
    }
    (*pOs) << '\n';
}


void FAFsmRenum::PrintOwsMap (std::ostream * pOs) const
{
    DebugLogAssert (pOs);

    for (int State = 0; State <= m_NewMaxState; ++State) {

        const int * pOws;
        const int OwsCount = m_state2ows.GetOws (State, &pOws);

        if (0 < OwsCount) {

            DebugLogAssert (pOws);

            (*pOs) << State << " -> " << OwsCount ;

            for (int i = 0; i < OwsCount; ++i) {

                const int Ow = pOws [i];
                (*pOs) << ' ' << Ow;
            }
            (*pOs) << '\n';
        }
    }
    (*pOs) << '\n';
}


void FAFsmRenum::Process (std::ostream * pOs, std::istream * pIs)
{
    DebugLogAssert (pOs);
    DebugLogAssert (pIs);

    ReadFsm (pIs);

    if (-1 != m_OldMaxState) {

        Remap ();
        Sort ();

        if (FAFsmConst::TYPE_MOORE_DFA == m_fsm_type) {

            ReadOwMap (pIs);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == m_fsm_type) {

            ReadOwsMap (pIs);
        }

        if (-1 == m_MaxIw) {
            m_MaxIw = 0;
        }

        PrintFsm (pOs);

        if (FAFsmConst::TYPE_MOORE_DFA == m_fsm_type) {

            PrintOwMap (pOs);

        } else if (FAFsmConst::TYPE_MOORE_MULTI_DFA == m_fsm_type) {

            PrintOwsMap (pOs);
        }

        Clear ();
    }
}

}
