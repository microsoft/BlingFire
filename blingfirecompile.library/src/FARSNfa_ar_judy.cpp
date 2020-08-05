/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARSNfa_ar_judy.h"
#include "FAAllocatorA.h"
#include "FAMultiMap_judy.h"
#include "FAFsmConst.h"
#include "FAUtils.h"

namespace BlingFire
{


FARSNfa_ar_judy::FARSNfa_ar_judy (FAAllocatorA * pAlloc) :
  m_IwCount (0),
  m_StateCount (0),
  m_min_final (0),
  m_pAlloc (pAlloc)
{
  m_initials.SetAllocator (m_pAlloc);
  m_initials.Create ();

  m_finals.SetAllocator (m_pAlloc);
  m_finals.Create ();

  m_state2iws.SetAllocator (m_pAlloc);
  m_state2iws.Create ();

  m_state_iw2dsts.SetAllocator (m_pAlloc);
  m_state_iw2dsts.Create ();
}


FARSNfa_ar_judy::~FARSNfa_ar_judy ()
{
  FARSNfa_ar_judy::Clear ();
}


void FARSNfa_ar_judy::Clear ()
{
  DebugLogAssert (m_state2iws.size () == m_state_iw2dsts.size ());

  const int StateCount = m_state2iws.size ();

  for (int State = 0; State < StateCount; ++State) {

    // get Iws array
    FAArray_cont_t < int > * pIws = m_state2iws [State];
    DebugLogAssert (pIws);

    // free it
    pIws->Clear ();
    FAFree (m_pAlloc, pIws);

    // get iw2dsts map
    FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [State];
    DebugLogAssert (pIw2Dsts);

    // free it
    pIw2Dsts->Clear ();
    delete pIw2Dsts;
  }

  m_state2iws.resize (0);
  m_state_iw2dsts.resize (0);
  m_initials.resize (0);
  m_finals.resize (0);
  m_IwCount = 0;
  m_StateCount = 0;
  m_min_final = -1;
}


void FARSNfa_ar_judy::SetMaxIw (const int MaxIw)
{
  DebugLogAssert (0 <= MaxIw);

  m_IwCount = MaxIw + 1;
}


void FARSNfa_ar_judy::SetMaxState (const int MaxState)
{
  DebugLogAssert (0 <= MaxState);

  if (0 != m_StateCount) {
    FARSNfa_ar_judy::Clear ();
  }

  m_StateCount = MaxState + 1;
}


void FARSNfa_ar_judy::create_state (const int State)
{
    // create an Iws container
    FAArray_cont_t < int > * pIws = (FAArray_cont_t < int > *)
      FAAlloc (m_pAlloc, sizeof (FAArray_cont_t < int >));

    pIws->SetAllocator (m_pAlloc);
    pIws->Create ();

    // add it
    m_state2iws [State] = pIws;

    // create 
    FAMultiMap_judy * pIw2Dsts = NEW FAMultiMap_judy;
    DebugLogAssert (pIw2Dsts);

    pIw2Dsts->SetAllocator (m_pAlloc);

    // add it
    m_state_iw2dsts [State] = pIw2Dsts;
}


void FARSNfa_ar_judy::Create ()
{
  DebugLogAssert (m_pAlloc);
  DebugLogAssert (0 < m_StateCount);

  m_state2iws.resize (m_StateCount);
  m_state_iw2dsts.resize (m_StateCount);

  for (unsigned int State = 0; State < m_StateCount; ++State) {

      create_state (State);
  }
}


void FARSNfa_ar_judy::AddStateCount (const int StatesCount)
{
  DebugLogAssert (m_pAlloc);
  DebugLogAssert (m_StateCount == m_state2iws.size ());
  DebugLogAssert (m_StateCount == m_state_iw2dsts.size ());
  DebugLogAssert (0 <= StatesCount);

  const int OldSize = m_StateCount;
  m_StateCount += StatesCount;

  m_state2iws.resize (m_StateCount);
  m_state_iw2dsts.resize (m_StateCount);

  for (unsigned int State = OldSize; State < m_StateCount; ++State) {

      create_state (State);
  }
}


void FARSNfa_ar_judy::AddIwCount (const int IwCount)
{
    DebugLogAssert (0 <= IwCount);

    m_IwCount += IwCount;
}


void FARSNfa_ar_judy::Prepare ()
{
  int * pBegin;
  int Size;

  /// make iws sorted
  const int StateCount =  m_state2iws.size ();

  for (int State = 0; State < StateCount; ++State) {

    FAArray_cont_t < int > * pIwsArray = m_state2iws [State];
    DebugLogAssert (pIwsArray);

    pBegin = pIwsArray->begin ();
    Size = pIwsArray->size ();

    if (false == FAIsSortUniqed (pBegin, Size)) {

        Size = FASortUniq (pBegin, pBegin + Size);
        pIwsArray->resize (Size, 0);
    }

    // make destination sets for all Iws sorted and uniqued

    FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [State];
    DebugLogAssert (pIw2Dsts);

    pIw2Dsts->SortUniq ();
  }

  pBegin = m_finals.begin ();
  Size = m_finals.size ();

  if (false == FAIsSortUniqed (pBegin, Size)) {

      Size = FASortUniq (pBegin, pBegin + Size);
      m_finals.resize (Size, 0);
  }

  pBegin = m_initials.begin ();
  Size = m_initials.size ();

  if (false == FAIsSortUniqed (pBegin, Size)) {

      Size = FASortUniq (pBegin, pBegin + Size);
      m_initials.resize (Size, 0);
  }
}


const int FARSNfa_ar_judy::GetInitials (const int ** ppStates) const
{
  DebugLogAssert (ppStates);

  const int InitialStates = m_initials.size ();

  if (0 == InitialStates) {

    *ppStates = NULL;
    return 0;
  }

  *ppStates = m_initials.begin ();

  DebugLogAssert (true == FAIsSorted (*ppStates, InitialStates));

  return InitialStates;
}


const bool FARSNfa_ar_judy::IsFinal (const int State) const
{
  if (-1 != m_min_final) {

    const bool is_final = (State >= m_min_final);
    return is_final;

  } else {

    return -1 != FAFind_log (m_finals.begin (), m_finals.size (), State);
  }
}


const bool FARSNfa_ar_judy::IsFinal (const int * pStates, const int Size) const
{
  DebugLogAssert (0 < Size && pStates);

  if (-1 != m_min_final) {

    const int MaxState = pStates [Size - 1];
    const bool is_final = (MaxState >= m_min_final);

    return is_final;

  } else {

    const int * pFinals = m_finals.begin ();
    const int Finals = m_finals.size ();

    for (int i = 0; i < Size; ++i) {

        const int State = pStates [i];

        if (-1 != FAFind_log (pFinals, Finals, State)) {
            return true;
        }
    }

    return false;
  }
}


const int FARSNfa_ar_judy::GetFinals (const int ** ppStates) const
{
  DebugLogAssert (ppStates);

  const int FinalStates = m_finals.size ();

  if (0 == FinalStates) {

    *ppStates = NULL;
    return 0;
  }

  *ppStates = m_finals.begin ();

  DebugLogAssert (true == FAIsSorted (*ppStates, FinalStates));

  return FinalStates;
}


const int FARSNfa_ar_judy::GetMaxIw () const
{
  return m_IwCount - 1;
}


const int FARSNfa_ar_judy::GetMaxState () const
{
  DebugLogAssert (m_StateCount == m_state2iws.size ());
  return m_StateCount - 1;
}


const int FARSNfa_ar_judy::GetIWs (const int State,
                                   const int ** ppIws) const
{
  DebugLogAssert (ppIws);
  DebugLogAssert (0 <= State && m_state2iws.size () > (unsigned int) State);

  const FAArray_cont_t < int > * pIws = m_state2iws [State];
  DebugLogAssert (pIws);
///  DebugLogAssert (true == FAIsSorted (pIws->begin (), pIws->size ()));

  *ppIws = pIws->begin ();
  return pIws->size ();
}


const int FARSNfa_ar_judy::
    GetDest (
        const int State, 
        const int Iw,
        const int ** ppIwDstStates
    ) const
{
    if (FAFsmConst::NFA_DEAD_STATE == State) {
        return -1;
    }

    DebugLogAssert (ppIwDstStates);
    DebugLogAssert (0 <= State && m_state_iw2dsts.size () > (unsigned int) State);

    const FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [State];
    DebugLogAssert (pIw2Dsts);

    // returns -1, if there is no mapping
    const int DstStates = pIw2Dsts->Get (Iw, ppIwDstStates);

#ifndef NDEBUG
    if (-1 != DstStates) {

    DebugLogAssert (true == FAIsSorted (*ppIwDstStates, DstStates));
    }
#endif

    return DstStates;
}


const int FARSNfa_ar_judy::
    GetDest (
        const int State,
        const int Iw,
        int * pDstStates,
        const int MaxCount
    ) const
{
    if (FAFsmConst::NFA_DEAD_STATE == State) {
        return -1;
    }

    DebugLogAssert (0 <= State && m_state_iw2dsts.size () > (unsigned int) State);

    const FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [State];
    DebugLogAssert (pIw2Dsts);

    // returns -1, if there is no mapping
    const int * pIwDstStates;
    const int DstStates = pIw2Dsts->Get (Iw, &pIwDstStates);

#ifndef NDEBUG
    if (-1 != DstStates) {
    DebugLogAssert (FAIsSortUniqed (pIwDstStates, DstStates));
    }
#endif

    if (0 < DstStates && MaxCount >= DstStates && NULL != pDstStates) {

    memcpy (pDstStates, pIwDstStates, sizeof (int) * DstStates);
    }

    return DstStates;
}


void FARSNfa_ar_judy::SetTransition (const int FromState, 
                                     const int Iw, 
                                     const int DstState)
{
  // adjust m_state_iw2dsts map
  DebugLogAssert (0 <= FromState && m_state_iw2dsts.size () > (unsigned int) FromState);

  FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [FromState];
  DebugLogAssert (pIw2Dsts);

  if (FAFsmConst::NFA_DEAD_STATE != DstState) {

    // insert a new destination state
    pIw2Dsts->Add (Iw, DstState);

  } else {

    const int * pDstSet;

    // check whether the destination set has not already been mapped
    // if not then add an empty set
    if (-1 == pIw2Dsts->Get (Iw, &pDstSet)) {

      pIw2Dsts->Set (Iw, NULL, 0);
    }
  }

  /// the code below is in-efficient !!!

  // setup m_state2iws
  DebugLogAssert (m_state2iws.size () > (unsigned int) FromState);

  FAArray_cont_t < int > * pIws = m_state2iws [FromState];
  DebugLogAssert (pIws);

  pIws->push_back (Iw);
}


void FARSNfa_ar_judy::SetTransition (const int FromState,
                                     const int Iw,
                                     const int * pDstStates,
                                     const int DstStatesCount)
{
    if (-1 != DstStatesCount) {

        DebugLogAssert (pDstStates);
        DebugLogAssert (0 < DstStatesCount);
        DebugLogAssert (true == FAIsSorted (pDstStates, DstStatesCount));

        // adjust m_state_iw2dsts map
        DebugLogAssert (0 <= FromState && \
                m_state_iw2dsts.size () > (unsigned int) FromState);

        FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [FromState];
        DebugLogAssert (pIw2Dsts);

        // do not add DeadState into the destination states
        if (FAFsmConst::NFA_DEAD_STATE != pDstStates [0]) {

            // add all destination states
            pIw2Dsts->Set (Iw, pDstStates, DstStatesCount);

        } else {

            // add all destination states except the DeadState
            pIw2Dsts->Set (Iw, pDstStates + 1, DstStatesCount - 1);
        }

        // adjust m_state2iws map
        DebugLogAssert (m_state2iws.size () > (unsigned int) FromState);

        FAArray_cont_t < int > * pIws = m_state2iws [FromState];
        DebugLogAssert (pIws);

        pIws->push_back (Iw);

    } else {

        // we should remove the transition
        DebugLogAssert (m_state_iw2dsts.size () > (unsigned int) FromState && 
                0 <= FromState);

        FAMultiMap_judy * pIw2Dsts = m_state_iw2dsts [FromState];
        DebugLogAssert (pIw2Dsts);

        // remove the destination states
        pIw2Dsts->Remove (Iw);

        FAArray_cont_t < int > * pIws = m_state2iws [FromState];
        DebugLogAssert (pIws);

        int * pIwsBegin = pIws->begin ();
        int IwsCount = pIws->size ();
        bool Found = false;

        for (int i = 0; i < IwsCount; ++i) {
            if (Found) {
                DebugLogAssert (0 < i);
                pIwsBegin [i - 1] = pIwsBegin [i];
            } else if (Iw == pIwsBegin [i]) {
                Found = true;
            }
        }
        if (Found) {
            DebugLogAssert (0 < IwsCount);
            pIws->resize (IwsCount - 1);
        }
    }
}


void FARSNfa_ar_judy::PrepareState (const int State)
{
  // setup m_state2iws
  DebugLogAssert (0 <= State && m_state2iws.size () > (unsigned int) State);

  FAArray_cont_t < int > * pIws = m_state2iws [State];
  DebugLogAssert (pIws);

  if (false == FAIsSortUniqed (pIws->begin (), pIws->size ())) {

    const int IwsNewSize = FASortUniq (pIws->begin (), pIws->end ());
    pIws->resize (IwsNewSize, 0);
  }
}


void FARSNfa_ar_judy::SetInitials (const int * pStates, const int StateCount)
{
  DebugLogAssert (pStates);
  DebugLogAssert (StateCount);
  DebugLogAssert (true == FAIsSorted (pStates, StateCount));

  m_initials.resize (StateCount, 0);

  for (int i = 0; i < StateCount; ++i) {

    m_initials [i] = pStates [i];
  }
}


void FARSNfa_ar_judy::SetFinals (const int * pStates, const int StateCount)
{
  DebugLogAssert (0 < StateCount && pStates);
  DebugLogAssert (true == FAIsSorted (pStates, StateCount));

  if (0 < StateCount) {
    m_finals.resize (StateCount, 0);
    for (int i = 0; i < StateCount; ++i) {
      m_finals [i] = pStates [i];
    }

    m_min_final = -1;

    // see whether MaxFinal == m_StateCount - 1
    const int MaxFinal = pStates [StateCount - 1];
    if ((unsigned int) MaxFinal == m_StateCount - 1) {

      // see whether final states are contiguous
      const int Delta = (MaxFinal - *pStates) + 1;

      if (Delta == StateCount) {
          // allow to use fast version of IsFinal ()
          m_min_final = *pStates;
      }
    }
  }
}

}

