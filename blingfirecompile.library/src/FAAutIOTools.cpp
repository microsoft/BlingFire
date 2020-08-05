/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAAutIOTools.h"
#include "FAAllocatorA.h"
#include "FAUtils.h"
#include "FARSNfaA.h"
#include "FARSDfaA.h"
#include "FAState2OwA.h"
#include "FAState2OwsA.h"
#include "FAMealyNfaA.h"
#include "FAMealyDfaA.h"
#include "FAStringTokenizer.h"
#include "FAFsmConst.h"
#include "FAException.h"

#include <string>

namespace BlingFire
{


FAAutIOTools::FAAutIOTools (FAAllocatorA * pAlloc) : 
    m_pAlloc (pAlloc)
{}


void FAAutIOTools::PrintHeader (std::ostream& os, const FARSNfaA * pNFA)
{
  FAAssert (pNFA, FAMsg::IOError);

  const int * pA;
  int i;

  const int MaxState = pNFA->GetMaxState ();
  os << "MaxState: " << MaxState << '\n';

  const int MaxIw = pNFA->GetMaxIw ();
  os << "MaxIw: " << MaxIw << '\n';

  // print out initial states
  const int InitialCount = pNFA->GetInitials (&pA);
  FAAssert (0 < InitialCount, FAMsg::IOError);
  FAAssert (pA, FAMsg::IOError);
  for (i = 0; i < InitialCount; ++i) {

    os << "initial: " << pA [i] << '\n';
  }

  // print out final states
  const int FinalCount = pNFA->GetFinals (&pA);
  FAAssert (0 < FinalCount, FAMsg::IOError);
  FAAssert (pA, FAMsg::IOError);
  for (i = 0; i < FinalCount; ++i) {

    os << "final: " << pA [i] << '\n';
  }
}


void FAAutIOTools::PrintHeader (std::ostream& os, const FARSDfaA * pDFA)
{
  FAAssert (pDFA, FAMsg::IOError);

  const int * pA;
  int i;

  const int MaxState = pDFA->GetMaxState ();
  os << "MaxState: " << MaxState << '\n';

  const int MaxIw = pDFA->GetMaxIw ();
  os << "MaxIw: " << MaxIw << '\n';

  // print out the initial state
  const int Initial = pDFA->GetInitial ();
  os << "initial: " << Initial << '\n';

  // print out final states
  const int FinalCount = pDFA->GetFinals (&pA);
  FAAssert (0 < FinalCount, FAMsg::IOError);
  FAAssert (pA, FAMsg::IOError);
  for (i = 0; i < FinalCount; ++i) {

    os << "final: " << pA [i] << '\n';
  }
}


void FAAutIOTools::
    PrintNfaCommon (std::ostream& os, const FARSNfaA * pNFA, const FAMealyNfaA * pOws)
{
  FAAssert (pNFA, FAMsg::IOError);

  const int * pA;
  int i;

  PrintHeader (os, pNFA);

  const int MaxState = pNFA->GetMaxState ();

  // print out the transitions
  for (i = 0; i <= MaxState; ++i) {

    // get the Iws
    const int IwCount = pNFA->GetIWs (i, &pA);

    for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

      FAAssert (pA, FAMsg::IOError);

      const int Iw = pA [iw_idx];
      const int * pDstStates;

      // get outgoing arcs for the Iw
      const int DstStates = pNFA->GetDest (i, Iw, &pDstStates);

      if (0 == DstStates) {

        if (pOws) {
            // print out the dead state transition
            os << i << ' ' << FAFsmConst::NFA_DEAD_STATE << ' ' << Iw  << " -1\n";
        } else {
            // print out the dead state transition
            os << i << ' ' << FAFsmConst::NFA_DEAD_STATE << ' ' << Iw << '\n';
        }

      } else {

        // make iteration thru the destination states

        FAAssert (pDstStates, FAMsg::IOError);

        for (int dst_idx = 0; dst_idx < DstStates; ++dst_idx) {

            const int DstState = pDstStates [dst_idx];

            if (pOws) {
                const int Ow = pOws->GetOw (i, Iw, DstState);
                os << i << ' ' << DstState << ' ' << Iw << ' ' << Ow << '\n';
            } else {
                os << i << ' ' << DstState << ' ' << Iw << '\n';
            }
        }

      } // of if (0 == DstStates) ...

    } // of for (int iw_idx = 0; ...

  } // of for (i = 0; ...

  os << '\n';
}


void FAAutIOTools::
    ReadNfaCommon (std::istream& is, FARSNfaA * pNFA, FAMealyNfaA * pOws)
{
    FAAssert (pNFA, FAMsg::IOError);

    std::string line;
    const char * pTmpStr;
    int TmpStrLen;
    bool res;
    int n = -1;
    int from = -1;
    int to = -1;
    int ow = -1;
    int prev_n = -1;
    int prev_from = -1;
    int MaxIw = -1;
    int MaxState = -1;

    FAStringTokenizer tokenizer;

    FAArray_cont_t < int > m_initials;
    FAArray_cont_t < int > m_finals;
    FAArray_cont_t < int > m_dst;
    FAArray_cont_t < int > m_ows;

    m_initials.SetAllocator (m_pAlloc);
    m_initials.Create ();

    m_finals.SetAllocator (m_pAlloc);
    m_finals.Create ();

    m_dst.SetAllocator (m_pAlloc);
    m_dst.Create ();

    // read the header
    for (int i = 0; i < 2; ++i) {

        if (!std::getline (is, line))
            return;

        if (is.eof ())
            return;

        tokenizer.SetString (line.c_str (), (int) line.length ());

        res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&n);
        FAAssert (res, FAMsg::IOError);

        if (0 == strncmp ("MaxState:", pTmpStr, TmpStrLen)) {
            MaxState = n;
            pNFA->SetMaxState (MaxState);
            continue;
        }
        if (0 == strncmp ("MaxIw:", pTmpStr, TmpStrLen)) {
            MaxIw = n;
            pNFA->SetMaxIw (MaxIw);
            continue;
        }
        FAAssert (0, FAMsg::IOError);
    }

    // get NFA ready
    pNFA->Create ();

    // read transitions
    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        if (line.empty ())
            break;

        const char * pLine = line.c_str ();
        FAAssert (pLine, FAMsg::IOError);

        if (0 == strncmp ("initial:", pLine, 8)) {
            n = atoi (&pLine [8]);
            FAAssert (0 <= n, FAMsg::IOError);
            FAAssert (MaxState >= n, FAMsg::IOError);
            m_initials.push_back (n);
            continue;
        }
        if (0 == strncmp ("final:", pLine, 6)) {
            n = atoi (&pLine [6]);
            FAAssert (0 <= n, FAMsg::IOError);
            FAAssert (MaxState >= n, FAMsg::IOError);
            m_finals.push_back (n);
            continue;
        }

        tokenizer.SetString (line.c_str (), (int) line.length ());

        res = tokenizer.GetNextInt (&from);
        res = res && tokenizer.GetNextInt (&to);
        res = res && tokenizer.GetNextInt (&n);
        FAAssert (res, FAMsg::IOError);

        // read Ow, if needed
        if (pOws) {
            res = tokenizer.GetNextInt (&ow);
            FAAssert (res, FAMsg::IOError);
        }

        // check what we have read
        FAAssert (0 <= from, FAMsg::IOError);
        FAAssert (MaxState >= from, FAMsg::IOError);
        FAAssert ((0 <= to && MaxState >= to ) || \
            FAFsmConst::NFA_DEAD_STATE == to, FAMsg::IOError);
        FAAssert (0 <= n, FAMsg::IOError);
        FAAssert (MaxIw >= n, FAMsg::IOError);

        // setup ow right away
        if (pOws && -1 != ow) {
            pOws->SetOw (from, n, to, ow);
        }

        // check whether we have to add transition
        if (prev_n != n || prev_from != from) {

            if (0 < m_dst.size ()) {
                // make sure the destination states are sorted
                const int NewSize = FASortUniq (m_dst.begin (), m_dst.end ());
                m_dst.resize (NewSize);
                // add transition
                pNFA->SetTransition (prev_from, prev_n, m_dst.begin (), m_dst.size ());
                m_dst.resize (0);
            }

            prev_n = n;
            prev_from = from;
        }

        m_dst.push_back (to);

    }// end of while ...

    // check whether there is something in m_dst
    if (0 < m_dst.size ()) {
        // make sure the destination states are sorted
        const int NewSize = FASortUniq (m_dst.begin (), m_dst.end ());
        m_dst.resize (NewSize);
        // add transition
        pNFA->SetTransition (from, n, m_dst.begin (), m_dst.size ());
    }

    int * pBegin = m_initials.begin ();
    int * pEnd = m_initials.end ();

    if (false == FAIsSortUniqed (pBegin, int (pEnd - pBegin))) {
        const int NewSize = FASortUniq (pBegin, pEnd);
        m_initials.resize (NewSize);
    }
    FAAssert (0 < m_initials.size (), FAMsg::IOError);
    pNFA->SetInitials (m_initials.begin (), m_initials.size ());

    pBegin = m_finals.begin ();
    pEnd = m_finals.end ();

    if (false == FAIsSortUniqed (pBegin, int (pEnd - pBegin))) {
        const int NewSize = FASortUniq (pBegin, pEnd);
        m_finals.resize (NewSize);
    }
    FAAssert (0 < m_finals.size (), FAMsg::IOError);
    pNFA->SetFinals (m_finals.begin (), m_finals.size ());

    // make Nfa ready to work
    pNFA->Prepare ();

    if (pOws)
        pOws->Prepare ();

    if (!FAIsValidNfa (pNFA)) {
        throw FAException (FAMsg::ObjectIsNotReady, __FILE__, __LINE__);
    }
}


void FAAutIOTools::
    PrintDfaCommon (std::ostream& os, const FARSDfaA * pDFA, const FAMealyDfaA * pOws)
{
  FAAssert (pDFA, FAMsg::IOError);

  PrintHeader (os, pDFA);

  // get the alphabet
  const int * pIws = NULL;
  const int IwCount = pDFA->GetIWs (&pIws);
  FAAssert (0 < IwCount && pIws, FAMsg::IOError);

  const int MaxState = pDFA->GetMaxState ();

  // print out the transitions
  for (int i = 0; i <= MaxState; ++i) {

    for (int iw_idx = 0; iw_idx < IwCount; ++iw_idx) {

      const int Iw = pIws [iw_idx];

      // get outgoing arcs for the Iw
      const int DstState = pDFA->GetDest (i, Iw);

      if (-1 == DstState)
        continue;

      // print out the transition
      if (pOws) {
        const int Ow = pOws->GetOw (i, Iw);
        os << i << ' ' << DstState << ' ' << Iw << ' ' << Ow << '\n';
      } else {
        os << i << ' ' << DstState << ' ' << Iw << '\n';
      }

    } // of for (int iw_idx = 0; ...
  } // of for (int i = 0; ...

  os << '\n';
}


void FAAutIOTools::
    ReadDfaCommon (std::istream& is, FARSDfaA * pDFA, FAMealyDfaA * pOws)
{
  FAAssert (pDFA, FAMsg::IOError);

  std::string line;
  const char * pTmpStr;
  int TmpStrLen;
  bool res;
  int n = -1;
  int from = -1;
  int to = -1;
  int ow = -1;
  int MaxIw = -1;
  int MaxState = -1;

  FAStringTokenizer tokenizer;

  FAArray_cont_t < int > m_finals;

  m_finals.SetAllocator (m_pAlloc);
  m_finals.Create ();

  // read the header
  for (int i = 0; i < 2; ++i) {

    if (!std::getline (is, line))
        return;

    if (is.eof ())
        return;

    tokenizer.SetString (line.c_str (), (int) line.length ());

    res = tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
    res = res && tokenizer.GetNextInt (&n);
    FAAssert (res, FAMsg::IOError);

    if (0 == strncmp ("MaxState:", pTmpStr, TmpStrLen)) {
        MaxState = n;
        pDFA->SetMaxState (MaxState);
        continue;
    }
    if (0 == strncmp ("MaxIw:", pTmpStr, TmpStrLen)) {
        MaxIw = n;
        pDFA->SetMaxIw (MaxIw);
        continue;
    }
    FAAssert (0, FAMsg::IOError);
  }

  // get DFA ready
  pDFA->Create ();

  // read transitions
  while (!is.eof ()) {

    if (!std::getline (is, line))
      break;

    if (line.empty ())
      break;

    const char * pLine = line.c_str ();
    FAAssert (pLine, FAMsg::IOError);

    if (0 == strncmp ("initial:", pLine, 8)) {

      n = atoi (&pLine [8]);
      FAAssert (0 <= n && MaxState >= n, FAMsg::IOError);

      pDFA->SetInitial (n);
      continue;
    }
    if (0 == strncmp ("final:", pLine, 6)) {

      n = atoi (&pLine [6]);
      FAAssert (0 <= n && MaxState >= n, FAMsg::IOError);

      m_finals.push_back (n);
      continue;
    }

    tokenizer.SetString (line.c_str (), (int) line.length ());

    res = tokenizer.GetNextInt (&from);
    res = res && tokenizer.GetNextInt (&to);
    res = res && tokenizer.GetNextInt (&n);
    FAAssert (res, FAMsg::IOError);

    if (pOws) {
        res = tokenizer.GetNextInt (&ow);
        FAAssert (res, FAMsg::IOError);
    }

    // check what we have read
    FAAssert (0 <= from && MaxState >= from, FAMsg::IOError);
    FAAssert ((0 <= to || FAFsmConst::DFA_DEAD_STATE == to) && \
        MaxState >= to, FAMsg::IOError);
    FAAssert (0 <= n && MaxIw >= n, FAMsg::IOError);

    // add transition
    pDFA->SetTransition (from, n, to);

    if (pOws && -1 != ow) {
        pOws->SetOw (from, n, ow);
    }
  }

  // setup finals
  int * pBegin = m_finals.begin ();
  int * pEnd = m_finals.end ();

  if (false == FAIsSortUniqed (pBegin, int (pEnd - pBegin))) {

    const int NewSize = FASortUniq (pBegin, pEnd);
    m_finals.resize (NewSize);
  }

  FAAssert (0 < m_finals.size (), FAMsg::IOError);
  pDFA->SetFinals (m_finals.begin (), m_finals.size ());

  // make containers ready
  pDFA->Prepare ();
  if (pOws) {
    pOws->Prepare ();
  }

  if (!FAIsValidDfa (pDFA)) {
    throw FAException (FAMsg::ObjectIsNotReady, __FILE__, __LINE__);
  }
}


void FAAutIOTools::Read (std::istream& is, FARSNfaA * pNFA)
{
    FAAutIOTools::ReadNfaCommon (is, pNFA, NULL);
}


void FAAutIOTools::Read (std::istream& is, FARSDfaA * pDFA)
{
    FAAutIOTools::ReadDfaCommon (is, pDFA, NULL);
}


void FAAutIOTools::Print (std::ostream& os, const FARSNfaA * pNFA)
{
    FAAutIOTools::PrintNfaCommon (os, pNFA, NULL);
}


void FAAutIOTools::Print (std::ostream& os, const FARSDfaA * pDFA)
{
    FAAutIOTools::PrintDfaCommon (os, pDFA, NULL);
}


void FAAutIOTools::Read (std::istream& is,
                         FARSDfaA * pDFA,
                         FAState2OwA * pOwMap)
{
    FAAssert (pDFA, FAMsg::IOError);
    FAAssert (pOwMap, FAMsg::IOError);

    // read RS DFA, now it is ready to use
    FAAutIOTools::Read (is, pDFA);

    // read State -> Ow map
    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int State = -1;
    int Ow = -1;

    FAStringTokenizer tokenizer;

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        tokenizer.SetString (line.c_str (), (int) line.length ());

        bool res = tokenizer.GetNextInt (&State);
        res = res && tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&Ow);
        FAAssert (res, FAMsg::IOError);
        FAAssert (0 == strncmp ("->", pTmpStr, TmpStrLen), FAMsg::IOError);
        FAAssert (0 <= State && pDFA->GetMaxState () >= State, FAMsg::IOError);

        pOwMap->SetOw (State, Ow);
    }
}


void FAAutIOTools::Print (std::ostream& os, 
                          const FARSDfaA * pDFA, 
                          const FAState2OwA * pOwMap)
{
    FAAssert (pDFA, FAMsg::IOError);
    FAAssert (pOwMap, FAMsg::IOError);

    // print RS DFA
    FAAutIOTools::Print (os, pDFA);

    // print State -> Output weight map
    const int MaxState = pDFA->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int Ow = pOwMap->GetOw (State);

        if (-1 != Ow) {
            os << State << " -> " << Ow << '\n';
        }
    }
    os << '\n';
}


void FAAutIOTools::Read (std::istream& is, 
                         FARSDfaA * pDFA, 
                         FAState2OwsA * pOwsMap)
{
    FAAssert (pDFA, FAMsg::IOError);
    FAAssert (pOwsMap, FAMsg::IOError);

    // read DFA, now it is ready to use
    FAAutIOTools::Read (is, pDFA);

    std::string line;
    const char * pTmpStr = NULL;
    int TmpStrLen = 0;
    int State = -1;
    int Size = -1;

    FAStringTokenizer tokenizer;

    FAArray_cont_t < int > ows;
    ows.SetAllocator (m_pAlloc);
    ows.Create ();

    while (!is.eof ()) {

        if (!std::getline (is, line))
            break;

        // drop reading on empty line
        if (line.empty ())
            break;

        tokenizer.SetString (line.c_str (), (int) line.length ());

        bool res = tokenizer.GetNextInt (&State);
        res = res && tokenizer.GetNextStr (&pTmpStr, &TmpStrLen);
        res = res && tokenizer.GetNextInt (&Size);
        FAAssert (res, FAMsg::IOError);
        FAAssert (0 <= State && pDFA->GetMaxState () >= State, FAMsg::IOError);

        FAAssert (0 == strncmp ("->", pTmpStr, TmpStrLen), FAMsg::IOError);
        FAAssert (0 <= Size, FAMsg::IOError);

        ows.resize (Size);

        res = tokenizer.GetArray (ows.begin (), ows.size ());
        FAAssert (res, FAMsg::IOError);

        pOwsMap->SetOws (State, ows.begin (), Size);
    }
}


void FAAutIOTools::Print (std::ostream& os, 
                          const FARSDfaA * pDFA, 
                          const FAState2OwsA * pOwsMap)
{
    FAAssert (pDFA, FAMsg::IOError);
    FAAssert (pOwsMap, FAMsg::IOError);

    FAAutIOTools::Print (os, pDFA);

    const int MaxState = pDFA->GetMaxState ();

    for (int State = 0; State <= MaxState; ++State) {

        const int * pOws;
        const int OwsCount = pOwsMap->GetOws (State, &pOws);

        if (0 < OwsCount) {

            FAAssert (pOws, FAMsg::IOError);

            os << State << " -> " << OwsCount ;

            for (int i = 0; i < OwsCount; ++i) {

                const int Ow = pOws [i];
                os << ' ' << Ow;
            }
            os << '\n';
        }
    }
    os << '\n';
}


void FAAutIOTools::
    Read (std::istream& is, FARSNfaA * pNFA, FAMealyNfaA * pOws)
{
    FAAutIOTools::ReadNfaCommon (is, pNFA, pOws);
}


void FAAutIOTools::
    Print (std::ostream& os, const FARSNfaA * pNFA, const FAMealyNfaA * pOws)
{
    FAAutIOTools::PrintNfaCommon (os, pNFA, pOws);
}


void FAAutIOTools::
    Read (std::istream& is, FARSDfaA * pDFA, FAMealyDfaA * pOws)
{
    FAAutIOTools::ReadDfaCommon (is, pDFA, pOws);
}


void FAAutIOTools::
    Print (std::ostream& os, const FARSDfaA * pDFA, const FAMealyDfaA * pOws)
{
    FAAutIOTools::PrintDfaCommon (os, pDFA, pOws);
}

}
