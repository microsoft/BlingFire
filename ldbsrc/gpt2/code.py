import os
import json
import regex as re
import blingfire

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
decoder = {v: k for k, v in encoder.items()}

with open("./merges.txt", encoding="utf-8") as merges_handle:
    bpe_merges = merges_handle.read().split("\n")[1:-1]
bpe_merges = [tuple(merge.split()) for merge in bpe_merges]
bpe_ranks = dict(zip(bpe_merges, range(len(bpe_merges))))



def bpe(token):

    word = tuple(token)
    pairs = get_pairs(word)

    if not pairs:
        return token

    while True:
        bigram = min(pairs, key=lambda pair: bpe_ranks.get(pair, float("inf")))
        if bigram not in bpe_ranks:
            break
        first, second = bigram
        new_word = []
        i = 0
        while i < len(word):
            try:
                j = word.index(first, i)
            except ValueError:
                new_word.extend(word[i:])
                break
            else:
                new_word.extend(word[i:j])
                i = j

            if word[i] == first and i < len(word) - 1 and word[i + 1] == second:
                new_word.append(first + second)
                i += 2
            else:
                new_word.append(word[i])
                i += 1
        new_word = tuple(new_word)
        word = new_word
        if len(word) == 1:
            break
        else:
            pairs = get_pairs(word)
    word = " ".join(word)
    return word


def tokenize_with_bpe(text):
    """ Tokenize a string. """
    bpe_tokens = []
    for token in re.findall(pat, text):
        token = "".join(
            byte_encoder[b] for b in token.encode("utf-8")
        )  # Maps all our bytes to unicode strings, avoiding controle tokens of the BPE (spaces in our case)
        bpe_tokens.extend(bpe_token for bpe_token in bpe(token).split(" "))
    return bpe_tokens




print("encoder:")
print(byte_encoder)

print("decoder:")
print(byte_decoder)

s = " Hello there, we're doing tokenization with BPE here. Привет все. Мы тут БПЕ испытываем.<|endoftext|>"

print("tokenization without bpe:")
print(tokenize_without_bpe(s))

print("tokenization with bpe:")
print(tokenize_with_bpe(s))
ids = [encoder[token] for token in tokenize_with_bpe(s)]
print(ids)

h = blingfire.load_model("../ldb/gpt2.bin")
ids2 = blingfire.text_to_ids(h, s, 128, no_padding=True)
print(ids2)

print(ids2 == ids)
