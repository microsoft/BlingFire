/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAEncoder_mask.h"
#include "FAEncodeUtils.h"

namespace BlingFire
{


const int FAEncoder_mask::GetMaxBytes () const
{
  return MAX_BYTES_PER_INT;
}


const int FAEncoder_mask::Encode (const int * pChain,
				  const int Size,
				  unsigned char * pOutBuffer) const
{
  DebugLogAssert (pChain);
  DebugLogAssert (pOutBuffer);
  DebugLogAssert (0 <= Size);

  unsigned char * pOut = (unsigned char *) pOutBuffer;

  for (int i = 0; i < Size; ++i) {

    const unsigned int E = pChain [i];

    *pOut = (unsigned char) (E >> 24);
    FAMaskZeroByte(pOut);
    pOut++;

    *pOut = (unsigned char) ((E & 0x00FF0000) >> 16);
    FAMaskZeroByte(pOut);
    pOut++;

    *pOut = (unsigned char) ((E & 0x0000FF00) >> 8);
    FAMaskZeroByte(pOut);
    pOut++;

    *pOut = (unsigned char) (E & 0x000000FF);
    FAMaskZeroByte(pOut);
    pOut++;
  }

  return int (pOut - pOutBuffer);
}

}
