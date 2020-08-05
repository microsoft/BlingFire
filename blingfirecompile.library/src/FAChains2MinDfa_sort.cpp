/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAChains2MinDfa_sort.h"
#include "FAAllocatorA.h"
#include "FAUtils.h"

namespace BlingFire
{


FAChains2MinDfa_sort::FAChains2MinDfa_sort (FAAllocatorA * pAlloc) :
  m_MaxIw (0),
  m_MaxState (-1),
  m_initial (0),
  m_cp_pos (0),
  m_last_state (0),
  m_pChain (NULL),
  m_ChainSize (0),
  m_pAlloc (pAlloc)
{
  m_state2iws.SetAllocator (m_pAlloc);
  m_state2iws.Create ();

  m_state2dsts.SetAllocator (m_pAlloc);
  m_state2dsts.Create ();

  m_info.SetAllocator (m_pAlloc);
  m_info.Create ();

  m_finals.SetAllocator (m_pAlloc);
  m_finals.Create ();

  m_stack.SetAllocator (m_pAlloc);
  m_stack.Create ();

  m_info2state.SetAllocator (m_pAlloc);
  m_info2state.SetEncoder (&m_encoder);

  m_alphabet.SetAllocator (pAlloc);
  m_alphabet.Create ();

  m_iw2bool.SetAllocator (pAlloc);
  m_iw2bool.Create ();

  m_deleted.Create (pAlloc);

  m_initial = AddState ();
}


FAChains2MinDfa_sort::~FAChains2MinDfa_sort ()
{
  const int RealSize = m_state2iws.size ();
  DebugLogAssert ((unsigned int) RealSize == m_state2dsts.size ());

  for (int State = 0; State < RealSize; ++State) {

    FAArray_cont_t < int > * pIws =  m_state2iws [State];
    DebugLogAssert (pIws);

    pIws->Clear ();
    FAFree (m_pAlloc, pIws);

    FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
    DebugLogAssert (pDsts);

    pDsts->Clear ();
    FAFree (m_pAlloc, pDsts);
  }

  m_state2dsts.resize (0);
}


void FAChains2MinDfa_sort::Clear ()
{
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());

  for (int State = 0; State <= m_MaxState; ++State) {

    FAArray_cont_t < int > * pIws =  m_state2iws [State];
    DebugLogAssert (pIws);

    pIws->resize (AVE_ARCS_PER_STATE + 1, 0);
    pIws->resize (1);

    // make it non-final by default
    (*pIws) [0] = 0;

    FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
    DebugLogAssert (pDsts);

    pDsts->resize (AVE_ARCS_PER_STATE, 0);
    pDsts->resize (0);
  }

  m_info.resize (0);
  m_finals.resize (0);
  m_stack.resize (0);

  m_info2state.Clear ();
  m_deleted.clear ();

  m_MaxIw = 0;
  m_MaxState = -1;
  m_initial = AddState ();
}


const int FAChains2MinDfa_sort::AddState ()
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());

  // get the deleted state number if there is any
  if (false == m_deleted.empty ()) {

    const int State = * m_deleted.top ();
    m_deleted.pop ();

    return State;
  }

  m_MaxState++;

  // check whether we have to allocate a new state
  if ((unsigned int) m_MaxState == m_state2iws.size ()) {

    FAArray_cont_t <int> * pIws =
      (FAArray_cont_t <int> *) FAAlloc (m_pAlloc, sizeof (FAArray_cont_t <int>));
    pIws->SetAllocator (m_pAlloc);
    pIws->Create (AVE_ARCS_PER_STATE + 1);
    pIws->resize (1);
    // make it non-final by default
    (*pIws) [0] = 0;

    FAArray_cont_t <int> * pDsts =
      (FAArray_cont_t <int> *) FAAlloc (m_pAlloc, sizeof (FAArray_cont_t <int>));
    pDsts->SetAllocator (m_pAlloc);
    pDsts->Create (AVE_ARCS_PER_STATE);

    m_state2iws.push_back (pIws);
    m_state2dsts.push_back (pDsts);
  }

  return m_MaxState;
}


void FAChains2MinDfa_sort::DeleteState (const int State)
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  FAArray_cont_t < int > * pIws =  m_state2iws [State];
  DebugLogAssert (pIws);

  pIws->resize (1);
  // make it non-final by default
  (*pIws) [0] = 0;

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  pDsts->resize (0);

  m_deleted.push (State);
}


void FAChains2MinDfa_sort::MakeFinal (const int State)
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  FAArray_cont_t < int > * pIws =  m_state2iws [State];
  DebugLogAssert (pIws);
  DebugLogAssert (0 < pIws->size ());

  // make it final
  (*pIws) [0] = 1;
}


const int FAChains2MinDfa_sort::GetMaxState () const
{
  return m_MaxState;
}


const int FAChains2MinDfa_sort::GetMaxIw () const
{
  return m_MaxIw;
}


const int FAChains2MinDfa_sort::GetInitial () const
{
  return m_initial;
}


const int FAChains2MinDfa_sort::GetFinals (const int ** ppStates) const
{
  DebugLogAssert (ppStates);

  *ppStates = m_finals.begin ();
  return m_finals.size ();
}


const int FAChains2MinDfa_sort::GetIWs (const int ** ppIws) const
{
  DebugLogAssert (ppIws);

  *ppIws = m_alphabet.begin ();
  return m_alphabet.size ();
}


const int FAChains2MinDfa_sort::
    GetIWs (__out_ecount_opt (MaxIwCount) int * pIws, const int MaxIwCount) const
{
    const int * pIws2;
    const int IwCount = GetIWs (&pIws2);

    if (0 < IwCount && IwCount <= MaxIwCount) {
        memcpy (pIws, pIws2, sizeof (int) * IwCount);
    }

    return IwCount;
}


const bool FAChains2MinDfa_sort::IsFinal (const int State) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (State <= m_MaxState);

  FAArray_cont_t < int > * pIws =  m_state2iws [State];
  DebugLogAssert (pIws);
  DebugLogAssert (0 < pIws->size ());

  return 1 == (*pIws) [0];
}


const int FAChains2MinDfa_sort::GetIWs (const int State,
                                        const int ** ppIws) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);
  DebugLogAssert (ppIws);

  const FAArray_cont_t < int > * pIws = m_state2iws [State];
  DebugLogAssert (pIws);

  *ppIws = pIws->begin () + 1;
  return pIws->size () - 1;
}


const int FAChains2MinDfa_sort::GetDest (const int State, const int Iw) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  FAArray_cont_t < int > * pIws =  m_state2iws [State];
  DebugLogAssert (pIws);

  const int * pIwsPtr = pIws->begin () + 1;
  const int IwsCount = pIws->size () - 1;

  DebugLogAssert (true == FAIsSortUniqed (pIwsPtr, IwsCount));

  const int Idx = FAFind_log (pIwsPtr, IwsCount, Iw);

  if (-1 == Idx)
      return -1;

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  return (*pDsts) [Idx];
}


void FAChains2MinDfa_sort::AddChain (const int * pChain, const int Size)
{
  DebugLogAssert (pChain);
  DebugLogAssert (0 < Size);

  m_pChain = pChain;
  m_ChainSize = Size;

  CalcCommonPrefix ();

  if (true == HasChildren (m_last_state)) {

    ReplaceOrRegister (m_last_state);
  }

  AddSuffix ();
}


const bool FAChains2MinDfa_sort::HasChildren (const int State) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  const FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  return 0 != pDsts->size ();
}


void FAChains2MinDfa_sort::BuildStateInfo (const int State)
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  /// get Iws set (with final or non-final information) and Dsts set

  FAArray_cont_t < int > * pIws =  m_state2iws [State];
  DebugLogAssert (pIws);
  DebugLogAssert (0 < pIws->size ());

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  /// calc the size of the state information

  const int IwsCount = pIws->size ();
  int InfoSize = IwsCount;

  const int DstsCount = pDsts->size ();
  InfoSize += DstsCount;

  /// enlarge the info container and copy the information

  m_info.resize (InfoSize);

  memcpy (m_info.begin (), 
	  pIws->begin (), 
	  sizeof (int) * IwsCount);

  memcpy (m_info.begin () + IwsCount, 
	  pDsts->begin (), 
	  sizeof (int) * DstsCount);
}


const int * FAChains2MinDfa_sort::RegLookUp () const
{
  const int * pStateNum = m_info2state.Get (m_info.begin (), m_info.size ());
  return pStateNum;
}


void FAChains2MinDfa_sort::RegAdd (const int State)
{
  m_info2state.Add (m_info.begin (), m_info.size (), State);
}


void FAChains2MinDfa_sort::DeleteChild (const int State,
					const int OldChild,
					const int NewChild)
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  /// !!! This code should be changed if :
  ///       it is actualy needed to update a transition with MaxIw only !!!

  // 1. Change transitions:
  // foreach (State, Iw, OldChild) do
  //   E -= (State, Iw, OldChild);
  //   E += (State, Iw, NewChild);

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  int * pDstsPtr = pDsts->begin ();
  DebugLogAssert (pDstsPtr);

  const int DstCount = pDsts->size ();

  for (int i = 0; i < DstCount; ++i) {

    const int Dst = pDstsPtr [i];

    if (OldChild == Dst) {

      pDstsPtr [i] = NewChild;
    }
  }

  // 2. Free OldChild's memory

  DeleteState (OldChild);
}


/// non-recursive version
void FAChains2MinDfa_sort::ReplaceOrRegister (const int State)
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);
  DebugLogAssert (true == m_stack.empty ());

  /// put states with children into the stack

  m_stack.push_back (State);

  int Child = GetMaxChild (State);
  DebugLogAssert (0 <= Child && Child <= m_MaxState);

  while (true) {

    m_stack.push_back (Child);

    if (false == HasChildren (Child))
      break;

    Child = GetMaxChild (Child);
    DebugLogAssert (0 <= Child && Child <= m_MaxState);
  }

  /// process them in the reverse order
  Child = m_stack [m_stack.size () - 1];
  m_stack.pop_back ();

  int Parent = m_stack [m_stack.size () - 1];
  m_stack.pop_back ();

  while (true) {

    BuildStateInfo (Child);

    // check whether same state already exist in the register
    const int * pSameChild = RegLookUp ();

    if (NULL == pSameChild) {

      // add a new state
      RegAdd (Child);

    } else if (Child != *pSameChild) {

      // delete Child
      DeleteChild (Parent, Child, *pSameChild);
    }

    // check whether the stack is empty
    if (true == m_stack.empty ())
      break;

    // get the next pair
    Child = Parent;

    Parent = m_stack [m_stack.size () - 1];
    m_stack.pop_back ();
  }
}


const int FAChains2MinDfa_sort::GetDest_fast (const int State, 
                                              const int Iw) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);

  const int DstsSize = pDsts->size ();

  if (0 != DstsSize) {

    FAArray_cont_t < int > * pIws =  m_state2iws [State];
    DebugLogAssert (pIws);
    DebugLogAssert (pIws->size () - 1 == (unsigned int) DstsSize);

    // get the most latelly added Iw
    const int LastIw = (*pIws) [DstsSize];

    // if it is not equal then it does not exist
    if (LastIw == Iw) {

      const int MaxChild = (*pDsts) [DstsSize - 1];

      return MaxChild;
    }
  }

  return -1;
}


const int FAChains2MinDfa_sort::GetMaxChild (const int State) const
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= State && State <= m_MaxState);

  FAArray_cont_t < int > * pDsts =  m_state2dsts [State];
  DebugLogAssert (pDsts);
  DebugLogAssert (0 != pDsts->size ());

  const int MaxChild = (*pDsts) [pDsts->size () - 1];

  return MaxChild;
}


void FAChains2MinDfa_sort::CalcCommonPrefix ()
{
  DebugLogAssert (m_pChain);

  m_last_state = m_initial;

  for (m_cp_pos = 0; m_cp_pos < m_ChainSize; ++m_cp_pos) {

    const int Iw = m_pChain [m_cp_pos];

    /// optimized
    const int DstState = GetDest_fast (m_last_state, Iw);

    if (-1 != DstState)
      m_last_state = DstState;
    else
      break;
  }
}


void FAChains2MinDfa_sort::AddSuffix ()
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 <= m_last_state && m_last_state <= m_MaxState);

  // proceed with the new states
  for (; m_cp_pos < m_ChainSize; ++m_cp_pos) {

    // get next Iw
    const int Iw = m_pChain [m_cp_pos];

    /// get Iws set
    FAArray_cont_t < int > * pIws = m_state2iws [m_last_state];
    DebugLogAssert (pIws);

    /// get Dsts set
    FAArray_cont_t < int > * pDsts =  m_state2dsts [m_last_state];
    DebugLogAssert (pDsts);

    // create a new state
    m_last_state = AddState ();

    // add the transition
    pIws->push_back (Iw, AVE_ARCS_PER_STATE);
    pDsts->push_back (m_last_state, AVE_ARCS_PER_STATE);
  }

  // add a final state
  MakeFinal (m_last_state);
}


void FAChains2MinDfa_sort::Prepare ()
{
  DebugLogAssert (m_state2iws.size () == m_state2dsts.size ());
  DebugLogAssert (m_MaxState < (int) m_state2iws.size ());
  DebugLogAssert (0 == m_finals.size ());

  /// replace or register the rest

  ReplaceOrRegister (m_initial);

  /// go through the states and build m_finals, m_alphabet and m_MaxIw

  m_MaxIw = -1;
  m_iw2bool.resize (0);
  m_alphabet.resize (0);

  for (int State = 0; State <= m_MaxState; ++State) {

    if (true == FAChains2MinDfa_sort::IsFinal (State)) {

      m_finals.push_back (State);
    }

    const int * pIws;
    const int IwsCount = GetIWs (State, &pIws);

    if (0 < IwsCount) {

        DebugLogAssert (pIws);
        const int CurrMaxIw = pIws [IwsCount - 1];

        // see whether we have to update m_MaxIw
        if (m_MaxIw < CurrMaxIw) {

            m_iw2bool.resize (CurrMaxIw + 1);
            m_iw2bool.set_bits (m_MaxIw + 1, CurrMaxIw, false);

            m_MaxIw = CurrMaxIw;
        }

        // update alphabet
        for (int i = 0; i < IwsCount; ++i) {

            const int Iw = pIws [i];

            if (false == m_iw2bool.get_bit (Iw)) {
  
                m_iw2bool.set_bit (Iw, true);
                m_alphabet.push_back (Iw, 10);
            }
        }

    } // of if if (0 < IwsCount) ...

  } // of for (int State = 0; ...

  const int NewSize = FASortUniq (m_alphabet.begin (), m_alphabet.end ());
  m_alphabet.resize (NewSize, 0);
}


void FAChains2MinDfa_sort::SetMaxState (const int /*MaxState*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetMaxIw (const int /*MaxIw*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::Create ()
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetInitial (const int /*State*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetFinals (const int * /*pStates*/, 
                                      const int /*StateCount*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetIWs (const int * /*pIws*/, const int /*IwsCount*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetTransition (const int /*FromState*/, 
                                          const int /*Iw*/, 
                                          const int /*DstState*/)
{
    // not implemented
    DebugLogAssert (false);
}

void FAChains2MinDfa_sort::SetTransition (const int /*FromState*/, 
                                          const int * /*pIws*/,
                                          const int * /*pDstStates*/, 
                                          const int /*Count*/)
{
    // not implemented
    DebugLogAssert (false);
}

}
