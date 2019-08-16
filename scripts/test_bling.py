import argparse
import sys
from blingfiretok import *

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model", default="./bert_base_tok.bin", help="bin file with compiled tokenization model")
parser.add_argument("-s", "--no-output", default=False, help="No output, False by default")
parser.add_argument("-n", "--no-process", default=False, help="No process, False by default")
args = parser.parse_args()

h = load_model(args.model)

for line in sys.stdin:

    line = line.strip()

    if not args.no_process:
        ids = text_to_ids(h, line, 128, 100)

        if not args.no_output:
            print(line)
            print(ids)

free_model(h)
