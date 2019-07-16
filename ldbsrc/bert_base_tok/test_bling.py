import sys
from blingfiretok import *


# load bert base tokenizer model, note one model can be used by multiple threads within the same process
h = load_model("./bert_base_tok.bin")

for line in sys.stdin:

    line = line.strip()
    print(line)

    line = text_to_words(line)
    # print(line)

    ids = text_to_ids(h, line, 128, 1)
    print(ids)

free_model(h)
