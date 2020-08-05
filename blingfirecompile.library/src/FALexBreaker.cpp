/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FALexBreaker.h"
#include "FAUtf8Utils.h"
#include "FAUtils_cl.h"

namespace BlingFire
{


FALexBreaker::FALexBreaker() :
    m_fInitialized (false), 
    m_fUseBytes (false),
    m_iXWordTag (0), 
    m_iSegTag (0), 
    m_iIgnoreTag (0),
    m_pbUTF8Text (NULL), 
    m_cbUTF8Text (0), 
    m_pTagSet (NULL),
    m_pOutText (NULL), 
    m_pOutNormText (NULL), 
    m_pNorm (NULL), 
    m_uUtf8Offset (0), 
    m_uTxtOffset (0),
    m_uTxtLen (0), 
    m_iTokensSize (0),
    m_uMaxCharsAtOnce (0),
    m_uMaxTokensSize (0)
{
}

FALexBreaker::~FALexBreaker ()
{
    Clear ();
}

void FALexBreaker::Clear ()
{
    m_fInitialized = false;
    m_fUseBytes = false;
    m_iXWordTag = 0;
    m_iSegTag = 0;
    m_iIgnoreTag = 0;
    m_pbUTF8Text = NULL;
    m_cbUTF8Text = 0;
    m_pTagSet = NULL;
    m_pOutText = NULL;
    m_pOutNormText = NULL;
    m_pNorm = NULL;
    m_uUtf8Offset = 0;
    m_uTxtOffset = 0;
    m_uTxtLen = 0;
    m_iTokensSize = 0;
    m_uMaxCharsAtOnce = 0;
    m_uMaxTokensSize = 0;
}

bool FALexBreaker::Initialize (const FAWbdConfKeeper * pConfig)
{
    if (m_fInitialized)
    {
        return false;
    }

    /// bad input parameters
    LogAssert(pConfig);

    /// return into the initial state
    FALexBreaker::Clear ();

    /// setup WBD configuration
    m_wbd.SetConf (pConfig);

    // get tag values code should know about
    m_iXWordTag = pConfig->GetWbdTagXWord ();
    m_iSegTag = pConfig->GetWbdTagSeg ();
    m_iIgnoreTag = pConfig->GetWbdTagIgnore ();

    // get normalization map pointer, can be NULL
    m_pNorm = pConfig->GetCharMap ();

    /// last check, fails now or never
    int Input [1] = { 0x20 };
    int Output [3];
    m_wbd.Process (Input, 1, Output, 3);

    m_fInitialized = true;

    return true;
}


void FALexBreaker::SetOutText (FAArray_cont_t < char > * pOutText)
{
    m_pOutText = pOutText;

}


void FALexBreaker::SetOutNormText (FAArray_cont_t < char > * pOutNormText)
{
    m_pOutNormText = pOutNormText;
}


void FALexBreaker::SetTagSet (const FATagSet * pTagSet)
{
    m_pTagSet = pTagSet;
}


void FALexBreaker::SetUseBytes (const bool fUseBytes)
{
    m_fUseBytes = fUseBytes;
}


void FALexBreaker::DeInitialize ()
{
    Clear ();
}


bool FALexBreaker::BreakText (const UInt8 *pbUTF8Text, size_t cbUTF8Text)
{
    LogAssert (m_fInitialized);
    LogAssert (pbUTF8Text || 0 == cbUTF8Text);
    LogAssert (cbUTF8Text < INT_MAX);

    if (0 == cbUTF8Text)
    {
        return true;
    }

    m_uMaxCharsAtOnce = (unsigned) cbUTF8Text;

    if (MaxBuffSize < m_uMaxCharsAtOnce)
    {
        m_uMaxCharsAtOnce = MaxBuffSize;
    }

    m_uMaxTokensSize = 3 * m_uMaxCharsAtOnce;

    m_pbUTF8Text = pbUTF8Text;
    m_cbUTF8Text = cbUTF8Text;

    return BreakText_int (pbUTF8Text, cbUTF8Text);
}


bool FALexBreaker::BreakText_int (const UInt8 *pbUTF8Text, size_t cbUTF8Text)
{
    m_uUtf8Offset = 0;
    size_t FromPos = 0;
    size_t LastToPos = FromPos;

    if (m_uMaxCharsAtOnce < cbUTF8Text)
    {
        for (size_t ToPos = 0; ToPos < cbUTF8Text; ++ToPos)
        {
            // get the text length
            size_t TxtLen = ToPos - FromPos + 1;

            // word-break the text from the FromPos to LastToPos, because
            // current TxtLen is already too big to fit the buffer
            if (m_uMaxCharsAtOnce < TxtLen)
            {
                // no LastToPos was ever found, just take next m_uMaxCharsAtOnce
                if (LastToPos == FromPos) 
                {
                    LastToPos = FromPos + m_uMaxCharsAtOnce - 1;
                }

                // get current substring
                const UInt8 * pbSubStr = pbUTF8Text + FromPos;
                const size_t cbSubStrLen = LastToPos - FromPos + 1;

                m_uUtf8Offset = FromPos;
                FromPos = LastToPos + 1;
                LastToPos = FromPos;

                int Count = -1;

                if (false == m_fUseBytes)
                {
                    // UTF-8 --> UTF-32 with offsets
                    Count = FAStrUtf8ToArray ((const char*)pbSubStr, (int)cbSubStrLen, m_pTxt, m_pOffsets, m_uMaxCharsAtOnce);
                }
                else
                {
                    // copy the bytes as is
                    for (size_t i = 0; i < cbSubStrLen; ++i)
                    {
                        DebugLogAssert (i < m_uMaxCharsAtOnce);

                        m_pTxt [i] = pbSubStr [i];
                        m_pOffsets [i] = (int) i;
                    }

                    Count = (int) cbSubStrLen;
                }

                if (0 > Count)
                {
                    return false;
                }

                DebugLogAssert (m_uMaxCharsAtOnce >= (unsigned) Count);

                m_uTxtLen = Count;

                // process a buffer of UTF-32LE text
                ProcessTxt ();

            }            
            // check for a good stop place
            else if (0x20 >= pbUTF8Text [ToPos])
            {
                // remember biggest ToPos
                LastToPos = ToPos;
            }
        } 
    }
    if (FromPos < cbUTF8Text)
    {
        m_uUtf8Offset = FromPos;

        // get current substring
        const UInt8 * pbSubStr = pbUTF8Text + FromPos;
        const size_t cbSubStrLen = cbUTF8Text - FromPos;

        // UTF-8 --> UTF-32 with offsets
        int Count = -1;

        if (false == m_fUseBytes)
        {
            // UTF-8 --> UTF-32 with offsets
            Count = FAStrUtf8ToArray ((const char*)pbSubStr, (int)cbSubStrLen, m_pTxt, m_pOffsets, m_uMaxCharsAtOnce);
        }
        else
        {
            // copy the bytes as is
            for (size_t i = 0; i < cbSubStrLen; ++i)
            {
                DebugLogAssert (i < m_uMaxCharsAtOnce);

                m_pTxt [i] = pbSubStr [i];
                m_pOffsets [i] = (int) i;
            }

            Count = (int) cbSubStrLen;
        }

        if (0 < Count) 
        {
            m_uTxtLen = Count;

            // process a buffer of UTF-32LE text
            ProcessTxt ();
        }
        else
        {       
            return false;
        }
    } // of if (FromPos < cbUTF8Text) ...

    return true;
}

void FALexBreaker::ProcessTxt ()
{
    m_uTxtOffset = 0;

    m_iTokensSize = m_wbd.Process (m_pTxt, m_uTxtLen, m_pTokens, m_uMaxTokensSize);

    while (0 < m_iTokensSize) 
    {
        // return words to the client
        ProcessTokens ();

        // see if we have more text to process
        if ((unsigned) m_iTokensSize < m_uMaxTokensSize)
        {
            break;
        }
        
        // see if we have more text to process
        const int LastTo = m_pTokens [m_iTokensSize - 1]; // [Tag, From, To]
        if ((unsigned) LastTo + 1 >= m_uTxtLen) 
        {
            break;
        }

        // adjust Offset2
        m_uTxtOffset += (LastTo + 1);
        
        // tokenize the rest
        m_iTokensSize = m_wbd.Process (m_pTxt + m_uTxtOffset, m_uTxtLen - m_uTxtOffset, m_pTokens, m_uMaxTokensSize);
    }
}

void FALexBreaker::ProcessTokens () const
{
    DebugLogAssert (0 == m_iTokensSize % 3);

    int NormWord [MaxWordLen];
    int NormWordLen = 0;
    int CxTokenFrom = -1;
    int CxTokenTo = -1;

    for (int i = 0; i < m_iTokensSize; i += 3) 
    {
        const int Tag = m_pTokens [i];

        if (Tag == m_iIgnoreTag) 
        {
            continue;
        }

        const int From = m_pTokens [i + 1];
        const int To = m_pTokens [i + 2];

        const int TxtFrom = From + m_uTxtOffset;
        const int TxtTo = To + m_uTxtOffset;
        DebugLogAssert (TxtTo >= TxtFrom && (unsigned) TxtTo < m_uMaxCharsAtOnce);

        // fitst see if there was a complex token but current token
        // is already outside its boundaries
        if (-1 != CxTokenFrom && TxtFrom > CxTokenTo) 
        {
            // put assembled complex token
            PutToken (m_iXWordTag, CxTokenFrom, CxTokenTo, NormWord, NormWordLen);
            
            // forget the complex token ever existed
            NormWordLen = 0;
            CxTokenFrom = -1;
            CxTokenTo = -1;
        }

        // see if there were no complex token read (the most common)
        if (-1 == CxTokenFrom) 
        {
            // check if this one is not a complex token
            if (Tag != m_iXWordTag) 
            {
                PutToken (Tag, TxtFrom, TxtTo);
            }
            else
            {
                CxTokenFrom = TxtFrom;
                CxTokenTo = TxtTo;
            }
        }
        // see if the current token is within the complex token
        else if (TxtTo <= CxTokenTo)
        {
            // check if that's a segment token, which we need to add
            if (Tag == m_iSegTag) 
            {
                const int SegLen = TxtTo - TxtFrom + 1;
                const int NewNormWordLen = NormWordLen + SegLen;
                
                if (NewNormWordLen <= MaxWordLen) 
                {
                    memcpy (NormWord + NormWordLen, m_pTxt + TxtFrom, SegLen * sizeof (int));
                    NormWordLen = NewNormWordLen;
                }
            } 
            else
            {
                PutToken (Tag, TxtFrom, TxtTo);
            }
        }
    }

    // see if complex token was the last one
    if (-1 != CxTokenFrom)
    {
        // put assembled complex token
        PutToken (m_iXWordTag, CxTokenFrom, CxTokenTo, NormWord, NormWordLen);
    }
}


inline void FALexBreaker::
    PutToken(const int Tag, const int TxtFrom, int TxtTo, const int * pNormTxt, int NormTxtLen) const
{
    // check the normalized word length
    LogAssert (MaxWordLen >= NormTxtLen);

    // UTF-32LE normalized word text buffer
    int NormTxt [MaxWordLen];
    // normalized word in UTF-8
    UInt8 NormUtf8 [FAUtf8Const::MAX_CHAR_SIZE * MaxWordLen];

    // final normalized UTF-8 pointer and length
    const UInt8 * pbNormUtf8;
    size_t cbNormUtf8Len;

    // no normalization done or it did not change the surface word
    bool fUseRaw = false;

    // readjust TxtTo in such way that at most MaxWordLen characters are taken
    const int MaxTxtTo = TxtFrom + MaxWordLen - 1;
    if (TxtTo > MaxTxtTo) 
    {
        TxtTo = MaxTxtTo;
    }

    // normalize Txt[TxtFrom..TxtTo], if pNormTxt/NormTxtLen is not provided
    if (0 >= NormTxtLen)
    {
        if (m_pNorm) 
        {
            const int TxtLen = TxtTo - TxtFrom + 1;
            NormTxtLen = FANormalize (m_pTxt + TxtFrom, TxtLen, NormTxt, MaxWordLen, m_pNorm);
            // see if the result of the word normalization is longer than the MaxWordLen
            if (MaxWordLen < NormTxtLen)
            {
                NormTxtLen = MaxWordLen;
            }
            pNormTxt = NormTxt;
            // see if normalization actually changed anything
            if (0 == memcmp (m_pTxt + TxtFrom, NormTxt, sizeof(int)*TxtLen))
            {
                fUseRaw = true;
            }
        }
        else 
        {        
            fUseRaw = true;
        }
    }

    // get surface string offsets
    const int Utf8From = m_pOffsets [TxtFrom];
    const int Utf8To = m_pOffsets [TxtTo];

    // surface string offset
    const size_t Offset = m_uUtf8Offset + Utf8From;
    // surface string length, the last character length can be greater than 1
    const size_t Length = Utf8To - Utf8From + FAUtf8Size (m_pTxt [TxtTo]);

    // see if we can just get a surface string
    if (fUseRaw)
    {
        // get a surface string pointer and length
        pbNormUtf8 = m_pbUTF8Text + Offset;
        cbNormUtf8Len = Length;
    }
    else
    {
        // convert UTF-32LE to UTF-8
        const int Utf8Len = FAArrayToStrUtf8 (pNormTxt, NormTxtLen, (char*) NormUtf8, FAUtf8Const::MAX_CHAR_SIZE * MaxWordLen);
        // we don't expect errors here because pTxt is a valid UTF-32LE
        LogAssert (0 <= Utf8Len);

        cbNormUtf8Len = Utf8Len;
        pbNormUtf8 = NormUtf8;
    }

    // the last check
    LogAssert (0 < Length && m_cbUTF8Text >= Length + Offset);

    // return an original word
    if (m_pOutText)
    {
        AddWord (m_pOutText, m_pbUTF8Text + Offset, Length, Tag);
    }
    // return normalized word
    if (m_pOutNormText)
    {
        AddWord (m_pOutNormText, pbNormUtf8, cbNormUtf8Len, Tag);
    }
}


inline void FALexBreaker::AddWord (
        FAArray_cont_t < char > * pOutText, 
        const UInt8 * pWord, 
        const size_t Length, 
        const int Tag
    ) const
{
    LogAssert (pOutText);

    const int OldSize = pOutText->size ();

    const char * pTagStr = NULL;
    int TagStrLen = 0;
    int DelimLen = 0;

    if (m_pTagSet)
    {
        TagStrLen = m_pTagSet->Tag2Str (Tag, &pTagStr);
        LogAssert (0 < TagStrLen && pTagStr);
        DelimLen = 1;
    }

    // calc new space
    int NewSize = OldSize + int (Length) + TagStrLen + DelimLen;
    // add space for the delimiter
    if (0 != OldSize)
    {
        NewSize++;
    }
    pOutText->resize (NewSize, NewSize);
    char * pOut = pOutText->begin () + OldSize;

    // copy the delimiter
    if (0 != OldSize)
    {
        *pOut++ = char (FAFsmConst::CHAR_SPACE);
    }

    for (size_t i = 0; i < Length; ++i)
    {
        char Symbol = pWord [i];

        if (FAFsmConst::CHAR_SPACE == Symbol)
        {
            Symbol = FAFsmConst::CHAR_MWE_DELIM;
        }

        *pOut++ = Symbol;
    }

    /// copy the tag string
    if (m_pTagSet)
    {
        *pOut++ = FAFsmConst::CHAR_TAG_DELIM;

        for (int i = 0; i < TagStrLen; ++i)
        {
            char Symbol = pTagStr [i];
            *pOut++ = Symbol;
        }
    }
}

}
