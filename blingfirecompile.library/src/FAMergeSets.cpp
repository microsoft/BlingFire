/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMergeSets.h"

namespace BlingFire
{


FAMergeSets::FAMergeSets (FAAllocatorA * pAlloc) :
    m_pE2P (NULL)
{
    m_e2p.SetAllocator (pAlloc);
    m_e2p.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();
}


void FAMergeSets::Prepare (const int Count)
{
    DebugLogAssert (0 <= Count);

    m_e2p.resize (Count + 1);
    memset (m_e2p.begin (), 0, sizeof (m_e2p [0]) * Count);
    m_pE2P = m_e2p.begin ();
}


const int FAMergeSets::GetSet (const int E)
{
    DebugLogAssert (0 == m_stack.size ());
    DebugLogAssert (0 <= E && (unsigned int) E < m_e2p.size ());
    DebugLogAssert (m_pE2P == m_e2p.begin ());

    int e = E;
    while (0 < m_pE2P [e]) {
        m_stack.push_back (e);
        e = m_pE2P [e] - 1;
        DebugLogAssert (0 <= e && (unsigned int) e < m_e2p.size ());
    }

    int StackSize = m_stack.size ();

    if (0 < StackSize) {

        const int * pStack = m_stack.begin ();

        while (0 < StackSize) {
            const int e2 = pStack [--StackSize];
            DebugLogAssert (0 <= e2 && (unsigned int) e2 < m_e2p.size ());
            m_pE2P [e2] = e + 1;
        }

        m_stack.resize (0);
    }

    return e;
}


void FAMergeSets::Merge (const int e1, const int e2)
{
    DebugLogAssert (0 <= e1 && (unsigned int) e1 < m_e2p.size ());
    DebugLogAssert (0 <= e2 && (unsigned int) e2 < m_e2p.size ());
    DebugLogAssert (m_pE2P == m_e2p.begin ());

    const int s1 = FAMergeSets::GetSet (e1);
    const int s2 = FAMergeSets::GetSet (e2);

    if (s1 == s2)
        return;

    const int r1 = - (m_pE2P [s1]);
    DebugLogAssert (0 <= r1);

    const int r2 = - (m_pE2P [s2]);
    DebugLogAssert (0 <= r2);

    if (r1 > r2) {
        m_pE2P [s2] = s1 + 1;
    } else {
        m_pE2P [s1] = s2 + 1;
        if (r1 == r2) {
            m_pE2P [s2]--;
        }
    }
}

}

