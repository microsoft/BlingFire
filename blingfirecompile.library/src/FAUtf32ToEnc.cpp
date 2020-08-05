/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAUtf32ToEnc.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"
#include "FAUtf16Utils.h"


#define FAIsUnicodeCp(cp) \
                ((cp == 1200) || \
                 (cp == 1201) || \
                 (cp == 12000) || \
                 (cp == 12001) || \
                 (cp == 65000) || \
                 (cp == 65001))

namespace BlingFire
{


FAUtf32ToEnc::FAUtf32ToEnc (FAAllocatorA * pAlloc) :
    m_is_utf16 (false),
    m_fBE (false),
    m_is_utf8 (false),
    m_cp (0)
{
    m_tmp_buff.SetAllocator (pAlloc);
    m_tmp_buff.Create (DefTmpBuffSize);
}


FAUtf32ToEnc::~FAUtf32ToEnc ()
{
}


void FAUtf32ToEnc::SetEncodingName (const char * pEncStr)
{
    DebugLogAssert (pEncStr);

    const bool fUtf16Be = FAIsUtf16BeEnc (pEncStr);
    const bool fUtf16Le = FAIsUtf16LeEnc (pEncStr);

    m_is_utf8 = FAIsUtf8Enc (pEncStr);

    m_is_utf16 = fUtf16Le || fUtf16Be;
    m_fBE = fUtf16Be;

    m_cp = FAEncoding2CodePage (pEncStr);

    if (0 == m_cp) {
        m_cp = atoi (pEncStr);
    }
}


const int FAUtf32ToEnc::Process (
        const int * pInStr,
        unsigned int InSize,
        __out_ecount(OutSize) char * pOutStr,
        const unsigned int MaxOutSize
    )
{
    DebugLogAssert (pInStr);
    DebugLogAssert (pOutStr);
    DebugLogAssert (m_cp);

    if (m_is_utf8) {

        if (0 == InSize)
            return 0;

        const int OutSize = \
            FAArrayToStrUtf8 (pInStr, InSize, pOutStr, MaxOutSize);

        if (OutSize > (int) MaxOutSize) {
            return -1;
        } else {
            return OutSize;
        }

    } else if (m_is_utf16) {

        if (0 == InSize)
            return 0;

        const unsigned int MaxOutWchars = MaxOutSize / sizeof (wchar_t);

        const int OutWchars = FAArrayToStrUtf16 (pInStr, InSize, \
            (wchar_t*) pOutStr, MaxOutWchars, m_fBE);

        if (-1 == OutWchars || OutWchars > (int) MaxOutWchars) {
            return -1;
        } else {
            return OutWchars * sizeof (wchar_t);
        }

    } else {

#ifndef BLING_FIRE_NOWINDOWS

        if (0 == InSize)
            return 0;

        /// make temporary buffer large enough
        m_tmp_buff.resize (2 * MaxOutSize);
        wchar_t * pTmpOut = m_tmp_buff.begin ();

        /// convert UTF-32LE into UTF-16LE
        const int OutWchars = FAArrayToStrUtf16 (pInStr, InSize, \
            (wchar_t*) pTmpOut, MaxOutSize, false);

        if (-1 == OutWchars || OutWchars > (int) MaxOutSize) {
            // happens only if input is not a UTF-32LE
            return -1;
        }

        BOOL UsedDefChar = FALSE;
        BOOL * pUsedDefChar = &UsedDefChar;
        DWORD dwFlags = WC_NO_BEST_FIT_CHARS;

        /// convert UTF-16LE into the specified CP
        const int OutSize = ::WideCharToMultiByte (m_cp, dwFlags, \
            pTmpOut, OutWchars, pOutStr, MaxOutSize, NULL, pUsedDefChar);

        /// check if the convertion is not possible
        if (0 >= OutSize || UsedDefChar) {
            return -1;
        }

        return OutSize;

#else

        return -1;

#endif

    } // of if (m_is_utf8) ...
}

}
