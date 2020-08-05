/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FARegexpLexer_triv.h"
#include "FAToken.h"
#include "FAUtils.h"
#include "FARegexpTree.h"

namespace BlingFire
{


FARegexpLexer_triv::FARegexpLexer_triv () :
  m_pRegexp (NULL),
  m_Length (0),
  m_pTokens (NULL)
{
  for (unsigned i = 0; i < AsciiSize; ++i) {

    m_char2type [i] = FARegexpTree::TYPE_SYMBOL;
  }

  m_char2type [(unsigned char)'.'] = FARegexpTree::TYPE_ANY;
  m_char2type [(unsigned char)'('] = FARegexpTree::TYPE_LBR;
  m_char2type [(unsigned char)')'] = FARegexpTree::TYPE_RBR;
  m_char2type [(unsigned char)'|'] = FARegexpTree::TYPE_DISJUNCTION;
  m_char2type [(unsigned char)'*'] = FARegexpTree::TYPE_ITERATION;
  m_char2type [(unsigned char)'+'] = FARegexpTree::TYPE_NON_EMPTY_ITERATION;
  m_char2type [(unsigned char)'?'] = FARegexpTree::TYPE_OPTIONAL;
  m_char2type [(unsigned char)'^'] = FARegexpTree::TYPE_L_ANCHOR;
  m_char2type [(unsigned char)'$'] = FARegexpTree::TYPE_R_ANCHOR;
  m_char2type [(unsigned char)'<'] = FARegexpTree::TYPE_LTRBR;
  m_char2type [(unsigned char)'>'] = FARegexpTree::TYPE_RTRBR;

  /// not used for now
  /// m_char2type [(unsigned char)'['] = FARegexpTree::TYPE_BOUND_LBR;
  /// m_char2type [(unsigned char)','] = FARegexpTree::TYPE_COMMA;
  /// m_char2type [(unsigned char)']'] = FARegexpTree::TYPE_BOUND_RBR;
  /// m_char2type [(unsigned char)'{'] = FARegexpTree::TYPE_INTERVAL_LBR;
  /// m_char2type [(unsigned char)'}'] = FARegexpTree::TYPE_INTERVAL_RBR;

}

FARegexpLexer_triv::~FARegexpLexer_triv ()
{}

void FARegexpLexer_triv::SetRegexp (const char * pRegexp, const int Length)
{
  m_pRegexp = pRegexp;
  m_Length = Length;
}

void FARegexpLexer_triv::SetTokens (FAArray_cont_t < FAToken > * pTokens)
{
  m_pTokens = pTokens;
}

const bool FARegexpLexer_triv::IsEscaped (const int Pos) const
{
    DebugLogAssert (m_pRegexp);
    DebugLogAssert (0 <= Pos && m_Length > Pos);

    return FAIsEscaped (Pos, m_pRegexp, m_Length);
}

const bool FARegexpLexer_triv::IsSpecial (const char C) const
{
  return (FARegexpTree::TYPE_SYMBOL != GetType (C));
}

const bool FARegexpLexer_triv::IsHex (const char C) const
{
    return isdigit ((unsigned char) C) || \
        ( 'A' <= C && C <= 'F' ) ||
        ( 'a' <= C && C <= 'f' );
}

const int FARegexpLexer_triv::GetType (const char C) const
{
  if (AsciiSize <= (const unsigned char)C)
    return FARegexpTree::TYPE_SYMBOL;

  return m_char2type [(const unsigned char)C];
}

void FARegexpLexer_triv::PushToken (const int Type,
				    const int Offset,
				    const int Length)
{
  m_pTokens->push_back (FAToken (Type, Offset, Length));
}


const int FARegexpLexer_triv::FindToken (const int Pos, int * pLength) const
{
  DebugLogAssert (pLength);
  DebugLogAssert (0 <= Pos && Pos < m_Length);

  /// get next symbol
  char c = m_pRegexp [Pos];
  /// setup token length
  int length = 0;

  /// suppose we have a digit
  while (isdigit ((unsigned char) c)) {

    length++;

    if (Pos + length == m_Length)
      break;

    c = m_pRegexp [Pos + length];

  } // of while

  /// have we found a digit?
  if (0 < length) {

    /// return it
    *pLength = length;
    return FARegexpTree::TYPE_SYMBOL;
  }

  /// see whether it is a special symbol
  if (IsSpecial (c)) {

    /// get char type
    const int Type = GetType (c);
    length = 1;

    /// if it is LRB then int should follow, e.g. <7
    if (FARegexpTree::TYPE_LTRBR == Type && Pos + length != m_Length) {

        c = m_pRegexp [Pos + length];

        while (isdigit ((unsigned char) c)) {
            length++;
            if (Pos + length == m_Length) {
                break;
            }
            c = m_pRegexp [Pos + length];
        }
    }

    /// return it
    *pLength = length;
    return Type;
  }

  return -1;
}


void FARegexpLexer_triv::Process ()
{
  DebugLogAssert (m_pTokens);
  DebugLogAssert (m_pRegexp);
  DebugLogAssert (0 <= m_Length);

  int pos = 0;
  int length;

  while (pos < m_Length) {

    const int Type = FindToken (pos, &length);

    if (-1 != Type) {

      PushToken (Type, pos, length);
      pos += length;

    } else {

      pos++;
    }

  } // of while
}

}

