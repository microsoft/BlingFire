/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATestCmpPosNfa.h"
#include "FARSNfaCA.h"
#include "FAMultiMapCA.h"

namespace BlingFire
{


FATestCmpPosNfa::FATestCmpPosNfa (FAAllocatorA * pAlloc) :
    m_pNfa1 (NULL),
    m_pNfa2 (NULL),
    m_pState2BegTrBr1 (NULL),
    m_pState2BegTrBr2 (NULL),
    m_pState2EndTrBr1 (NULL),
    m_pState2EndTrBr2 (NULL),
    m_MaxTrBrBegSize (0),
    m_MaxTrBrEndSize (0)
{
    m_tmp_arr1.SetAllocator (pAlloc);
    m_tmp_arr1.Create ();

    m_tmp_arr2.SetAllocator (pAlloc);
    m_tmp_arr2.Create ();
}


void FATestCmpPosNfa::SetFsm1 (const FARSNfaCA * pNfa1, 
                               const FAMultiMapCA * pState2BegTrBr1,
                               const FAMultiMapCA * pState2EndTrBr1)
{
    m_pNfa1 = pNfa1;
    m_pState2BegTrBr1 = pState2BegTrBr1;
    m_pState2EndTrBr1 = pState2EndTrBr1;
}


void FATestCmpPosNfa::SetFsm2 (const FARSNfaCA * pNfa2, 
                               const FAMultiMapCA * pState2BegTrBr2,
                               const FAMultiMapCA * pState2EndTrBr2)
{
    m_pNfa2 = pNfa2;
    m_pState2BegTrBr2 = pState2BegTrBr2;
    m_pState2EndTrBr2 = pState2EndTrBr2;
}


const bool FATestCmpPosNfa::CheckMaxTrBrSize ()
{
    m_MaxTrBrBegSize = 0;
    m_MaxTrBrEndSize = 0;

    if (m_pState2BegTrBr1 && m_pState2BegTrBr2) {

        const int MaxCount1 = m_pState2BegTrBr1->GetMaxCount ();
        const int MaxCount2 = m_pState2BegTrBr2->GetMaxCount ();

        if (MaxCount1 != MaxCount2) {

            // FAMultiMapCA::GetMaxCount (TrBrBeg) are different
            DebugLogAssert (0);
            return false;

        } else {

            m_MaxTrBrBegSize = MaxCount1;
        }
    }
    if (m_pState2EndTrBr1 && m_pState2EndTrBr2) {

        const int MaxCount1 = m_pState2EndTrBr1->GetMaxCount ();
        const int MaxCount2 = m_pState2EndTrBr2->GetMaxCount ();

        if (MaxCount1 != MaxCount2) {

            // FAMultiMapCA::GetMaxCount (TrBrEnd) are different
            DebugLogAssert (0);
            return false;

        } else {

            m_MaxTrBrEndSize = MaxCount1;
        }
    }

    return true;
}


const bool FATestCmpPosNfa::CheckInitials () const
{
    // check initial states
    const int * pIniStates1;
    const int IniCount1 = m_pNfa1->GetInitials (&pIniStates1);

    const int * pIniStates2;
    const int IniCount2 = m_pNfa2->GetInitials (&pIniStates2);

    if (IniCount1 != IniCount2) {
        // FARSNfaCA::GetInitials are different
        DebugLogAssert (0);
        return false;
    }
    if (0 < IniCount1 && 
        0 != memcmp (pIniStates1, pIniStates2, sizeof (int) * IniCount1)) {
        // FARSNfaCA::GetInitials are different
        DebugLogAssert (0);
        return false;
    }

    return true;
}


const bool FATestCmpPosNfa::CheckState2BegTrBr (const int State)
{
    // check whether beginning triangular brackets are the same
    if (m_pState2BegTrBr1 && m_pState2BegTrBr2) {

        // get size
        int BegTrBrCount1 = m_pState2BegTrBr1->Get (State, NULL, 0);
        int BegTrBrCount2 = m_pState2BegTrBr2->Get (State, NULL, 0);

        if (0 == BegTrBrCount1)
            BegTrBrCount1 = -1;
        if (0 == BegTrBrCount2)
            BegTrBrCount2 = -1;

        if (BegTrBrCount1 != BegTrBrCount2 || m_MaxTrBrBegSize < BegTrBrCount1) {
            // FAMultiMapCA::Get (TrBrBeg) are different
            DebugLogAssert (0);
            return false;
        }
        if (0 < BegTrBrCount1) {

            m_tmp_arr1.resize (BegTrBrCount1);
            int * pArr1 = m_tmp_arr1.begin ();
            m_tmp_arr2.resize (BegTrBrCount1);
            int * pArr2 = m_tmp_arr2.begin ();

            m_pState2BegTrBr1->Get (State, pArr1, BegTrBrCount1);
            m_pState2BegTrBr2->Get (State, pArr2, BegTrBrCount1);

            if (0 != memcmp (pArr1, pArr2, sizeof (int) * BegTrBrCount1)) {
                // FAMultiMapCA::Get (TrBrBeg) are different
                DebugLogAssert (0);
                return false;
            }
        }
    } // of if (m_pState2BegTrBr1 && m_pState2BegTrBr2)

    return true;
}


const bool FATestCmpPosNfa::CheckState2EndTrBr (const int State)
{
    // check whether beginning triangular brackets are the same
    if (m_pState2EndTrBr1 && m_pState2EndTrBr2) {

        // get size
        int EndTrBrCount1 = m_pState2EndTrBr1->Get (State, NULL, 0);
        int EndTrBrCount2 = m_pState2EndTrBr2->Get (State, NULL, 0);

        if (0 == EndTrBrCount1)
            EndTrBrCount1 = -1;
        if (0 == EndTrBrCount2)
            EndTrBrCount2 = -1;

        if (EndTrBrCount1 != EndTrBrCount2 || m_MaxTrBrEndSize < EndTrBrCount1) {
            // FAMultiMapCA::Get (TrBrEnd) are different
            DebugLogAssert (0);
            return false;
        }
        if (0 < EndTrBrCount1) {

            m_tmp_arr1.resize (EndTrBrCount1);
            int * pArr1 = m_tmp_arr1.begin ();
            m_tmp_arr2.resize (EndTrBrCount1);
            int * pArr2 = m_tmp_arr2.begin ();

            m_pState2EndTrBr1->Get (State, pArr1, EndTrBrCount1);
            m_pState2EndTrBr2->Get (State, pArr2, EndTrBrCount1);

            if (0 != memcmp (pArr1, pArr2, sizeof (int) * EndTrBrCount1)) {
                // FAMultiMapCA::Get (TrBrEnd) are different
                DebugLogAssert (0);
                return false;
            }
        }
    } // of if (m_pState2EndTrBr1 && m_pState2EndTrBr2)

    return true;
}


const bool FATestCmpPosNfa::CheckDest (const int State, const int Iw)
{
    DebugLogAssert (m_pNfa1 && m_pNfa2);

    const int DestCount1 = m_pNfa1->GetDest (State, Iw, NULL, 0);
    const int DestCount2 = m_pNfa2->GetDest (State, Iw, NULL, 0);

    if (DestCount1 != DestCount2) {
        // FARSNfaCA::GetDest are different
        DebugLogAssert (0);
        return false;
    }
    if (0 < DestCount1) {

        m_tmp_arr1.resize (DestCount1);
        int * pArr1 = m_tmp_arr1.begin ();
        m_tmp_arr2.resize (DestCount1);
        int * pArr2 = m_tmp_arr2.begin ();

        m_pNfa1->GetDest (State, Iw, pArr1, DestCount1);
        m_pNfa2->GetDest (State, Iw, pArr2, DestCount1);

        if (0 != memcmp (pArr1, pArr2, sizeof (int) * DestCount1)) {
            // FARSNfaCA::GetDest are different
            DebugLogAssert (0);
            return false;
        }
    }

    return true;
}


const bool FATestCmpPosNfa::Process (const int MaxState, const int MaxIw)
{
    DebugLogAssert (m_pNfa1 && m_pNfa2);

    if (false == CheckInitials ()) {
        return false;
    }

    if (false == CheckMaxTrBrSize ()) {
        return false;
    }

    for (int State = 0; State <= MaxState; ++State) {

        if (false == CheckState2BegTrBr (State)) {
            return false;
        }
        if (false == CheckState2EndTrBr (State)) {
            return false;
        }

        for (int Iw = 0; Iw <= MaxIw; ++Iw) {

            if (false == CheckDest (State, Iw)) {
                return false;
            }

        } // of for (int Iw = 0; ...

    } // of for (int State = 0; ...

    return true;
}

}

