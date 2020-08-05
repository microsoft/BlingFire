/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMap_ar_uniq.h"
#include "FAAllocatorA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMultiMap_ar_uniq::FAMultiMap_ar_uniq () :
    m_MaxCount (0),
    m_pAlloc (NULL)
{}


FAMultiMap_ar_uniq::~FAMultiMap_ar_uniq ()
{
    FAMultiMap_ar_uniq::Clear ();
}


void FAMultiMap_ar_uniq::SetAllocator (FAAllocatorA * pAlloc)
{
    m_pAlloc = pAlloc;

    m_key2idx.SetAllocator (pAlloc);
    m_key2idx.Create ();

    m_idx2arr.SetAllocator (pAlloc);
    m_idx2arr.Create ();
}


void FAMultiMap_ar_uniq::Clear ()
{
    m_MaxCount = 0;

    m_key2idx.resize (0);
    m_hash2idx.Clear ();

    const int Count = m_idx2arr.size ();
    for (int i = 0; i < Count; ++i) {

        m_idx2arr [i].Clear ();
    }
    m_idx2arr.resize (0);
}


void FAMultiMap_ar_uniq::Add (const int /*Key*/, const int /*Value*/)
{
    // not implemented
    DebugLogAssert (0);
}


const int FAMultiMap_ar_uniq::
    Get (
            const int Key,
            __out_ecount_opt(MaxCount) int * pValues,
            const int MaxCount
        ) const
{
    DebugLogAssert (0 <= Key);

    const int Size = m_key2idx.size ();

    if (Key < Size) {

        const int Idx = m_key2idx [Key];

        if (0 <= Idx) {

            DebugLogAssert ((unsigned int) Idx < m_idx2arr.size ());

            const FAArray_cont_t < int > * pArr = &(m_idx2arr [Idx]);
            DebugLogAssert (pArr);

            const int ArrSize = pArr->size ();

            if (NULL != pValues && ArrSize <= MaxCount && 0 < ArrSize) {

                const int * pBegin = pArr->begin ();
                memcpy (pValues, pBegin, ArrSize * sizeof (int));
            }

            return ArrSize;
        }
    }

    return -1;
}


const int FAMultiMap_ar_uniq::GetMaxCount () const
{
    return m_MaxCount;
}


const int FAMultiMap_ar_uniq::Get (const int Key, const int ** ppValues) const
{
    DebugLogAssert (0 <= Key);
    DebugLogAssert (ppValues);

    const int Size = m_key2idx.size ();

    if (Key < Size) {

        const int Idx = m_key2idx [Key];

        if (0 <= Idx) {

            DebugLogAssert ((unsigned int) Idx < m_idx2arr.size ());

            const FAArray_cont_t < int > * pArr = &(m_idx2arr [Idx]);
            DebugLogAssert (pArr);

            const int * pArrData = pArr->begin ();
            const int ArrSize = pArr->size ();

            *ppValues = pArrData;
            return ArrSize;
        }
    }

    return -1;
}


const int FAMultiMap_ar_uniq::hash_key (const int * pValues, int Size)
{
    DebugLogAssert (0 <= Size);

    unsigned long Key = 0;

    while (Size--) {
        DebugLogAssert (pValues);
        Key = Key * 33 + *pValues++;
    }

    return Key;
}


void FAMultiMap_ar_uniq::ensure (const int Key)
{
    const int KeyCount = m_key2idx.size ();

    if (Key >= KeyCount) {

        m_key2idx.resize (Key + 1);

        for (int key = KeyCount; key <= Key; ++key) {
            m_key2idx [key] = -1;
        }
    }
}


const bool FAMultiMap_ar_uniq::
    equal (const int Idx, const int * pValues, const int Size) const
{
    DebugLogAssert (0 <= Idx && m_idx2arr.size () > (unsigned int) Idx);

    const FAArray_cont_t < int > * pArr = &(m_idx2arr [Idx]);
    DebugLogAssert (pArr);

    const int ArrSize = pArr->size ();
    if (Size != ArrSize)
        return false;

    const int * pArrData = pArr->begin ();

    for (int i = 0; i < Size; ++i) {

        DebugLogAssert (pValues && pArrData);

        if (pValues [i] != pArrData [i])
            return false;
    }

    return true;
}


void FAMultiMap_ar_uniq::
    Set (const int Key, const int * pValues, const int ValuesCount)
{
    DebugLogAssert (0 <= Key);

    if (m_MaxCount < ValuesCount)
        m_MaxCount = ValuesCount;

    ensure (Key);
    const int Idx = m_key2idx [Key];

    // see whether such Key does not exists
    if (-1 == Idx) {

        // try to find somthing existing
        const int HashKey = hash_key (pValues, ValuesCount);
        const int * pIdx2 = m_hash2idx.Get (HashKey);

        if (pIdx2 && equal (*pIdx2, pValues, ValuesCount)) {

            m_key2idx [Key] = *pIdx2;

        } else {

            const int NewIdx = m_idx2arr.size ();
            m_idx2arr.resize (NewIdx + 1);

            m_key2idx [Key] = NewIdx;
            if (!pIdx2) {
                m_hash2idx.Set (HashKey, NewIdx);
            }

            FAArray_cont_t < int > * pNewArr = &(m_idx2arr [NewIdx]);
            DebugLogAssert (pNewArr);

            pNewArr->SetAllocator (m_pAlloc);
            pNewArr->Create ();
            pNewArr->resize (ValuesCount, 0);

            if (0 < ValuesCount) {

                int * pDst = pNewArr->begin ();
                DebugLogAssert (pDst);
                memcpy (pDst, pValues, ValuesCount * sizeof (int));
            }
        }

    } else {

        // see whether it differs
        if (false == equal (Idx, pValues, ValuesCount)) {
            // not implemented
            DebugLogAssert (0);
        }
    }
}


const int FAMultiMap_ar_uniq::Next (int * pKey, const int ** ppValues) const
{
    DebugLogAssert (pKey);
    DebugLogAssert (ppValues);
    DebugLogAssert (-1 == *pKey || 0 <= *pKey);

    const int KeyCount = m_key2idx.size ();

    // return MinKey if -1, otherwise next one
    if (-1 == *pKey)
        (*pKey) = 0;
    else
        (*pKey)++;

    // skip non-initialized keys
    int key = *pKey;
    for (; key < KeyCount; ++key) {
        if (-1 != m_key2idx [key])
            break;
    }
    *pKey = key;

    // see whether no more keys exist
    if (KeyCount == *pKey)
        return -1;

    // return the values
    return FAMultiMap_ar_uniq::Get (*pKey, ppValues);
}


const int FAMultiMap_ar_uniq::Prev (int * pKey, const int ** ppValues) const
{
    DebugLogAssert (pKey);
    DebugLogAssert (ppValues);
    DebugLogAssert (-1 == *pKey || 0 <= *pKey);

    const int MaxKey = m_key2idx.size () - 1;

    // return MaxKey if -1, otherwise previous one
    if (-1 == *pKey)
        (*pKey) = MaxKey;
    else
        (*pKey)--;

    // skip non-initialized keys
    int key = *pKey;
    for (; key >= 0; --key) {
        if (-1 != m_key2idx [key])
            break;
    }
    *pKey = key;

    // see whether no more keys exist
    if (-1 == *pKey)
        return -1;

    // return the values
    return FAMultiMap_ar_uniq::Get (*pKey, ppValues);
}


void FAMultiMap_ar_uniq::SortUniq ()
{
    m_MaxCount = 0;

    const int Count = m_idx2arr.size ();

    for (int i = 0; i < Count; ++i) {

        FAArray_cont_t < int > * pValues = &(m_idx2arr [i]);
        DebugLogAssert (pValues);

        int * pBegin = pValues->begin ();
        int * pEnd = pValues->end ();

        if (false == FAIsSortUniqed (pBegin, int (pEnd - pBegin))) {

            const int NewSize = FASortUniq (pBegin, pEnd);
            pValues->resize (NewSize);
        }

        const int NewSize = pValues->size ();
        if (m_MaxCount < NewSize)
            m_MaxCount = NewSize;
    }
}

}
