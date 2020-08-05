/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAWreRulesConfKeeper.h"
#include "FAFsmConst.h"
#include "FALimits.h"
#include "FAException.h"
#include "FALDB.h"
#include "FAWREConf_pack.h"
#include "FAMultiMap_pack.h"

namespace BlingFire
{


FAWreRulesConfKeeper::FAWreRulesConfKeeper () :
    m_pLDB (NULL),
    m_pWre (NULL),
    m_pActs (NULL),
    m_IgnoreCase (false),
    m_MinUniProb (float (FAFsmConst::MIN_LOG_PROB))
{}


FAWreRulesConfKeeper::~FAWreRulesConfKeeper ()
{
    FAWreRulesConfKeeper::Clear ();
}


void FAWreRulesConfKeeper::SetLDB (const FALDB * pLDB)
{
    m_pLDB = pLDB;
}


void FAWreRulesConfKeeper::Init (const int * pValues, const int Size)
{
    FAAssert (m_pLDB && (pValues || 0 >= Size), FAMsg::InternalError);

    FAWreRulesConfKeeper::Clear ();

    for (int i = 0; i < Size; ++i) {

        const int Param = pValues [i];

        switch (Param) {

        case FAFsmConst::PARAM_IGNORE_CASE:
        {
            m_IgnoreCase = true;
            break;
        }
        case FAFsmConst::PARAM_MIN_UNI_PROB:
        {
            const int MinProbInt = pValues [++i];
            FAAssert (0 <= MinProbInt && 100 >= MinProbInt, \
                FAMsg::InitializationError);

            if (0 == MinProbInt)
                m_MinUniProb = float (FAFsmConst::MIN_LOG_PROB);
            else if (100 == MinProbInt)
                m_MinUniProb = float (FAFsmConst::MAX_LOG_PROB);
            else
                m_MinUniProb = log (float (MinProbInt) / 100.0f);

            break;
        }
        case FAFsmConst::PARAM_WRE_CONF:
        {
            const int DumpNum = pValues [++i];
            const unsigned char * pDump = m_pLDB->GetDump (DumpNum);
            FAAssert (pDump, FAMsg::InitializationError);

            if (!m_pWre) {
                m_pWre = NEW FAWREConf_pack;
                FAAssert (m_pWre, FAMsg::InitializationError);
            }
            m_pWre->SetImage (pDump);
            break;
        }
        case FAFsmConst::PARAM_ACTS:
        {
            const int DumpNum = pValues [++i];
            const unsigned char * pDump = m_pLDB->GetDump (DumpNum);
            FAAssert (pDump, FAMsg::InitializationError);

            if (!m_pActs) {
                m_pActs = NEW FAMultiMap_pack;
                FAAssert (m_pActs, FAMsg::InitializationError);
            }
            m_pActs->SetImage (pDump);
            break;
        }

        default:
            FAError (FAMsg::InitializationError);
        }
    } // of for (int i = 0; ...
}


void FAWreRulesConfKeeper::Clear ()
{
    if (m_pWre) {
        delete m_pWre;
        m_pWre = NULL;
    }
    if (m_pActs) {
        delete m_pActs;
        m_pActs = NULL;
    }

    m_IgnoreCase = false;
    m_MinUniProb = float (FAFsmConst::MIN_LOG_PROB);
}


const FAWREConfCA * FAWreRulesConfKeeper::GetWre () const
{
    return m_pWre;
}

const FAMultiMapCA * FAWreRulesConfKeeper::GetActions () const
{
    return m_pActs;
}

const bool FAWreRulesConfKeeper::GetIgnoreCase() const
{
    return m_IgnoreCase;
}

const float FAWreRulesConfKeeper::GetMinUniProb () const
{
    return m_MinUniProb;
}

}
