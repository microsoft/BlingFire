/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FALimits.h"
#include "FADictStr2SuffixRule.h"
#include "FAUtf32Utils.h"
#include "FAUtils.h"
#include "FAException.h"

#include <sstream>
#include <string>

namespace BlingFire
{


FADictStr2SuffixRule::
    FADictStr2SuffixRule (FAAllocatorA * pAlloc) :
        m_pInTr (NULL),
        m_pOutTr (NULL),
        m_IgnoreCase (false),
        m_UsePref (false),
        m_pFrom (NULL),
        m_pTo (NULL),
        m_pFromTag (NULL),
        m_pToTag (NULL),
        m_FromLen (0),
        m_ToLen (0),
        m_FromTagLen (0),
        m_ToTagLen (0),
        m_SuffPos (0),
        m_SuffLen (0),
        m_SuffCut (0),
        m_PrefPos (0),
        m_PrefLen (0),
        m_PrefCut (0),
        m_pCharMap (NULL)
{
    m_out_buff.SetAllocator (pAlloc);
    m_out_buff.Create ();

    m_tmp_from.SetAllocator (pAlloc);
    m_tmp_from.Create (FALimits::MaxWordLen * 2);
    m_tmp_from.resize (FALimits::MaxWordLen * 2);

    m_tmp_to.SetAllocator (pAlloc);
    m_tmp_to.Create (FALimits::MaxWordLen * 2);
    m_tmp_to.resize (FALimits::MaxWordLen * 2);
}


void FADictStr2SuffixRule::
    SetInTr (const FATransformCA_t < int > * pInTr)
{
    m_pInTr = pInTr;
}


void FADictStr2SuffixRule::
    SetOutTr (const FATransformCA_t < int > * pOutTr)
{
    m_pOutTr = pOutTr;
}


void FADictStr2SuffixRule::SetIgnoreCase (const bool IgnoreCase)
{
    m_IgnoreCase = IgnoreCase;
}


void FADictStr2SuffixRule::SetCharmap (const FAMultiMapCA * pCharMap)
{
    m_pCharMap = pCharMap;
}


void FADictStr2SuffixRule::SetUsePref (const bool UsePref)
{
    m_UsePref = UsePref;
}


inline void FADictStr2SuffixRule::
    Split (const int * pStr, const int StrLen)
{
    DebugLogAssert (pStr && 0 < StrLen);

    m_pFrom = pStr;
    m_pTo = NULL;
    m_FromLen = 0;
    m_ToLen = 0;
    m_pFromTag = NULL;
    m_pToTag = NULL;
    m_FromTagLen = 0;
    m_ToTagLen = 0;

    int FromPos = 0;
    int ToPos = -1;
    int FromTagPos = -1;
    int ToTagPos = -1;

    for (int i = 0; i < StrLen; ++i) {

        const int Symbol = pStr [i];

        if ((unsigned int) '\t' == (unsigned int) Symbol) {

            if (-1 == ToPos) {

                m_FromLen = i - FromPos;
                ToPos = i + 1;
                m_ToLen = StrLen - ToPos;

            } else if (-1 == FromTagPos) {

                m_ToLen = i - ToPos;
                FromTagPos = i + 1;
                m_FromTagLen = StrLen - FromTagPos;

            } else if (-1 == ToTagPos) {

                m_FromTagLen = i - FromTagPos;
                ToTagPos = i + 1;
                m_ToTagLen = StrLen - ToTagPos;

                break;
            }
        }
    } // of for (int i = 0; ...

    if (-1 != ToPos) {
        m_pTo = pStr + ToPos;
    }
    if (-1 != FromTagPos) {
        m_pFromTag = pStr + FromTagPos;
    }
    if (-1 != ToTagPos) {
        m_pToTag = pStr + ToTagPos;
    }
}


inline void FADictStr2SuffixRule::
    ApplyInTr ()
{
    DebugLogAssert (m_pInTr);

    int * pTmpFrom = m_tmp_from.begin ();
    const int TmpFromSize = m_tmp_from.size ();

    const int NewSize = \
        m_pInTr->Process (m_pFrom, m_FromLen, pTmpFrom, TmpFromSize);

    if (NewSize > TmpFromSize) {

        m_tmp_from.resize (NewSize + 10);

        pTmpFrom = m_tmp_from.begin ();
        m_pInTr->Process (m_pFrom, m_FromLen, pTmpFrom, NewSize);
    }

    if (0 < NewSize) {

        m_pFrom = pTmpFrom;
        m_FromLen = NewSize;
    }
}


inline void FADictStr2SuffixRule::
    ApplyOutTr ()
{
    DebugLogAssert (m_pOutTr);

    int * pTmpTo = m_tmp_to.begin ();
    const int TmpToSize = m_tmp_to.size ();

    const int NewSize = \
        m_pOutTr->Process (m_pTo, m_ToLen, pTmpTo, TmpToSize);

    if (NewSize > TmpToSize) {

        m_tmp_to.resize (NewSize + 10);

        pTmpTo = m_tmp_to.begin ();
        m_pOutTr->Process (m_pTo, m_ToLen, pTmpTo, NewSize);
    }

    if (0 < NewSize) {

        m_pTo = pTmpTo;
        m_ToLen = NewSize;
    }
}


inline void FADictStr2SuffixRule::ToLower ()
{
    /// case-fold the destination
    int * pTmpTo = m_tmp_to.begin ();

    if (pTmpTo == m_pTo) {

        FAUtf32StrLower (pTmpTo, m_ToLen);

    } else {

        DebugLogAssert (m_tmp_to.size () > (unsigned int) m_ToLen);

        for (int i = 0; i < m_ToLen; ++i) {
            const int Symbol = m_pTo [i];
            pTmpTo [i] = FAUtf32ToLower (Symbol);
        }
        m_pTo = pTmpTo;
    }

    /// case-fold the source
    int * pTmpFrom = m_tmp_from.begin ();

    if (pTmpFrom == m_pFrom) {

        FAUtf32StrLower (pTmpFrom, m_FromLen);

    } else {

        DebugLogAssert (m_tmp_from.size () > (unsigned int) m_FromLen);

        for (int i = 0; i < m_FromLen; ++i) {
            const int Symbol = m_pFrom [i];
            pTmpFrom [i] = FAUtf32ToLower (Symbol);
        }
        m_pFrom = pTmpFrom;
    }
}


inline void FADictStr2SuffixRule::MapChars ()
{
    DebugLogAssert (m_pCharMap);

    int * pTmpTo = m_tmp_to.begin ();

    // in-place allowed
    const int NewLen1 = FANormalizeWord (m_pTo, m_ToLen, \
        pTmpTo, m_tmp_to.size (), m_pCharMap);

    // normalization results into an empty string
    FAAssert (0 < NewLen1 || 0 == m_ToLen, FAMsg::InternalError);
    m_ToLen = NewLen1;
    m_pTo = pTmpTo;

    int * pTmpFrom = m_tmp_from.begin ();

    // in-place allowed
    const int NewLen2 = FANormalizeWord (m_pFrom, m_FromLen, \
        pTmpFrom, m_tmp_from.size (), m_pCharMap);

    // normalization results into an empty string
    FAAssert (0 < NewLen2 || 0 == m_FromLen, FAMsg::InternalError);
    m_FromLen = NewLen2;
    m_pFrom = pTmpFrom;
}


inline void FADictStr2SuffixRule::CalcSuffPref ()
{
    DebugLogAssert (FALimits::MaxWordSize >= m_FromLen && \
            FALimits::MaxWordSize >= m_ToLen);

    // reset coordinates
    m_SuffPos = 0;
    m_SuffLen = m_ToLen;
    m_SuffCut = m_FromLen;
    m_PrefPos = 0;
    m_PrefLen = 0;
    m_PrefCut = 0;

    if (false == m_UsePref) {

        int MinLen = m_ToLen;
        if (MinLen > m_FromLen)
            MinLen = m_FromLen;

        for (; m_SuffPos < MinLen; ++m_SuffPos) {
            if (m_pFrom [m_SuffPos] != m_pTo [m_SuffPos])
                break;
        }
        m_SuffLen = m_ToLen - m_SuffPos;
        m_SuffCut = m_FromLen - m_SuffPos;

    } else {

        /// solve a maximum common substring problem using DP
        m_out_buff.resize ((m_FromLen + 1) * (m_ToLen + 1));
        int * pLCS = m_out_buff.begin ();

        int i, j;
        int CommonLen = 0;
        int CommonI = -1;
        int CommonJ = -1;

        pLCS [0] = 0;

        for (i = 1; i <= m_FromLen; ++i) {
            pLCS [i] = 0;
        }
        for (j = 1; j <= m_ToLen; ++j) {
            pLCS [(m_FromLen + 1) * j] = 0;
        }
        for (i = 1; i <= m_FromLen; ++i) {
            for (j = 1; j <= m_ToLen; ++j) {

                int * pScore = & pLCS [((m_FromLen + 1) * j) + i];

                if (m_pFrom [i - 1] == m_pTo [j - 1]) {
                    *pScore = pLCS [((m_FromLen + 1) * (j - 1)) + i - 1] + 1;
                } else {
                    *pScore = 0;
                }
                if (CommonLen < *pScore) {
                    CommonLen = *pScore;
                    CommonI = i - *pScore;
                    CommonJ = j - *pScore;
                }
            }
        }
        // if common substring exists and is greater than the limit
        if (1 < CommonLen) {

            m_SuffPos = CommonJ + CommonLen;
            m_SuffLen = m_ToLen - m_SuffPos;
            m_SuffCut = m_FromLen - (CommonI + CommonLen);

            m_PrefPos = 0;
            m_PrefLen = CommonJ;
            m_PrefCut = CommonI;
        }
    } // of if (false == m_UsePref) ...
}


inline void FADictStr2SuffixRule::BuildOutput ()
{
    FAAssert (m_pFrom && 0 < m_FromLen, FAMsg::InternalError);
    FAAssert (0 <= m_SuffCut && 0 <= m_SuffLen && 0 <= m_SuffPos, \
        FAMsg::InternalError);
    FAAssert (0 <= m_PrefCut && 0 <= m_PrefLen && 0 <= m_PrefPos, \
        FAMsg::InternalError);

    // build "m_SuffCut" string
    std::ostringstream NumOs;
    NumOs << m_SuffCut;
    const std::string & NumStr = NumOs.str ();
    const char * pStripLenStr = NumStr.c_str ();
    const int StripLenStrLen = int (NumStr.length ());
    DebugLogAssert (pStripLenStr && 0 < StripLenStrLen);

    // build "m_PrefCut" string
    std::ostringstream NumOs2;
    NumOs2 << m_PrefCut;
    const std::string & NumStr2 = NumOs2.str ();
    const char * pStripLenStr2 = NumStr2.c_str ();
    const int StripLenStrLen2 = int (NumStr2.length ());
    DebugLogAssert (pStripLenStr2 && 0 < StripLenStrLen2);

    // output size
    int OutSize = m_FromLen + StripLenStrLen + 6;

    if (m_pFromTag) {
        DebugLogAssert (0 < m_FromTagLen);
        OutSize += (2 + m_FromTagLen);
    }
    if (m_pToTag) {
        DebugLogAssert (0 < m_ToTagLen);
        OutSize += (2 + m_ToTagLen);
    }
    if (0 < m_SuffLen) {
        OutSize += (1 + m_SuffLen);
    }
    if (0 < m_PrefLen || 0 < m_PrefCut) {
        OutSize += (2 + StripLenStrLen2);
        if (0 < m_PrefLen) {
            OutSize += (1 + m_PrefLen);
        }
    }

    m_out_buff.resize (OutSize);
    int * pOut = m_out_buff.begin ();

    // copy from_word
    memcpy (pOut, m_pFrom, sizeof (int) * m_FromLen);
    pOut += m_FromLen;

    // copy " --> "
    *pOut++ = (unsigned int) ' ';
    *pOut++ = (unsigned int) '-';
    *pOut++ = (unsigned int) '-';
    *pOut++ = (unsigned int) '>';
    *pOut++ = (unsigned int) ' ';

    // copy tags, if any
    if (m_pFromTag) {
        *pOut++ = (unsigned int) '[';
        memcpy (pOut, m_pFromTag, sizeof (int) * m_FromTagLen);
        pOut += m_FromTagLen;
        *pOut++ = (unsigned int) ']';
    }
    if (m_pToTag) {
        *pOut++ = (unsigned int) '[';
        memcpy (pOut, m_pToTag, sizeof (int) * m_ToTagLen);
        pOut += m_ToTagLen;
        *pOut++ = (unsigned int) ']';
    }
    // copy "-m_SuffCut";
    *pOut++ = (unsigned int) '-';
    for (int i = 0; i < StripLenStrLen; ++i) {
        *pOut++ = (unsigned int) pStripLenStr [i];
    }
    // copy "+Suffix"
    if (0 < m_SuffLen) {
        *pOut++ = (unsigned int) '+';
        memcpy (pOut, m_pTo + m_SuffPos, sizeof (int) * m_SuffLen);
        pOut += m_SuffLen;
    }

    // copy "\t-m_PrefCut+Prefix"
    if (0 < m_PrefLen || 0 < m_PrefCut) {
        *pOut++ = (unsigned int) '\t';
        *pOut++ = (unsigned int) '-';
        for (int i = 0; i < StripLenStrLen2; ++i) {
            *pOut++ = (unsigned int) pStripLenStr2 [i];
        }
        if (0 < m_PrefLen) {
            *pOut++ = (unsigned int) '+';
            memcpy (pOut, m_pTo + m_PrefPos, sizeof (int) * m_PrefLen);
        }
    }
}


const int FADictStr2SuffixRule::
    Process (const int * pStr, const int StrLen, const int ** pOutStr)
{
    DebugLogAssert (pOutStr);
    DebugLogAssert (pStr && 0 < StrLen);

    m_out_buff.resize (0);

    Split (pStr, StrLen);

    if (!m_pTo || FALimits::MaxWordLen < m_FromLen || \
         FALimits::MaxWordLen < m_ToLen) {
        return -1;
    }

    if (m_IgnoreCase) {
        ToLower ();
    }
    if (m_pCharMap) {
        MapChars ();
    }
    if (m_pInTr) {
        ApplyInTr ();
    }
    if (m_pOutTr) {
        ApplyOutTr ();
    }

    CalcSuffPref ();

    BuildOutput ();

    *pOutStr = m_out_buff.begin ();
    return m_out_buff.size ();
}

}

