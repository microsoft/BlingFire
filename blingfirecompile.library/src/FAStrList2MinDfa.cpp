/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAStrList2MinDfa.h"
#include "FAException.h"

namespace BlingFire
{


FAStrList2MinDfa::FAStrList2MinDfa (FAAllocatorA * pAlloc) :
    m_pStrList (NULL),
    m_Size (0),
    m_recode (pAlloc),
    m_chains2dfa (pAlloc)
{
    m_chain.SetAllocator (pAlloc);
    m_chain.Create ();
}


void FAStrList2MinDfa::SetStrList (const char * pStrList, const int Size)
{
    m_pStrList = pStrList;
    m_Size = Size;
}


void FAStrList2MinDfa::SetEncodingName (const char * pEncStr)
{
    m_recode.SetEncodingName (pEncStr);
}


void FAStrList2MinDfa::Process ()
{
    DebugLogAssert (0 < m_Size && m_pStrList);

    bool IsEmpty = true;

    const char * pStr = m_pStrList;
    const char * pBegin = m_pStrList;
    const char * const pEnd = m_pStrList + m_Size;

    while (pEnd > pBegin) {

        // str delimiter found
        if (0 == *pBegin) {

            const int StrLen = int (pBegin - pStr);

            // skip empty lines
            if (0 == StrLen) {
                pBegin++;
                pStr = pBegin;
                continue;
            }

            const int MaxChainSize = StrLen;
            m_chain.resize (MaxChainSize);
            int * pChain = m_chain.begin ();

            const int ActCount = \
                m_recode.Process (pStr, StrLen, pChain, MaxChainSize);

            // bad encoding
            FAAssert (0 < ActCount, FAMsg::SyntaxError);
            // buffer is too small -- fatal internal error
            FAAssert (ActCount <= MaxChainSize, FAMsg::InternalError);

            // add chain
            m_chains2dfa.AddChain (pChain, ActCount);
            IsEmpty = false;

            pBegin++;
            pStr = pBegin;

        } else {

            pBegin++;
        }

    } // of while (pEnd > pBegin) ...

    // no empty lists allowed
    FAAssert (false == IsEmpty, FAMsg::SyntaxError);

    // build DFA
    m_chains2dfa.Process ();
}


const FARSDfaA * FAStrList2MinDfa::GetRsDfa () const
{
    const FARSDfaA * pDfa = m_chains2dfa.GetRsDfa ();
    return pDfa;
}


}
