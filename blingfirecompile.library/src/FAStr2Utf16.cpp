/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAStr2Utf16.h"
#include "FAUtils.h"
#include "FAUtf8Utils.h"

namespace BlingFire
{


FAStr2Utf16::FAStr2Utf16 (FAAllocatorA * pAlloc) :
    m_is_utf8 (false),
#ifdef HAVE_ICONV_LIB
    m_conv_type (0)
#else
    m_cp (0)
#endif
{
    m_tmp_buff.SetAllocator (pAlloc);
    m_tmp_buff.Create (DefTmpBuffSize);
}


FAStr2Utf16::~FAStr2Utf16 ()
{
#ifdef HAVE_ICONV_LIB
    if (0 != m_conv_type) {
        iconv_close (m_conv_type);
        m_conv_type = 0;
    }
#endif
}


void FAStr2Utf16::SetEncodingName (const char * pEncStr)
{
    DebugLogAssert (pEncStr);

    m_is_utf8 = FAIsUtf8Enc (pEncStr);

#ifdef HAVE_ICONV_LIB
    if (0 != m_conv_type) {
        iconv_close (m_conv_type);
    }

    m_conv_type = iconv_open ("UTF-16LE", pEncStr);
#else
    m_cp = FAEncoding2CodePage (pEncStr);

    if (0 == m_cp) {
        m_cp = atoi (pEncStr);
    }
#endif
}


const int FAStr2Utf16::Process (
        const char * pInStr,
        unsigned int InSize, 
        __out_ecount(OutSize) int * pOutStr,
        const unsigned int OutSize
    )
{
    DebugLogAssert (pInStr);
    DebugLogAssert (pOutStr);

    if (m_is_utf8) {

        // check for Byte-Order-Mark (Utf-8 encoded U+FEFF symbol)
        if (3 <= InSize) {

            if (0xEF == (unsigned char) pInStr [0] && 
                0xBB == (unsigned char) pInStr [1] && 
                0xBF == (unsigned char) pInStr [2]) {
                pInStr += 3;
                InSize -= 3;
            }
        }

        if (0 == InSize)
            return 0;

        const int ActualCount = \
            FAStrUtf8ToArray (pInStr, InSize, pOutStr, OutSize);

        if (ActualCount > (int) OutSize)
            return -1;
        else
            return ActualCount;

    } else {

        if (0 == InSize)
            return 0;

        /// make temporary buffer large enough
        m_tmp_buff.resize (OutSize);

#ifdef HAVE_ICONV_LIB

        DebugLogAssert (m_conv_type);

        size_t InLeft = InSize;
        size_t OutLeft = OutSize * sizeof (wchar_t);
        char * pOutBuf = (char *) m_tmp_buff.begin ();

        const int Ret = \
            iconv (m_conv_type, (char **)&pInStr, &InLeft, &pOutBuf, &OutLeft);

        if (-1 == Ret)
            return -1;

        const unsigned short * pTmpOut = m_tmp_buff.begin ();

        const int ActualCount = \
            int ((unsigned short*) pOutBuf - m_tmp_buff.begin ());
#else

        DebugLogAssert (m_cp);

        wchar_t * pTmpOut = m_tmp_buff.begin ();

        const int ActualCount = \
            ::MultiByteToWideChar (m_cp, 0, pInStr, InSize, pTmpOut, OutSize);

        if (0 >= ActualCount)
            return -1;

#endif // HAVE_ICONV_LIB

        if ((unsigned int) ActualCount <= OutSize) {
            for (int i = 0; i < ActualCount; ++i) {
                pOutStr [i] = (unsigned int) pTmpOut [i];
            }
        }

        return ActualCount;

    } // of if (m_is_utf8) ...
}

}
