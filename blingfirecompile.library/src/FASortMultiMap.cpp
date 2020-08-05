/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASortMultiMap.h"
#include "FAMultiMapA.h"
#include "FAFsmConst.h"

#include <algorithm>

namespace BlingFire
{


FASortMultiMap::FASortMultiMap (FAAllocatorA * pAlloc) :
    m_pMap (NULL),
    m_Direction (FAFsmConst::DIR_L2R)
{
    m_keys.SetAllocator (pAlloc);
    m_keys.Create ();
}


FASortMultiMap::_TKeyCmp_l2r::_TKeyCmp_l2r (const FAMultiMapA * pMap)
{
    m_pMap = pMap;
}


const bool FASortMultiMap::_TKeyCmp_l2r::
    operator () (const int Key1, const int Key2) const
{
    DebugLogAssert (m_pMap);

    const int * pValues1;
    const int Size1 = m_pMap->Get (Key1, &pValues1);
    DebugLogAssert (0 < Size1 && pValues1);

    const int * pValues2;
    const int Size2 = m_pMap->Get (Key2, &pValues2);
    DebugLogAssert (0 < Size2 && pValues2);

    int CommonLen = Size1;
    if (Size1 > Size2)
        CommonLen = Size2;

    for (int i = 0; i < CommonLen; ++i) {

        const int Val1 = pValues1 [i];
        const int Val2 = pValues2 [i];

        if (Val1 == Val2)
            continue;
        else if (Val1 < Val2)
            return true;
        else
            return false;
    }

    if (Size1 < Size2)
        return true;
    else
        return false;
}


FASortMultiMap::_TKeyCmp_r2l::_TKeyCmp_r2l (const FAMultiMapA * pMap)
{
    m_pMap = pMap;
}


const bool FASortMultiMap::_TKeyCmp_r2l::
    operator () (const int Key1, const int Key2) const
{
    DebugLogAssert (m_pMap);

    const int * pValues1;
    const int Size1 = m_pMap->Get (Key1, &pValues1);
    DebugLogAssert (0 < Size1 && pValues1);

    const int * pValues2;
    const int Size2 = m_pMap->Get (Key2, &pValues2);
    DebugLogAssert (0 < Size2 && pValues2);

    const int * pPtr1 = pValues1 + Size1;
    const int * pPtr2 = pValues2 + Size2;

    while (pPtr1 != pValues1 && \
           pPtr2 != pValues2) {

        pPtr1--;
        pPtr2--;

        const int Val1 = *pPtr1;
        const int Val2 = *pPtr2;

        if (Val1 == Val2)
            continue;
        else if (Val1 < Val2)
            return true;
        else
            return false;
        
    }

    if (pPtr1 == pValues1 && \
        pPtr2 != pValues2)
        return true;
    else
        return false;
}


void FASortMultiMap::SetMultiMap (const FAMultiMapA * pMap)
{
    m_pMap = pMap;
}


void FASortMultiMap::SetDirection (const int Direction)
{
    m_Direction = Direction;
}


const int FASortMultiMap::GetKeyOrder (const int ** ppKeys) const
{
    DebugLogAssert (ppKeys);

    *ppKeys = m_keys.begin ();
    const int KeyCount = m_keys.size ();
    return KeyCount;
}


void FASortMultiMap::Process ()
{
    DebugLogAssert (m_pMap);

    const int * pValues;
    int Key = -1;
    int Size = m_pMap->Prev (&Key, &pValues);

    while (-1 != Size) {

        m_keys.push_back (Key);
        Size = m_pMap->Prev (&Key, &pValues);
    }

    int * pBegin = m_keys.begin ();
    int * pEnd = m_keys.end ();

    if (FAFsmConst::DIR_L2R == m_Direction) {

        std::sort (pBegin, pEnd, _TKeyCmp_l2r (m_pMap));

    } else {
        DebugLogAssert (FAFsmConst::DIR_R2L == m_Direction);
        std::sort (pBegin, pEnd, _TKeyCmp_r2l (m_pMap));
    }
}

}

