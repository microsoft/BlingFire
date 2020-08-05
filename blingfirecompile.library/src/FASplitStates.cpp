/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASplitStates.h"
#include "FARSNfaA.h"
#include "FAUtils.h"

namespace BlingFire
{


FASplitStates::FASplitStates (FAAllocatorA * pAlloc) :
    m_pNfa (NULL),
    m_pS2C (NULL),
    m_Size (0),
    m_split_sets (pAlloc)
{
    m_dsts2num.SetAllocator (pAlloc);
    m_dsts2num.SetEncoder (&m_enc);

    m_alphabet.SetAllocator (pAlloc);
    m_alphabet.Create ();

    m_info.SetAllocator (pAlloc);
    m_info.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();
}


void FASplitStates::Clear ()
{
    m_alphabet.Clear ();
    m_alphabet.Create ();
    m_info.Clear ();
    m_info.Create ();
    m_tmp.Clear ();
    m_tmp.Create ();
    m_split_sets.Clear ();
    m_dsts2num.Clear ();
}


void FASplitStates::SetNfa (const FARSNfaA * pNfa)
{
    m_pNfa = pNfa;
}


void FASplitStates::SetS2C (const int * pS2C, const int Size)
{
    m_pS2C = pS2C;
    m_Size = Size;

    m_split_sets.Prepare (Size);

    // avoid grouping states with different colors
    m_split_sets.AddInfo (pS2C, Size);
}


inline const int FASplitStates::GetDstId (const int * pDsts, const int Dsts)
{
    DebugLogAssert (0 < Dsts && pDsts);
    DebugLogAssert (FAIsSortUniqed (pDsts, Dsts));

    // remap destination set into color set

    m_tmp.resize (0);

    for (int i = 0; i < Dsts; ++i) {

        const int Dst = pDsts [i];

        DebugLogAssert (0 <= Dst && Dst < m_Size);
        const int C = m_pS2C [Dst];
        DebugLogAssert (-1 != C);

        m_tmp.push_back (C);
    }

    const int NewSize = \
        FASortUniq (m_tmp.begin (), m_tmp.end ());
    m_tmp.resize (NewSize);
    DebugLogAssert (0 < NewSize);

    // assign id for each unique color set

    int Id = m_dsts2num.GetIdx (m_tmp.begin (), NewSize);

    if (-1 == Id) {
        Id = m_dsts2num.Add (m_tmp.begin (), NewSize, 0);
        DebugLogAssert (-1 != Id);
    }

    return Id;
}


void FASplitStates::Process ()
{
    DebugLogAssert (m_pNfa && m_pS2C);

    int DstsId;

    FAGetAlphabet (m_pNfa, &m_alphabet);

    const int * pIws = m_alphabet.begin ();
    const int IwCount = m_alphabet.size ();
    DebugLogAssert (0 < IwCount && pIws);
    DebugLogAssert (FAIsSortUniqed (pIws, IwCount));

    DebugLogAssert (m_Size <= m_pNfa->GetMaxState () + 1);
    const int StateCount = m_Size;

    m_info.resize (StateCount);
    int * pInfo = m_info.begin ();
    DebugLogAssert (pInfo);

    for (int i = 0; i < IwCount; ++i) {

        const int Iw = pIws [i];

        for (int State = 0; State < StateCount; ++State) {

            const int Color = m_pS2C [State];

            if (-1 == Color) {

                pInfo [State] = -1;

            } else {

                const int * pDsts;
                const int Dsts = m_pNfa->GetDest (State, Iw, &pDsts);

                if (0 < Dsts) {
                    DstsId = GetDstId (pDsts, Dsts);
                } else {
                    DstsId = 0;
                }

                pInfo [State] = DstsId;
            }

        } // of for (int State = 0; ...

        m_split_sets.AddInfo (pInfo, StateCount);

    } // for (int i = 0; ...

    m_split_sets.Process ();

    m_dsts2num.Clear ();
}


const int FASplitStates::GetS2C (const int ** ppS2C)
{
    return m_split_sets.GetClasses (ppS2C);
}

}
