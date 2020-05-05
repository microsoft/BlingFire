import unicodedata

#
# This script computes a set of letters that are from vocab words of the vocab.txt
#  if a token is a single letter token, then we don't include Chinese characters and punctuations
#  since they never go in a sequence
#

def is_bert_chinese(l):
    # [\x3400-\x4DBF\x4E00-\x9FFF\xF900-\xFAFF\x20000-\x2A6DF\x2A700-\x2B73F\x2B740-\x2B81F\x2B820-\x2CEAF\x2F800-\x2FA1F]
    i = ord(l)
    if  (i >= 0x3400 and i <=0x4DBF) or (i >= 0x4E00 and i <=0x9FFF) or \
        (i >= 0xF900 and i <=0xFAFF) or (i >= 0x20000 and i <=0x2A6DF) or \
        (i >= 0x2A700 and i <=0x2B73F) or (i >= 0x2B740 and i <=0x2B81F) or \
        (i >= 0x2B820 and i <=0x2CEAF) or (i >= 0x2F800 and i <=0x2FA1F):
        return True
    return False


def is_letter(l):
    cat = unicodedata.category(l)
    if cat.startswith('L') or cat.startswith('M') or cat.startswith('N'):
         return True
    return False


alphabet = {}
with open("vocab.txt") as f:
    for line in f:
        line = line.strip()
        if 2 < len(line) and line[0] == '#' and line[1] == '#':
            line = line[2:]
        if 1 < len(line):
            for a in line:
                if False == is_bert_chinese(line[0]) and True == is_letter(a):
                    alphabet[a]=1
        elif 0 < len(line):
            if False == is_bert_chinese(line[0]) and True == is_letter(line[0]):
                alphabet[line[0]]=1

def my_hex(i):
    return "\\" + "{0:#0{1}x}".format(i, 6)[1:]

start = -2
prev_char = -2
for c in range(0x10ffff):
    if chr(c) in alphabet:
        if c != prev_char + 1:
            if start >= 0:
                if (start != prev_char):
                    print(my_hex(start) + "-" + my_hex(prev_char), end="")
                else:
                    print(my_hex(start), end="")
            start = c
        prev_char = c

print()
