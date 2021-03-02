using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

/// <summary>
/// This is C# interface for blingfiretokdll.dll
/// </summary>
namespace BlingFire
{
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
            int MaxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[MaxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            Int32 actualLength = TextToSentences(paraBytes, (Int32)paraBytes.Length, outputBytes, MaxLength);
            if (0 < actualLength - 1 && actualLength <= MaxLength)
            {
                Array.Resize(ref outputBytes, actualLength);
                string sentencesStr = Encoding.UTF8.GetString(outputBytes);
                var sentences = sentencesStr.Split(_justNewLineChar, StringSplitOptions.RemoveEmptyEntries);
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

        public static IEnumerable<Tuple<string, int, int>> GetSentencesWithOffsets(Span<byte> paraBytes)
        {
            // use Bling Fire TOK for sentence breaking
            int MaxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[MaxLength];
            int[] startOffsets = new int[MaxLength];
            int[] endOffsets = new int[MaxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            Int32 actualLength = TextToSentencesWithOffsets(paraBytes, (Int32)paraBytes.Length, outputBytes, startOffsets, endOffsets, MaxLength);
            if (0 < actualLength - 1 && actualLength <= MaxLength)
            {
                Array.Resize(ref outputBytes, actualLength);
                string sentencesStr = Encoding.UTF8.GetString(outputBytes);
                var sentences = sentencesStr.Split(_justNewLineChar, StringSplitOptions.RemoveEmptyEntries);
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
            int MaxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[MaxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            Int32 actualLength = TextToWords(paraBytes, (Int32)paraBytes.Length, outputBytes, MaxLength);
            if (0 < actualLength - 1 && actualLength <= MaxLength)
            {
                Array.Resize(ref outputBytes, actualLength);
                string wordsStr = Encoding.UTF8.GetString(outputBytes);
                var words = wordsStr.Split(_justSpaceChar, StringSplitOptions.RemoveEmptyEntries);
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
            int MaxLength = (2 * paraBytes.Length) + 1;
            byte[] outputBytes = new byte[MaxLength];
            int[] startOffsets = new int[MaxLength];
            int[] endOffsets = new int[MaxLength];

            // native call returns '\n' delimited sentences, and adds 0 byte at the end
            Int32 actualLength = TextToWordsWithOffsets(paraBytes, (Int32)paraBytes.Length, outputBytes, startOffsets, endOffsets, MaxLength);
            if (0 < actualLength - 1 && actualLength <= MaxLength)
            {
                Array.Resize(ref outputBytes, actualLength);
                string wordsStr = Encoding.UTF8.GetString(outputBytes);
                var words = wordsStr.Split(_justSpaceChar, StringSplitOptions.RemoveEmptyEntries);
                for (int i = 0; i < words.Length; ++i)
                {
                    yield return new Tuple<string, int, int>(words[i], startOffsets[i], endOffsets[i]);
                }
            }
        }


        //
        // expose Bling Fire interfaces
        //

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentences(in byte InUtf8Str, Int32 InUtf8StrLen, ref byte OutBuff, Int32 MaxBuffSize);
        public static Int32 TextToSentences(Span<byte> InUtf8Str, Int32 InUtf8StrLen, Span<byte> OutBuff, Int32 MaxBuffSize)
        {
            return TextToSentences(
                in MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                MaxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWords(in byte InUtf8Str, Int32 InUtf8StrLen, ref byte OutBuff, Int32 MaxBuffSize);
        public static Int32 TextToWords(Span<byte> InUtf8Str, Int32 InUtf8StrLen, Span<byte> OutBuff, Int32 MaxBuffSize)
        {
            return TextToWords(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                MaxBuffSize
                );
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithModel(in byte InUtf8Str, Int32 InUtf8StrLen, ref byte OutBuff, Int32 MaxBuffSize, UInt64 model);
        public static Int32 TextToWords(Span<byte> InUtf8Str, Int32 InUtf8StrLen, Span<byte> OutBuff, Int32 MaxBuffSize, UInt64 model)
        {
            return TextToSentencesWithModel(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                MaxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithModel(in byte InUtf8Str, Int32 InUtf8StrLen, ref byte OutBuff, Int32 MaxBuffSize, UInt64 model);
        public static Int32 TextToWordsWithModel(Span<byte> InUtf8Str, Int32 InUtf8StrLen, Span<byte> OutBuff, Int32 MaxBuffSize, UInt64 model)
        {
            return TextToWordsWithModel(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                MaxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithOffsets(
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref byte OutBuff,
            ref int StartOffsets,
            ref int EndOffsets,
            Int32 MaxBuffSize);
        public static Int32 TextToSentencesWithOffsets(
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<byte> OutBuff,
            Span<int> StartOffsets,
            Span<int> EndOffsets,
            Int32 MaxBuffSize)
        {
            return TextToSentencesWithOffsets(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                ref MemoryMarshal.GetReference(StartOffsets),
                ref MemoryMarshal.GetReference(EndOffsets),
                MaxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithOffsets(
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref byte OutBuff,
            ref int StartOffsets,
            ref int EndOffsets,
            Int32 MaxBuffSize);
        public static Int32 TextToWordsWithOffsets(
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<byte> OutBuff,
            Span<int> StartOffsets,
            Span<int> EndOffsets,
            Int32 MaxBuffSize)
        {
            return TextToSentencesWithOffsets(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                ref MemoryMarshal.GetReference(StartOffsets),
                ref MemoryMarshal.GetReference(EndOffsets),
                MaxBuffSize);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToSentencesWithOffsetsWithModel(
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref byte OutBuff,
            ref int StartOffsets,
            ref int EndOffsets,
            Int32 MaxBuffSize,
            UInt64 model);
        public static Int32 TextToSentencesWithOffsetsWithModel(
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<byte> OutBuff,
            Span<int> StartOffsets,
            Span<int> EndOffsets,
            Int32 MaxBuffSize,
            UInt64 model)
        {
            return TextToSentencesWithOffsetsWithModel(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                ref MemoryMarshal.GetReference(StartOffsets),
                ref MemoryMarshal.GetReference(EndOffsets),
                MaxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 TextToWordsWithOffsetsWithModel(
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref byte OutBuff,
            ref int StartOffsets,
            ref int EndOffsets,
            Int32 MaxBuffSize,
            UInt64 model);
        public static Int32 TextToWordsWithOffsetsWithModel(
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<byte> OutBuff,
            Span<int> StartOffsets,
            Span<int> EndOffsets,
            Int32 MaxBuffSize,
            UInt64 model)
        {
            return TextToWordsWithOffsetsWithModel(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                ref MemoryMarshal.GetReference(StartOffsets),
                ref MemoryMarshal.GetReference(EndOffsets),
                MaxBuffSize,
                model);
        }

        [DllImport(BlingFireTokDllName)]
        static extern int TextToIds(
            UInt64 model,
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref int TokenIds,
            Int32 MaxBuffSize,
            int UnkId);
        public static int TextToIds(
            UInt64 model,
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<int> TokenIds,
            Int32 MaxBuffSize,
            int UnkId)
        {
            return TextToIds(
                model,
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(TokenIds),
                MaxBuffSize,
                UnkId);
        }

        [DllImport(BlingFireTokDllName)]
        static extern int TextToIdsWithOffsets(
           UInt64 model,
           in byte InUtf8Str,
           Int32 InUtf8StrLen,
           ref int TokenIds,
           ref int StartOffsets,
           ref int EndOffsets,
           Int32 MaxBuffSize,
           int UnkId);
        public static int TextToIdsWithOffsets(
            UInt64 model,
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<int> TokenIds,
            Span<int> StartOffsets,
            Span<int> EndOffsets,
            Int32 MaxBuffSize,
            int UnkId)
        {
            return TextToIdsWithOffsets(
                model,
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(TokenIds),
                ref MemoryMarshal.GetReference(StartOffsets),
                ref MemoryMarshal.GetReference(EndOffsets),
                MaxBuffSize,
                UnkId);
        }

        [DllImport(BlingFireTokDllName)]
        static extern Int32 NormalizeSpaces(
            in byte InUtf8Str,
            Int32 InUtf8StrLen,
            ref byte OutBuff,
            Int32 MaxBuffSize,
            Int32 utf32SpaceCode);
        public static int NormalizeSpaces(
            Span<byte> InUtf8Str,
            Int32 InUtf8StrLen,
            Span<byte> OutBuff,
            Int32 MaxBuffSize,
            Int32 utf32SpaceCode)
        {
            return NormalizeSpaces(
                MemoryMarshal.GetReference(InUtf8Str),
                InUtf8StrLen,
                ref MemoryMarshal.GetReference(OutBuff),
                MaxBuffSize,
                utf32SpaceCode);
        }

        private static char[] _justNewLineChar = new char[] { '\n' };
        private static char[] _justSpaceChar = new char[] { ' ' };
    }
}