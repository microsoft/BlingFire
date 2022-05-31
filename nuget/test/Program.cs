using System;
using BlingFire;

namespace BlingUtilsTest
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Start C# test...");


            // see the version of the DLL
            Console.WriteLine(String.Format("Bling Fire version: {0}", BlingFireUtils.GetBlingFireTokVersion()));

            string input = "Autophobia, also called monophobia, isolophobia, or eremophobia, is the specific phobia of isolation. I saw a girl with a telescope. Я увидел девушку с телескопом.";
            byte[] inBytes = System.Text.Encoding.UTF8.GetBytes(input);

            int MaxOutLength = 2*inBytes.Length + 1;
            byte[] outputBytes = new byte[MaxOutLength];
            Int32 actualLength = 0;


            // break text into sentences (uses built in model)
            actualLength = BlingFireUtils.TextToSentences(inBytes, (Int32)inBytes.Length, outputBytes, MaxOutLength);
            if (0 < actualLength && actualLength < MaxOutLength)
            {
                Console.WriteLine(String.Format("BlingFireUtils.TextToSentences: '{0}'", System.Text.Encoding.UTF8.GetString(outputBytes, 0, actualLength)));
            }
            else 
            {
                Console.WriteLine("BlingFireUtils.TextToWords: ERROR");
            }
            // another way to do the same
            Console.WriteLine("BlingFireUtils.GetWords: '{0}'", string.Join("\n", BlingFireUtils.GetSentences(input)));


            // break text into tokens (uses built in model)
            actualLength = BlingFireUtils.TextToWords(inBytes, (Int32)inBytes.Length, outputBytes, MaxOutLength);
            if (0 < actualLength && actualLength < MaxOutLength)
            {
                Console.WriteLine(String.Format("BlingFireUtils.TextToWords: '{0}'", System.Text.Encoding.UTF8.GetString(outputBytes, 0, actualLength)));
            }
            else 
            {
                Console.WriteLine("BlingFireUtils.TextToWords: ERROR");
            }
            // another way to do the same
            Console.WriteLine("BlingFireUtils.GetWords: '{0}'", string.Join(" ", BlingFireUtils.GetWords(input)));

            actualLength = BlingFireUtils.NormalizeSpaces(inBytes, (Int32)inBytes.Length, outputBytes, MaxOutLength, 9601);
            if (0 < actualLength && actualLength < MaxOutLength)
            {
                Console.WriteLine(String.Format("BlingFireUtils.NormalizeSpaces: '{0}'", System.Text.Encoding.UTF8.GetString(outputBytes, 0, actualLength)));
            }
            else 
            {
                Console.WriteLine("BlingFireUtils.TextToWords: ERROR");
            }


            // load BERT base tokenization model
            var h1 = BlingFireUtils.LoadModel("./bin/Debug/net6.0/bert_base_tok.bin");
            var h1_i2w = BlingFireUtils.LoadModel("./bin/Debug/net6.0/bert_base_tok.i2w");
            Console.WriteLine(String.Format("Model handle: {0}", h1));

            // load XLNET tokenization model
            var h2 = BlingFireUtils.LoadModel("./bin/Debug/net6.0/xlnet.bin");
            var h2_i2w = BlingFireUtils.LoadModel("./bin/Debug/net6.0/xlnet.i2w");
            Console.WriteLine(String.Format("Model handle: {0}", h2));

            // load XLM Roberta tokenization model
            var h3 = BlingFireUtils.LoadModel("./bin/Debug/net6.0/xlm_roberta_base.bin");
            var h3_i2w = BlingFireUtils.LoadModel("./bin/Debug/net6.0/xlm_roberta_base.i2w");
            Console.WriteLine(String.Format("Model handle: {0}", h3));

            // allocate space for ids
            int[] Ids =  new int[128];


            // tokenize with loaded BERT tokenization model and output the ids
            Console.WriteLine("Model: bert_base_tok");
            var outputCount = BlingFireUtils.TextToIds(h1, inBytes, inBytes.Length, Ids, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Array.Resize(ref Ids, outputCount);
                Console.WriteLine(String.Format("return array: [{0}]", string.Join(", ", Ids)));
                Array.Resize(ref Ids, 128);

                Console.WriteLine("IdsToText: " + BlingFireUtils.IdsToText(h1_i2w, Ids));
            }

            // tokenize with loaded XLNET tokenization model and output the ids
            Console.WriteLine("Model: xlnet");
            outputCount = BlingFireUtils.TextToIds(h2, inBytes, inBytes.Length, Ids, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Array.Resize(ref Ids, outputCount);
                Console.WriteLine(String.Format("return array: [{0}]", string.Join(", ", Ids)));
                Array.Resize(ref Ids, 128);

                Console.WriteLine("IdsToText: " + BlingFireUtils.IdsToText(h2_i2w, Ids));
            }

            // tokenize with loaded XLM Roberta tokenization model and output the ids
            Console.WriteLine("Model: xlm_roberta_base");
            outputCount = BlingFireUtils2.TextToIds(h3, inBytes, inBytes.Length, Ids, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Array.Resize(ref Ids, outputCount);
                Console.WriteLine(String.Format("return array: [{0}]", string.Join(", ", Ids)));
                Array.Resize(ref Ids, 128);

                Console.WriteLine("IdsToText: " + BlingFireUtils2.IdsToText(h3_i2w, Ids));
            }


            // allocate space for offsets
            int[] Starts =  new int[128];
            int[] Ends =  new int[128];

            // get ids with offsets

            // tokenize with loaded BERT tokenization model and output ids and start and end offsets
            outputCount = BlingFireUtils.TextToIdsWithOffsets(h1, inBytes, inBytes.Length, Ids, Starts, Ends, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Console.Write("tokens from offsets: [");
                for(int i = 0; i < outputCount; ++i)
                {
                    int startOffset = Starts[i] >= 0 ? Starts[i] : 0;
                    int endOffset = Ends[i] >= 0 ? Ends[i] : 0;
                    int surfaceLen = endOffset - startOffset + 1;

                    string token = System.Text.Encoding.UTF8.GetString(inBytes, startOffset, surfaceLen);
                    Console.Write(String.Format("'{0}'/{1} ", token, Ids[i]));
                }
                Console.WriteLine("]");
            }

            // tokenize with loaded XLNET tokenization model and output ids and start and end offsets
            outputCount = BlingFireUtils.TextToIdsWithOffsets(h2, inBytes, inBytes.Length, Ids, Starts, Ends, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Console.Write("tokens from offsets: [");
                for(int i = 0; i < outputCount; ++i)
                {
                    int startOffset = Starts[i] >= 0 ? Starts[i] : 0;
                    int endOffset = Ends[i] >= 0 ? Ends[i] : 0;
                    int surfaceLen = endOffset - startOffset + 1;

                    string token = System.Text.Encoding.UTF8.GetString(inBytes, startOffset, surfaceLen);
                    Console.Write(String.Format("'{0}'/{1} ", token, Ids[i]));
                }
                Console.WriteLine("]");
            }

            // tokenize with loaded XLM Roberta tokenization model and output ids and start and end offsets
            outputCount = BlingFireUtils.TextToIdsWithOffsets(h3, inBytes, inBytes.Length, Ids, Starts, Ends, Ids.Length, 0);
            Console.WriteLine(String.Format("return length: {0}", outputCount));
            if (outputCount >= 0)
            {
                Console.Write("tokens from offsets: [");
                for(int i = 0; i < outputCount; ++i)
                {
                    int startOffset = Starts[i] >= 0 ? Starts[i] : 0;
                    int endOffset = Ends[i] >= 0 ? Ends[i] : 0;
                    int surfaceLen = endOffset - startOffset + 1;

                    string token = System.Text.Encoding.UTF8.GetString(inBytes, startOffset, surfaceLen);
                    Console.Write(String.Format("'{0}'/{1} ", token, Ids[i]));
                }
                Console.WriteLine("]");
            }

            // free loaded models
            BlingFireUtils.FreeModel(h1);
            BlingFireUtils.FreeModel(h2);
            BlingFireUtils.FreeModel(h3);
            BlingFireUtils.FreeModel(h1_i2w);
            BlingFireUtils.FreeModel(h2_i2w);
            BlingFireUtils.FreeModel(h3_i2w);


            // Test syllabification API's

            // break text into tokens first
            actualLength = BlingFireUtils2.TextToWords(inBytes, (Int32)inBytes.Length, outputBytes, MaxOutLength);
            string [] tokens = null;
            if (0 < actualLength && actualLength < MaxOutLength)
            {
                tokens = System.Text.Encoding.UTF8.GetString(outputBytes, 0, actualLength).Split(' ');
            }

            var hy = BlingFireUtils2.LoadModel("./bin/Debug/net6.0/syllab.bin");
            Console.WriteLine(String.Format("Model handle: {0}", hy));

            foreach(var token in tokens)
            {
                inBytes = System.Text.Encoding.UTF8.GetBytes(token);
                int outputLen = BlingFireUtils2.WordHyphenationWithModel(inBytes, inBytes.Length, outputBytes, MaxOutLength, hy, 0x2D);

                if (0 < outputLen)
                {
                    string hyphenatedToken = System.Text.Encoding.UTF8.GetString(outputBytes, 0, outputLen);
                    Console.Write(String.Format("'{0}' ", hyphenatedToken));
                }
            }
            Console.WriteLine("");

            BlingFireUtils2.FreeModel(hy);

            Console.WriteLine("Test Complete");
        }
    }
}
