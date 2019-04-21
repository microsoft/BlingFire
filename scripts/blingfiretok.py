import os
from ctypes import *
import inspect
import os.path
import numpy as np
import platform

filename = inspect.getframeinfo(inspect.currentframe()).filename
path = os.path.dirname(os.path.abspath(filename))

# load the DLL
blingfire = None

# detect windows
if platform.system() == "Windows":
    blingfire = cdll.LoadLibrary(os.path.join(path, "blingfiretokdll.dll"))
# detect Mac OSX
elif platform.system() == "Darwin":
    pass
else:
# detect linux
    if platform.libc_ver()[0] == 'glibc' and platform.libc_ver()[1][0] == '2':
        blingfire = cdll.LoadLibrary(os.path.join(path, "libblingfiretokdll_1404.so"))
    else:
        blingfire = cdll.LoadLibrary(os.path.join(path, "libblingfiretokdll.so"))


def text_to_sentences(s):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 2)
    o_bytes_count = len(o_bytes)

    # identify paragraphs
    o_len = blingfire.TextToSentences(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def text_to_words(s):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 2)
    o_bytes_count = len(o_bytes)

    # identify paragraphs
    o_len = blingfire.TextToWords(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


# returns the current version of the DLL's algo
def get_blingfiretok_version():
    return blingfire.GetBlingFireTokVersion()


# Returns numpy array with data type unsigned int32 array
def text_to_hashes(s, word_n_grams, bucketSize):
    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = (c_uint32 * (2 * len(s_bytes)))()
    o_bytes_count = len(o_bytes)

    # identify paragraphs
    o_len = blingfire.TextToHashes(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_int(word_n_grams), c_int(bucketSize))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # return numpy array without copying
    return np.frombuffer(o_bytes, dtype=c_uint32, count = o_len)

