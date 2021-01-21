import unicodedata


alphabet = {}
with open("pos.dict.utf8") as f:
    for line in f:
        token = line.split("\t")[0]
        for a in token:
            alphabet[a]=1


def is_useful(input_string, normalized_input):
    if input_string in alphabet:
        return True
    for t in normalized_input:
        if t in alphabet:
            return True
    return False


for c in range(0x10ffff):

    input_string = chr(c)
    name = unicodedata.name(input_string, None)

    if None != name:

        ## normalized_input = unicodedata.normalize('NFKC', input_string)
        normalized_input = input_string.lower()

        if normalized_input != input_string and is_useful(input_string, normalized_input):

            # print a comment
            print("# " + name + ": " + input_string + " --> " + normalized_input)

            # comment out crazy long normalizations (19 cases will be gone)
            if len(normalized_input) > 4:
                print("## too long, uncomment if you want it ##", end="")

            # print input character
            print("\\" + hex(c)[1:], end="")

            # print output as a code if it is just one, or as a UTF-8 string if it is more than one
            if len(normalized_input) > 0:
                print(" " + (("\\" + hex(ord(normalized_input[0]))[1:]) if 1 == len(normalized_input) else normalized_input), end="")

            print()
            print()
