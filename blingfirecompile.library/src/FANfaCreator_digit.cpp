/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FANfaCreator_digit.h"

namespace BlingFire
{


FANfaCreator_digit::FANfaCreator_digit (FAAllocatorA * pAlloc) :
    FANfaCreator_base (pAlloc)
{}


void FANfaCreator_digit::SetTransition (const int FromState,
                                        const int ToState,
                                        const int LabelOffset,
                                        const int LabelLength)
{
    if (0 > LabelLength || 0 > LabelOffset) {
        FANfaCreator_base:: \
            SetTransition (FromState, ToState, LabelOffset, LabelLength);
        return;
    }

    DebugLogAssert (m_pRegexp);

    const int Iw = atoi (&m_pRegexp [LabelOffset]);

    m_tmp_nfa.SetTransition (FromState, Iw, ToState);
}

}
