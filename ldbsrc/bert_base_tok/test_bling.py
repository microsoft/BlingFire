import sys
import os
import blingfire
# from blingfiretok import *


# load bert base tokenizer model, note one model can be used by multiple threads within the same process
# h = load_model("./bert_base_tok.bin")
h = blingfire.load_model(os.path.join(os.path.dirname(blingfire.__file__), "bert_base_tok.bin"))

for line in sys.stdin:

    line = line.strip()
    print(line)

    #line = text_to_words(line)
    #print(line)

    ids = blingfire.text_to_ids(h, line, 128, 100)
    print(ids)

blingfire.free_model(h)
