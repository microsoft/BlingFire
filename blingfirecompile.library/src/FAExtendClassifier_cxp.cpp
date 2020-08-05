/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAExtendClassifier_cxp.h"
#include "FARSDfaA.h"
#include "FAState2OwsA.h"
#include "FAException.h"

namespace BlingFire
{


FAExtendClassifier_cxp::
    FAExtendClassifier_cxp (FAAllocatorA * pAlloc) :
        m_pDfa (NULL),
        m_pOws (NULL),
        m_MaxProb (DefMaxProb),
        m_MaxClass (0),
        m_new_ows (pAlloc)
{
    m_State2Count.SetAllocator (pAlloc);
    m_State2Count.Create ();

    m_StateClass2Count.SetAllocator (pAlloc);
    m_StateClass2Count.SetCopyChains (true);

    m_ows_ar.SetAllocator (pAlloc);
    m_ows_ar.Create ();
}


inline void FAExtendClassifier_cxp::Clear ()
{
    m_State2Count.Clear ();
    m_State2Count.Create ();

    m_StateClass2Count.Clear ();

    m_ows_ar.Clear ();
    m_ows_ar.Create ();

    m_MaxClass = 0;
}


void FAExtendClassifier_cxp::SetMaxProb (const int MaxProb)
{
    m_MaxProb = MaxProb;
}


void FAExtendClassifier_cxp::
    SetFsm (const FARSDfaA * pDfa, const FAState2OwsA * pOwsMap)
{
    m_pDfa = pDfa;
    m_pOws = pOwsMap;

    m_State2Count.resize (0);

    /// figure out what states need the extension
    if (m_pDfa && m_pOws) {

        const int MaxState = m_pDfa->GetMaxState ();
        if (-1 == MaxState) {
            return;
        }

        m_State2Count.resize (MaxState + 1);

        const int * pOws;

        for (int i = 0; i <= MaxState; ++i) {

            if (-1 == m_pOws->GetOws (i, &pOws)) {
                // extension is needed
                m_State2Count [i] = 0;
            } else {
                // extension is not needed
                m_State2Count [i] = -1;
            }
        }
    } // of if (m_pDfa && m_pOws) ...
}


inline void FAExtendClassifier_cxp::
    UpdateState (const int State, const int Class, const int Freq)
{
    DebugLogAssert (0 < Freq && 0 <= Class);    
    DebugLogAssert (0 <= State && State <= m_pDfa->GetMaxState ());
    DebugLogAssert (m_pDfa && m_pDfa->GetMaxState () + 1 == (int)m_State2Count.size ());

    // check whether this State needs the extension
    if (-1 == m_State2Count [State]) {
        return;
    }

    // update the State count
    m_State2Count [State] += Freq;

    int Pair [2];

    Pair [0] = State;
    Pair [1] = Class;

    // update the <State, Class> count
    const int * pFreq = m_StateClass2Count.Get (Pair, 2);

    if (NULL == pFreq) {

        m_StateClass2Count.Add (Pair, 2, Freq);

    } else {

        const int NewFreq = *pFreq + Freq;
        FAAssert (0 < NewFreq, FAMsg::LimitIsExceeded); // int overflow
        m_StateClass2Count.Add (Pair, 2, NewFreq);
    }
}


void FAExtendClassifier_cxp::
    AddStat (
            const int * pChain, 
            const int Size, 
            const int Class, 
            const int Freq
        )
{
    DebugLogAssert (pChain && 0 < Size && 0 <= Class && 0 < Freq);
    DebugLogAssert (m_pDfa && m_pOws);
    DebugLogAssert (m_pDfa->GetMaxState () + 1 == (int) m_State2Count.size ());

    int State = m_pDfa->GetInitial ();
    if (-1 == State) {
        return;
    }

    // update the MaxClass 
    if (m_MaxClass < Class) {
        m_MaxClass = Class;
    }

    // update State information
    UpdateState (State, Class, Freq);

    // match the CHAIN thru the automaton
    for (int Pos = 0; Pos < Size && -1 != State; ++Pos) {

        const int Iw = pChain [Pos];

        State = m_pDfa->GetDest (State, Iw);
        if (-1 == State) {
            return;
        }

        // update State information
        UpdateState (State, Class, Freq);
    }
}


void FAExtendClassifier_cxp::Process ()
{
    int Pair [2];
    const int * pOws;

    // clear the previous results
    m_new_ows.Clear ();

    if (!m_pDfa || !m_pOws || -1 == m_pDfa->GetMaxState ()) {
        return;
    }

    DebugLogAssert (m_pDfa->GetMaxState () + 1 == (int) m_State2Count.size ());

    const int MaxState = m_pDfa->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int StateFreq = m_State2Count [State];
        DebugLogAssert (0 <= StateFreq || -1 == StateFreq);

        if (-1 == StateFreq) {

            /// copy the reaction from the original map
            const int OwCount = m_pOws->GetOws (State, &pOws);
            DebugLogAssert (0 <= OwCount);
            m_new_ows.SetOws (State, pOws, OwCount);

        } else if (0 < StateFreq) {

            /// build a new reaction vector of Class*P(Class|State)
            Pair [0] = State;

            for (int Class = 0; Class <= m_MaxClass; ++Class) {

                // get the <State, Class> count
                Pair [1] = Class;
                const int * pClassFreq = m_StateClass2Count.Get (Pair, 2);

                // see if there is any value
                if (pClassFreq) {

                    const int ClassFreq = *pClassFreq;
                    DebugLogAssert (0 < ClassFreq && ClassFreq <= StateFreq);

                    int Prob = int (0.5f + \
                        (float (m_MaxProb * ClassFreq) / float (StateFreq)));
                    DebugLogAssert (0 <= Prob);

                    if (Prob > m_MaxProb) {
                        Prob = m_MaxProb;
                    }
                    /// if (0 < Prob) {
                        // make an Ow == Class x Prob
                        const int Ow = (Class * (m_MaxProb + 1)) + Prob;
                        m_ows_ar.push_back (Ow);
                    /// }
                }
            }

            const int OwCount = m_ows_ar.size ();
            if (0 < OwCount) {
                m_new_ows.SetOws (State, m_ows_ar.begin (), OwCount);                
            }

            m_ows_ar.resize (0);

        } // of if (-1 == Count) ...
    } // of for (int State = 0; ...

    // clear all but m_new_ows
    Clear ();
}


const FAState2OwsA * FAExtendClassifier_cxp::GetOws () const
{
    return & m_new_ows;
}

}
