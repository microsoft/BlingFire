/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMultiMap_judy.h"
#include "FAAllocatorA.h"
#include "FAArray_cont_t.h"
#include "FAUtils.h"

namespace BlingFire
{


FAMultiMap_judy::FAMultiMap_judy () :
  m_pAlloc (NULL),
  m_MaxCount (0)
{
}

FAMultiMap_judy::~FAMultiMap_judy ()
{
  FAMultiMap_judy::Clear ();
}

void FAMultiMap_judy::SetAllocator (FAAllocatorA * pAlloc)
{
  m_pAlloc = pAlloc;

  if (NULL != pAlloc) {
  
    m_idx2vals.SetAllocator (pAlloc);
    m_idx2vals.Create ();

    m_deleted.Create (pAlloc);
  }
}

const int FAMultiMap_judy::
    Get (const int Key, const int ** ppValues) const
{
  DebugLogAssert (ppValues);

  /// see whether something is associated with the Key
  const int * pIdx = m_key2idx.Get (Key);

  if (NULL == pIdx) {
    /// no, nothing
    return -1;
  }

  const int Idx = *pIdx;

  /// get pointer of the idx-th element of the m_idx2vals
  const FAArray_cont_t <int> * pArrVals = & (m_idx2vals [Idx]);
  DebugLogAssert (pArrVals);

  *ppValues = pArrVals->begin ();
  const int ValsCount = pArrVals->size ();

  return ValsCount;
}


const int FAMultiMap_judy::
    Get (
            const int Key,
            __out_ecount_opt(MaxCount) int * pValues,
            const int MaxCount
        ) const
{
  /// see whether something is associated with the Key
  const int * pIdx = m_key2idx.Get (Key);

  if (NULL == pIdx) {
    /// no, nothing
    return -1;
  }

  const int Idx = *pIdx;

  /// get pointer of the idx-th element of the m_idx2vals
  const FAArray_cont_t <int> * pArrVals = & (m_idx2vals [Idx]);
  DebugLogAssert (pArrVals);

  /// get the number of the values
  const int ValuesCount = pArrVals->size ();

  if (NULL != pValues && ValuesCount <= MaxCount && 0 < ValuesCount) {

    const int * pBegin = pArrVals->begin ();
    memcpy (pValues, pBegin, ValuesCount * sizeof (int));
  }

  return ValuesCount;
}


const int FAMultiMap_judy::GetMaxCount () const
{
    return m_MaxCount;
}


inline const int  FAMultiMap_judy::GetNewIdx ()
{
   if (m_deleted.empty ()) {

        const int NewIdx = m_idx2vals.size ();

        // allocate one more array
        m_idx2vals.resize (NewIdx + 1);

        // initialize array object
        FAArray_cont_t < int > * pNewArr = & (m_idx2vals [NewIdx]);
        DebugLogAssert (pNewArr);

        pNewArr->SetAllocator (m_pAlloc);
        pNewArr->Create ();

        return NewIdx;

    } else {

        const int * pIdx = m_deleted.top ();
        DebugLogAssert (pIdx);

        const int Idx = *pIdx;
        m_deleted.pop ();

        DebugLogAssert (0 == m_idx2vals [Idx].size ());

        return Idx;
    }
}


void FAMultiMap_judy::
    Set (const int Key, const int * pValues, const int ValuesCount)
{
    DebugLogAssert (0 <= ValuesCount);
    DebugLogAssert (m_pAlloc);

    if (m_MaxCount < ValuesCount)
        m_MaxCount = ValuesCount;

    /// see whether something has already been associated with the Key
    const int * pIdx = m_key2idx.Get (Key);

    int Idx;

    if (NULL == pIdx) {

      Idx = GetNewIdx ();

    } else {

      Idx = *pIdx;
    }

    FAArray_cont_t <int> * pArray = & (m_idx2vals [Idx]);
    DebugLogAssert (pArray);

    pArray->resize (ValuesCount, 0);

    if (0 < ValuesCount) {

        int * pValuesCopy = pArray->begin ();
        memcpy (pValuesCopy, pValues, sizeof (int) * ValuesCount);
    }

    /// store Key -> Idx pair, if needed
    if (NULL == pIdx) {

        m_key2idx.Set (Key, Idx);
    }
}


void FAMultiMap_judy::Add (const int Key, const int Value)
{
    /// see whether something is associated with the Key
    const int * pIdx = m_key2idx.Get (Key);

    if (NULL == pIdx) {

      FAMultiMap_judy::Set (Key, &Value, 1);

    } else {

      const int Idx = *pIdx;
      FAArray_cont_t <int> * pArray = & (m_idx2vals [Idx]);
      DebugLogAssert (pArray);

      const int Count = pArray->size ();

      /// add value into the array
      pArray->push_back (Value, Count);

      const int NewCount = Count + 1;

      if (m_MaxCount < NewCount)
          m_MaxCount = NewCount;
    }
}


const int FAMultiMap_judy::Next (int * pKey, const int ** ppValues) const
{
  DebugLogAssert (ppValues);

  const int * pIdx = m_key2idx.Next (pKey);

  if (NULL == pIdx) {
    return -1;
  }

  const int Idx = *pIdx;
  const FAArray_cont_t <int> * pArray =  & (m_idx2vals [Idx]);
  DebugLogAssert (pArray);

  *ppValues = pArray->begin ();
  const int ValuesCount = pArray->size ();

  return ValuesCount;
}


const int FAMultiMap_judy::Prev (int * pKey, const int ** ppValues) const
{
  DebugLogAssert (ppValues);

  const int * pIdx = m_key2idx.Prev (pKey);

  if (NULL == pIdx) {
    return -1;
  }

  const int Idx = *pIdx;
  const FAArray_cont_t <int> * pArray =  & (m_idx2vals [Idx]);
  DebugLogAssert (pArray);

  *ppValues = pArray->begin ();
  const int ValuesCount = pArray->size ();

  return ValuesCount;
}


void FAMultiMap_judy::Remove (const int Key)
{
  /// see whether something is associated with the Key
  const int * pIdx = m_key2idx.Get (Key);

  if (NULL != pIdx) {

    const int Idx = *pIdx;
    FAArray_cont_t <int> * pArray =  & (m_idx2vals [Idx]);
    DebugLogAssert (pArray);

    pArray->resize (0);
    m_deleted.push (Idx);

    /// remove the Key
    m_key2idx.Remove (Key);
  }
}


void FAMultiMap_judy::Clear ()
{
  /// TODO: remove this, make all objects created in constructor
  if (m_pAlloc) {

    m_MaxCount = 0;

    m_key2idx.Clear ();

    const int ArrCount = m_idx2vals.size ();

    for (int i = 0; i < ArrCount; ++i) {

      FAArray_cont_t <int> * pArray =  & (m_idx2vals [i]);
      DebugLogAssert (pArray);
      pArray->Clear ();
    }

    m_idx2vals.resize (0);

    m_deleted.clear ();
  }
}


void FAMultiMap_judy::SortUniq ()
{
    int NewSize;

    m_MaxCount = 0;

    const int ArrCount = m_idx2vals.size ();

    for (int i = 0; i < ArrCount; ++i) {

      FAArray_cont_t <int> * pArray =  & (m_idx2vals [i]);
      DebugLogAssert (pArray);

      int * pBegin = pArray->begin ();
      int * pEnd = pArray->end ();

      if (false == FAIsSortUniqed (pBegin, int (pEnd - pBegin))) {

        NewSize = FASortUniq (pBegin, pEnd);
        pArray->resize (NewSize);

      } else {

        NewSize = pArray->size ();
      }

      if (m_MaxCount < NewSize)
          m_MaxCount = NewSize;
    }
}

}
