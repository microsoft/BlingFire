/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAGcLDB.h"
#include "FAFsmConst.h"
#include "FAUtils_cl.h"
#include "FAException.h"

namespace BlingFire
{


FAGcLDB::FAGcLDB (FAAllocatorA *) :
    FALDB (),
    m_ppMealyWres (NULL),
    m_Count (0)
{}

FAGcLDB::~FAGcLDB ()
{
    FAGcLDB::Clear ();
}

void FAGcLDB::Clear ()
{
    for (int i = 0; i < m_Count; ++i) {
        if (m_ppMealyWres [i]) {
            delete m_ppMealyWres [i];
        }
    }

    delete [] m_ppMealyWres;
    m_ppMealyWres = NULL;
    m_Count = 0;
}

const FAWREConfCA * FAGcLDB::GetCommon () const
{
    return & m_common_wre;
}

const FAWREConfCA * FAGcLDB::GetRule (const int i) const
{
    if (0 > i || i > m_Count) {
        return NULL;
    }

    const FAWREConf_pack * pR = m_ppMealyWres [i];
    return pR;
}

void FAGcLDB::SetImage (const unsigned char * pImgDump)
{
    Clear ();

    FALDB::SetImage (pImgDump);

    const int * pValues;
    const int Size = m_Conf.Get (FAFsmConst::FUNC_WRE, &pValues);
    FAAssert (pValues && 0 < Size, FAMsg::InitializationError);

    DebugLogAssert (NULL == m_ppMealyWres);
    m_ppMealyWres = NEW FAWREConf_pack* [Size];
    FAAssert (m_ppMealyWres, FAMsg::InternalError);

    memset (m_ppMealyWres, 0, sizeof (FAWREConf_pack*) * Size);

    /// initialize a common automaton
    const unsigned char * pDump = GetDump (1);
    m_common_wre.SetImage (pDump);

    int DumpCount = 0;
    m_Count = 0;

    /// initialize rules automata (if any)
    for (int i = 0; i < Size; ++i) {

        const int Param = pValues [i];

        if (FAFsmConst::PARAM_FSM_COUNT == Param) {

            const int FsmCount = pValues [++i];

            FAAssert (0 == FsmCount || 1 == FsmCount, \
                FAMsg::InitializationError);

            if (1 == FsmCount) {

                pDump = GetDump (2 + DumpCount++); // 2-based

                FAWREConf_pack * pR = NEW FAWREConf_pack;
                FAAssert (pR, FAMsg::InternalError);

                DebugLogAssert (NULL == m_ppMealyWres [m_Count]);
                m_ppMealyWres [m_Count] = pR;

                pR->SetImage (pDump);
            }

            // increment the rule count
            m_Count++;

        } // of if (FAFsmConst::PARAM_FSM_COUNT == Param) ...
    } // of for (int i = 0; ...

    DebugLogAssert (m_Count <= Size);
}

}
