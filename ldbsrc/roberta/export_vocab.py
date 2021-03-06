import os
import json
import regex as re

pat = re.compile(r"""'s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+""")

def bytes_to_unicode():
    """
    Returns list of utf-8 byte and a mapping to unicode strings. We specifically avoids mapping to whitespace/control
    characters the bpe code barfs on.

    The reversible bpe codes work on unicode strings. This means you need a large # of unicode characters in your vocab
    if you want to avoid UNKs. When you're at something like a 10B token dataset you end up needing around 5K for
    decent coverage. This is a signficant percentage of your normal, say, 32K bpe vocab. To avoid that, we want lookup
    tables between utf-8 bytes and unicode strings.
    """
    bs = (
        list(range(ord("!"), ord("~") + 1)) + list(range(ord("¡"), ord("¬") + 1)) + list(range(ord("®"), ord("ÿ") + 1))
    )
    cs = bs[:]
    n = 0
    for b in range(2 ** 8):
        if b not in bs:
            bs.append(b)
            cs.append(2 ** 8 + n)
            n += 1
    cs = [chr(n) for n in cs]
    return dict(zip(bs, cs))


def get_pairs(word):
    """
    Return set of symbol pairs in a word.

    Word is represented as tuple of symbols (symbols being variable-length strings).
    """
    pairs = set()
    prev_char = word[0]
    for char in word[1:]:
        pairs.add((prev_char, char))
        prev_char = char
    return pairs


byte_encoder = bytes_to_unicode()
byte_decoder = {v: k for k, v in byte_encoder.items()}

def tokenize_without_bpe(text):
    """ Tokenize a string. """
    tokens = []
    for token in re.findall(pat, text):
        token = "".join(
            byte_encoder[b] for b in token.encode("utf-8")
        )  # Maps all our bytes to unicode strings, avoiding controle tokens of the BPE (spaces in our case)
        tokens.extend(bpe_token for bpe_token in token.split(" "))
    return tokens



with open("./vocab.json", encoding="utf-8") as vocab_handle:
    encoder = json.load(vocab_handle)

with open("./merges.txt", encoding="utf-8") as merges_handle:
    bpe_merges = merges_handle.read().split("\n")[1:-1]
bpe_merges = [tuple(merge.split()) for merge in bpe_merges]
bpe_ranks = dict(zip(bpe_merges, range(len(bpe_merges))))

# compute mapping of a merged pair to a merge rank
bpe_ranks_merged = {(k[0] + k[1]) : v for k, v in bpe_ranks.items()}


# generate tagset.txt and pos.dict.utf8 from vocab.json
# Notes: 
#   1. first 256 entries in the vocab.json are for bytes and are always included
#   2. all other entries match between merges and the vocab.json
#   3. these byte-level tokenizes roberta/gpt2 start ids from 0, since tags cannot be 0 (TODO: fix it)
#       we need to +1 for all the tags, we can subtract 1 in place once tokenization is done
with open("./pos.dict.utf8", "w", encoding='utf-8') as d:
    with open("./tagset.txt", "w", encoding='utf-8') as t:
        MinId = -1
        MaxId = -1
        for token, i in encoder.items():

            i = i + 1
            if -1 == MinId or MinId > i:
                MinId = i
            if -1 == MaxId or MaxId < i:
                MaxId = i

            # make rank to be equal to -ID
            rank = -1.0 * i
            
            # read the rank from the bpe_ranks instead of using the id from a dictionary
            if token in bpe_ranks_merged:
                rank = -1.0 * bpe_ranks_merged[token]
            else:
                print("WARNING: token: \"" + token + "\": " + str(i-1) + " is not in merges.txt, using it's ID as a rank.")

            # make sure there no 0 ranks
            rank = -0.00001 if rank == 0.0 else rank

            # tokens are encoded byte sequences, so let's decode them
            decoded_token = []
            IsFirst = True
            ContainsZeroBytes = False
            for c in token:
                if IsFirst and c == "Ġ":
                    decoded_token.append(0x2581)
                elif c in byte_decoder:
                    b = byte_decoder[c]
                    if b <= 3 :
                        ContainsZeroBytes = True
                    decoded_token.append(b)
                else:
                    raise "ERROR: Cannot decode a character from a token. Check that the token was encoded."
                IsFirst = False

            if ContainsZeroBytes == False:
                IsFirst = True
                for num in decoded_token:
                    if IsFirst:
                        print(num, end="", file=d)
                    else:
                        print(" " + str(num), end="", file=d)
                    IsFirst = False

                print("\tWORD_ID_" + str(i) + "\t" + str(rank), file=d)
            else:
                print("WARNING: token: \"" + token + "\": " + str(i-1) + " contains 0 byte, might cause problems during the dictionary compilation. Discarding...")

        for i in range(MinId, MaxId + 1):
            print("WORD_ID_" + str(i) + " " + str(i), file=t)
