/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMap_judy.h"

#ifdef HAVE_JUDY_LIB

namespace BlingFire
{


FAMap_judy::FAMap_judy () :
  m_map ((Pvoid_t) NULL)
{
}

FAMap_judy::~FAMap_judy ()
{
  FAMap_judy::Clear ();
}

void FAMap_judy::Clear ()
{
  // free map, if needed
  if (NULL != m_map) {

    JudyLFreeArray (&m_map, NULL);
    DebugLogAssert (NULL == m_map);
  }
}

const int* FAMap_judy::Get (const int Key) const
{
  const int * pValue;

  // get the value by the key
  pValue = (const int *) JudyLGet (m_map, (Word_t) Key, NULL);
  /// JLG ((void*)pValue, m_map, (Word_t) Key);
  return pValue;
}

void FAMap_judy::Set (const int Key, const int Value)
{
  Word_t * pValue;

  // add Key into the map and get the pointer to the data, JLI - call
  pValue = (Word_t *) JudyLIns (&m_map, (Word_t) Key, NULL);
  DebugLogAssert (NULL != pValue);

  // initialize value
  *pValue = Value;
}

const int * FAMap_judy::Next (int * pKey) const
{
  DebugLogAssert (pKey);

  const int * pValue;

  // make special processing for the first key
  if (0 == *pKey) {

    pValue = (const int *) JudyLFirst (m_map, (Word_t *) pKey, NULL);

  } else {

    pValue = (const int *) JudyLNext (m_map, (Word_t *) pKey, NULL);
  }

  return pValue;
}

const int * FAMap_judy::Prev (int * pKey) const
{
  DebugLogAssert (pKey);

  const int * pValue;

  // make special processing for the last key
  if (-1 == *pKey) {

    pValue = (const int *) JudyLLast (m_map, (Word_t *) pKey, NULL);

  } else {

    pValue = (const int *) JudyLPrev (m_map, (Word_t *) pKey, NULL);
  }

  return pValue;
}

void FAMap_judy::Remove (const int Key)
{
  JudyLDel (&m_map, (Word_t) Key, NULL);
}

}

// of ifdef HAVE_JUDY_LIB
#endif
