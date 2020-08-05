/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAEncoder_pref_mask.h"
#include "FAEncodeUtils.h"

namespace BlingFire
{


const int FAEncoder_pref_mask::GetMaxBytes () const
{
  return MAX_BYTES_PER_INT;
}

const int FAEncoder_pref_mask::Encode (const int * pChain,
                                       const int Size,
                                       unsigned char * pOutBuffer) const
{
  DebugLogAssert (pChain);
  DebugLogAssert (pOutBuffer);
  DebugLogAssert (0 <= Size);

  unsigned char * pOut = pOutBuffer;

  for (int i = 0; i < Size; ++i) {

    const unsigned int V = ((const unsigned int *)pChain) [i];
    FAEncodeIntPrefixMaskZero(V, pOut);
  }

  return int (pOut - pOutBuffer);
}

}
