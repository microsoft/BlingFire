/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASummTagScores.h"

namespace BlingFire
{


FASummTagScores::FASummTagScores () :
    m_TotalCount (0),
    m_pCounts (NULL),
    m_pScores (NULL),
    m_MaxTag (0),
    m_UnkScore (DefUnkScore)
{}


FASummTagScores::~FASummTagScores ()
{
    if (m_pCounts) {
        delete [] m_pCounts;
        m_pCounts = NULL;
    }
    if (m_pScores) {
        delete [] m_pScores;
        m_pScores = NULL;
    }
}


void FASummTagScores::SetUnkScore (const float Score)
{
    m_UnkScore = Score;
}


void FASummTagScores::SetMaxTag (const int MaxTag)
{
    LogAssert (0 <= MaxTag);

    m_MaxTag = MaxTag;

    if (m_pCounts) {
        delete [] m_pCounts;
        m_pCounts = NULL;
    }
    if (m_pScores) {
        delete [] m_pScores;
        m_pScores = NULL;
    }

    m_pCounts = new int [m_MaxTag + 1];
    LogAssert (m_pCounts);

    m_pScores = new float [m_MaxTag + 1];
    LogAssert (m_pScores);
}


void FASummTagScores::Clear ()
{
    LogAssert (m_pCounts && m_pScores && 0 <= m_MaxTag);

    m_TotalCount = 0;

    for (int Tag = 0; Tag <= m_MaxTag; ++Tag) {
        m_pCounts [Tag] = 0;
        m_pScores [Tag] = 0;
    }
}


void FASummTagScores::
    AddScores (const int * pTags, const float * pScores, const int Count)
{
    if (0 < Count) {

        // increment the total count
        m_TotalCount++;

        // update tag's counts
        for (int i = 0; i < Count; ++i) {

            // get the next tag
            const int Tag = pTags [i];
            LogAssert (0 <= Tag && Tag <= m_MaxTag);
            // get it's score
            const float Score = pScores [i];
            // increment the count
            m_pCounts [Tag]++;
            // add up the score
            m_pScores [Tag] += Score;
        }

    } // of if (0 < Count) ...
}


void FASummTagScores::Process ()
{
    if (0 >= m_TotalCount) {
        return;
    }

    int Tag;

    // add penalties for not-found tags
    for (Tag = 0; Tag <= m_MaxTag; ++Tag) {

        // get the amount of times we had a score for the tag
        const int Count = m_pCounts [Tag];

        // get the amount of times we did not have a score for the tag
        const int NotFoundCount = m_TotalCount - Count;

        // add a score for the unfound cases
        if (0 < NotFoundCount) {
            m_pScores [Tag] += (m_UnkScore * NotFoundCount);
        }

        // normalize the score
        m_pScores [Tag] = m_pScores [Tag] / m_TotalCount;
    }
}


const int FASummTagScores::GetScores (const float ** ppScores, const int ** ppCounts) const
{
    LogAssert (ppScores && ppCounts);

    *ppScores = m_pScores;
    *ppCounts = m_pCounts;

    return m_MaxTag + 1;
}


const int FASummTagScores::GetTotalCount () const
{
    return m_TotalCount;
}

}

