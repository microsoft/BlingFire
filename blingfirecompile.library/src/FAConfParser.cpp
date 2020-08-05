/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAConfParser.h"
#include "FAMultiMapA.h"
#include "FAUtils.h"
#include "FAException.h"

#include <string>

namespace BlingFire
{


FAConfParser::FAConfParser (FAAllocatorA * pAlloc) :
    m_pConfIs (NULL),
    m_pConfMap (NULL),
    m_str2id (pAlloc),
    m_CurrSecId (-1),
    m_pFirstToken (NULL),
    m_FirstTokenLen (-1)
{
    m_tokenizer2.SetSpaces (", \r");
}


void FAConfParser::SetConfStream (std::istream * pConfIs)
{
    m_pConfIs = pConfIs;
}


void FAConfParser::SetConfMap (FAMultiMapA * pConfMap)
{
    m_pConfMap = pConfMap;
}


void FAConfParser::AddSection (const char * pSecName, const int SecId)
{
    DebugLogAssert (pSecName);

    std::string str = \
        std::string ("[") + std::string (pSecName) + std::string ("]");

    m_str2id.Add (str.c_str (), (int) str.length (), SecId);
}


void FAConfParser::AddParam (const char * pParamName, const int ParamId)
{
    DebugLogAssert (pParamName);

    std::string str (pParamName);

    m_str2id.Add (str.c_str (), (int) str.length (), ParamId);
}


void FAConfParser::AddNumParam (const char * pParamName, const int ParamId)
{
    DebugLogAssert (pParamName);

    std::string str = std::string ("Num.") + std::string (pParamName);

    m_str2id.Add (str.c_str (), (int) str.length (), ParamId);
}


void FAConfParser::AddStrParam (const char * pParamName, 
                                const int ParamId, 
                                const char * pValueStr, 
                                const int ValueId)
{
    DebugLogAssert (pParamName && pValueStr);

    std::string str1 = std::string ("Str.") + std::string (pParamName);

    m_str2id.Add (str1.c_str (), (int) str1.length (), ParamId);

    std::string str2 = std::string (pParamName) + \
        std::string (" ") + std::string (pValueStr);

    m_str2id.Add (str2.c_str (), (int) str2.length (), ValueId);
}


const bool FAConfParser::ProcessSection ()
{
    DebugLogAssert (0 < m_FirstTokenLen && m_pFirstToken);

    if (2 < m_FirstTokenLen && 
        '[' == *m_pFirstToken && 
        ']' == m_pFirstToken [m_FirstTokenLen - 1]) {

        const bool Res = \
            m_str2id.Get (m_pFirstToken, m_FirstTokenLen, &m_CurrSecId);

        if (!Res) {
            const std::string ErrMsg = \
                std::string ("Unknown section name: ") + \
                std::string (m_pFirstToken, m_FirstTokenLen);

            FASyntaxError (NULL, -1, -1, ErrMsg.c_str ());
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

        return true;
    }

    return false;
}


const bool FAConfParser::ProcessParam ()
{
    DebugLogAssert (0 < m_FirstTokenLen && m_pFirstToken);
    DebugLogAssert (-1 != m_CurrSecId);

    int ParamId;
    const bool Res = \
        m_str2id.Get (m_pFirstToken, m_FirstTokenLen, &ParamId);

    if (Res) {
        m_pConfMap->Add (m_CurrSecId, ParamId);
        return true;
    }

    return false;
}


const bool FAConfParser::ProcessNumParam ()
{
    DebugLogAssert (0 < m_FirstTokenLen && m_pFirstToken);
    DebugLogAssert (-1 != m_CurrSecId);

    std::string str = \
        std::string ("Num.") + std::string (m_pFirstToken, m_FirstTokenLen);

    int ParamId;
    const bool Res = \
        m_str2id.Get (str.c_str (), (int) str.length (), &ParamId);

    if (Res) {
        int Value;
        m_tokenizer.GetNextInt (&Value);

        m_pConfMap->Add (m_CurrSecId, ParamId);
        m_pConfMap->Add (m_CurrSecId, Value);

        return true;
    }

    return false;
}


const bool FAConfParser::ProcessStrParam ()
{
    DebugLogAssert (0 < m_FirstTokenLen && m_pFirstToken);
    DebugLogAssert (-1 != m_CurrSecId);

    std::string str = \
        std::string ("Str.") + std::string (m_pFirstToken, m_FirstTokenLen);

    int ParamId;
    bool Res = m_str2id.Get (str.c_str (), (int) str.length (), &ParamId);

    if (Res) {

        std::string str1 (m_pFirstToken, m_FirstTokenLen);

        const char * pValStr = nullptr;
        int ValStrLen;
        m_tokenizer.GetNextStr (&pValStr, &ValStrLen);

        std::string str2 = \
            str1 + std::string (" ") + std::string (pValStr, ValStrLen);

        int Value;
        Res = m_str2id.Get (str2.c_str (), (int) str2.length (), &Value);

        if (Res) {

            m_pConfMap->Add (m_CurrSecId, ParamId);
            m_pConfMap->Add (m_CurrSecId, Value);

        } else {

            // interpreter pValStr as a comma-separated array
            m_tokenizer2.SetString (pValStr, ValStrLen);

            // make iteration thru all the elements
            while (m_tokenizer2.GetNextStr (&pValStr, &ValStrLen)) {

                std::string str3 = \
                    str1 + std::string (" ") + \
                    std::string (pValStr, ValStrLen);

                const char * pElStr = str3.c_str ();
                const int ElStrLen = (int) str3.length ();

                Res = m_str2id.Get (pElStr, ElStrLen, &Value);

                if (!Res) {
                    const std::string ErrMsg = \
                        std::string ("Unknown parameter/value: ") + \
                        std::string (str2.c_str (), str2.length ());

                    FASyntaxError (NULL, -1, -1, ErrMsg.c_str ());
                    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
                }

                m_pConfMap->Add (m_CurrSecId, ParamId);
                m_pConfMap->Add (m_CurrSecId, Value);

            } // of while (m_tokenizer2.GetNextStr ...

        } // of if (Res) ...

        return true;

    }  // of if (Res) ...

    return false;
}


void FAConfParser::Process ()
{
    DebugLogAssert (m_pConfIs && m_pConfMap);

    std::string line;

    // read the input
    while (!m_pConfIs->eof ()) {

        if (!std::getline (*m_pConfIs, line))
            break;

        const char * pLineStr = line.c_str ();
        int StrLen = (int) line.length ();

        if (0 < StrLen) {
            DebugLogAssert (pLineStr);
            if (0x0D == (unsigned char) pLineStr [StrLen - 1])
                StrLen--;
        }
        if (0 == StrLen)
            continue;

        // skip comments
        if ('#' == *pLineStr) {
            continue;
        }

        // setup string
        m_tokenizer.SetString (pLineStr, StrLen);
        // get first token
        m_tokenizer.GetNextStr (&m_pFirstToken, &m_FirstTokenLen);
        DebugLogAssert (m_pFirstToken && 0 < m_FirstTokenLen);

        if (ProcessSection ()) {
            continue;
        } else if (ProcessNumParam ()) {
            continue;
        } else if (ProcessStrParam ()) {
            continue;
        } else if (ProcessParam ()) {
            continue;
        } else {

            const std::string ErrMsg = \
                std::string ("Unknown syntax in line: ") + \
                std::string (pLineStr, StrLen);

            FASyntaxError (NULL, -1, -1, ErrMsg.c_str ());
            throw FAException (FAMsg::IOError, __FILE__, __LINE__);
        }

    } // of while ...
}

}
