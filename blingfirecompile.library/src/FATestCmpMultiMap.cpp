/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestCmpMultiMap.h"
#include "FAMultiMapCA.h"

namespace BlingFire
{


FATestCmpMultiMap::FATestCmpMultiMap (FAAllocatorA * pAlloc) :
    m_pMMap1 (NULL),
    m_pMMap2 (NULL),
    m_pChainBuff1 (NULL),
    m_MaxChainSize1 (0),
    m_pChainBuff2 (NULL),
    m_MaxChainSize2 (0)
{
    m_chain1.SetAllocator (pAlloc);
    m_chain1.Create ();

    m_chain2.SetAllocator (pAlloc);
    m_chain2.Create ();
}


void FATestCmpMultiMap::SetMap1 (const FAMultiMapCA * pMMap1)
{
    m_pMMap1 = pMMap1;
}


void FATestCmpMultiMap::SetMap2 (const FAMultiMapCA * pMMap2)
{
    m_pMMap2 = pMMap2;
}


inline const int FATestCmpMultiMap::
    GetChain1 (const int Key, const int ** ppChain)
{
    DebugLogAssert (m_pMMap1 && ppChain);

    // check whether values were explicitly stored
    int Count = m_pMMap1->Get (Key, ppChain);

    // check whether values were implicitly stored
    if (-1 == Count) {

        Count = m_pMMap1->Get (Key, m_pChainBuff1, m_MaxChainSize1);
        *ppChain = m_pChainBuff1;
    }

    return Count;
}


inline const int FATestCmpMultiMap::
    GetChain2 (const int Key, const int ** ppChain)
{
    DebugLogAssert (m_pMMap2 && ppChain);

    // check whether values were explicitly stored
    int Count = m_pMMap2->Get (Key, ppChain);

    // check whether values were implicitly stored
    if (-1 == Count) {

        Count = m_pMMap2->Get (Key, m_pChainBuff2, m_MaxChainSize2);
        *ppChain = m_pChainBuff2;
    }

    return Count;
}


inline const bool FATestCmpMultiMap::
    Equal (
            const int * pChain1, 
            const int Size1,
            const int * pChain2, 
            const int Size2
        ) const
{
    // TODO: fix FAMultiMap_ar, so it will be able to keep gaps
    if ((-1 == Size1 || 0 == Size1) && \
        (-1 == Size2 || 0 == Size2)) {
        return true;
    }

    if (Size1 != Size2) {
        return false;
    }

    for (int i = 0; i < Size1; ++i) {

        const int Val1 = pChain1 [i];
        const int Val2 = pChain2 [i];

        if (Val1 != Val2)
            return false;
    }

    return true;
}


const bool FATestCmpMultiMap::Process (const int MaxKey)
{
    DebugLogAssert (m_pMMap1 && m_pMMap2);

    // setup chain buffers

    m_MaxChainSize1 = m_pMMap1->GetMaxCount ();
    if (0 >= m_MaxChainSize1) {
        return false;
    } else {
        m_chain1.resize (m_MaxChainSize1);
        m_pChainBuff1 = m_chain1.begin ();
    }

    m_MaxChainSize2 = m_pMMap2->GetMaxCount ();
    if (0 >= m_MaxChainSize2) {
        return false;
    } else {
        m_chain2.resize (m_MaxChainSize2);
        m_pChainBuff2 = m_chain2.begin ();
    }

    // compare the outputs

    for (int Key = 0; Key <= MaxKey; ++Key) {

        const int * pVals1;
        const int Size1 = GetChain1 (Key, &pVals1);
        if (Size1 > m_MaxChainSize1) {
            return false;
        }

        const int * pVals2;
        const int Size2 = GetChain2 (Key, &pVals2);
        if (Size2 > m_MaxChainSize2) {
            return false;
        }

        if (!Equal (pVals1, Size1, pVals2, Size2)) {
            return false;
        }
    }

    return true;
}

}

