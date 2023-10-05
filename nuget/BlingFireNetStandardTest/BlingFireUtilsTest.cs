// <copyright file="BlingFireUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
//     Licensed under the MIT License.
// </copyright>

using BlingFireNetStandard;

namespace BlingFireNetStandardTest
{
    [TestClass]
    public class BlingFireUtilsTest
    {
        private readonly string m_testInput;

        public BlingFireUtilsTest()
        {
            m_testInput = "input text";
        }

        [TestMethod]
        public void ShouldLoadRuntimeDll()
        {
            Assert.AreNotEqual(0, BlingFireNetStandardUtils.GetBlingFireVersion());
        }

        [TestMethod]
        public void ShouldTokenizeText()
        {
            var tokenizer = BlingFireNetStandardUtils.LoadBertBaseTokenizer();
            byte[] inputBytes = System.Text.Encoding.UTF8.GetBytes($"[CLS]{m_testInput}[SEP]");
            int[] outputIds = new int[10];
            int outputCount = BlingFireNetStandardUtils.TokenizeText(tokenizer, inputBytes, inputBytes.Length, outputIds, outputIds.Length, 100);
            BlingFireNetStandardUtils.FreeTokenizer(tokenizer);
            Assert.AreEqual(4, outputCount);
            CollectionAssert.AreEqual(new int[]{ 101, 7953, 3793, 102, 0, 0, 0, 0, 0, 0 }, outputIds);
        }

        [TestMethod]
        public void ShouldBreakTextToWords()
        {
            Assert.AreEqual(2, BlingFireNetStandardUtils.GetWords(m_testInput).ToList().Count);
        }
    }
}