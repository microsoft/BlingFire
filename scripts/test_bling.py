import argparse
import sys
from blingfire import *

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model", default="./bert_base_tok.bin", help="bin file with compiled tokenization model")
parser.add_argument("-s", "--no-output", default=False, help="No output, False by default")
parser.add_argument("-n", "--no-process", default=False, help="No process, False by default")
parser.add_argument("-i", "--print-input", default=False, help="Prints input string")
parser.add_argument("-p", "--no-padding", default=False, help="Does not pad to the max length")
parser.add_argument("-u", "--unk", default=100, help="Unknown token id")
parser.add_argument("-x", "--max-length", default=128, help="Maximum number of ids to return")
parser.add_argument("-a", "--single-space", default=False, help="Output ids single as a single space delimited string")

args = parser.parse_args()

h = load_model(args.model)

max_length = int(args.max_length)
unk = int(args.unk)
no_padding = args.no_padding

for line in sys.stdin:

    line = line.strip()

    if args.print_input:
        print(line)

    if not args.no_process:
        ids = text_to_ids(h, line, max_length, unk, no_padding)

        if not args.no_output:
            if args.single_space:
                print(" ".join([str(i) for i in ids]));
            else:
                print(ids)

free_model(h)
