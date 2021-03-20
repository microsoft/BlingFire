import sys
import argparse
import numpy as np
import unicodedata
import blingfire
import fasttext

np.set_printoptions(linewidth=72)

parser = argparse.ArgumentParser()
parser.add_argument("-m", "--model", default="./xlnet.bin", help="bin file with compiled tokenization model")
parser.add_argument("-f", "--fasttext", default="./model.bin", help="FastText model bin file")
parser.add_argument("-k", "--nfkc", default=False, help="enables external NFKC normalization")
parser.add_argument("-c", "--nfc", default=False, help="enables external NFC normalization")
parser.add_argument("-w", "--why", default=False, help="estimate contributions of each token / word ngram")
parser.add_argument("-u", "--unk", default=0, help="Unknown token ID, default 0")
parser.add_argument("-n", "--ngram", default=2, help="word ngram order, default 2")

args = parser.parse_args()


h = blingfire.load_model(args.model)
f = fasttext.load_model(args.fasttext)


unk = int(args.unk)

for line in sys.stdin:

    line = line.strip()

    # see if external normalization is needed
    if args.nfc:
        line = unicodedata.normalize('NFC', line)

    # see if external normalization is needed
    if args.nfkc:
        line = unicodedata.normalize('NFKC', line)

    # does normalization if model has it, tokenization and returns ids and offsets for UTF-8 text
    ids = blingfire.text_to_ids(h, line, 1024, unk, True)
    idss = ' '.join(map(str, ids))

    predictions = f.predict(idss)

    print(str(predictions) + "\t" + str(ids) + "\t" + line)

    if args.why:
        word_count = np.shape(f.words)[0]
        buckets = np.shape(f.get_input_matrix())[0] - word_count
        ngram_order = int(args.ngram)

        # get the hashes
        a = blingfire.text_to_hashes(idss, ngram_order, buckets)
        print(f'Hashes from Bling Fire: {a}')
        a = a.reshape((ngram_order, len(ids)), order='C')
        print(f"Word Count: {word_count}, Buckets: {buckets}, Ngram Order: {ngram_order}\nHashes:\n{a}")

        # get the embedding matrix
        m = f.get_input_matrix()

        # for each input token
        for i in range(len(ids)):
            # for each ngram order
            for j in range(ngram_order):
                # get the BlingFire's id
                id = ids[i]
                # compute a token vector importance / contribution
                if j == 0:
                    # use word id for the look up
                    a[j][i] = np.linalg.norm(m[f.get_word_id(str(id))])
                else:
                    # use ngram hash for look up
                    ngram_hash = a[j][i]
                    a[j][i] = np.linalg.norm(np.linalg.norm(m[word_count + ngram_hash]))

        print(f"Importance:\n{a}")


blingfire.free_model(h)
