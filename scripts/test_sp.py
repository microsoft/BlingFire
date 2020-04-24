import sys
import sentencepiece as spm
import argparse
import numpy as np

np.set_printoptions(linewidth=72)

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model", default="./bert_base_tok.bin", help="bin file with compiled tokenization model")
parser.add_argument("-s", "--no-output", default=False, help="No output, False by default")
parser.add_argument("-n", "--no-process", default=False, help="No process, False by default")
args = parser.parse_args()

sp = spm.SentencePieceProcessor()
sp.Load(args.model)

for line in sys.stdin:

    line = line.strip()

    if not args.no_process:

        ids = sp.EncodeAsIds(line)

        # see if the output is disabled
        if not args.no_output:

            print("INPUT:")
            print(line)

            print("IDS:")
            print(ids)
            print()
