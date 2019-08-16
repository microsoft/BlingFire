from blingfire import *

s = "Hello, World! Hello, Café! How do I renew my virtual smart card?: /Microsoft IT/ 'virtual' smart card certificates for DirectAccess are valid for one year. In order to get to microsoft.com we need to type pi@1.2.1.2."
print("Input: " + s)

print("Tokenizer Version: " + str(get_blingfiretok_version()))

sents = text_to_sentences(s).split('\n')
for sent in sents:
    print("Raw sentence: " + sent)
    print("Tokenized sentence: " + text_to_words(sent))

print("Tokenized all at once: " + text_to_words(s))


print("Text to hashes examples: ")
s2 = "How to debug blingfiretok ?"
word_n_gram = 3
bucketSize = 2000000
output_array = text_to_hashes(s2, word_n_gram, bucketSize)
# reshape numpy array each row is a set of n-gram hashes.
o_hash_arr = np.reshape(output_array, (word_n_gram, (int)(len(output_array)/word_n_gram)))

print("Words: " + text_to_words(s2))
print("Unigram hashes", str(o_hash_arr[0][:]))
print("Bigram hashes", str(o_hash_arr[1][:]))
print("Trigram hashes", str(o_hash_arr[2][:]))


# load bert base tokenizer model, note one model can be used by multiple threads within the same process
h = load_model("./bert_base_tok.bin")
print("Model Handle: %s" % h)

ids = text_to_ids(h, "Apple pie.", 64, 1)
print(ids)
ids = text_to_ids(h, "Эpple pie.", 64, 1)
print(ids)

print(s)
ids = text_to_ids(h, s, 64, 1)
print(ids)

print(s+s)
ids = text_to_ids(h, s+s, 64, 1)
print(ids)

free_model(h)
print("Model Freed")
