/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMergeDumps.h"
#include "FAAllocatorA.h"
#include "FAException.h"
#include "FAFsmConst.h"
#include "FAUtils_cl.h"

namespace BlingFire
{


FAMergeDumps::FAMergeDumps (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc)
{
    m_dumps.SetAllocator (pAlloc);
    m_dumps.Create ();

    m_sizes.SetAllocator (pAlloc);
    m_sizes.Create ();
}


FAMergeDumps::~FAMergeDumps ()
{
    FAMergeDumps::Clear ();
}


void FAMergeDumps::AddDumpFile (const char * pFileName)
{
    DebugLogAssert (m_pAlloc);
    DebugLogAssert (pFileName);

    FILE * file = NULL;
    int res = fopen_s (&file, pFileName, "rb");

    if (0 != res || NULL == file) {
        throw FAException (FAMsg::ReadError, __FILE__, __LINE__);
    }

    res = fseek (file, 0, SEEK_END);
    if (0 != res) {
        throw FAException (FAMsg::ReadError, __FILE__, __LINE__);
    }

    const unsigned int Size = ftell (file);
    DebugLogAssert (0 < Size);

    res = fseek (file, 0, SEEK_SET);
    if (0 != res) {
        throw FAException (FAMsg::ReadError, __FILE__, __LINE__);
    }

    unsigned char * pImageDump = (unsigned char *) FAAlloc (m_pAlloc, Size);
    DebugLogAssert (pImageDump);

    const size_t ActSize = fread (pImageDump, sizeof (char), Size, file);

    if (ActSize != Size) {
        throw FAException (FAMsg::ReadError, __FILE__, __LINE__);
    }

    fclose (file);

    m_dumps.push_back (pImageDump);
    m_sizes.push_back (Size);
}


void FAMergeDumps::Clear ()
{
    const int DumpCount = m_dumps.size ();

    for (int i = 0; i < DumpCount; ++i) {

        unsigned char * pImageDump = m_dumps [i];
        DebugLogAssert (pImageDump);

        FAFree (m_pAlloc, pImageDump);
    }

    m_dumps.resize (0);
    m_sizes.resize (0);
}


void FAMergeDumps::AddValidationDump ()
{
    DebugLogAssert (0 < m_dumps.size () && m_dumps.size () == m_sizes.size ());

    unsigned int * pVal = (unsigned int *) FAAlloc (m_pAlloc, FAFsmConst::VALIDATION_COUNT * sizeof(unsigned int));
    DebugLogAssert (pVal);

    memset(pVal, 0, FAFsmConst::VALIDATION_COUNT * sizeof(unsigned int));

    const int DumpCount = (int) m_dumps.size ();
    for (int i = 0; i < DumpCount; ++i) {

        LogAssert (0 <= m_sizes [i]);

        pVal [FAFsmConst::VALIDATION_SIZE] += m_sizes [i];
        pVal [FAFsmConst::VALIDATION_HASH] = FAGetCrc32 (m_dumps [i], (size_t) m_sizes [i], pVal [FAFsmConst::VALIDATION_HASH]);
    }

    m_dumps.push_back ((unsigned char *) pVal);
    m_sizes.push_back (FAFsmConst::VALIDATION_COUNT * sizeof(unsigned int));
}


void FAMergeDumps::Save (std::ostream * pOs)
{
    DebugLogAssert (pOs);
    DebugLogAssert (0 < m_dumps.size () && m_dumps.size () == m_sizes.size ());

    int i;

    // always add the last validation dump
    AddValidationDump ();

    // convert sizes into entry offsets
    const int DumpCount = m_dumps.size ();
    pOs->write ((const char *) &DumpCount, sizeof (int));

    // initial Offset will be sizeof (int) * (DumpCount + 1);
    int Offset = sizeof (int) * (DumpCount + 1);

    for (i = 0; i < DumpCount; ++i) {

        const int CurrSize = m_sizes [i];
        pOs->write ((const char *) &Offset, sizeof (int));
        Offset += CurrSize;
    }

    for (i = 0; i < DumpCount; ++i) {

        const int CurrSize = m_sizes [i];
        DebugLogAssert (0 < CurrSize);
        const unsigned char * pImageDump = m_dumps [i];
        DebugLogAssert (pImageDump);

        pOs->write ((const char *) pImageDump, CurrSize);
    }
}

}
