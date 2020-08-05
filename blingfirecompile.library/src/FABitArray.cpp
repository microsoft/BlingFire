/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FABitArray.h"
#include "FAAllocatorA.h"

namespace BlingFire
{


FABitArray::FABitArray () :
    m_ar (NULL),
    m_ar_size (0),
    m_size (0),
    m_pAlloc (NULL)
{}


FABitArray::~FABitArray ()
{
    FABitArray::Clear ();
}


void FABitArray::SetAllocator (FAAllocatorA* pAlloc)
{
    m_pAlloc = pAlloc;
}


void FABitArray::Create ()
{
    DebugLogAssert (m_pAlloc);

    m_size = 0;
    m_ar_size = 1;
    m_ar = (unsigned int *) FAAlloc (m_pAlloc, m_ar_size * sizeof (unsigned int));
}


void FABitArray::Clear ()
{
    if (0 < m_ar_size) {

        DebugLogAssert (m_pAlloc);
        FAFree (m_pAlloc, m_ar);
    }

    m_ar = NULL;
    m_ar_size = 0;
    m_size = 0;
}


void FABitArray::set_bits (const unsigned int FromIdx,
                           const unsigned int ToIdx,
                           const bool Val)
{
    DebugLogAssert (FromIdx < size ());
    DebugLogAssert (ToIdx < size ());

    unsigned int HiFromIdx = FromIdx >> 5;
    unsigned int HiToIdx = ToIdx >> 5;

    if (HiFromIdx < HiToIdx) {

        const unsigned int LoFromIdx = FromIdx & 31;
        const unsigned int LoToIdx = ToIdx & 31;

        if (Val) {

            if (0 != LoFromIdx) {

                unsigned int Mask = 0;
                for (unsigned int i = LoFromIdx; i <= 31; ++i) {
                    Mask |= (1 << i);
                }
                m_ar [HiFromIdx] |= Mask;
                HiFromIdx++;
            }
            if (0 != LoToIdx) {

                unsigned int Mask = 0;
                for (unsigned int i = 0; i <= LoToIdx; ++i) {
                    Mask |= (1 << i);
                }
                m_ar [HiToIdx] |= Mask;
                HiToIdx--;
            }
            for (unsigned int i = HiFromIdx; i <= HiToIdx; ++i) {
                m_ar [i] = 0xffffffff;
            }

        } else {

            if (0 != LoFromIdx) {

                unsigned int Mask = 0;
                for (unsigned int i = LoFromIdx; i <= 31; ++i) {
                    Mask |= (1 << i);
                }
                m_ar [HiFromIdx] &= ~Mask;
                HiFromIdx++;
            }
            if (0 != LoToIdx) {

                unsigned int Mask = 0;
                for (unsigned int i = 0; i <= LoToIdx; ++i) {
                    Mask |= (1 << i);
                }
                m_ar [HiToIdx] &= ~Mask;
                HiToIdx--;
            }
            for (unsigned int i = HiFromIdx; i <= HiToIdx; ++i) {
                m_ar [i] = 0;
            }
        }

    } else {

        for (unsigned int i = FromIdx; i <= ToIdx; ++i) {
            set_bit (i, Val);
        }
    }
}


void FABitArray::push_back (const bool Val)
{
    DebugLogAssert (m_ar);

    resize (m_size + 1);
    set_bit (m_size - 1, Val);
}


void FABitArray::push_back (const bool Val, const int Delta)
{
    DebugLogAssert (m_ar);

    resize (m_size + 1, Delta);
    set_bit (m_size - 1, Val);
}


void FABitArray::pop_back ()
{
    m_size--;
}


const unsigned int FABitArray::size () const
{
    return m_size;
}


void FABitArray::clear ()
{
    m_size = 0;
}


void FABitArray::resize (const unsigned int N, const unsigned int Delta)
{
    // get the required number of ints
    const unsigned int N2 = ((31 + N) >> 5);

    // check whether we have to allocate more memory
    if (m_ar_size < N2) {

        DebugLogAssert (m_pAlloc);

        // get the delta in ints
        const unsigned int Delta2 = (Delta >> 5);

        m_ar_size = N2 + Delta2;
        m_ar = (unsigned int *) 
          FAReAlloc (m_pAlloc, m_ar, m_ar_size * sizeof (unsigned int));
    }

    m_size = N;
}


void FABitArray::resize (const unsigned int N)
{
    // get the required number of ints
    const unsigned int N2 = ((31 + N) >> 5);

    // check whether we have to allocate more memory
    if (m_ar_size < N2) {

        DebugLogAssert (m_pAlloc);
        m_ar_size = N2 + 32;
        m_ar = (unsigned int *) 
          FAReAlloc (m_pAlloc, m_ar, m_ar_size * sizeof (unsigned int));
    }

    m_size = N;
}

const unsigned int * FABitArray::begin () const
{
    return m_ar;
}

}

