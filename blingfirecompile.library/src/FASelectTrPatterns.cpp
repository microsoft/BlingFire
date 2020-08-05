/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASelectTrPatterns.h"
#include "FALimits.h"
#include "FARSDfaCA.h"
#include "FAMealyDfaCA.h"
#include "FAMultiMapCA.h"
#include "FAArrayCA.h"
#include "FAUtils.h"
#include "FAFsmConst.h"

namespace BlingFire
{


FASelectTrPatterns::FASelectTrPatterns (FAAllocatorA * pAlloc) :
    m_pDfa (NULL),
    m_pMealy (NULL),
    m_pK2I (NULL),
    m_pI2Info (NULL),
    m_NoEmpty (false)
{
    m_used_ids.SetAllocator (pAlloc);
    m_used_ids.Create ();

    m_tmp.SetAllocator (pAlloc);
    m_tmp.Create ();

    m_pos2ids_ends.SetAllocator (pAlloc);
    m_pos2ids_cover.SetAllocator (pAlloc);
}


FASelectTrPatterns::~FASelectTrPatterns ()
{}


void FASelectTrPatterns::SetNoEmpty (const bool NoEmpty)
{
    m_NoEmpty = NoEmpty;
}


void FASelectTrPatterns::
    SetFsm (
        const FARSDfaCA * pDfa, 
        const FAMealyDfaCA * pMealy, 
        const FAOw2IwCA * pOw2Iw
    )
{
    m_pDfa = pDfa;
    m_pMealy = pMealy;

    m_mph_tools.SetRsDfa (pDfa);
    m_mph_tools.SetMealy (pMealy);
    m_mph_tools.SetOw2Iw (pOw2Iw);
}


void FASelectTrPatterns::SetK2I (const FAArrayCA * pK2I)
{
    m_pK2I = pK2I;
}


void FASelectTrPatterns::SetI2Info (const FAMultiMapCA * pI2Info)
{
    m_pI2Info = pI2Info;
}


void FASelectTrPatterns::
    CalcCover (const int * pIws, const int * pOws, const int Count)
{
    DebugLogAssert (m_pI2Info);

    int i;

    m_pos2ids_cover.Clear ();
    m_tmp.resize (Count);

    // mark all positions as unsolved
    for (i = 0; i < Count; ++i) {
        const int Ow = pOws [i];
        DebugLogAssert (0 <= Ow);
        m_tmp [i] = - (Ow + 1);
    }
    // make enumeration of all trigered patterns
    for (i = 0; i < Count; ++i) {

        const int * pIds;
        const int IdCount = m_pos2ids_ends.Get (i, &pIds);

        for (int j = 0; j < IdCount; ++j) {

            DebugLogAssert (pIds);
            const int Id = pIds [j];

            const int InfoId = m_pK2I->GetAt (Id);
            DebugLogAssert (0 <= InfoId);

            // get pattern's Ows
            const int * pPatOws;
            int OwsLen = m_pI2Info->Get (InfoId, &pPatOws);
            DebugLogAssert (0 < OwsLen && pPatOws);

            // skip stored Freq
            OwsLen--;
            pPatOws++;

            const int AlignPos = i + 1 - OwsLen;
            // check that pattern aligns
            FAAssert (0 <= AlignPos, FAMsg::InternalError);

            for (int k = AlignPos; k <= i; ++k) {

                // validate
                DebugLogAssert (pPatOws);
                const int PatOw = pPatOws [k - AlignPos];
                DebugLogAssert (0 <= PatOw);

                if (FAFsmConst::HYPH_DONT_CARE == PatOw)
                    continue;

                m_pos2ids_cover.Add (k, Id);

                const int PatOw1 = PatOw + 1;

                if (PatOw1 == m_tmp [k]) {
                    // no work to do
                    continue;
                } else if (-PatOw1 == m_tmp [k]) {
                    m_tmp [k] = PatOw1;
                    continue;
                } else {
                    PutConflict (pIws + 1, Count - 2, k);
                }
            } // of for (int k = AlignPos; ...

        } // of for (int j = 0; j < IdCount; ...

    } // of for (i = 0; i < Count; ...

    m_pos2ids_cover.SortUniq ();

    // see whether there is anything unsolved here (excluding anchors)
    for (i = 1; i < Count - 1; ++i) {
        // react only for hyphenation points (not for missed empty outputs)
        if (-1 > m_tmp [i])
            break;
    }
    if (i < Count - 1) {
        PutUnsolved (pIws + 1, m_tmp.begin () + 1, Count - 2);
    }
}


inline const int FASelectTrPatterns::
    GetDcCount (const int * pOws, const int Count)
{
    int DcCount = 0;

    for (int i = 0; i < Count; ++i) {
        if (FAFsmConst::HYPH_DONT_CARE == pOws [i]) {
            DcCount++;
        }
    }

    return DcCount;
}


const bool FASelectTrPatterns::Better (const int Id1, const int Id2) const
{
    DebugLogAssert (m_pI2Info && m_pK2I);
    DebugLogAssert (Id1 != Id2);

    const int InfoId1 = m_pK2I->GetAt (Id1);
    DebugLogAssert (0 <= InfoId1);

    const int * pPatOws1;
    const int Len1 = m_pI2Info->Get (InfoId1, &pPatOws1) - 1;
    DebugLogAssert (0 < Len1 && pPatOws1);
    const int Freq1 = *pPatOws1++;

    const int InfoId2 = m_pK2I->GetAt (Id2);
    DebugLogAssert (0 <= InfoId2);

    const int * pPatOws2;
    const int Len2 = m_pI2Info->Get (InfoId2, &pPatOws2) - 1;
    DebugLogAssert (0 < Len2 && pPatOws2);
    const int Freq2 = *pPatOws2++;

    if (Freq1 > Freq2) {

        return true;

    } else if (Freq1 == Freq2 && Len1 < Len2) {

        return true;

    } else if (Freq1 == Freq2 && Len1 == Len2) {

        const int Count1 = GetDcCount (pPatOws1, Len1);
        const int Count2 = GetDcCount (pPatOws2, Len2);

        if (Count1 < Count2) {
            return true;
        } else if (Count1 == Count2 && Id1 < Id2) {
            return true;
        }
    }

    return false;
}


void FASelectTrPatterns::UpdateBest (const int * pOws, const int Count)
{
    DebugLogAssert (pOws);

    int i, j;

    /// collect selected patterns from this run
    m_tmp.resize (0);

    const int * pUsedIds = m_used_ids.begin ();
    const int UsedIdsCount = m_used_ids.size ();
    DebugLogAssert (FAIsSortUniqed (pUsedIds, UsedIdsCount));

    for (i = 0; i < Count; ++i) {

        // skip non-hyphenating position, if asked
        if (m_NoEmpty) {
            const int Ow = pOws [i];
            if (FAFsmConst::HYPH_NO_HYPH == Ow)
                continue;
        }

        const int * pIds;
        const int IdCount = m_pos2ids_cover.Get (i, &pIds);

        if (0 < IdCount) {

            DebugLogAssert (pIds);

            /// see if any selected pattern covers the given position
            for (j = 0; j < IdCount; ++j) {
                const int Id = pIds [j];
                // log search in all selected patterns
                if (-1 != FAFind_log (pUsedIds, UsedIdsCount, Id)) {
                    break;
                }
                // linear search in newly selected patters
                if (-1 != FAFind_linear (m_tmp.begin (), m_tmp.size (), Id)){
                    break;
                }
            }
            if (j < IdCount) {
                continue;
            }

            /// find the best one
            int BestId = pIds [0];
            for (j = 1; j < IdCount; ++j) {
                const int Id = pIds [j];
                if (true == Better (Id, BestId)) {
                    BestId = Id;
                }
            }

            m_tmp.push_back (BestId);
        }
    }

    const int NewPatCount = m_tmp.size ();

    if (0 < NewPatCount) {

        m_used_ids.resize (UsedIdsCount + NewPatCount);
        int * pOut = m_used_ids.begin () + UsedIdsCount;
        memcpy (pOut, m_tmp.begin (), sizeof (int) * NewPatCount);

        const int NewCount = \
            FASortUniq (m_used_ids.begin (), m_used_ids.end ());
        m_used_ids.resize (NewCount);
    }
}


void FASelectTrPatterns::
    AddIwsOws (const int * pIws, const int * pOws, const int Count)
{
    DebugLogAssert (m_pDfa && m_pMealy && m_pK2I);
    DebugLogAssert (0 < Count && pIws && pOws);

    m_pos2ids_ends.Clear ();

    for (int From = 0; From < Count; ++From) {

        int Id = 0;
        int State = m_pDfa->GetInitial ();

        for (int i = From; i < Count; ++i) {

            const int Iw = pIws [i];
            int ow = 0;
            State = m_pMealy->GetDestOw (State, Iw, &ow);

            if (-1 == State)
                break;

            Id += ow;

            if (m_pDfa->IsFinal (State)) {
                // MPH and K2I inconsistency found
                FAAssert (0 <= Id && Id < m_pK2I->GetCount (), FAMsg::InternalError);
                m_pos2ids_ends.Add (i, Id);
            }

        } // of for (int i = From; ... 
    } // of for (int From = 0; ...

    // make set property
    m_pos2ids_ends.SortUniq ();

    CalcCover (pIws, pOws, Count);

    UpdateBest (pOws, Count);
}


void FASelectTrPatterns::Process ()
{
    DebugLogAssert (m_pK2I);

    const int MaxPatLen = FALimits::MaxWordSize + 2;
    m_tmp.resize (MaxPatLen);
    int * pPat = m_tmp.begin ();

    const int * pUsedIds = m_used_ids.begin ();
    const int UsedIdsCount = m_used_ids.size ();

    for (int i = 0; i < UsedIdsCount; ++i) {

        const int Id = pUsedIds [i];
        DebugLogAssert (0 <= Id && Id < m_pK2I->GetCount ());

        const int PatLen = m_mph_tools.GetChain (Id, pPat, MaxPatLen);
        FAAssert (0 < PatLen && PatLen <= MaxPatLen, FAMsg::InternalError);

        const int InfoId = m_pK2I->GetAt (Id);
        DebugLogAssert (0  <= InfoId);

        const int * pPatOws;
        const int OwsLen = m_pI2Info->Get (InfoId, &pPatOws) - 1;
        DebugLogAssert (0 < OwsLen && pPatOws);
        pPatOws++;

        FAAssert (OwsLen == PatLen, FAMsg::InternalError);

        PutPattern (pPat, pPatOws, PatLen);
    }
}


void FASelectTrPatterns::PutPattern (const int *, const int *, const int)
{}

void FASelectTrPatterns::PutUnsolved (const int *, const int *, const int)
{}

void FASelectTrPatterns::PutConflict (const int *, const int, const int)
{}


}
