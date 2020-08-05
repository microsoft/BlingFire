/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAChain2Num_hash.h"
#include "FAAllocatorA.h"
#include "FAException.h"
#include "FALimits.h"

namespace BlingFire
{


FAChain2Num_hash::FAChain2Num_hash () :
  m_pAlloc (NULL)
{}


FAChain2Num_hash::~FAChain2Num_hash ()
{
  FAChain2Num_hash::Clear ();
}


void FAChain2Num_hash::SetEncoder (const FAEncoderA * /*pEncoder*/)
{}


void FAChain2Num_hash::SetCopyChains (const bool /*CopyChains*/)
{}


void FAChain2Num_hash::SetAllocator (FAAllocatorA * pAlloc)
{
  m_pAlloc = pAlloc;

  if (pAlloc) {

    m_csets.SetAllocator (pAlloc);
    m_csets.Create ();

    m_i2chain.SetAllocator (pAlloc);
    m_i2chain.Create ();
  
    m_i2value.SetAllocator (pAlloc);
    m_i2value.Create ();

    m_i_gaps.Create (pAlloc);
    m_j_gaps.Create (pAlloc);
  }
}


void FAChain2Num_hash::Clear ()
{
    /// TODO: remove this, make all objects created in constructor
    if (m_pAlloc) {

        m_key2idx.Clear ();
        m_i2value.resize (0);
        m_i_gaps.clear ();
        m_j_gaps.clear ();

        int i;

        const int ChainCount = m_i2chain.size ();

        for (i = 0; i < ChainCount; ++i) {

            int * pChain = m_i2chain [i];

            if (pChain) {
                FAFree (m_pAlloc, pChain);
            }
        }
        m_i2chain.resize (0);

        const int ColisionCount = m_csets.size ();

        for (i = 0; i < ColisionCount; ++i) {

            FAArray_cont_t < int > * pSet = &(m_csets [i]);
            DebugLogAssert (pSet);
            pSet->Clear ();
        }
        m_csets.resize (0);
    }
}


const int FAChain2Num_hash::Chain2Key (const int * pChain, int Size)
{
    DebugLogAssert (pChain);
    DebugLogAssert (0 < Size);

    unsigned long Key = 0;

    while (Size--) {

        Key = Key * 33 + *pChain++;
    }

    return int (Key);
}


const bool FAChain2Num_hash::Equal (const int * pChain1,
                                    const int * pChain2,
                                    const int Size)
{
  int i = 0;

  while (i < Size) {

    if (*pChain1 != *pChain2)
      return false;

    pChain1++;
    pChain2++;
    i++;
  }

  return true;
}


inline const int FAChain2Num_hash::
    Chain2Idx (const int * pChain, const int Size) const
{
    DebugLogAssert (pChain && 0 < Size);

    const int Key = Chain2Key (pChain, Size);
    const int * pIdx = m_key2idx.Get (Key);

    if (pIdx) {

        const int Idx = *pIdx;
        DebugLogAssert (0 != Idx);

        if (0 < Idx) {

            const int i = Idx - 1;

            // keeps chains in the following format:  [N, a_1, a_2, ..., a_N]
            const int * pStoredChain = m_i2chain [i];
            DebugLogAssert (pStoredChain);
            const int StoredSize = *pStoredChain++;

            if (StoredSize == Size && \
                true == Equal (pStoredChain, pChain, Size)) {
                return i;
            }

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

                const int * pStoredChain = m_i2chain [i];
                DebugLogAssert (pStoredChain);
                const int StoredSize = *pStoredChain++;

                if (StoredSize == Size && \
                    true == Equal (pStoredChain, pChain, Size)) {
                  return i;
                }

            } // of for (int k = 0; ...

        } // of if (0 < Idx) ...

    } // of if (pIdx) ...

    return -1;
}


const int * FAChain2Num_hash::
    Get (const int * pChain, const int Size) const
{
    DebugLogAssert (pChain && 0 < Size);
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());

    const int i = Chain2Idx (pChain, Size);

    if (-1 != i) {

        const int * pValue = & (m_i2value [i]);
        return pValue;

    } else {

        return NULL;
    }
}


inline const int FAChain2Num_hash::
    AddNewChain (const int * pChain, 
                 const int Size, 
                 const int Value)
{
    DebugLogAssert (0 < Size && pChain);
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());

    /// overflow check: sizeof (int) * (Size + 1)
    if (0 > Size || FALimits::MaxChainSize < Size) {
        throw FAException (FAMsg::LimitIsExceeded, __FILE__, __LINE__);
    }

    // allocate memory for a chain copy
    int * pChainCopy = (int *) FAAlloc (m_pAlloc, sizeof (int) * (Size + 1));
    // copy chain
    *pChainCopy = Size;
    memcpy (pChainCopy + 1, pChain, sizeof (int) * Size);

    // store chain pointer and value
    if (m_i_gaps.empty ()) {

        const int NewIdx = m_i2chain.size ();

        m_i2value.push_back (Value);
        m_i2chain.push_back (pChainCopy);

        return NewIdx;

    } else {

        const int * pNewIdx = m_i_gaps.top ();
        DebugLogAssert (pNewIdx);

        const int NewIdx = *pNewIdx;
        m_i_gaps.pop ();

        m_i2value [NewIdx] = Value;
        m_i2chain [NewIdx] = pChainCopy;

        return NewIdx;
    }
}


inline const int FAChain2Num_hash::
    AddNewCSet (const int i1, const int i2)
{
    int NewJ;
    FAArray_cont_t < int > * pCSet;

    // see if there are no unused collision sets
    if (m_j_gaps.empty ()) {

        NewJ = m_csets.size ();
        m_csets.resize (NewJ + 1);

        pCSet = &(m_csets [NewJ]);
        DebugLogAssert (pCSet);

        pCSet->SetAllocator (m_pAlloc);
        pCSet->Create (2);
        pCSet->resize (2);

    } else {

        const int * pNewJ = m_j_gaps.top ();
        DebugLogAssert (pNewJ);

        NewJ = *pNewJ;
        m_j_gaps.pop ();

        pCSet = &(m_csets [NewJ]);
        DebugLogAssert (pCSet);

        pCSet->resize (2);
    }

    int * pCSetValues = pCSet->begin ();

    pCSetValues [0] = i1;
    pCSetValues [1] = i2;

    return NewJ;
}


const int FAChain2Num_hash::
    Add (const int * pChain, const int Size, const int Value)
{
    DebugLogAssert (pChain && 0 < Size);
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());

    const int i = Chain2Idx (pChain, Size);

    // see whether such chain does not exist
    if (-1 == i) {

        // add new <chain, value> pair
        const int NewI = AddNewChain (pChain, Size, Value);

        // generate the Key, 
        // TODO: fix: Chain2Idx has already made these two look-ups
        const int Key = Chain2Key (pChain, Size);
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

                pCSet->push_back (NewI, 2);

            } else {

                const int OldI = Idx - 1;
                DebugLogAssert (0 <= OldI);

                const int NewJ = AddNewCSet (OldI, NewI);
                m_key2idx.Set (Key, - (NewJ + 1));

            } // of if (0 > Idx)

        } // of if (!pIdx) ...

        return NewI;

    } else {

        m_i2value [i] = Value;
        return i;
    }
}


const int FAChain2Num_hash::GetChainCount () const
{
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());

    const int ChainCount = m_i2chain.size ();
    return ChainCount;
}


const int FAChain2Num_hash::
    GetChain (const int Idx, const int ** ppChain) const
{
    DebugLogAssert (ppChain);
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());
    DebugLogAssert (0 <= Idx && (unsigned int) Idx < m_i2chain.size ());

    const int * pStoredChain = m_i2chain [Idx];

    // see whether chain was not deleted and we are not in the "gap"
    if (pStoredChain) {

        const int Size = *pStoredChain++;
        *ppChain = pStoredChain;

        return Size;

    } else {

        return -1;
    }
}


const int FAChain2Num_hash::GetValue (const int Idx) const
{
    DebugLogAssert (m_i2chain.size () == m_i2value.size ());
    DebugLogAssert (0 <= Idx && (unsigned int) Idx < m_i2chain.size ());

    const int Value = m_i2value [Idx];
    return Value;
}


const int FAChain2Num_hash::GetIdx (const int * pChain, const int Size) const
{
    DebugLogAssert (pChain && 0 < Size);

    const int ChainIdx = Chain2Idx (pChain, Size);
    return ChainIdx;
}


void FAChain2Num_hash::Remove (const int * pChain, const int Size)
{
    DebugLogAssert (pChain && 0 < Size);

    const int Key = Chain2Key (pChain, Size);
    const int * pIdx = m_key2idx.Get (Key);

    // have nothing to do
    if (NULL == pIdx) {
        return;
    }

    const int Idx = *pIdx;
    DebugLogAssert (0 != Idx);

    // see if there are no collisions for this Key
    if (0 < Idx) {

        const int i = Idx - 1;

        // keeps chains in the followinf format:  [N, a_1, a_2, ..., a_N]
        int * pStoredChain = m_i2chain [i];
        DebugLogAssert (pStoredChain);
        const int StoredSize = *pStoredChain;

        // Make sure stored chain and pChain equal, otherwise we have nothing
        // to do here.
        if (StoredSize == Size && \
            true == Equal (pStoredChain + 1, pChain, Size)) {

            // remove the Key
            m_key2idx.Remove (Key);
            // free Stored Chain memory
            FAFree (m_pAlloc, pStoredChain);
            m_i2chain [i] = NULL;
            // put i into deleted indices
            m_i_gaps.push (i);
        }

    } else {

        const int j = (-Idx) - 1;
        DebugLogAssert (0 <= j && (unsigned int) j < m_csets.size ());

        FAArray_cont_t < int > * pCSet = &(m_csets [j]);
        DebugLogAssert (pCSet);

        const int SetSize = pCSet->size ();
        int * pSetBuff = pCSet->begin ();
        DebugLogAssert (0 < SetSize && pSetBuff);

        int k;

        for (k = 0; k < SetSize; ++k) {

            const int i = pSetBuff [k];

            int * pStoredChain = m_i2chain [i];
            DebugLogAssert (pStoredChain);
            const int StoredSize = *pStoredChain;

            if (StoredSize == Size && \
                true == Equal (pStoredChain + 1, pChain, Size)) {

                // free Stored Chain memory
                FAFree (m_pAlloc, pStoredChain);
                m_i2chain [i] = NULL;
                // put i into deleted indices
                m_i_gaps.push (i);
                // stop searching, k is the index of matched chain in the set
                break;
            }

        } // of for (int k = 0; ...

        // see whether something was deleted
        if (k < SetSize) {

            // move left all CSet values after k
            for (; k < SetSize - 1; ++k) {
                pSetBuff [k] = pSetBuff [k + 1];
            }
            pCSet->pop_back ();

            // see whether pCSet contained only 2 elements
            // (one have been deleted and now there are no collisions)
            if (2 == SetSize) {

                const int i = (*pCSet) [0];
                m_key2idx.Set (Key, i + 1);

                pCSet->resize (0);
                m_j_gaps.push (j);
            }
        } // of if (k < SetSize) ...

    } // of if (0 < Idx) ...
}

}
