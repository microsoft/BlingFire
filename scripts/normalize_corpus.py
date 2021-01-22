import sys
import argparse
import unicodedata
from blingfire import *

sys.stdin.reconfigure(encoding='utf-8')

parser = argparse.ArgumentParser()
parser.add_argument("-k", "--nfkc", default=False, help="enables external NFKC normalization")
parser.add_argument("-c", "--nfc", default=False, help="enables external NFC normalization")
parser.add_argument("-i", "--ignore-case", default=False, help="Lower-cases the intput")
args = parser.parse_args()


for line in sys.stdin:

    # see if external normalization is needed
    if args.nfc:
        line = unicodedata.normalize('NFC', line)

    # see if external normalization is needed
    if args.nfkc:
        line = unicodedata.normalize('NFKC', line)

    if args.ignore_case:
        line = line.lower()

    # remove leading / trailing spaces
    line = line.strip()

    # shrink spaces and convert any space characters to 0x20
    line = normalize_spaces(line)

    print(line)
