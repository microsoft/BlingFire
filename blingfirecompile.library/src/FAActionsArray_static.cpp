/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAActionsArray_static.h"

namespace BlingFire
{


FAActionsArray_static::
    FAActionsArray_static (const FAActionsA ** pActsArr, const int Count) :
        m_pActsArr (pActsArr),
        m_Count (Count)
{}


FAActionsArray_static::~FAActionsArray_static ()
{}


const int FAActionsArray_static::GetStageCount () const
{
    return m_Count;
}


const FAActionsA * FAActionsArray_static::GetStage (const int Num) const
{
    if (0 <= Num && m_Count > Num)
        return m_pActsArr [Num];
    else
        return NULL;
}

}
