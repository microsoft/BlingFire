/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAFloatArrayPack.h"

namespace BlingFire
{


FAFloatArrayPack::FAFloatArrayPack (FAAllocatorA * pAlloc) :
    m_pArr (NULL),
    m_Size (0)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}

void FAFloatArrayPack::SetArray (const float * pArr, const int Size)
{
    DebugLogAssert ((0 < Size && pArr) || 0 == Size);

    m_pArr = pArr;
    m_Size = Size;
}

void FAFloatArrayPack::Process ()
{
    // allocate memory    
    const int Size = sizeof (int) + (sizeof (float) * m_Size);
    m_dump.resize (Size);

    unsigned char * pDump = m_dump.begin ();

    // store the size
    *((int *) pDump) = m_Size;

    // store the elements
    if (0 < m_Size) {
        float * pArr = (float *) (pDump + sizeof (int)) ;
        memcpy (pArr, m_pArr, sizeof (float) * m_Size);
    }
}

const int FAFloatArrayPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_dump.begin ();
    return m_dump.size ();
}

}
