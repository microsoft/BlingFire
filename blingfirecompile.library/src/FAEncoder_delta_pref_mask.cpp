/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAEncoder_delta_pref_mask.h"
#include "FAEncodeUtils.h"

namespace BlingFire
{


const int FAEncoder_delta_pref_mask::GetMaxBytes () const
{
  return MAX_BYTES_PER_INT;
}


const int FAEncoder_delta_pref_mask::Encode (const int * pChain,
					     const int Size,
					     unsigned char * pOutBuffer) const
{
  DebugLogAssert (pChain);
  DebugLogAssert (pOutBuffer);
  DebugLogAssert (0 < Size);

  unsigned char * pOut = pOutBuffer;

  unsigned int PrevV = ((const unsigned int *)pChain) [0];
  FAEncodeIntPrefixMaskZero (PrevV, pOut);

  for (int i = 1; i < Size; ++i) {

    const unsigned int V = ((const unsigned int *)pChain) [i];
    const unsigned int Delta = V - PrevV;

    FAEncodeIntPrefixMaskZero (Delta, pOut);
    PrevV = V;
  }

  return int (pOut - pOutBuffer);
}

}
