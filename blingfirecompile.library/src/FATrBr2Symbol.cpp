/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATrBr2Symbol.h"
#include "FARegexpTree.h"
#include "FAUtils.h"
#include "FAException.h"
#include "FAFsmConst.h"

namespace BlingFire
{


const FAToken FATrBr2Symbol::m_left_br (FARegexpTree::TYPE_LBR, -1, 0);
const FAToken FATrBr2Symbol::m_right_br (FARegexpTree::TYPE_RBR, -1, 0);


FATrBr2Symbol::FATrBr2Symbol (FAAllocatorA * pAlloc) :
    m_pRegexp (NULL),
    m_Length (0),
    m_pTokens (NULL)
{
    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();

    m_tmp_tokens.SetAllocator (pAlloc);
    m_tmp_tokens.Create ();
}


void FATrBr2Symbol::SetRegexp (const char * pRegexp, const int Length)
{
    m_pRegexp = pRegexp;
    m_Length = Length;
}


void FATrBr2Symbol::SetTokens (FAArray_cont_t < FAToken > * pTokens)
{
    m_pTokens = pTokens;
}


void FATrBr2Symbol::Process ()
{
    DebugLogAssert (m_pTokens);
    DebugLogAssert (m_pRegexp && 0 < m_Length);

    m_tmp_tokens.resize (0);

    int i;
    const int Count = m_pTokens->size ();

    for (i = 0; i < Count; ++i) {

        const FAToken & token = (*m_pTokens) [i];
        const int Type = token.GetType ();

        // if openning triangular bracket
        if (FARegexpTree::TYPE_LTRBR == Type) {

            const int Offset = token.GetOffset ();
            const int Length = token.GetLength ();

            DebugLogAssert (0 <= Offset);
            DebugLogAssert (Offset + Length + 1 <= m_Length);

            int TrBr = 0;

            if (0 < Length) {
                TrBr = atoi (m_pRegexp + Offset + 1);
            }

            m_stack.push_back (TrBr);

            // add LBR, SYMBOL::<TrBr, FAFsmConst::TRBR_LEFT>, LBR tokens
            m_tmp_tokens.push_back (m_left_br);
            m_tmp_tokens.push_back ( \
              FAToken (FARegexpTree::TYPE_SYMBOL, TrBr, FAFsmConst::TRBR_LEFT));
            m_tmp_tokens.push_back (m_left_br);

        // if closing triangular bracket
        } else if (FARegexpTree::TYPE_RTRBR == Type) {

            if (0 == m_stack.size ()) {
                FASyntaxError (m_pRegexp, m_Length, -1, \
                    "Unbalanced triangular brackets");
                throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
            }

            const int TrBr = m_stack [m_stack.size () - 1];
            m_stack.pop_back ();

            // add RBR, SYMBOL::<TrBr, FAFsmConst::TRBR_RIGHT>, RBR tokens
            m_tmp_tokens.push_back (m_right_br);
            m_tmp_tokens.push_back ( \
              FAToken (FARegexpTree::TYPE_SYMBOL, TrBr, FAFsmConst::TRBR_RIGHT));
            m_tmp_tokens.push_back (m_right_br);

        } else {

            // push token as-is
            m_tmp_tokens.push_back (token);

        } // of if (FARegexpTree::TYPE_LTRBR == Type) ...

    } // of for (int i = 0; ...

    if (0 != m_stack.size ()) {
        FASyntaxError (m_pRegexp, m_Length, -1, \
            "Unbalanced triangular brackets");
        throw FAException (FAMsg::InternalError, __FILE__, __LINE__);
    }

    m_pTokens->resize (0);

    const int OutCount = m_tmp_tokens.size ();

    for (i = 0; i < OutCount; ++i) {

        const FAToken & t = m_tmp_tokens [i];
        m_pTokens->push_back (t);
    }
}

}

