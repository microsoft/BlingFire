/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAStr2Int_hash.h"

namespace BlingFire
{


FAStr2Int_hash::FAStr2Int_hash (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc)
{
    m_i2str.SetAllocator (pAlloc);
    m_i2str.Create ();

    m_i2value.SetAllocator (pAlloc);
    m_i2value.Create ();

    m_csets.SetAllocator (pAlloc);
    m_csets.Create ();
}


FAStr2Int_hash::~FAStr2Int_hash ()
{
    FAStr2Int_hash::Clear ();
}


const int FAStr2Int_hash::Str2Key (const char * pStr, const int Size)
{
    DebugLogAssert (pStr && 0 < Size);

    unsigned long Key = 0;

    int ChainSize = Size / sizeof (int);    
    const int * pChain = (const int *) pStr;

    while (ChainSize--) {
        Key = Key * 33 + *pChain++;
    }

    int SuffixSize = Size % sizeof (int);

    if (0 < SuffixSize) {

        const char * pSuffix = pStr + Size - SuffixSize;

        while (SuffixSize--) {
            Key = Key * 33 + *pSuffix++;
        }
    }

    return Key;
}


const bool FAStr2Int_hash::Equal (const int i,
                                  const char * pStr, 
                                  const int Size) const
{
    DebugLogAssert (pStr && 0 < Size);
    DebugLogAssert (0 <= i && (unsigned int) i < m_i2str.size ());

    const FAArray_cont_t < char > * pKeptStr = &(m_i2str [i]);
    DebugLogAssert (pKeptStr);

    if (pKeptStr->size () == (unsigned int) Size) {

        const char * pKeptStrPtr = pKeptStr->begin ();

        int ChainSize = Size / sizeof (int);    

        const int * pChain1 = (const int *) pStr;
        const int * pChain2 = (const int *) pKeptStrPtr;

        while (ChainSize--) {
            if (*pChain1 != *pChain2)
                return false;
            pChain1++;
            pChain2++;
        }

        int SuffixSize = Size % sizeof (int);

        if (0 < SuffixSize) {

            const char * pSuffix1 = pStr + Size - SuffixSize;
            const char * pSuffix2 = pKeptStrPtr + Size - SuffixSize;

            while (SuffixSize--) {
                if (*pSuffix1 != *pSuffix2)
                    return false;
                pSuffix1++;
                pSuffix2++;
            }
        }

        return true;

    } else {

        return false;
    }
}


const int FAStr2Int_hash::Str2Idx (const char * pStr, const int Size) const
{
    DebugLogAssert (pStr && 0 < Size);

    const int Key = Str2Key (pStr, Size);
    const int * pIdx = m_key2idx.Get (Key);

    if (pIdx) {

        const int Idx = *pIdx;
        DebugLogAssert (0 != Idx);

        if (0 < Idx) {

            const int i = Idx - 1;
            if (true == Equal (i, pStr, Size))
                return i;

        } else {

            const int j = (-Idx) - 1;
            DebugLogAssert (0 <= j && (unsigned int) j < m_csets.size ());

            const FAArray_cont_t < int > * pCSet = &(m_csets [j]);
            DebugLogAssert (pCSet);

            const int SetSize = pCSet->size ();
            const int * pSetBuff = pCSet->begin ();
            DebugLogAssert (0 < SetSize && pSetBuff);

            for (int k = 0; k < SetSize; ++k) {

                const int i = pSetBuff [k];
                if (true == Equal (i, pStr, Size))
                    return i;
            }
        } // of if (0 < Idx) ...

    } // of if (pIdx) ...

    return -1;
}


const bool FAStr2Int_hash::Get (const char * pStr, 
                                const int Size, 
                                int * pValue) const
{
    DebugLogAssert (pStr && 0 < Size);
    DebugLogAssert (m_i2str.size () == m_i2value.size ());

    const int i = Str2Idx (pStr, Size);

    if (-1 != i) {

        *pValue = m_i2value [i];
        return true;

    } else {

        return false;
    }
}


void FAStr2Int_hash::PushBack (const char * pStr,
                               const int Size, 
                               const int Value)
{
    DebugLogAssert (0 < Size && pStr);

    // add value
    m_i2value.push_back (Value);

    // add string
    const int NewI = m_i2str.size ();
    m_i2str.resize (NewI + 1);

    FAArray_cont_t < char > * pKeptStr = &(m_i2str [NewI]);
    DebugLogAssert (pKeptStr);

    pKeptStr->SetAllocator (m_pAlloc);
    pKeptStr->Create ();
    pKeptStr->resize (Size, 0);

    memcpy (pKeptStr->begin (), pStr, Size);
}


void FAStr2Int_hash::PushBackCSet ()
{
    const int NewJ = m_csets.size ();
    m_csets.resize (NewJ + 1);

    FAArray_cont_t < int > * pCSet = &(m_csets [NewJ]);
    DebugLogAssert (pCSet);

    pCSet->SetAllocator (m_pAlloc);
    pCSet->Create ();
}


const int FAStr2Int_hash::Add (const char * pStr, 
                               const int Size, 
                               const int Value)
{
    DebugLogAssert (pStr && 0 < Size);
    DebugLogAssert (m_i2str.size () == m_i2value.size ());

    const int i = Str2Idx (pStr, Size);

    // see whether such string does not exist
    if (-1 == i) {

        const int NewI = m_i2str.size ();
        PushBack (pStr, Size, Value);

        const int Key = Str2Key (pStr, Size);
        const int * pIdx = m_key2idx.Get (Key);

        // see whether Key does not exist
        if (!pIdx) {

            m_key2idx.Set (Key, NewI + 1);

        } else {

            const int Idx = *pIdx;

            // see whether there is collition set
            if (0 > Idx) {

                const int j = (-Idx) - 1;
                DebugLogAssert (0 <= j && (unsigned int) j < m_csets.size ());

                FAArray_cont_t < int > * pCSet = &(m_csets [j]);
                DebugLogAssert (pCSet);

                pCSet->push_back (NewI, 5);

            } else {

                const int NewJ = m_csets.size ();
                PushBackCSet ();

                FAArray_cont_t < int > * pCSet = &(m_csets [NewJ]);
                DebugLogAssert (pCSet);

                const int OldI = Idx - 1;
                DebugLogAssert (0 <= OldI);

                pCSet->push_back (OldI, 5);
                pCSet->push_back (NewI, 5);

                m_key2idx.Set (Key, - (NewJ + 1));

            } // of if (0 > Idx)

        } // of if (!pIdx) ...

        return NewI;

    } else {

        m_i2value [i] = Value;
        return i;
    }
}


const int FAStr2Int_hash::GetStrCount () const
{
    DebugLogAssert (m_i2str.size () == m_i2value.size ());

    const int Count = m_i2str.size ();
    return Count;
}


const int FAStr2Int_hash::GetStr (const int Idx, const char ** ppStr) const
{
    DebugLogAssert (ppStr);
    DebugLogAssert (0 <= Idx && (unsigned int) Idx < m_i2str.size ());
    DebugLogAssert (m_i2str.size () == m_i2value.size ());

    const FAArray_cont_t < char > * pKeptStr = &(m_i2str [Idx]);
    DebugLogAssert (pKeptStr);

    *ppStr = pKeptStr->begin ();
    return pKeptStr->size ();
}


const int FAStr2Int_hash::GetValue (const int Idx) const
{
    DebugLogAssert (0 <= Idx && (unsigned int) Idx < m_i2value.size ());
    DebugLogAssert (m_i2str.size () == m_i2value.size ());

    const int Value = m_i2value [Idx];
    return Value;
}


void FAStr2Int_hash::Clear ()
{
    m_key2idx.Clear ();
    m_i2value.resize (0);

    int i;

    const int Count = m_i2str.size ();
    for (i = 0; i < Count; ++i) {

        FAArray_cont_t < char > * pKeptStr = &(m_i2str [i]);
        DebugLogAssert (pKeptStr);
        pKeptStr->Clear ();
    }
    m_i2str.resize (0);

    const int ColisionCount = m_csets.size ();
    for (i = 0; i < ColisionCount; ++i) {

        FAArray_cont_t < int > * pSet = &(m_csets [i]);
        DebugLogAssert (pSet);
        pSet->Clear ();
    }
    m_csets.resize (0);
}

}
