/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FASetUtils.h"
#include "FAUtils.h"

namespace BlingFire
{


FASetUtils::FASetUtils (FAAllocatorA * pAlloc) :
    m_pAlloc (pAlloc)
{
  DebugLogAssert (NULL != m_pAlloc);

  m_results.SetAllocator (m_pAlloc);
  m_results.Create ();

  /// create the default number of results
  SetResCount (3);
}


FASetUtils::~FASetUtils ()
{
  const int ResCount = GetResCount ();

  for (int i = 0; i < ResCount; ++i) {

      FAArray_cont_t < int > * pResSet = m_results [i];
      DebugLogAssert (pResSet);
      delete pResSet;
  }

  m_results.resize (0);
}


void FASetUtils::Clear ()
{
  const int ResCount = GetResCount ();

  if (3 < ResCount) {

    for (int i = 3; i < ResCount; ++i) {
        FAArray_cont_t < int > * pResSet = m_results [i];
        DebugLogAssert (pResSet);
        delete pResSet;
    }

    m_results.resize (3);
  }
}


const int FASetUtils::GetResCount () const
{
  return m_results.size ();
}


void FASetUtils::SetResCount (const int ResCount)
{
    const int OldResCount = m_results.size ();

    if (ResCount > OldResCount) {

        m_results.resize (ResCount);

        for (int i = OldResCount; i < ResCount; ++i) {

            FAArray_cont_t < int > * pResSet = NEW FAArray_cont_t < int > ();
            DebugLogAssert (pResSet);

            pResSet->SetAllocator (m_pAlloc);
            pResSet->Create ();
            m_results [i] = pResSet;
        }
    }
}


void FASetUtils::Difference (const int * pSet1, const int Size1, const int Res1,
                             const int * pSet2, const int Size2, const int Res2)
{
  DebugLogAssert (Res1 < GetResCount ());
  DebugLogAssert (Res2 < GetResCount ());

  // pSet1 and pSet2 have to be sorted and uniqed
  DebugLogAssert (true == FAIsSortUniqed (pSet1, Size1));
  DebugLogAssert (true == FAIsSortUniqed (pSet2, Size2));

  // keeps Set1 - Set2
  FAArray_cont_t < int > * pRes1 = m_results [Res1];
  DebugLogAssert (pRes1);
  pRes1->resize (0);

  // keeps Set2 - Set1
  FAArray_cont_t < int > * pRes2 = m_results [Res2];
  DebugLogAssert (pRes2);
  pRes2->resize (0);

  int i = 0;
  int j = 0;

  // process two sets until some is out of range
  while (i < Size1 && j < Size2) {

    const int V1 = pSet1 [i];
    const int V2 = pSet2 [j];

    if (V1 < V2) {

      i++;
      pRes1->push_back (V1);

    } else if (V1 > V2) {

      j++;
      pRes2->push_back (V2);

    } else {

      i++;
      j++;
    }
  } // of while

  // do the rest
  for (;i < Size1; ++i) {
    const int V1 = pSet1 [i];
    pRes1->push_back (V1);
  }
  for (;j < Size2; ++j) {
    const int V2 = pSet2 [j];
    pRes2->push_back (V2);
  }
}


void FASetUtils::Intersect (const int * pSet1, const int Size1,
                            const int * pSet2, const int Size2,
                            const int Res)
{
  DebugLogAssert (Res < GetResCount ());

  // pSet1 and pSet2 have to be sorted and uniqed
  DebugLogAssert (true == FAIsSortUniqed (pSet1, Size1));
  DebugLogAssert (true == FAIsSortUniqed (pSet2, Size2));

  // keeps Set1 & Set2
  FAArray_cont_t < int > * pRes3 = m_results [Res];
  DebugLogAssert (pRes3);
  pRes3->resize (0);

  int i = 0;
  int j = 0;

  // process two sets until some is out of range
  while (i < Size1 && j < Size2) {

    const int V1 = pSet1 [i];
    const int V2 = pSet2 [j];

    if (V1 < V2) {

      i++;

    } else if (V1 > V2) {

      j++;

    } else {

      i++;
      j++;
      pRes3->push_back (V1);
    }

  } // of while
}


void FASetUtils::
    SelfIntersect (const int * pSet1, const int Size1, const int Res)
{
  DebugLogAssert (Res < GetResCount ());

  FAArray_cont_t < int > * pSet2Arr = m_results [Res];
  DebugLogAssert (pSet2Arr);
  const int * pSet2 = pSet2Arr->begin ();
  const int Size2 = pSet2Arr->size ();

  // pSet1 and pSet2 have to be sorted and uniqed
  DebugLogAssert (true == FAIsSortUniqed (pSet1, Size1));
  DebugLogAssert (true == FAIsSortUniqed (pSet2, Size2));

  int i = 0;
  int j = 0;
  int k = 0; // resulting set size

  // process two sets until some is out of range
  while (i < Size1 && j < Size2) {

    const int V1 = pSet1 [i];
    const int V2 = pSet2 [j];

    if (V1 < V2) {

      i++;

    } else if (V1 > V2) {

      j++;

    } else {

      i++;
      j++;
      (*pSet2Arr) [k++] = V1;
    }

  } // of while

  DebugLogAssert (k <= Size2);
  pSet2Arr->resize (k);
}


// Res == 2
void FASetUtils::Union (const int * pSet1, const int Size1,
                        const int * pSet2, const int Size2,
                        const int Res)
{
  DebugLogAssert (Res < GetResCount ());
  // pSet1 and pSet2 have to be sorted and uniq !!!
  DebugLogAssert (true == FAIsSortUniqed (pSet1, Size1));
  DebugLogAssert (true == FAIsSortUniqed (pSet2, Size2));

  FAArray_cont_t < int > * pRes3 = m_results [Res];
  DebugLogAssert (pRes3);

  pRes3->resize (0);

  int i = 0;
  int j = 0;

  // process two sets until some is out of range
  while (i < Size1 && j < Size2) {

    const int V1 = pSet1 [i];
    const int V2 = pSet2 [j];

    int V;

    if (V1 < V2) {

      V = V1;
      i++;

    } else if (V1 > V2) {

      V = V2;
      j++;

    } else {

      V = V1;
      i++;
      j++;
    }

    pRes3->push_back (V);

  } // of while

  // do the rest
  for (;i < Size1; ++i) {

    const int V = pSet1 [i];
    pRes3->push_back (V);
  }
  for (;j < Size2; ++j) {

    const int V = pSet2 [j];
    pRes3->push_back (V);
  }
}


void FASetUtils::SelfUnion (const int * pSet1, const int Size1, const int Res)
{
  DebugLogAssert (Res < GetResCount ());
  DebugLogAssert (true == FAIsSortUniqed (pSet1, Size1));

  FAArray_cont_t < int > * pSet2Arr = m_results [Res];
  DebugLogAssert (pSet2Arr);

  const int Size2 = pSet2Arr->size ();

  int i = 0;
  int j = 0;

  // process two sets until some is out of range
  while (i < Size1 && j < Size2) {

    const int V1 = pSet1 [i];
    const int V2 = (*pSet2Arr) [j];

    int V;

    if (V1 < V2) {

      V = V1;
      i++;

    } else if (V1 > V2) {

      V = V2;
      j++;

    } else {

      V = V1;
      i++;
      j++;
    }

    pSet2Arr->push_back (V);

  } // of while

  // do the rest
  for (;i < Size1; ++i) {

    const int V = pSet1 [i];
    pSet2Arr->push_back (V);
  }
  for (;j < Size2; ++j) {

    const int V = (*pSet2Arr) [j];
    pSet2Arr->push_back (V);
  }

  /// copy memory from the pSet2Arr->begin () + Size2 into the pSet2Arr->begin ()
  int * pBegin2 = pSet2Arr->begin ();
  const int NewSize2 = pSet2Arr->size ();

  for (i = Size2; i < NewSize2; ++i) {

      const int V = pBegin2 [i];
      pBegin2 [i - Size2] = V;
  }
  pSet2Arr->resize (NewSize2 - Size2);
}


void FASetUtils::UnionN (const int ** pSetArray,
                         const int * pSizeArray, 
                         const int Size,
                         const int Res)
{
  DebugLogAssert (pSetArray);
  DebugLogAssert (pSizeArray);
  DebugLogAssert (Res < GetResCount ());

  FAArray_cont_t < int > * pRes = m_results [Res];
  DebugLogAssert (pRes);

  pRes->resize (0);

  for (int i = 0; i < Size; ++i) {

    const int * pSet = pSetArray [i];
    const int SetSize = pSizeArray [i];

    if (0 < SetSize) {

      const int OldSize = pRes->size ();
      pRes->resize (OldSize + SetSize);

      DebugLogAssert (pSet);
      memcpy (pRes->begin () + OldSize, pSet, SetSize * sizeof(int));
    }
  }

  const int NewSize = FASortUniq (pRes->begin (), pRes->end ());
  pRes->resize (NewSize);
}


const int FASetUtils::GetRes (const int ** ppSet, const int Res) const
{
  DebugLogAssert (Res < GetResCount ());
  DebugLogAssert (ppSet);

  const FAArray_cont_t < int > * pRes = m_results [Res];
  DebugLogAssert (pRes);

  *ppSet = pRes->begin ();
  return pRes->size ();
}


const int FASetUtils::GetSize (const int Res) const
{
    DebugLogAssert (Res < GetResCount ());

    const FAArray_cont_t < int > * pRes = m_results [Res];
    DebugLogAssert (pRes);

    return pRes->size ();
}


const int FASetUtils::GetRes (int ** ppSet, const int Res)
{
  DebugLogAssert (Res < GetResCount ());
  DebugLogAssert (ppSet);

  FAArray_cont_t < int > * pRes = m_results [Res];
  DebugLogAssert (pRes);

  *ppSet = pRes->begin ();
  return pRes->size ();
}


void FASetUtils::Resize (const int Size, const int Res)
{
    DebugLogAssert (Res < GetResCount ());
    DebugLogAssert (0 <= Size);

    FAArray_cont_t < int > * pRes = m_results [Res];
    DebugLogAssert (pRes);

    pRes->resize (Size);
}


void FASetUtils::SetRes (const int * pSet, const int Size, const int Res)
{
    DebugLogAssert (Res < GetResCount ());

    FAArray_cont_t < int > * pRes = m_results [Res];
    DebugLogAssert (pRes);

    pRes->resize (Size);

    if (0 < Size) {

        DebugLogAssert (pSet);
        memcpy (pRes->begin (), pSet, Size * sizeof (int));
    }
}


void FASetUtils::PushBackRes (const int * pSet, const int Size, const int Res)
{
  DebugLogAssert (Res < GetResCount ());

  FAArray_cont_t < int > * pRes = m_results [Res];
  DebugLogAssert (pRes);

  if (1 < Size) {

    DebugLogAssert (pSet);

    const int OldSize = pRes->size ();
    pRes->resize (OldSize + Size);
    memcpy (pRes->begin () + OldSize, pSet, sizeof (int) * Size);

  } else if (1 == Size) {

    DebugLogAssert (pSet);

    pRes->push_back (*pSet);
  }
}


void FASetUtils::PrepareRes (const int Res)
{
  DebugLogAssert (Res < GetResCount ());

  FAArray_cont_t < int > * pRes = m_results [Res];
  DebugLogAssert (pRes);

  const int Size = pRes->size ();

  if (1 < Size) {

      int * pBegin = pRes->begin ();

      if (false == FAIsSortUniqed (pBegin, Size)) {

          const int NewSize = FASortUniq (pBegin, pBegin + Size);
          pRes->resize (NewSize);
      }
  }
}

}
