import unicodedata

# iterate over all unicode character code points
for i in range(0x20, 0x10FFFF):
    try:
        # get the text with one character
        text = "".join([chr(i)])

        # decompose text into NFD
        text = unicodedata.normalize("NFD", text)

        # recombine without modifiers
        output = []
        for char in text:
            cat = unicodedata.category(char)
            if cat == "Mn":
                continue
            output.append(char)
        norm_text = "".join(output)

        # drop the case
        ### norm_text = norm_text.lower()

        # compare that it is different from the original
        if norm_text != text:
            print("# %s --> %s\n\\x%04x \\x%04x\n" % (text, norm_text, i, ord(norm_text[0])))

    except:
        pass

