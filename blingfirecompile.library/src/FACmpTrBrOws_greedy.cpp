/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACmpTrBrOws_greedy.h"
#include "FAMultiMapCA.h"

namespace BlingFire
{


FACmpTrBrOws_greedy::FACmpTrBrOws_greedy () :
    m_pOw2TrBrs (NULL)
{
}


void FACmpTrBrOws_greedy::SetTrBrMap (const FAMultiMapCA * pOw2TrBrs)
{
    m_pOw2TrBrs = pOw2TrBrs;
}


inline const int FACmpTrBrOws_greedy::
    TrBr2Val (const int TrBr)
{
    // -1
    if (-1 == TrBr) {

        return 0;
    // N>
    } else if (1 & TrBr) {

        return (TrBr >> 1) + 1;
    // <N
    } else {

        return -((TrBr >> 1) + 1);
    }
}


inline const int FACmpTrBrOws_greedy::
    CmpTrBr (const int TrBr1, const int TrBr2)
{
    const int Val1 = TrBr2Val (TrBr1);
    const int Val2 = TrBr2Val (TrBr2);

    return Val1 - Val2;
}


const bool FACmpTrBrOws_greedy::Less (const int Val1, const int Val2) const
{
    DebugLogAssert (m_pOw2TrBrs);

    if (Val1 == Val2) {
        return false;
    }

    const int * pTrBrs1 = NULL;
    int Size1 = 0;
    if (-1 != Val1) {
        Size1 = m_pOw2TrBrs->Get (Val1, &pTrBrs1);
    }

    const int * pTrBrs2 = NULL;
    int Size2 = 0;
    if (-1 != Val2) {
        Size2 = m_pOw2TrBrs->Get (Val2, &pTrBrs2);
    }

    int MaxSize = Size1;
    if (Size2 > Size1) {
        MaxSize = Size2;
    }

    for (int i = 0; i < MaxSize; ++i) {

        int TrBr1 = -1;
        if (Size1 > i) {
            DebugLogAssert (pTrBrs1);
            TrBr1 = pTrBrs1 [i];
        }

        int TrBr2 = -1;
        if (Size2 > i) {
            DebugLogAssert (pTrBrs2);
            TrBr2 = pTrBrs2 [i];
        }

        const int Cmp = CmpTrBr (TrBr1, TrBr2);
    
        if (0 > Cmp) {
            return true;
        } else if (0 < Cmp) {
            return false;
        }
    }

    return false;
}

}
