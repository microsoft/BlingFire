/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAFsmConst.h"
#include "FAPrintUtils.h"
#include "FARegexpTree.h"
#include "FARegexpTree2Funcs.h"
#include "FAWREToken.h"
#include "FAToken.h"
#include "FAUtf8Utils.h"
#include "FATagSet.h"
#include "FAException.h"
#include "FALimits.h"

#include <string>
#include <iomanip>

namespace BlingFire
{


void FAPrintRegexpTree (
        std::ostream& os,
        const FARegexpTree * pTree,
        const char * pRegexp
    )
{
  DebugLogAssert (pTree);
  DebugLogAssert (pRegexp);

  os << "digraph RegexpTree {\n";

  const int MaxNodeId = pTree->GetMaxNodeId ();

  for (int i = 0; i <= MaxNodeId; ++i) {

    if (pTree->IsDeleted (i))
        continue;

    const int NodeType = pTree->GetType (i);

    os << i << " [label=\"" << i << " : ";

    const int TrBr = pTree->GetTrBr (i);
    if (-1 != TrBr) {
        os << " <" << TrBr << " >";
    }

    switch (NodeType) {

    case FARegexpTree::TYPE_SYMBOL:
      {
        const int Offset = pTree->GetOffset (i);
        const int Length = pTree->GetLength (i);

        if (-1 != Offset) {

            std::string label;
            label.assign (pRegexp, Offset, Length);
            os << label;

        } else {

            os << '$';
        }
        break;
      }
    case FARegexpTree::TYPE_ANY:
      {
        os << "ANY";
        break;
      }
    case FARegexpTree::TYPE_CONCAT:
      {
        os << "CONCAT";
        break;
      }
    case FARegexpTree::TYPE_DISJUNCTION:
      {
        os << "DISJUNCTION";
        break;
      }
    case FARegexpTree::TYPE_ITERATION:
      {
        os << "ITERATION";
        break;
      }
    case FARegexpTree::TYPE_NON_EMPTY_ITERATION:
      {
        os << "NON_EMPTY_ITERATION";
        break;
      }
    case FARegexpTree::TYPE_OPTIONAL:
      {
        os << "OPTIONAL";
        break;
      }
    case FARegexpTree::TYPE_EPSILON:
      {
        os << "EPSILON";
        break;
      }
    }; // of switch

    const int ParentId = pTree->GetParent (i);
    if (-1 != ParentId) {
      os << " P:" << ParentId << ' ';
    }

    os << "\"];\n";

    const int LeftId = pTree->GetLeft (i);
    if (-1 != LeftId) {
      os << i << " -> " << LeftId << " [label=\"0\"];\n";
    }
    const int RightId = pTree->GetRight (i);
    if (-1 != RightId) {
      os << i << " -> " << RightId << " [label=\"1\"];\n";
    }

  } // of for

  os << "}\n";
}


void FAPrintRegexpFuncs (
        std::ostream& os,
        const FARegexpTree2Funcs * pFuncs,
        const FARegexpTree * pTree
    )
{
  DebugLogAssert (pFuncs);
  DebugLogAssert (pTree);

  const int * pSet;
  int Size;
  int i;

  os << "digraph RegexpFuncs {\n";

  const int MaxNodeId = pTree->GetMaxNodeId ();

  for (i = 0; i <= MaxNodeId; ++i) {

    os << i << " [label=\"";

    // print first pos
    Size = pFuncs->GetFirstPos (i, &pSet);
    FAPrintArray (os, pSet, Size);

    os << ", ";

    // print last pos
    Size = pFuncs->GetLastPos (i, &pSet);
    FAPrintArray (os, pSet, Size);

    os << "\"];\n";

    const int LeftId = pTree->GetLeft (i);
    if (-1 != LeftId) {
      os << i << " -> " << LeftId << " [label=\"0\"];\n";
    }
    const int RightId = pTree->GetRight (i);
    if (-1 != RightId) {
      os << i << " -> " << RightId << " [label=\"1\"];\n";
    }

  } // of for

  // print out follow pos
  os << "node [ shape = text ];\n"
     << MaxNodeId + 1 << " [ label = \"FollowPos:\\n";

  const int MaxPos = pFuncs->GetMaxPos ();

  // i < MaxPos is not a mistake as follow pos is not defined for MaxPos
  for (i = 0; i < MaxPos; ++i) {

    os << i << " -> ";
    Size = pFuncs->GetFollowPos (i, &pSet);
    FAPrintArray (os, pSet, Size);
    os << "\\n";
  }
  os << "\"];\n";

  // end of printing
  os << "}\n";
}


std::ostream& operator <<(std::ostream& os, const FAToken& Token)
{
  os << "[" <<  Token.GetType ()
     << ", " << Token.GetOffset ()
     << ", " << Token.GetLength () << "]";

  return os;
}


std::ostream& operator <<(std::ostream& os, const FAWREToken& WREToken)
{
    const char * pStr = WREToken.GetStr ();
    DebugLogAssert (pStr);

    int i;

    const bool WordsDisj = WREToken.GetWordsDisj ();
    const int WordCount = WREToken.GetWordCount ();

    for (i = 0; i < WordCount; ++i) {

        const FAToken * pSimpleToken = WREToken.GetWordToken (i);
        DebugLogAssert (pSimpleToken);

        const int Type = pSimpleToken->GetType ();
        const int Offset = pSimpleToken->GetOffset ();
        const int Length = pSimpleToken->GetLength ();

        std::string text (pStr + Offset, Length);

        if (FAWREToken::TT_POSITIVE_WORD == Type)
            os << "+";
        else 
            os << "-";

        os << '"' << text << "_" << Length << '"';

        if (i + 1 != WordCount && true == WordsDisj)
            os << '|';
    }

    os << '/';

    const bool RegexpsDisj = WREToken.GetRegexpsDisj ();
    const int RegexpCount = WREToken.GetRegexpCount ();

    for (i = 0; i < RegexpCount; ++i) {

        const FAToken * pSimpleToken = WREToken.GetRegexpToken (i);
        DebugLogAssert (pSimpleToken);

        const int Type = pSimpleToken->GetType ();
        const int Offset = pSimpleToken->GetOffset ();
        const int Length = pSimpleToken->GetLength ();

        std::string text (pStr + Offset, Length);

        if (FAWREToken::TT_POSITIVE_REGEXP == Type)
            os << "+";
        else 
            os << "-";

        os << '\'' << text << "_" << Length << '\'';

        if (i + 1 != RegexpCount && true == RegexpsDisj)
            os << '|';
    }

    os << '/';

    const bool DictsDisj = WREToken.GetDictsDisj ();
    const int DictCount = WREToken.GetDictCount ();

    for (i = 0; i < DictCount; ++i) {

        const FAToken * pSimpleToken = WREToken.GetDictToken (i);
        DebugLogAssert (pSimpleToken);

        const int Type = pSimpleToken->GetType ();
        const int Offset = pSimpleToken->GetOffset ();
        const int Length = pSimpleToken->GetLength ();

        std::string text (pStr + Offset, Length);

        if (FAWREToken::TT_POSITIVE_DICT == Type)
            os << "+";
        else 
            os << "-";

        os << text << "_" << Length;

        if (i + 1 != DictCount) {
            if (DictsDisj)
                os << '|';
            else
                os << ',';
        }
    }

    return os;
}


template<>
void FAPrintArray (std::ostream& os, const FAToken * pA, const int Size)
{
  if (NULL != pA) {

    os << "[";
    for (int i = 0; i < Size; ++i) {

      os << " " << pA [i];
    }
    os << " ]";

  } else {

    os << "<NULL>";
  }
}

template<>
void FAPrintArray (std::ostream& os, const FAWREToken * pA, const int Size)
{
  if (NULL != pA) {

    os << "[";
    for (int i = 0; i < Size; ++i) {

      os << " " << pA [i];
    }
    os << " ]";

  } else {

    os << "<NULL>";
  }
}


template<>
void FAPrintArray (std::ostream& os, const float * pA, const int Size)
{
  if (NULL != pA) {

    os << "[";
    for (int i = 0; i < Size; ++i) {

      os << " " << pA [i];
    }
    os << " ]";

  } else {

    os << "<NULL>";
  }
}


void FAPrintWordList (
        std::ostream& os, 
        const int * pWordList, 
        const int Count,
        const char Delim
    )
{
    if (NULL != pWordList) {

        if (0 < Count) {

            const int MaxStrLen = Count * FAUtf8Const::MAX_CHAR_SIZE;
            char * pTmpStr = NEW char [MaxStrLen];
            DebugLogAssert (pTmpStr);

            const int TmpStrLen = \
                FAArrayToStrUtf8 (pWordList, Count, pTmpStr, MaxStrLen);

            if (0 > TmpStrLen || MaxStrLen < TmpStrLen) {
                throw FAException (FAMsg::IOError, __FILE__, __LINE__);
            }

            for (int i = 0; i < TmpStrLen; ++i) {

                const char Symbol = pTmpStr [i];

                if (0 != Symbol)
                    os << Symbol;
                else
                    os << Delim;
            }

            delete [] pTmpStr;

        } else {

            os << "<EMPTY>" << Delim;
        }

    } else {

        os << "<NULL>";
    }
}


void FAPrintSplitting (
        std::ostream& os, 
        const int * pChain,
        const int ChainSize,
        const int * pEnds,
        const int EndsCount,
        const char Delim
    )
{
    FAAssert (pChain && 0 < ChainSize, FAMsg::InvalidParameters);

    if (-1 == EndsCount || 0 == EndsCount) {

        os << "NONE";

    } else {

        int BeginPos = 0;
        for (int i = 0; i < EndsCount; ++i) {

            const int EndPos = pEnds [i];
            const int SegmentSize = EndPos - BeginPos + 1;
            DebugLogAssert (0 < SegmentSize && SegmentSize <= ChainSize);

            if (0 != i) {
                os << Delim;
            }

            FAPrintWordList (os, pChain + BeginPos, SegmentSize);
            BeginPos = EndPos + 1;
        }
    }
}


void FAPrintTagList (
        std::ostream& os,
        const FATagSet * pTagSet,
        const int * pTags,
        const int Count,
        const char Delim
    )
{
    if (-1 == Count || 0 == Count) {

        os << "NONE";

    } else {

        if (pTagSet) {
            for (int i = 0; i < Count; ++i) {

                DebugLogAssert (pTags);
                const int Tag = pTags [i];

                const char * pTagStr;
                const int TagStrLen = pTagSet->Tag2Str (Tag, &pTagStr);

                if (0 != i) {
                    os << Delim;
                }
                if (0 < TagStrLen) {
                    std::string TagText (pTagStr, TagStrLen);
                    os << TagText;
                } else {
                    os << "???" << Tag;
                }
            }
        } else {
            for (int i = 0; i < Count; ++i) {

                DebugLogAssert (pTags);
                const int Tag = pTags [i];

                if (0 != i) {
                    os << Delim;
                }

                os << Tag;
            }
        } // of if (pTagSet) ...
    } // of if (-1 == Count || 0 == Count) ...
}


void FAPrintTaggedWord (
        std::ostream& os,
        const FATagSet * pTagSet,
        const int * pWord,
        const int WordLen,
        const int Tag
    )
{
    int j;
    const int MaxSize = FAUtf8Const::MAX_CHAR_SIZE + 2;
    char Utf8Symbol [MaxSize];

    if (!pTagSet) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }
    if (0 >= WordLen) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    // print the word
    for (j = 0; j < WordLen; ++j) {

        int Symbol = pWord [j];

        if (FAFsmConst::CHAR_SPACE == Symbol)
            Symbol = FAFsmConst::CHAR_MWE_DELIM;

        char * pEnd = FAIntToUtf8 (Symbol, Utf8Symbol, MaxSize - 1);
        *pEnd = 0;

        os << Utf8Symbol;
    }

    // print word-tag delimiter
    os << char (FAFsmConst::CHAR_TAG_DELIM);

    const char * pTagStr;
    const int TagStrLen = pTagSet->Tag2Str (Tag, &pTagStr);

    if (0 >= Tag || 0 >= TagStrLen) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    // print the tag
    for (j = 0; j < TagStrLen; ++j) {
        DebugLogAssert (FAFsmConst::CHAR_TAG_DELIM != pTagStr [j]);
        os << pTagStr [j];
    }
}


void FAPrintWord (
        std::ostream& os,
        const int * pWord,
        const int WordLen
    )
{
    FAAssert (0 <= WordLen && WordLen <= FALimits::MaxWordLen, \
        FAMsg::InvalidParameters);

    if (0 < WordLen) {

        // allocate a fixed buffer for a single word
        const int MaxStrLen = \
            FALimits::MaxWordLen * FAUtf8Const::MAX_CHAR_SIZE + 1;
        char WordStr [MaxStrLen];

        // convert UTF-32 --> UTF-8
        const int StrLen = \
            FAArrayToStrUtf8 (pWord, WordLen, WordStr, MaxStrLen);
        FAAssert (0 < StrLen && StrLen < MaxStrLen - 1, \
            FAMsg::InvalidParameters);

        // change space into MWE delimiter
        for (int i = 0; i < StrLen; ++i) {
            if (WordStr [i] == FAFsmConst::CHAR_SPACE) {
                WordStr [i] = FAFsmConst::CHAR_MWE_DELIM ;
            }
        }

        // add 0 and print the word out
        WordStr [StrLen] = 0;
        os << WordStr ;

    } else {

        os << "<EMPTY>" ;
    }
}



void FAPrintValue (
        std::ostream& os,
        const int Value,
        const int Width,
        const bool Hex
    )
{
    if (!Hex) {
        os << std::setw (Width) << std::setfill ('0') << std::dec << Value;
    } else {
        os << std::setw (Width) << std::setfill ('0') << std::hex << Value;
    }
}


void FAPrintChain (
        std::ostream& os,
        const int * pChain,
        const int ChainSize,
        const int Dir,
        const int Width,
        const bool Hex
    )
{
    DebugLogAssert (pChain && 0 <= ChainSize);

    if (0 >= ChainSize) {
        return;
    }

    if (FAFsmConst::DIR_L2R == Dir) {

        FAPrintValue (os, pChain [0], Width, Hex);
        for (int i = 1; i < ChainSize; ++i) {
            os << ' ';
            FAPrintValue (os, pChain [i], Width, Hex);
        }

    } else if (FAFsmConst::DIR_R2L == Dir) {

        FAPrintValue (os, pChain [ChainSize - 1], Width, Hex);
        for (int i = ChainSize - 2; i >= 0; --i) {
            os << ' ';
            FAPrintValue (os, pChain [i], Width, Hex);
        }

    } else {

        DebugLogAssert (FAFsmConst::DIR_AFF == Dir);

        FAPrintValue (os, pChain [ChainSize - 1], Width, Hex);
        const int ChainSize_2 = ChainSize >> 1;

        for (int i = 0; i < ChainSize_2; ++i) {

            os << ' ';
            FAPrintValue (os, pChain [i], Width, Hex);

            const int j = ChainSize - 2 - i;
            if (j > i) {
                os << ' ';
                FAPrintValue (os, pChain [j], Width, Hex);
            }
        }
    } // of if ...
}


void FAPrintHyphWord (
        std::ostream& os,
        const int * pWord,
        const int * pHyphs,
        const int WordLen
    )
{
    const int MaxSize = FAUtf8Const::MAX_CHAR_SIZE + 2;
    char Utf8Symbol [MaxSize];

    if (0 >= WordLen || FALimits::MaxWordLen < WordLen) {
        throw FAException (FAMsg::InvalidParameters, __FILE__, __LINE__);
    }

    for (int i = 0; i < WordLen; ++i) {

        const int Symbol = pWord [i];

        char * pEnd = FAIntToUtf8 (Symbol, Utf8Symbol, MaxSize - 1);
        FAAssert (NULL != pEnd, FAMsg::IOError);
        *pEnd = 0;

        os << Utf8Symbol;

        const int HyphId = pHyphs [i];

        if (FAFsmConst::HYPH_NO_HYPH < HyphId) {

            os << '[';

            const int Op = HyphId & 0xf;
            const int HyphSymbol = HyphId >> 4;

            if (0 < HyphSymbol) {
                char * pEndHyph = FAIntToUtf8 (HyphSymbol, Utf8Symbol, MaxSize - 1);
                FAAssert (NULL != pEndHyph, FAMsg::IOError);
                *pEndHyph = 0;
            } else {
                Utf8Symbol [0] = 0;
            }

            switch (Op) {
                case FAFsmConst::HYPH_SIMPLE_HYPH:
                    os << '=';
                    break;

                case FAFsmConst::HYPH_ADD_BEFORE:
                    os << Utf8Symbol << '=';
                    break;

                case FAFsmConst::HYPH_CHANGE_BEFORE:
                    os << 'X' << Utf8Symbol << '=';
                    break;

                case FAFsmConst::HYPH_DELETE_BEFORE:
                    os << "X=";
                    break;

                case FAFsmConst::HYPH_CHANGE_AFTER:
                    os << "=X" << Utf8Symbol;
                    break;

                case FAFsmConst::HYPH_DEL_AND_CHANGE:
                    os << "X=X" << Utf8Symbol;
                    break;

                default:
                    // unknown Op value
                    throw FAException (FAMsg::IOError, __FILE__, __LINE__);
            };

            os << ']';

        } // of if (FAFsmConst::HYPH_NO_HYPH < HyphId) ...

    } // for (int i = 0; ...
}

}

