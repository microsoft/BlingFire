using System;
using BlingFire;

namespace BlingUtilsTest
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Start C# test...");
            Console.WriteLine(String.Format("Bling Fire version: {0}", BlingFireUtils.GetBlingFireTokVersion()));
            Console.WriteLine("Test Complete");
        }
    }
}
