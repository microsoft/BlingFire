/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FACssLDB.h"
#include "FAFsmConst.h"

namespace BlingFire
{


FACssLDB::FACssLDB (FAAllocatorA * pAlloc) :
    FALDB (),
    m_pAlloc (pAlloc),
    m_fTagDict (false),
    m_fW2TP (false),
    m_fOic (false),
    m_fCssRules (false),
    m_fGlobal (false)
{}


FACssLDB::~FACssLDB ()
{
    FACssLDB::Clear ();
}


void FACssLDB::Clear ()
{
    m_fTagDict = false;
    m_fW2TP = false;
    m_fOic = false;
    m_fCssRules = false;
    m_fGlobal = false;

    m_tag_dict.Clear ();
    m_w2tp.Clear ();
    m_oic.Clear ();
    m_css_rules.Clear ();
    m_global.Clear ();
}


void FACssLDB::Init ()
{
    const int * pValues;
    int Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_TAG_DICT, &pValues);
    m_tag_dict.Init (pValues, Size);
    m_fTagDict = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_W2TP, &pValues);
    m_w2tp.Initialize (this, pValues, Size);
    m_fW2TP = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_OIC_RULES, &pValues);
    m_oic.Init (pValues, Size);
    m_fOic = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_CSS_RULES, &pValues);
    m_css_rules.Init (pValues, Size);
    m_fCssRules = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_GLOBAL, &pValues);
    m_global.Init (pValues, Size);
    m_fGlobal = 0 < Size;
}


void FACssLDB::SetImage (const unsigned char * pImgDump)
{
    m_tag_dict.SetLDB (this);
    m_oic.SetLDB (this);
    m_css_rules.SetLDB (this);
    m_global.SetLDB (this);

    FACssLDB::Clear ();

    FALDB::SetImage (pImgDump);

    FACssLDB::Init ();
}



const FAWgConfKeeper * FACssLDB::GetW2TPConf () const
{
    if (!m_fW2TP) {
        return NULL;
    }
    return & m_w2tp;
}


const FADictConfKeeper * FACssLDB::GetTagDictConf () const
{
    if (!m_fTagDict) {
        return NULL;
    }
    return & m_tag_dict;
}

const FAWreRulesConfKeeper * FACssLDB::GetOicConf () const
{
    if (!m_fOic) {
        return NULL;
    }
    return & m_oic;
}

const FAWreRulesConfKeeper * FACssLDB::GetCssRulesConf () const
{
    if (!m_fCssRules) {
        return NULL;
    }
    return & m_css_rules;
}


const FAGlobalConfKeeper * FACssLDB::GetGlobalConf () const
{
    if (!m_fGlobal) {
        return NULL;
    }
    return & m_global;
}

}
