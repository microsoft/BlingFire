// <copyright file="BlingFireUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
//     Licensed under the MIT License.
// </copyright>

using BlingFire;

namespace BlingFireNetStandard
{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

    /// <summary>
    /// This is C# interface for blingfiretokdll.dll
    /// For API description please see blingfiretokdll.cpp comments.
    /// </summary>
    public static class BlingFireNetStandardUtils
    {
        private const string c_bertBaseTokenizerModel = "BlingFireBinary/bert_base_tok.bin";
        private const string c_bertCasedTokenizerModel = "BlingFireBinary/bert_base_cased_tok.bin";
        private const string c_robertaTokenizerModel = "BlingFireBinary/roberta_tok.bin";
        private static readonly char[] s_gJustNewLineChar = new char[] { '\n' };
        private static readonly char[] s_gJustSpaceChar = new char[] { ' ' };

        public static int GetBlingFireVersion()
        {
            return BlingFireUtils.GetBlingFireTokVersion();
        }
        public static ulong LoadTokenizer(string tokenizerName)
        {
            return BlingFireUtils.LoadModel(System.Text.Encoding.UTF8.GetBytes(tokenizerName));
        }
        public static ulong LoadBertBaseTokenizer()
        {
            return LoadTokenizer(c_bertBaseTokenizerModel);
        }
        public static ulong LoadBertCasedTokenizer()
        {
            return LoadTokenizer(c_bertCasedTokenizerModel);
        }
        public static ulong LoadRobertaTokenizer()
        {
            return LoadTokenizer(c_robertaTokenizerModel);
        }

        public static void FreeTokenizer(ulong tokenizer)
        {
            int result = BlingFireUtils.FreeModel(tokenizer);
        }

        public static int TextToSentences([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, int inUtf8StrLen,
            byte[] outBuff, int maxBuffSize)
        {
            return BlingFireUtils.TextToSentences(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize);
        }

        public static int TextToWords([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, int inUtf8StrLen, byte[] outBuff, int maxBuffSize)
        {
            return BlingFireUtils.TextToWords(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize);
        }

        public static int TextToSentencesWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen, byte[] outBuff, int maxBuffSize, ulong model)
        {
            return BlingFireUtils.TextToSentencesWithModel(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize, model);
        }

        public static int TextToWordsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, int inUtf8StrLen,
            byte[] outBuff, int maxBuffSize, ulong model)
        {
            return BlingFireUtils.TextToWordsWithModel(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize, model);
        }

        public static int TextToSentencesWithOffsets([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, int maxBuffSize)
        {
            return BlingFireUtils.TextToSentencesWithOffsets(inUtf8Str, inUtf8StrLen, outBuff, startOffsets, endOffsets,
                maxBuffSize);
        }

        public static int TextToWordsWithOffsets([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, int inUtf8StrLen,
            byte[] outBuff, int[] startOffsets, int[] endOffsets, int maxBuffSize)
        {
            return BlingFireUtils.TextToWordsWithOffsets(inUtf8Str, inUtf8StrLen, outBuff, startOffsets, endOffsets,
                maxBuffSize);
        }

        public static int TextToSentencesWithOffsetsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, int maxBuffSize, ulong model)
        {
            return BlingFireUtils.TextToSentencesWithOffsetsWithModel(inUtf8Str, inUtf8StrLen, outBuff, startOffsets,
                endOffsets, maxBuffSize, model);
        }

        public static int TextToWordsWithOffsetsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, int maxBuffSize, ulong model)
        {
            return BlingFireUtils.TextToWordsWithOffsetsWithModel(inUtf8Str, inUtf8StrLen, outBuff, startOffsets,
                endOffsets, maxBuffSize, model);
        }

        public static int WordHyphenationWithModel(
            [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen,
            byte[] outBuff,
            int maxBuffSize,
            ulong model,
            int utf32HyCode)
        {
            return BlingFireUtils.WordHyphenationWithModel(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize, model,
                utf32HyCode);
        }

        public static int TextToIds(
            ulong model,
            [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen,
            int[] tokenIds,
            int maxBuffSize,
            int unkId)
        {
            return BlingFireUtils.TextToIds(model, inUtf8Str, inUtf8StrLen, tokenIds, maxBuffSize, unkId);
        }

        public static int TextToIdsWithOffsets(
            ulong model,
            [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen,
            int[] tokenIds,
            int[] startOffsets,
            int[] endOffsets,
            int maxBuffSize,
            int unkId)
        {
            return BlingFireUtils.TextToIdsWithOffsets(model, inUtf8Str, inUtf8StrLen, tokenIds, startOffsets,
                endOffsets, maxBuffSize, unkId);
        }

        public static int NormalizeSpaces([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, int inUtf8StrLen,
            byte[] outBuff, int maxBuffSize, int utf32SpaceCode)
        {
            return BlingFireUtils.NormalizeSpaces(inUtf8Str, inUtf8StrLen, outBuff, maxBuffSize, utf32SpaceCode);
        }

        public static int SetNoDummyPrefix(ulong model, bool fNoDummyPrefix)
        {
            return BlingFireUtils.SetNoDummyPrefix(model, fNoDummyPrefix);
        }

        public static int IdsToText(ulong model, int[] ids, int idsCount, byte[] outBuff, int maxBuffSize,
            bool skipSpecialTokens)
        {
            return BlingFireUtils.IdsToText(model, ids, idsCount, outBuff, maxBuffSize, skipSpecialTokens);
        }

        public static IEnumerable<string> GetSentences(string paragraph)
        {
            // use Bling Fire TOK for sentence breaking
            byte[] paraBytes = Encoding.UTF8.GetBytes(paragraph);
            int maxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[maxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            int actualLength = TextToSentences(paraBytes, paraBytes.Length, outputBytes, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string sentencesStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                string[] sentences = sentencesStr.Split(s_gJustNewLineChar, StringSplitOptions.RemoveEmptyEntries);
                foreach (string s in sentences)
                {
                    yield return s;
                }
            }
        }

        public static IEnumerable<Tuple<string, int, int>> GetSentencesWithOffsets(string paragraph)
        {
            // use Bling Fire TOK for sentence breaking
            return GetSentencesWithOffsets(Encoding.UTF8.GetBytes(paragraph));
        }

        public static IEnumerable<Tuple<string, int, int>> GetSentencesWithOffsets(byte[] paraBytes)
        {
            // use Bling Fire TOK for sentence breaking
            int maxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[maxLength];
            int[] startOffsets = new int[maxLength];
            int[] endOffsets = new int[maxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            int actualLength = TextToSentencesWithOffsets(paraBytes, paraBytes.Length, outputBytes, startOffsets, endOffsets, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string sentencesStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                string[] sentences = sentencesStr.Split(s_gJustNewLineChar, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < sentences.Length; ++i)
                {
                    yield return new Tuple<string, int, int>(sentences[i], startOffsets[i], endOffsets[i]);
                }
            }
        }

        public static IEnumerable<string> GetWords(string sentence)
        {
            // use Bling Fire TOK for sentence breaking
            byte[] paraBytes = Encoding.UTF8.GetBytes(sentence);
            int maxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[maxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            int actualLength = TextToWords(paraBytes, paraBytes.Length, outputBytes, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string wordsStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                string[] words = wordsStr.Split(s_gJustSpaceChar, StringSplitOptions.RemoveEmptyEntries);
                foreach (string w in words)
                {
                    yield return w;
                }
            }
        }

        public static IEnumerable<Tuple<string, int, int>> GetWordsWithOffsets(string sentence)
        {
            // use Bling Fire TOK for sentence breaking
            byte[] paraBytes = Encoding.UTF8.GetBytes(sentence);
            int maxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[maxLength];
            int[] startOffsets = new int[maxLength];
            int[] endOffsets = new int[maxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            int actualLength = TextToWordsWithOffsets(paraBytes, paraBytes.Length, outputBytes, startOffsets, endOffsets, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string wordsStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                string[] words = wordsStr.Split(s_gJustSpaceChar, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < words.Length; ++i)
                {
                    yield return new Tuple<string, int, int>(words[i], startOffsets[i], endOffsets[i]);
                }
            }
        }

        // Helper, returns generated string rather than UTF-8 bytes
        public static string TokensToText(ulong model, int[] ids, bool skipSpecialTokens = true)
        {
            if (null == ids || 0 == ids.Length)
            {
                return string.Empty;
            }

            // guess maximum needed buffer size
            int maxOutputSize = Math.Max(4096, ids.Length * 32);
            byte[] outputBytes = new byte[maxOutputSize];
            int actualLength = IdsToText(model, ids, ids.Length, outputBytes, outputBytes.Length, skipSpecialTokens);

            // if the buffer is too small call it again with a bigger buffer
            if (0 < actualLength && actualLength > outputBytes.Length)
            {
                outputBytes = new byte[actualLength];
                actualLength = IdsToText(model, ids, ids.Length, outputBytes, outputBytes.Length, skipSpecialTokens);
            }

            // see if the results are ready
            if (0 < actualLength && actualLength <= outputBytes.Length)
            {
                return Encoding.UTF8.GetString(outputBytes, 0, actualLength);
            }

            return string.Empty;
        }

        public static int TokenizeText(
            ulong model,
            [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
            int inUtf8StrLen,
            int[] tokenIds,
            int maxBuffSize,
            int unkId)
        {
            return TextToIds(model, inUtf8Str, inUtf8StrLen, tokenIds, maxBuffSize, unkId);
        }
    }
}