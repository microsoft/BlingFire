/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAResolveMatch_nest.h"
#include "FAUtils_cl.h"

#include <algorithm>


#define FAResolveMatch_nest_PackRes(Rule, From, To) \
    (unsigned int) ( (((unsigned char)  From) << 24) | \
                     (((unsigned char) (DefMaxPos - To)) << 16) | \
                     ((unsigned short) Rule) )

#define FAResolveMatch_nest_Res2From(Res) \
    ((unsigned char)(Res >> 24))

#define FAResolveMatch_nest_Res2To(Res) \
    ((unsigned char) (DefMaxPos - ((0x00FF0000 & Res) >> 16)))

#define FAResolveMatch_nest_Res2Rule(Res) \
    ((unsigned short) (0x0000FFFF & Res))

namespace BlingFire
{


FAResolveMatch_nest::FAResolveMatch_nest (FAAllocatorA * pAlloc)
{
    m_results.SetAllocator (pAlloc);
    m_results.Create ();

    m_rule2pos.SetAllocator (pAlloc);
    m_rule2pos.Create ();

    m_stack.SetAllocator (pAlloc);
    m_stack.Create ();
}


FAResolveMatch_nest::~FAResolveMatch_nest ()
{}


void FAResolveMatch_nest::
    AddResult (const int Rule, const int From, const int To)
{
    DebugLogAssert (From <= To);
    DebugLogAssert (0 <= Rule && DefMaxRule >= Rule);
    DebugLogAssert (0 <= From && DefMaxPos >= From);
    DebugLogAssert (0 <= To && DefMaxPos >= To);

    unsigned int Res = FAResolveMatch_nest_PackRes (Rule, From, To);
    m_results.push_back (Res);


    const int OldSize = m_rule2pos.size ();

    if (Rule >= OldSize) {

        m_rule2pos.resize (Rule + 1);

        int * pBegin = m_rule2pos.begin () + OldSize;
        const int Delta = Rule - OldSize + 1;

        memset (pBegin, -1, sizeof (m_rule2pos [0]) * Delta);
    }
}


inline void FAResolveMatch_nest::SortResults ()
{
    const int ResCount = m_results.size ();
    unsigned int * pResults = m_results.begin ();

    // if results don't overlap they are already sorted
    if (false == FAIsSorted (pResults, ResCount)) {
        // sort the results, special encoding allows integer comparison here
        // TODO: make something faster here
        std::sort (pResults, pResults + ResCount);
    }
}


inline void FAResolveMatch_nest::FilterResults_uniq ()
{
    int ResCount = m_results.size ();
    unsigned int * pResults = m_results.begin ();
    DebugLogAssert (0 < ResCount && pResults);
    DebugLogAssert (FAIsSorted (pResults, ResCount));

    unsigned int * pEnd = pResults + ResCount;
    ResCount = int (std::unique (pResults, pEnd) - pResults);
    DebugLogAssert (0 < ResCount);

    m_results.resize (ResCount);
}


/// removes:
/// 1. overlappings, e.g. ([)] --> (), (<>[){}] --> (<>){}, ...
/// 2. the same rule inclusions, e.g. Ng1(Ng2()) --> Ng1()
inline void FAResolveMatch_nest::FilterResults_overlap_include ()
{
    int ResCount = m_results.size ();
    unsigned int * pResults = m_results.begin ();
    DebugLogAssert (0 < ResCount && pResults);
    DebugLogAssert (FAIsSorted (pResults, ResCount));

    int * pRule2To = m_rule2pos.begin ();

#ifndef NDEBUG
    const int Rule2PosSize = m_rule2pos.size ();
    for (int j = 0; j < Rule2PosSize; ++j) {
        DebugLogAssert (-1 == pRule2To [j]);
    }
#endif

    m_stack.resize (0);

    unsigned int TopRes = 0;

    int TopFrom = 0;
    int TopTo = 0;

    int InPos = 0;
    int OutPos = 0;

    while (InPos < ResCount) {

        const unsigned int Res = pResults [InPos];

        const int From = FAResolveMatch_nest_Res2From (Res);
        const int To = FAResolveMatch_nest_Res2To (Res);

        // top level or included
        if ((0 == m_stack.size ()) || (TopFrom <= From && TopTo >= To)) {

            const int Rule = FAResolveMatch_nest_Res2Rule (Res);
            DebugLogAssert (0 <= Rule && m_rule2pos.size () > (unsigned int) Rule);

            const int LastTo = pRule2To [Rule];

            // check for absence of the same-rule inclusion
            if (-1 == LastTo || LastTo < From) {

                TopRes = Res;
                TopFrom = From;
                TopTo = To;

                m_stack.push_back (Res);
                pRule2To [Rule] = To;

                pResults [OutPos++] = Res;
            }

            InPos++;

        // does not overlap e.g. []()
        } else if (TopTo < From) {

            DebugLogAssert (0 < m_stack.size ());
            m_stack.pop_back ();

            if (0 < m_stack.size ()) {

                TopRes = m_stack [m_stack.size () - 1];

                TopFrom = FAResolveMatch_nest_Res2From (TopRes);
                TopTo = FAResolveMatch_nest_Res2To (TopRes);
            }

        } else {

            InPos++;
        }
    }

    DebugLogAssert (0 < OutPos);

    for (int i = 0; i < OutPos; ++i) {

        const unsigned int Res = pResults [i];

        const int Rule = FAResolveMatch_nest_Res2Rule (Res);
        DebugLogAssert (0 <= Rule && m_rule2pos.size () > (unsigned int) Rule);

        pRule2To [Rule] = -1;
    }

    m_results.resize (OutPos);
}


void FAResolveMatch_nest::Process ()
{
    if (0 == m_results.size ())
        return;

    SortResults ();
    FilterResults_uniq ();
    FilterResults_overlap_include ();
}


const int FAResolveMatch_nest::GetCount () const
{
    const int Count = m_results.size ();
    return Count;
}


void FAResolveMatch_nest::
    GetResult (const int i, int * pRule,  int * pFrom, int * pTo) const
{
    DebugLogAssert (pRule && pFrom && pTo);
    DebugLogAssert (0 <= i && m_results.size () > (unsigned int) i);

    const unsigned int Res = m_results [i];

    *pFrom = FAResolveMatch_nest_Res2From (Res);
    *pTo = FAResolveMatch_nest_Res2To (Res);
    *pRule = FAResolveMatch_nest_Res2Rule (Res);

    DebugLogAssert (0 <= *pRule && DefMaxRule >= *pRule);
    DebugLogAssert (0 <= *pFrom && DefMaxPos >= *pFrom);
    DebugLogAssert (0 <= *pTo && DefMaxPos >= *pTo);
    DebugLogAssert (*pFrom <= *pTo);
}


void FAResolveMatch_nest::Clear ()
{
    m_results.resize (0);
}

}
