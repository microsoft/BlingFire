/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAChain2Num_judy.h"
#include "FAAllocatorA.h"
#include "FAEncoderA.h"

#ifdef HAVE_JUDYSL_LIB

#include <Judy.h>

namespace BlingFire
{


FAChain2Num_judy::FAChain2Num_judy () :
  m_map ((Pvoid_t) NULL)
{
  m_pEncoder = &m_def_enc;
}

FAChain2Num_judy::~FAChain2Num_judy ()
{
  FAChain2Num_judy::Clear ();
}


void FAChain2Num_judy::SetAllocator (FAAllocatorA * /*pAlloc*/)
{
}

void FAChain2Num_judy::SetEncoder (const FAEncoderA * pEncoder)
{
  m_pEncoder = pEncoder;

  if (NULL == m_pEncoder) {

    m_pEncoder = &m_def_enc;
  }
}

void FAChain2Num_judy::Clear ()
{
  // free map, if needed
  if (NULL != m_map) {

    JudySLFreeArray (&m_map, PJE0);
    DebugLogAssert (NULL == m_map);
  }
}

const int * FAChain2Num_judy::Get (const int * pChain, const int Size) const
{
  const int * pValue;

  DebugLogAssert (NULL != pChain);
  DebugLogAssert (NULL != m_pEncoder);

  // allocate buffer
  char * pTmpBuff = (char *) alloca (m_pEncoder->GetMaxBytes () * Size + 1);
  DebugLogAssert (pTmpBuff);

  // make encoding
  const int BytesUsed = 
    m_pEncoder->Encode (pChain, Size, (unsigned char *) pTmpBuff);
  pTmpBuff [BytesUsed] = 0;

  // get the pointer to the value
  pValue = (const int *) JudySLGet (m_map, pTmpBuff, PJE0);

  return pValue;
}

const int FAChain2Num_judy::Add (const int * pChain, 
                                 const int Size, 
                                 const int Value)
{
  int * pValue;

  DebugLogAssert (Size < MAX_CHAIN_SIZE);
  DebugLogAssert (NULL != m_pEncoder);

  // buffer size assertion
  DebugLogAssert ((m_pEncoder->GetMaxBytes () * Size) + 1 < CHAIN_BUFF_SIZE);

  // make encoding
  const int BytesUsed = 
    m_pEncoder->Encode (pChain, Size, m_buff);
  m_buff [BytesUsed] = 0;

  // get the pointer to the value
  pValue = (int *) JudySLIns (&m_map, (const char *) m_buff, PJE0);
  DebugLogAssert (NULL != pValue);

  // initialize value
  *pValue = Value;

  return 0;
}

void FAChain2Num_judy::Remove (const int * pChain, const int Size)
{
    DebugLogAssert (NULL != pChain);

    // make encoding
    const int BytesUsed = m_pEncoder->Encode (pChain, Size, m_buff);
    m_buff [BytesUsed] = 0;

    // remove the pair
    JudySLDel (&m_map, (const char *) m_buff, PJE0);
}

const int FAChain2Num_judy::GetChainCount () const
{
  /// not supported
  DebugLogAssert (0);

  return -1;
}

const int FAChain2Num_judy::GetChain (const int /*ChainIdx*/,
                                      const int ** /*pChain*/) const
{
  /// not supported
  DebugLogAssert (0);

  return -1;
}

const int FAChain2Num_judy::GetValue (const int /*Idx*/) const
{
  /// not supported
  DebugLogAssert (0);

  return 0;
}

}

// of ifdef HAVE_JUDYSL_LIB
#endif
