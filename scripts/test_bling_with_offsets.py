import sys
from blingfire import *
import argparse
import numpy as np
import unicodedata

np.set_printoptions(linewidth=72)

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model", default="./xlnet.bin", help="bin file with compiled tokenization model")
parser.add_argument("-p", "--piece", default=None, help="optional sentence piece model file, if specified is used for comparison only")
parser.add_argument("-k", "--nfkc", default=False, help="enables external NFKC normalization")
parser.add_argument("-c", "--nfc", default=False, help="enables external NFC normalization")
parser.add_argument("-u", "--unk", default=0, help="Unknown token ID, default 0")
parser.add_argument("-s", "--no-output", default=False, help="No output, False by default")
parser.add_argument("-n", "--no-process", default=False, help="No process, False by default")
args = parser.parse_args()

sp = None

if args.piece != None:
    import sentencepiece as spm
    sp = spm.SentencePieceProcessor()
    sp.Load(args.piece)

h = load_model(args.model)


def find_first_different(ids1, ids2):
    for i in range(0, min(len(ids1), len(ids2))):
        if ids1[i] != ids2[i]:
            return i
    return -1


unk = int(args.unk)

for line in sys.stdin:

    line = line.strip()

    if not args.no_process:

        # see if external normalization is needed
        if args.nfc:
            line = unicodedata.normalize('NFC', line)

        # see if external normalization is needed
        if args.nfkc:
            line = unicodedata.normalize('NFKC', line)

        # does normalization if model has it, tokenization and returns ids and offsets for UTF-8 text
        utf8_s = line.encode("utf-8")
        ids, starts, ends = utf8text_to_ids_with_offsets(h, utf8_s, 128, unk, True)

        # see if the output is disabled
        if not args.no_output:

            print("INPUT:")
            print(line)

            print("IDS:")
            print(ids)

            print("STARTS:")
            print(starts)

            print("ENDS:")
            print(ends)

            print("TOKENS FROM OFFSETS:")
            for i in range(0, len(starts)):
                if starts[i] == -1 and ends[i] == -1:
                    t = "^"
                else:
                    s = starts[i] if starts[i] >= 0 else 0
                    e = ends[i] if ends[i] >= 0 else 0
                    t = utf8_s[s : e + 1].decode('utf-8')
                print("[" + t + "]", end=' ')
            print()

            ids2 = None
            if args.piece != None:
                ids2 = np.asarray(sp.EncodeAsIds(line))
                if not np.array_equal(ids2, ids) and len(ids) < 128:
                    print('ERROR:')
                    print("Expected IDS:")
                    print(ids2)
                    print("Expected Tokens:")
                    sp_tokens = sp.EncodeAsPieces(line)
                    print(sp_tokens)
                    e = find_first_different(ids, ids2)
                    if -1 != e:
                        print("First difference: " + 
                                str(ids[e]) + " vs " + str(ids2[e]) + ", " + 
                                utf8_s[starts[e] : ends[e]+1].decode('utf-8') + " vs " + sp_tokens[e] + 
                                " at position " + str(e))
                    print()
            print()

free_model(h)
