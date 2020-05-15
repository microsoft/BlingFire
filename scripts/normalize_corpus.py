import sys
import argparse
import unicodedata
from blingfire import *

sys.stdin.reconfigure(encoding='utf-8')

parser = argparse.ArgumentParser()
parser.add_argument("-k", "--nfkc", default=False, help="enables external NFKC normalization")
parser.add_argument("-c", "--nfc", default=False, help="enables external NFC normalization")
args = parser.parse_args()


for line in sys.stdin:

    # see if external normalization is needed
    if args.nfc:
        line = unicodedata.normalize('NFC', line)

    # see if external normalization is needed
    if args.nfkc:
        line = unicodedata.normalize('NFKC', line)

    # remove leading / trailing spaces
    line = line.strip()

    # shrink spaces and convert any space characters to 0x20
    line = normalize_spaces(line)

    print(line)
