/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAToken.h"

namespace BlingFire
{


FAToken::FAToken () :
  m_type (-1),
  m_offset (-1),
  m_length (-1)
{}

FAToken::FAToken (const int Type, const int Offset, const int Length) :
  m_type (Type),
  m_offset (Offset),
  m_length (Length)
{}

const int FAToken::GetType () const
{
  return m_type;
}

void FAToken::SetType (const int Type)
{
  m_type = Type;
}

const int FAToken::GetOffset () const
{
  return m_offset;
}

void FAToken::SetOffset (const int Offset)
{
  m_offset = Offset;
}

const int FAToken::GetLength () const
{
  return m_length;
}

void FAToken::SetLength (const int Length)
{
  m_length = Length;
}

}

