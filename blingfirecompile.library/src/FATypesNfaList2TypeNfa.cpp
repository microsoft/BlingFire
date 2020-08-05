/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FATypesNfaList2TypeNfa.h"
#include "FARSDfaA.h"
#include "FAUtils.h"

namespace BlingFire
{

FATypesNfaList2TypeNfa::FATypesNfaList2TypeNfa (FAAllocatorA * pAlloc) :
    m_CurrTypeIw (0),
    m_nfa (pAlloc),
    m_common (pAlloc),
    m_eremoval (pAlloc)
{
    m_type2id.SetAllocator (pAlloc);

    m_arr.SetAllocator (pAlloc);
    m_arr.Create ();
}


void FATypesNfaList2TypeNfa::SetEpsilonIw (const int EpsilonIw)
{
    m_common.SetEpsilonIw (EpsilonIw);
    m_eremoval.SetEpsilonIw (EpsilonIw);
}


void FATypesNfaList2TypeNfa::SetAnyIw (const int AnyIw)
{
    m_eremoval.SetAnyIw (AnyIw);
}


void FATypesNfaList2TypeNfa::SetInitialTypeIw (const int InitialTypeIw)
{
    m_InitialTypeIw = InitialTypeIw;
    m_CurrTypeIw = InitialTypeIw;
}


const FARSNfaA * FATypesNfaList2TypeNfa::GetNfa () const
{
    return & m_nfa;
}


inline void FATypesNfaList2TypeNfa::Clear_a ()
{
    m_CurrTypeIw = m_InitialTypeIw;
    m_arr.resize (0);
    m_common.Clear ();
    m_type2id.Clear ();
}


void FATypesNfaList2TypeNfa::Clear ()
{
    FATypesNfaList2TypeNfa::Clear_a ();
    m_nfa.Clear ();
}


void FATypesNfaList2TypeNfa::AddTypeInfo (const int TypeIw)
{
    const int MaxState = m_nfa.GetMaxState ();
    const int MaxIw = m_nfa.GetMaxIw ();

    // adjust MaxIw
    if (MaxIw < TypeIw) {
        m_nfa.AddIwCount (TypeIw - MaxIw);
    }

    // add one more state
    m_nfa.AddStateCount (1);

    // add transition from all the final states to the newly added
    const int NewState = MaxState + 1;
    const int * pFinals;
    const int FinalsCount = m_nfa.GetFinals (&pFinals);

    m_arr.resize (FinalsCount);

    for (int i = 0; i < FinalsCount; ++i) {

        DebugLogAssert (pFinals);
        // get next final
        const int FinalState = pFinals [i];
        // store it in
        m_arr [i] = FinalState;
        // add transition
        m_nfa.SetTransition (FinalState, TypeIw, NewState);
        //  make this state ready
        m_nfa.PrepareState (FinalState);
    }

    // add one more final
    m_arr.push_back (NewState);

    // setup final states
    m_nfa.SetFinals (m_arr.begin (), m_arr.size ());
}


const int FATypesNfaList2TypeNfa::GetType (const char * pTypeStr)
{
    DebugLogAssert (pTypeStr);

    const int TypeStrLen = (const int) strlen (pTypeStr);
    DebugLogAssert (0 < TypeStrLen);

    int ChainLen = TypeStrLen / sizeof (int);

    if (TypeStrLen % sizeof (int)) {

        ChainLen++;
        m_arr.resize (ChainLen);
        m_arr [ChainLen - 1] = 0;

    } else {

        m_arr.resize (ChainLen);
    }

    memcpy (m_arr.begin (), pTypeStr, TypeStrLen);

    const int * pTypeIw = m_type2id.Get (m_arr.begin (), m_arr.size ());

    if (NULL != pTypeIw) {
        return *pTypeIw;
    }

    m_type2id.Add (m_arr.begin (), m_arr.size (), m_CurrTypeIw);
    m_CurrTypeIw++;

    return m_CurrTypeIw - 1;
}


void FATypesNfaList2TypeNfa::
    AddTyNfa (const char * pTypeStr, const FARSNfaA * pNfa)
{
    DebugLogAssert (pTypeStr && pNfa);
    DebugLogAssert (-1 != pNfa->GetMaxState ());

    /// copy pNfa --> m_nfa
    FACopyNfa (&m_nfa, pNfa);

    /// retrieve data type
    const int TypeIw = GetType (pTypeStr);

    /// add data type information to the final states
    AddTypeInfo (TypeIw);

    /// add nfa
    m_common.AddNfa (&m_nfa);

    // return nfa into initial state
    m_nfa.Clear ();
}


void FATypesNfaList2TypeNfa::
    AddTyDfa (const char * pTypeStr, const FARSDfaA * pDfa)
{
    DebugLogAssert (pTypeStr && pDfa);
    DebugLogAssert (-1 != pDfa->GetMaxState ());

    /// copy pNfa --> m_nfa
    FACopyDfa2Nfa (&m_nfa, pDfa);

    /// retrieve data type
    const int TypeIw = GetType (pTypeStr);

    /// add data type information to the final states
    AddTypeInfo (TypeIw);

    /// add nfa
    m_common.AddNfa (&m_nfa);

    // return nfa into initial state
    m_nfa.Clear ();
}


void FATypesNfaList2TypeNfa::Process ()
{
    /// make the common NFA ready
    m_common.Process ();

    // get read interface to the common nfa
    const FARSNfaA * pCommonNfa = m_common.GetCommonNfa ();
    DebugLogAssert (pCommonNfa);

    // remove epsilon transitions and store the output into m_nfa
    m_eremoval.SetInNfa (pCommonNfa);
    m_eremoval.SetOutNfa (&m_nfa);
    m_eremoval.Process ();

    FATypesNfaList2TypeNfa::Clear_a ();
}

}
