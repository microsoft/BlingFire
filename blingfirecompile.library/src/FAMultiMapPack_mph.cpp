/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMapPack_mph.h"
#include "FAFsmConst.h"
#include "FAMultiMapA.h"

namespace BlingFire
{


FAMultiMapPack_mph::FAMultiMapPack_mph (FAAllocatorA * pAlloc) :
    m_pMMap (NULL),
    m_Direction (FAFsmConst::DIR_L2R),
    m_mmap_sort (pAlloc),
    m_arr2dfa (pAlloc),
    m_dfa2mph (pAlloc),
    m_state2ows (pAlloc),
    m_fsm2dump (pAlloc)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();

    m_chain_buff.SetAllocator (pAlloc);
    m_chain_buff.Create ();
}


void FAMultiMapPack_mph::SetMultiMap (const FAMultiMapA * pMMap)
{
    m_pMMap = pMMap;

    if (NULL != pMMap) {
        m_MaxChainSize = pMMap->GetMaxCount ();
        m_chain_buff.resize (m_MaxChainSize);
        m_pChainBuff = m_chain_buff.begin ();
    }
}


void FAMultiMapPack_mph::SetDirection (const int Direction)
{
    DebugLogAssert (FAFsmConst::DIR_L2R == Direction || \
            FAFsmConst::DIR_R2L == Direction);

    m_Direction = Direction;
    m_mmap_sort.SetDirection (Direction);
}


const FAMapA * FAMultiMapPack_mph::GetOld2New () const
{
    return & m_old2new;
}


const int FAMultiMapPack_mph::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);

    *ppDump = m_dump.begin ();
    const int DumpSize = m_dump.size ();

    return DumpSize;
}


inline void FAMultiMapPack_mph::Clear ()
{
    m_arr2dfa.Clear ();
    m_state2ows.Clear ();
    m_old2new.Clear ();
    m_dump.resize (0);
}


inline const int FAMultiMapPack_mph::
    GetChain (const int Key, const int ** ppChain)
{
    DebugLogAssert (ppChain);

    if (FAFsmConst::DIR_L2R == m_Direction) {

        return m_pMMap->Get (Key, ppChain);

    } else {

        const int Count = m_pMMap->Get (Key, m_pChainBuff, m_MaxChainSize);
        DebugLogAssert (0 < Count && Count <= m_MaxChainSize);

        // reverse the chain
        const int Count_2 = Count >> 1;

        for (int i = 0; i < Count_2; ++i) {

            const int Tmp = m_pChainBuff [i];
            m_pChainBuff [i] = m_pChainBuff [Count - i - 1];
            m_pChainBuff [Count - i - 1] = Tmp;
        }

        *ppChain = m_pChainBuff;
        return Count;

    } // of if (FAFsmConst::DIR_L2R == m_Direction) ...
}


inline void FAMultiMapPack_mph::ProcessChains ()
{
    DebugLogAssert (m_pMMap);
    DebugLogAssert (FAFsmConst::DIR_L2R == m_Direction || \
            FAFsmConst::DIR_R2L == m_Direction);

    const int * pSortedKeys;
    const int KeyCount = m_mmap_sort.GetKeyOrder (&pSortedKeys);
    DebugLogAssert (0 < KeyCount && pSortedKeys);

    for (int i = 0; i < KeyCount; ++i) {

        const int Key = pSortedKeys [i];

        m_old2new.Set (Key, i);

        const int * pValues;
        const int ValCount = GetChain (Key, &pValues);
        DebugLogAssert (0 < ValCount && pValues);

        m_arr2dfa.AddChain (pValues, ValCount);
    }
}


void FAMultiMapPack_mph::Process ()
{
    DebugLogAssert (m_pMMap);
    DebugLogAssert (FAFsmConst::DIR_L2R == m_Direction || \
            FAFsmConst::DIR_R2L == m_Direction);

    // clear from previous run
    Clear ();

    // sort
    m_mmap_sort.SetMultiMap (m_pMMap);
    m_mmap_sort.Process ();

    // traverse arrays of values in lexicographic order and build min DFA
    ProcessChains ();
    m_arr2dfa.Prepare ();

    // build MPH from min DFA
    m_dfa2mph.SetRsDfa (&m_arr2dfa);
    m_dfa2mph.SetState2Ows (&m_state2ows);
    m_dfa2mph.Process ();

    // build Moore-Multi DFA dump
    m_fsm2dump.SetDfa (&m_arr2dfa);
    m_fsm2dump.SetState2Ows (&m_state2ows);
    m_fsm2dump.Process ();

    // get FSM's dump
    const unsigned char * pFsmDump;
    const int FsmDumpSize = m_fsm2dump.GetDump (&pFsmDump);
    DebugLogAssert (0 < FsmDumpSize && pFsmDump);

    m_dump.resize (FsmDumpSize + (2 * sizeof (int)));
    unsigned char * pCurrPtr = m_dump.begin ();
    DebugLogAssert (pCurrPtr);

    // copy m_MaxChainSize, m_Direction and DFA dump
    DebugLogAssert (0 < m_MaxChainSize);
    *(int *)(pCurrPtr) = m_MaxChainSize;
    pCurrPtr += sizeof (int);
    *(int *)(pCurrPtr) = m_Direction;
    pCurrPtr += sizeof (int);

    memcpy (pCurrPtr, pFsmDump, FsmDumpSize);
}

}
