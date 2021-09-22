// <copyright file="BlingFireUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
//     Licensed under the MIT License.
// </copyright>

namespace BlingFire
{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

    /// <summary>
    /// This is C# interface for blingfiretokdll.dll
    /// 
    /// For API description please see blingfiretokdll.cpp comments.
    ///
    /// </summary>
    public static class BlingFireUtils
    {
        private const string BlingFireTokDllName = "blingfiretokdll";

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 GetBlingFireTokVersion();

        [DllImport(BlingFireTokDllName)]
        public static extern UInt64 LoadModel(byte[] modelName);

        public static UInt64 LoadModel(string modelName)
        {
            return LoadModel(System.Text.Encoding.UTF8.GetBytes(modelName));
        }

        [DllImport(BlingFireTokDllName)]
        public static extern int FreeModel(UInt64 model);

        public static IEnumerable<string> GetSentences(string paragraph)
        {
            // use Bling Fire TOK for sentence breaking
            byte[] paraBytes = Encoding.UTF8.GetBytes(paragraph);
            int maxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[maxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            Int32 actualLength = TextToSentences(paraBytes, (Int32)paraBytes.Length, outputBytes, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string sentencesStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                var sentences = sentencesStr.Split(g_justNewLineChar, StringSplitOptions.RemoveEmptyEntries);
                foreach (var s in sentences)
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
            Int32 actualLength = TextToSentencesWithOffsets(paraBytes, (Int32)paraBytes.Length, outputBytes, startOffsets, endOffsets, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string sentencesStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                var sentences = sentencesStr.Split(g_justNewLineChar, StringSplitOptions.RemoveEmptyEntries);
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
            Int32 actualLength = TextToWords(paraBytes, (Int32)paraBytes.Length, outputBytes, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string wordsStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                var words = wordsStr.Split(g_justSpaceChar, StringSplitOptions.RemoveEmptyEntries);
                foreach (var w in words)
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
            Int32 actualLength = TextToWordsWithOffsets(paraBytes, (Int32)paraBytes.Length, outputBytes, startOffsets, endOffsets, maxLength);
            if (0 < actualLength - 1 && actualLength <= maxLength)
            {
                string wordsStr = Encoding.UTF8.GetString(outputBytes, 0, actualLength);
                var words = wordsStr.Split(g_justSpaceChar, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < words.Length; ++i)
                {
                    yield return new Tuple<string, int, int>(words[i], startOffsets[i], endOffsets[i]);
                }
            }
        }

        // Helper, returns generated string rather than UTF-8 bytes
        public static string IdsToText(UInt64 model, int[] ids, bool skipSpecialTokens = true)
        {
            if (null == ids || 0 == ids.Length)
            {
                return String.Empty;
            }

            // guess maximum needed buffer size
            int MaxOutputSize = Math.Max(4096, ids.Length * 32);
            byte [] outputBytes = new byte[MaxOutputSize];
            Int32 actualLength = IdsToText(model, ids, (Int32)ids.Length, outputBytes, (Int32)outputBytes.Length, skipSpecialTokens);

            // if the buffer is too small call it again with a bigger buffer
            if (0 < actualLength && actualLength > outputBytes.Length)
            {
                outputBytes = new byte[actualLength];
                actualLength = IdsToText(model, ids, (Int32)ids.Length, outputBytes, (Int32)outputBytes.Length, skipSpecialTokens);
            }

            // see if the results are ready
            if (0 < actualLength && actualLength <= outputBytes.Length)
            {
                return Encoding.UTF8.GetString(outputBytes, 0, actualLength);
            }

            return String.Empty;
        }



        //
        // expose Bling Fire interfaces
        //

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToSentences([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, Int32 maxBuffSize);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToWords([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, Int32 maxBuffSize);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToSentencesWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, Int32 maxBuffSize, UInt64 model);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToWordsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, Int32 maxBuffSize, UInt64 model);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToSentencesWithOffsets([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, Int32 maxBuffSize);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToWordsWithOffsets([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, Int32 maxBuffSize);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToSentencesWithOffsetsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, Int32 maxBuffSize, UInt64 model);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 TextToWordsWithOffsetsWithModel([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, int[] startOffsets, int[] endOffsets, Int32 maxBuffSize, UInt64 model);

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 WordHyphenationWithModel(
                [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, 
                Int32 inUtf8StrLen, 
                byte[] outBuff, 
                Int32 maxBuffSize, 
                UInt64 model, 
                Int32 utf32HyCode
            );

        [DllImport(BlingFireTokDllName)]
        public static extern int TextToIds(
                UInt64 model,
                [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
                Int32 inUtf8StrLen,
                int[] tokenIds,
                Int32 maxBuffSize,
                int unkId
            );

        [DllImport(BlingFireTokDllName)]
        public static extern int TextToIdsWithOffsets(
                UInt64 model,
                [MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str,
                Int32 inUtf8StrLen,
                int[] tokenIds,
                int[] startOffsets,
                int[] endOffsets,
                Int32 maxBuffSize,
                int unkId
            );

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 NormalizeSpaces([MarshalAs(UnmanagedType.LPArray)] byte[] inUtf8Str, Int32 inUtf8StrLen, byte[] outBuff, Int32 maxBuffSize, Int32 utf32SpaceCode);

        [DllImport(BlingFireTokDllName)]
        public static extern int SetNoDummyPrefix(UInt64 model, bool fNoDummyPrefix);

        [DllImport(BlingFireTokDllName)]
        public static extern int IdsToText(
                UInt64 model,
                int[] ids,
                Int32 idsCount,
                byte[] outBuff, 
                Int32 maxBuffSize,
                bool skipSpecialTokens
            );


        private static char[] g_justNewLineChar = new char[] { '\n' };
        private static char[] g_justSpaceChar = new char[] { ' ' };
    }
}