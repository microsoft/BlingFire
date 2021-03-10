#
# This is a syllabification model, input should be one word, output is syllabified word
#

~/BlingFire/ldbsrc$ printf "syllabified\nsyllabification\ndescomposición\nen\nsílabas\nразбивка\nна\nслога\n" | \
                    test_ldb --ldb=ldb/syllab.bin --command=w2h

syl[=]la[=]bi[=]fied
syl[=]la[=]bi[=]fi[=]ca[=]tion
des[=]com[=]po[=]si[=]ción
en
sí[=]la[=]bas
раз[=]бивка
на
сло[=]га


# This model contains words for several European languages. 

# To add another language, add 100K-200K hyphenated words into the end of the w2h.dict.utf8 and recompile.

# A new function is added to the API:
## C++/C# WordHyphenationWithModel
## Python word_hyphenation_with_model

Example in Python:

In [1]: from blingfire import *

In [2]: h = load_model('/home/sergei/BlingFire2/BlingFire/dist-pypi/blingfire/syllab.bin')

In [3]: print(word_hyphenation_with_model(h, 'syllabification'))
syl-la-bi-fi-ca-tion

In [4]: print(word_hyphenation_with_model(h, 'syllabified'))
syl-la-bi-fied

In [5]: free_model(h)
