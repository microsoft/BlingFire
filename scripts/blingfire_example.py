import os
import blingfire

# one time load the model (we are using the one that comes with the package)
h = blingfire.load_model(os.path.join(os.path.dirname(blingfire.__file__), "bert_base_cased_tok.bin"))
h_i2w = blingfire.load_model(os.path.join(os.path.dirname(blingfire.__file__), "bert_base_cased_tok.i2w"))

# input
s = "This is a test. Эpple pie. How do I renew my virtual smart card?"
print(s)

ids = blingfire.text_to_ids(h, s, 128, 100)  # sequence length: 128, oov id: 100
print(ids)                                   # returns a numpy array of length 128 (padded or trimmed)

text = blingfire.ids_to_text(h_i2w, ids)     # take a numpy array of ids
print(text)                                  # returns a string

text = blingfire.ids_to_text(h_i2w, ids, skip_special_tokens=False)     # generate text with special tokens included
print(text)                                  


# free the model at the end
blingfire.free_model(h)
blingfire.free_model(h_i2w)
