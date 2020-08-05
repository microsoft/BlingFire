/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FALadConfKeeper.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FALDB.h"
#include "FAMultiMapCA.h"
#include "FAMultiMap_pack_fixed.h"

namespace BlingFire
{


FALadConfKeeper::FALadConfKeeper () :
    m_Order (0),
    m_MinOrder (0),
    m_MaxCount (0),
    m_MaxTag (0),
    m_UnkTag (0),
    m_MinMatchRatio (0),
    m_MinWordMatchRatio (0),
    m_pCharMap (NULL),
    m_pC2SMap (NULL),
    m_pS2LMap (NULL),
    m_MinScriptTag (0),
    m_MaxScriptTag (0)
{}


FALadConfKeeper::~FALadConfKeeper ()
{
    FALadConfKeeper::Clear ();
}


void FALadConfKeeper::Initialize (const FALDB * pLDB, const int * pValues, const int Size)
{
    LogAssert (pLDB);
    LogAssert (pValues || 0 >= Size);

    FALadConfKeeper::Clear ();

    for (int i = 0; i < Size; ++i) {

        const int Param = pValues [i];

        switch (Param) {

        case FAFsmConst::PARAM_ORDER:
        {
            m_Order = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_MIN_ORDER:
        {
            m_MinOrder = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_UNKNOWN:
        {
            m_UnkTag = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_MAX_COUNT:
        {
            m_MaxCount = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_RATIO:
        {
            m_MinMatchRatio = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_RATIO2:
        {
            m_MinWordMatchRatio = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_MAX_TAG:
        {
            m_MaxTag = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_CHARMAP:
        {
            const int DumpNum = pValues [++i];
            const unsigned char * pDump = pLDB->GetDump (DumpNum);
            LogAssert (pDump);

            if (!m_pCharMap) {
                m_pCharMap = NEW FAMultiMap_pack_fixed;
                LogAssert (m_pCharMap);
            }
            m_pCharMap->SetImage (pDump);

            break;
        }
        case FAFsmConst::PARAM_C2S_MAP:
        {
            const int DumpNum = pValues [++i];
            const unsigned char * pDump = pLDB->GetDump (DumpNum);
            LogAssert (pDump);

            if (!m_pC2SMap) {
                m_pC2SMap = NEW FAMultiMap_pack_fixed;
                LogAssert (m_pC2SMap);
            }
            m_pC2SMap->SetImage (pDump);

            break;
        }
        case FAFsmConst::PARAM_S2L_MAP:
        {
            const int DumpNum = pValues [++i];
            const unsigned char * pDump = pLDB->GetDump (DumpNum);
            LogAssert (pDump);

            if (!m_pS2LMap) {
                m_pS2LMap = NEW FAMultiMap_pack_fixed;
                LogAssert (m_pS2LMap);
            }
            m_pS2LMap->SetImage (pDump);

            break;
        }
        case FAFsmConst::PARAM_SCRIPT_MIN:
        {
            m_MinScriptTag = pValues [++i];
            break;
        }
        case FAFsmConst::PARAM_SCRIPT_MAX:
        {
            m_MaxScriptTag = pValues [++i];
            break;
        }
        default:
            LogAssert (false);

        } // of switch (Param) ...
    }

    if (0 < Size) {
        LogAssert (0 < m_Order && m_Order < FALimits::MaxWordLen);
        LogAssert (0 < m_MinOrder && m_MinOrder < FALimits::MaxWordLen);
        LogAssert (0 < m_UnkTag);
        LogAssert (0 < m_MaxCount);
        LogAssert (0 <= m_MinMatchRatio && m_MinMatchRatio <= 100);
        LogAssert (0 <= m_MinWordMatchRatio && m_MinWordMatchRatio <= 100);
        LogAssert (0 < m_MaxTag);
        LogAssert (m_pC2SMap && m_pS2LMap);
        LogAssert ((!m_pC2SMap && !m_pS2LMap) || (m_pC2SMap && m_pS2LMap));
        LogAssert (!m_pC2SMap || 
            (m_MinScriptTag <= m_MaxScriptTag && m_MinScriptTag > m_MaxTag));
    }
}


void FALadConfKeeper::Clear ()
{
    m_Order = 0;
    m_MinOrder = 0;
    m_MaxCount = 0;
    m_MaxTag = 0;
    m_UnkTag = 0;
    m_MinMatchRatio = 0;
    m_MinWordMatchRatio = 0;
    m_MinScriptTag = 0;
    m_MaxScriptTag = 0;

    if (m_pCharMap) {
        delete m_pCharMap;
        m_pCharMap = NULL;
    }
    if (m_pC2SMap) {
        delete m_pC2SMap;
        m_pC2SMap = NULL;
    }
    if (m_pS2LMap) {
        delete m_pS2LMap;
        m_pS2LMap = NULL;
    }
}


const int FALadConfKeeper::GetOrder () const
{
    return m_Order;
}

const int FALadConfKeeper::GetMinOrder () const
{
    return m_MinOrder;
}

const int FALadConfKeeper::GetMaxCount () const
{
    return m_MaxCount;
}

const int FALadConfKeeper::GetMaxTag () const
{
    return m_MaxTag;
}

const int FALadConfKeeper::GetUnkTag () const
{
    return m_UnkTag;
}

const int FALadConfKeeper::GetMinMatchRatio () const
{
    return m_MinMatchRatio;
}

const int FALadConfKeeper::GetMinWordMatchRatio () const
{
    return m_MinWordMatchRatio;
}

const FAMultiMapCA * FALadConfKeeper::GetCharMap () const
{
    return m_pCharMap;
}

const FAMultiMapCA * FALadConfKeeper::GetC2SMap () const
{
    return m_pC2SMap;
}

const FAMultiMapCA * FALadConfKeeper::GetS2LMap () const
{
    return m_pS2LMap;
}

const int FALadConfKeeper::GetMinScriptTag () const
{
    return m_MinScriptTag;
}

const int FALadConfKeeper::GetMaxScriptTag () const
{
    return m_MaxScriptTag;
}

}
