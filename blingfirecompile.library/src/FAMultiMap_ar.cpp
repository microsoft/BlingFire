/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMap_ar.h"
#include "FAAllocatorA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMultiMap_ar::FAMultiMap_ar () :
    m_MaxCount (0),
    m_pAlloc (NULL)
{}


FAMultiMap_ar::~FAMultiMap_ar ()
{
    FAMultiMap_ar::Clear ();
}


void FAMultiMap_ar::Clear ()
{
    m_MaxCount = 0;

    const int Count = m_key2vals.size ();

    for (int Key = 0; Key < Count; ++Key) {

        m_key2vals [Key].Clear ();
    }

    m_key2vals.Clear ();
}


void FAMultiMap_ar::SetAllocator (FAAllocatorA * pAlloc)
{
    m_pAlloc = pAlloc;

    m_key2vals.SetAllocator (pAlloc);
    m_key2vals.Create ();
}


const int FAMultiMap_ar::
    Get (
            const int Key, 
            __out_ecount_opt(MaxCount) int * pValues, 
            const int MaxCount
        ) const
{
    DebugLogAssert (0 <= Key);

    const int KeyCount = m_key2vals.size ();

    if (KeyCount > Key) {

        const FAArray_cont_t < int > * pValuesArr = &(m_key2vals [Key]);
        DebugLogAssert (pValuesArr);

        const int ValCount = pValuesArr->size ();

        if (NULL != pValues && ValCount <= MaxCount && 0 < ValCount) {

            const int * pBegin = pValuesArr->begin ();
            memcpy (pValues, pBegin, ValCount * sizeof (int));
        }

        return ValCount;

    } else {

        return -1;
    }
}


const int FAMultiMap_ar::GetMaxCount () const
{
    return m_MaxCount;
}


const int FAMultiMap_ar::Get (const int Key, const int ** ppValues) const
{
    DebugLogAssert (0 <= Key);
    DebugLogAssert (ppValues);

    const int KeyCount = m_key2vals.size ();

    if (KeyCount > Key) {

        const FAArray_cont_t < int > * pValues = &(m_key2vals [Key]);
        DebugLogAssert (pValues);

        *ppValues = pValues->begin ();
        const int ValCount = pValues->size ();

        return ValCount;

    } else {

        return -1;
    }
}


void FAMultiMap_ar::ensure (const int Key)
{
    DebugLogAssert (0 <= Key);

    const int KeyCount = m_key2vals.size ();

    // see whether we should allocate new X entry
    if (KeyCount <= Key) {

        m_key2vals.resize (Key + 1, 10);

        for (int key = KeyCount; key <= Key; ++key) {

            FAArray_cont_t < int > * pValues = &(m_key2vals [key]);
            DebugLogAssert (pValues);

            pValues->SetAllocator (m_pAlloc);
            pValues->Create ();
        }
    }
}


void FAMultiMap_ar::
    Set (const int Key, const int * pValues, const int ValuesCount)
{
    DebugLogAssert (0 <= Key);

    if (m_MaxCount < ValuesCount)
        m_MaxCount = ValuesCount;

    // ensure there is entry for X allocated
    ensure (Key);

    FAArray_cont_t < int > * pValuesArr = &(m_key2vals [Key]);
    DebugLogAssert (pValuesArr);

    pValuesArr->resize (ValuesCount, 0);

    if (0 < ValuesCount) {

        int * pDst = pValuesArr->begin ();
        DebugLogAssert (pDst);

        memcpy (pDst, pValues, ValuesCount * sizeof (int));
    }
}


void FAMultiMap_ar::Add (const int Key, const int Value)
{
    DebugLogAssert (0 <= Key);

    // ensure there is entry for X allocated
    ensure (Key);

    FAArray_cont_t < int > * pValues = &(m_key2vals [Key]);
    DebugLogAssert (pValues);

    pValues->push_back (Value);

    const int NewCount = pValues->size ();
    if (m_MaxCount < NewCount)
        m_MaxCount = NewCount;
}


const int FAMultiMap_ar::Next (int * pKey, const int ** ppValues) const
{
    DebugLogAssert (pKey);
    DebugLogAssert (ppValues);
    DebugLogAssert (-1 == *pKey || 0 <= *pKey);

    if (-1 == *pKey)
        (*pKey) = 0;
    else
        (*pKey)++;

    return FAMultiMap_ar::Get (*pKey, ppValues);
}


const int FAMultiMap_ar::Prev (int * pKey, const int ** ppValues) const
{
    DebugLogAssert (pKey);
    DebugLogAssert (ppValues);
    DebugLogAssert (-1 == *pKey || 0 <= *pKey);

    const int MaxKey = m_key2vals.size () - 1;

    if (-1 == *pKey)
        (*pKey) = MaxKey;
    else
        (*pKey)--;

    if (-1 == *pKey)
        return -1;

    return FAMultiMap_ar::Get (*pKey, ppValues);
}


void FAMultiMap_ar::SortUniq ()
{
    const int KeyCount = m_key2vals.size ();

    m_MaxCount = 0;

    for (int key = 0; key < KeyCount; ++key) {

        FAArray_cont_t < int > * pValues = &(m_key2vals [key]);
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

    } // of for ...
}

}
