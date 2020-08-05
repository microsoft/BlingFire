/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAParseTree_rbt.h"
#include "FATaggedTextA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAParseTree_rbt::FAParseTree_rbt (FAAllocatorA * pAlloc) :
    m_pIn (NULL),
    m_pOut (NULL),
    m_fCanUpdate (false)
{
    m_From2To.SetAllocator (pAlloc);
    m_From2To.Create ();

    m_From2Tag.SetAllocator (pAlloc);
    m_From2Tag.Create ();
}


FAParseTree_rbt::~FAParseTree_rbt ()
{}


void FAParseTree_rbt::SetInText (const FATaggedTextCA * pIn)
{
    m_pIn = pIn;
}


void FAParseTree_rbt::SetOutText (FATaggedTextA * pOut)
{
    m_pOut = pOut;
}


const int FAParseTree_rbt::GetUpperLabels (const int ** ppLabels) const
{
    DebugLogAssert (ppLabels);
    *ppLabels = m_From2To.begin ();
    return m_From2To.size ();
}


void FAParseTree_rbt::Init (const int Count)
{
    DebugLogAssert (m_pIn && Count == m_pIn->GetWordCount ());

    m_From2To.resize (Count);
    m_From2Tag.resize (Count);

    for (int i = 0; i < Count; ++i) {

        const int Tag = m_pIn->GetTag (i);
        m_From2Tag [i] = Tag;

        m_From2To [i] = i;
    }

    m_fCanUpdate = true;
    m_fChanged = false;
}


void FAParseTree_rbt::AddNode (const int Tag, const int From, const int To)
{
    DebugLogAssert (m_fCanUpdate);
    DebugLogAssert (m_From2To.size () == m_From2Tag.size ());
    DebugLogAssert (0 <= From && m_From2To.size () > (unsigned) From);
    DebugLogAssert (0 <= From && m_From2Tag.size () > (unsigned) From);
    DebugLogAssert (0 <= To && m_From2To.size () > (unsigned) To);
    DebugLogAssert (0 < (- Tag) && (- Tag) <= FALimits::MaxTag);

    m_From2To [From] = To;
    m_From2Tag [From] = - Tag; // as it is negated for constituents

    m_fChanged = true;
}


void FAParseTree_rbt::Update ()
{
    DebugLogAssert (m_pIn && m_pOut);

    m_fCanUpdate = false;

    // see if the text has not been modified
    if (!m_fChanged) {
        FACopyTaggedText (m_pOut, m_pIn);
        return;
    }

    m_pOut->Clear ();

    // MWE have the same length limit as an ordinary word
    const int MaxMweLen = FALimits::MaxWordLen + 1;
    int MweBuff [MaxMweLen];

    const int Count = m_pIn->GetWordCount ();
    DebugLogAssert ((unsigned) Count == m_From2To.size ());
    DebugLogAssert ((unsigned) Count == m_From2Tag.size ());

    // fill in the output container
    int i = 0;

    while (i < Count) {

        // get i-th word attributes
        const int To = m_From2To [i];
        DebugLogAssert (To < Count);
        const int Tag = m_From2Tag [i];
        DebugLogAssert (0 < Tag && Tag <= FALimits::MaxTag);
        const int Offset = m_pIn->GetOffset (i);
        DebugLogAssert (0 <= Offset);

        // see if no MWE grouping was done (the most common situation)
        if (To == i) {

            // copy a single word
            const int * pWord;
            const int WordLen = m_pIn->GetWord (i, &pWord);
            m_pOut->AddWord (pWord, WordLen, Tag, Offset);
            i++;

        // MWE grouping was done
        } else {

            int MweLen = 0;
            int j = i;

            // try to fit MWE into a MaxMweLen
            for (; j <= To; ++j) {

                const int * pWord;
                const int WordLen = m_pIn->GetWord (j, &pWord);

                // see if the current token can be added
                if (MweLen + WordLen + 1 <= MaxMweLen) {
                    // copy token text
                    memcpy (MweBuff + MweLen, pWord, sizeof (int) * WordLen);
                    // copy a delimiter
                    MweBuff [MweLen + WordLen] = int(' ');
                    // increase MWE length
                    MweLen += WordLen + 1;
                } else {
                    // MWE is too long, delete it
                    // (this should only happen if the grammar is bad)
                    m_From2To [i] = i;
                    m_From2Tag [i] = m_pIn->GetTag (i);
                    break;
                }
            }
            // copied successfully
            if (To + 1 == j) {
                // MweLen - 1, ignores trailing delimiter
                m_pOut->AddWord (MweBuff, MweLen - 1, Tag, Offset);
                // skip the next word
                i = To + 1;
            }
        } // of if (To == i) ... 
    } // of while (i < Count) ...
}


const int FAParseTree_rbt::GetUpperNodes (const int **) const
{
    // not implemented
    DebugLogAssert (0);
    return -1;
}

const int FAParseTree_rbt::GetNext (const int) const
{
    // not implemented
    DebugLogAssert (0);
    return -1;
}

const int FAParseTree_rbt::GetChild (const int) const
{
    // not implemented
    DebugLogAssert (0);
    return -1;
}

const int FAParseTree_rbt::GetLabel (const int) const
{
    // not implemented
    DebugLogAssert (0);
    return -1;
}

}

