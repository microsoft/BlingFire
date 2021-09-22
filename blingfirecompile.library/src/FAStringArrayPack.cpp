/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAStringArrayPack.h"

namespace BlingFire
{

FAStringArrayPack::FAStringArrayPack (FAAllocatorA * pAlloc) :
    m_pBuff (NULL),
    m_pOffsets (NULL)
{
    m_dump.SetAllocator (pAlloc);
    m_dump.Create ();
}

void FAStringArrayPack::SetArray (const FAArray_cont_t < unsigned char > * pBuff, const FAArray_cont_t < int > * pOffsets)
{
    m_pBuff = pBuff;
    m_pOffsets = pOffsets;
}

void FAStringArrayPack::Process ()
{
    // allocate memory    
    m_dump.resize (sizeof (int) + (sizeof (int) * m_pOffsets->size()) + sizeof (int) + m_pBuff->size());
    unsigned char * pDump = m_dump.begin ();

    // store the number of strings in the array
    *((unsigned int *) pDump) = m_pOffsets->size();
    pDump += sizeof(int);

    // store the offsets
    for(int i = 0; i < (int) m_pOffsets->size(); ++i) {

        const int Offset = (*m_pOffsets)[i];
        FAAssert(Offset >= 0 && (unsigned int) Offset < m_pBuff->size(), FAMsg::InternalError);

        *((unsigned int *) pDump) = Offset;
        pDump += sizeof(int);
    }

    // store the text length (or an offset to the end)
    *((unsigned int *) pDump) = m_pBuff->size();
    pDump += sizeof(int);

    // store the text
    if (0 < m_pBuff->size()) {
        unsigned char * pArr = (unsigned char *) pDump;
        memcpy (pArr, m_pBuff->begin(), m_pBuff->size());
    }
}

const int FAStringArrayPack::GetDump (const unsigned char ** ppDump) const
{
    DebugLogAssert (ppDump);
    *ppDump = m_dump.begin ();
    return m_dump.size ();
}

}
