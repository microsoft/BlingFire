/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAStringTokenizer.h"

namespace BlingFire
{


const char * const FAStringTokenizer::DefSpaces = " \t\r\n";


FAStringTokenizer::FAStringTokenizer () :
    m_pStr (NULL),
    m_StrLen (-1),
    m_CurrPos (-1),
    m_pSpaces (DefSpaces)
{}


void FAStringTokenizer::SetSpaces (const char * pSpaces)
{
    if (NULL != pSpaces)
        m_pSpaces = pSpaces;
    else
        m_pSpaces = DefSpaces;
}


void FAStringTokenizer::SetString (const char * pStr, const int StrLen)
{
    m_pStr = pStr;
    m_StrLen = StrLen;
    m_CurrPos = 0;
}


inline const bool FAStringTokenizer::
    IsSpace (const unsigned char Symbol) const
{
    DebugLogAssert (m_pSpaces);
    const unsigned char * pSpace = (const unsigned char *) m_pSpaces;

    while (0 != *pSpace) {
        if (Symbol == *pSpace++)
            return true;
    }

    return false;
}


const bool FAStringTokenizer::
    GetNextStr (const char ** ppStrToken, int * pTokenLen)
{
    DebugLogAssert (ppStrToken && pTokenLen);

    if (m_CurrPos >= m_StrLen)
        return false;

    const unsigned char C = m_pStr [m_CurrPos];

    // eat the leading spaces if needed
    if (IsSpace (C))
        m_CurrPos += ((const int) strspn (m_pStr + m_CurrPos, m_pSpaces));

    // nothing to do here
    if (m_CurrPos >= m_StrLen)
        return false;

    // find the end of the string
    const int StrTokenLen = \
        (const int) strcspn (m_pStr + m_CurrPos, m_pSpaces);

    *ppStrToken = m_pStr + m_CurrPos;
    *pTokenLen = StrTokenLen;

    m_CurrPos += StrTokenLen;

    return true;
}


const bool FAStringTokenizer::GetNextInt (int * pIntToken, const int Base)
{
    DebugLogAssert (pIntToken);

    if (m_CurrPos >= m_StrLen)
        return false;

    const unsigned char C = m_pStr [m_CurrPos];

    // eat the leading spaces if needed
    if (IsSpace (C))
        m_CurrPos += ((const int) strspn (m_pStr + m_CurrPos, m_pSpaces));

    // nothing to do here
    if (m_CurrPos >= m_StrLen)
        return false;

    // find the end of the integer string
    const int StrTokenLen = \
        ((const int) strcspn (m_pStr + m_CurrPos, m_pSpaces));

    // convert string to int
    *pIntToken = strtol (m_pStr + m_CurrPos, NULL, Base);

    m_CurrPos += StrTokenLen;

    return true;
}


const bool FAStringTokenizer::GetArray (
        __out_ecount(ArraySize) int * pArray, 
        const int ArraySize
    )
{
    if (0 == ArraySize)
        return true;

    DebugLogAssert (0 < ArraySize && pArray);

    if (m_CurrPos >= m_StrLen)
        return false;

    for (int i = 0; i < ArraySize; ++i) {

        int IntToken = -1;

        if (false == GetNextInt (&IntToken))
            return false;

        pArray [i] = IntToken;
    }

    return true;
}


const int FAStringTokenizer::GetArray2 (
        __out_ecount(ArrayMaxSize) int * pArray,
        const int ArrayMaxSize
    )
{
    if (m_CurrPos >= m_StrLen)
        return 0;

    int * pOutPtr = pArray;
    int IntToken = -1;
    int count = 0;

    while (GetNextInt (&IntToken) && count < ArrayMaxSize) {

        DebugLogAssert (pOutPtr);
        *pOutPtr = IntToken;
        pOutPtr++;
        count++;
    }

    return count;
}

}
