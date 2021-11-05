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
    /// This is C# interface for blingfiretokdll.dll, it uses Span<T> hence more efficient however 
    /// is not supported by older .Net frameworks
    /// 
    /// For API description please see blingfiretokdll.cpp comments.
    ///
    /// </summary>
    public static class BlingFireUtils2
    {
        private const string BlingFireTokDllName = "blingfiretokdll";

        [DllImport(BlingFireTokDllName)]
        public static extern Int32 GetBlingFireTokVersion();

        [DllImport(BlingFireTokDllName)]
        static extern UInt64 LoadModel(byte[] modelName);

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
        static extern Int32 TextToSentences(in byte inUtf8Str, Int32 inUtf8StrLen, ref byte outBuff, Int32 maxBuffSize);

        public static Int32 TextToSentences(Span<byte> inUtf8Str, Int32 inUtf8StrLen, Span<byte> outBuff, Int32 maxBuffSize)
        {
            return TextToSentences(
                in MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWords(in byte inUtf8Str, Int32 inUtf8StrLen, ref byte outBuff, Int32 maxBuffSize);

        public static Int32 TextToWords(Span<byte> inUtf8Str, Int32 inUtf8StrLen, Span<byte> outBuff, Int32 maxBuffSize)
        {
            return TextToWords(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithModel(in byte inUtf8Str, Int32 inUtf8StrLen, ref byte outBuff, Int32 maxBuffSize, UInt64 model);

        public static Int32 TextToSentencesWithModel(Span<byte> inUtf8Str, Int32 inUtf8StrLen, Span<byte> outBuff, Int32 maxBuffSize, UInt64 model)
        {
            return TextToSentencesWithModel(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithModel(in byte inUtf8Str, Int32 inUtf8StrLen, ref byte outBuff, Int32 maxBuffSize, UInt64 model);

        public static Int32 TextToWordsWithModel(Span<byte> inUtf8Str, Int32 inUtf8StrLen, Span<byte> outBuff, Int32 maxBuffSize, UInt64 model)
        {
            return TextToWordsWithModel(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithOffsets(
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref byte outBuff,
            ref int startOffsets,
            ref int endOffsets,
            Int32 maxBuffSize);

        public static Int32 TextToSentencesWithOffsets(
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<byte> outBuff,
            Span<int> startOffsets,
            Span<int> endOffsets,
            Int32 maxBuffSize)
        {
            return TextToSentencesWithOffsets(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                ref MemoryMarshal.GetReference(startOffsets),
                ref MemoryMarshal.GetReference(endOffsets),
                maxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithOffsets(
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref byte outBuff,
            ref int startOffsets,
            ref int endOffsets,
            Int32 maxBuffSize);

        public static Int32 TextToWordsWithOffsets(
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<byte> outBuff,
            Span<int> startOffsets,
            Span<int> endOffsets,
            Int32 maxBuffSize)
        {
            return TextToWordsWithOffsets(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                ref MemoryMarshal.GetReference(startOffsets),
                ref MemoryMarshal.GetReference(endOffsets),
                maxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithOffsetsWithModel(
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref byte outBuff,
            ref int startOffsets,
            ref int endOffsets,
            Int32 maxBuffSize,
            UInt64 model);

        public static Int32 TextToSentencesWithOffsetsWithModel(
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<byte> outBuff,
            Span<int> startOffsets,
            Span<int> endOffsets,
            Int32 maxBuffSize,
            UInt64 model)
        {
            return TextToSentencesWithOffsetsWithModel(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                ref MemoryMarshal.GetReference(startOffsets),
                ref MemoryMarshal.GetReference(endOffsets),
                maxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithOffsetsWithModel(
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref byte outBuff,
            ref int startOffsets,
            ref int endOffsets,
            Int32 maxBuffSize,
            UInt64 model);

        public static Int32 TextToWordsWithOffsetsWithModel(
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<byte> outBuff,
            Span<int> startOffsets,
            Span<int> endOffsets,
            Int32 maxBuffSize,
            UInt64 model)
        {
            return TextToWordsWithOffsetsWithModel(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                ref MemoryMarshal.GetReference(startOffsets),
                ref MemoryMarshal.GetReference(endOffsets),
                maxBuffSize,
                model);
        }


        [DllImport(BlingFireTokDllName)]
        static extern Int32 WordHyphenationWithModel(
            in byte inUtf8Str, 
            Int32 inUtf8StrLen, 
            ref byte outBuff, 
            Int32 maxBuffSize, 
            UInt64 model, 
            Int32 utf32HyCode);

        public static Int32 WordHyphenationWithModel(
            Span<byte> inUtf8Str, 
            Int32 inUtf8StrLen, 
            Span<byte> outBuff, Int32 maxBuffSize, 
            UInt64 model, 
            Int32 utf32HyCode = 0x2D)
        {
            return WordHyphenationWithModel(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize,
                model,
                utf32HyCode);
        }


        [DllImport(BlingFireTokDllName)]
        static extern int TextToIds(
            UInt64 model,
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref int tokenIds,
            Int32 maxBuffSize,
            int unkId);

        public static int TextToIds(
            UInt64 model,
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<int> tokenIds,
            Int32 maxBuffSize,
            int unkId)
        {
            return TextToIds(
                model,
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(tokenIds),
                maxBuffSize,
                unkId);
        }

        [DllImport(BlingFireTokDllName)]
        static extern int TextToIdsWithOffsets(
           UInt64 model,
           in byte inUtf8Str,
           Int32 inUtf8StrLen,
           ref int tokenIds,
           ref int startOffsets,
           ref int endOffsets,
           Int32 maxBuffSize,
           int unkId);

        public static int TextToIdsWithOffsets(
            UInt64 model,
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<int> tokenIds,
            Span<int> startOffsets,
            Span<int> endOffsets,
            Int32 maxBuffSize,
            int unkId)
        {
            return TextToIdsWithOffsets(
                model,
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(tokenIds),
                ref MemoryMarshal.GetReference(startOffsets),
                ref MemoryMarshal.GetReference(endOffsets),
                maxBuffSize,
                unkId);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 NormalizeSpaces(
            in byte inUtf8Str,
            Int32 inUtf8StrLen,
            ref byte outBuff,
            Int32 maxBuffSize,
            Int32 utf32SpaceCode);

        public static int NormalizeSpaces(
            Span<byte> inUtf8Str,
            Int32 inUtf8StrLen,
            Span<byte> outBuff,
            Int32 maxBuffSize,
            Int32 utf32SpaceCode)
        {
            return NormalizeSpaces(
                MemoryMarshal.GetReference(inUtf8Str),
                inUtf8StrLen,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize,
                utf32SpaceCode);
        }

        [DllImport(BlingFireTokDllName)]
        public static extern int SetNoDummyPrefix(UInt64 model, bool fNoDummyPrefix);

        [DllImport(BlingFireTokDllName)]
        static extern int IdsToText(
            UInt64 model,
            in int ids,
            Int32 idsCount,
            ref byte outBuff,
            Int32 maxBuffSize,
            bool skipSpecialTokens);

        public static int IdsToText(
            UInt64 model,
            Span<int> ids,
            Int32 idsCount,
            Span<byte> outBuff,
            Int32 maxBuffSize,
            bool skipSpecialTokens)
        {
            return IdsToText(
                model,
                MemoryMarshal.GetReference(ids),
                idsCount,
                ref MemoryMarshal.GetReference(outBuff),
                maxBuffSize,
                skipSpecialTokens);
        }


        private static char[] g_justNewLineChar = new char[] { '\n' };
        private static char[] g_justSpaceChar = new char[] { ' ' };
    }
}