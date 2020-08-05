/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FALadLDB.h"

namespace BlingFire
{


FALadLDB::FALadLDB () :
    FALDB (),
    m_fW2TP (false),
    m_fN2TP (false),
    m_fU2L (false),
    m_fLAD (false)
{}


FALadLDB::~FALadLDB ()
{
    FALadLDB::Clear ();
}


void FALadLDB::Clear ()
{
    m_fW2TP = false;
    m_fN2TP = false;
    m_fU2L = false;
    m_fLAD = false;

    m_w2tp.Clear ();
    m_n2tp.Clear ();
    m_u2l.Clear ();
    m_lad.Clear ();
}


void FALadLDB::Init ()
{
    const int * pValues;
    int Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_W2TP, &pValues);
    m_w2tp.Initialize (this, pValues, Size);
    m_fW2TP = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_N2TP, &pValues);
    m_n2tp.Initialize (this, pValues, Size);
    m_fN2TP = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_U2L, &pValues);
    m_u2l.Initialize (this, pValues, Size);
    m_fU2L = 0 < Size;

    pValues = NULL;
    Size = m_Conf.Get (FAFsmConst::FUNC_LAD, &pValues);
    m_lad.Initialize (this, pValues, Size);
    m_fLAD = 0 < Size;
}


void FALadLDB::SetImage (const unsigned char * pImgDump)
{
    Clear ();

    FALDB::SetImage (pImgDump);

    if (NULL != pImgDump) {
        Init ();
    }
}


const FAWgConfKeeper * FALadLDB::GetW2TPConf () const
{
    if (!m_fW2TP) {
        return NULL;
    }
    return & m_w2tp;
}


const FAWgConfKeeper * FALadLDB::GetN2TPConf () const
{
    if (!m_fN2TP) {
        return NULL;
    }
    return & m_n2tp;
}


const FAWbdConfKeeper * FALadLDB::GetU2LConf () const
{
    if (!m_fU2L) {
        return NULL;
    }
    return & m_u2l;
}


const FALadConfKeeper * FALadLDB::GetLadConf () const
{
    if (!m_fLAD) {
        return NULL;
    }
    return & m_lad;
}

}
