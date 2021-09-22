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
    blingfire = cdll.LoadLibrary(os.path.join(path, "libblingfiretokdll.dylib"))
else:
# detect linux
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


def text_to_sentences_with_model(h, s):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 2)
    o_bytes_count = len(o_bytes)

    # identify sentences
    o_len = blingfire.TextToSentencesWithModel(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_void_p(h))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def normalize_spaces(s, uSpace = 0x20):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 3)
    o_bytes_count = len(o_bytes)

    # normalize spaces
    o_len = blingfire.NormalizeSpaces(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_int(uSpace))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def text_to_words(s):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 3)
    o_bytes_count = len(o_bytes)

    # identify words
    o_len = blingfire.TextToWords(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def text_to_words_with_model(h, s):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 3)
    o_bytes_count = len(o_bytes)

    # identify words
    o_len = blingfire.TextToWordsWithModel(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_void_p(h))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''

    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def word_hyphenation_with_model(h, s, uHy = 0x2D):

    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")

    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 4)
    o_bytes_count = len(o_bytes)

    # identify words
    o_len = blingfire.WordHyphenationWithModel(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_void_p(h), c_int(uHy))

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
    o_bytes = (c_int32 * (word_n_grams * len(s_bytes)))()
    o_bytes_count = len(o_bytes)

    # identify paragraphs
    o_len = blingfire.TextToHashes(c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_int(word_n_grams), c_int(bucketSize))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return None

    # return numpy array without copying
    return np.frombuffer(o_bytes, dtype=c_int32, count=o_len)


def text_to_token_with_offsets(s, text_to_token_f, split_byte):    
    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")
 
    # allocate the output buffer
    o_bytes = create_string_buffer(len(s_bytes) * 2)
    o_bytes_count = len(o_bytes)
    
    # buffers for word beging and end
    o_start_offsets = (c_int32 * o_bytes_count)()
    o_end_offsets = (c_int32 * o_bytes_count)()
 
    # identify paragraphs
    o_len = text_to_token_f(
        c_char_p(s_bytes), c_int(len(s_bytes)), 
        byref(o_bytes), byref(o_start_offsets), byref(o_end_offsets), 
        c_int(o_bytes_count))

    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return '', []

    num_tokens = o_bytes.value.count(split_byte) + 1
          
    utf8_offsets = [ o for start_end in zip(o_start_offsets[:num_tokens], o_end_offsets[:num_tokens]) for o in start_end]
    
    string_offsets = []    
    
    # Map the utf8 offsets to offsets in the original string - will break for "extended grapheme" 
    # This seems to be undoing the FAUtf8Size based mapping being done inside TextToWordsWithOffsets and 
    # TextToSentencesWithOffsets.     
    string_offset = 0
    is_end_offset = False
    for utf8_offset, b in enumerate(s_bytes):
        if b & 0xC0 != 0x80:
            while len(string_offsets) < len(utf8_offsets) and utf8_offsets[len(string_offsets)] + is_end_offset == utf8_offset:
                string_offsets.append(string_offset)
                is_end_offset = not is_end_offset
            string_offset += 1
 
    if len(string_offsets) < num_tokens * 2:
        string_offsets.append(len(s))
            
    assert len(string_offsets) == num_tokens * 2, '%s != %s' % (len(string_offsets), num_tokens * 2)
 
    token_begin_end = [ (b, e) for b, e in zip(string_offsets[::2], string_offsets[1::2])]
        
    # compute the unicode string from the UTF-8 bytes
    out_string = o_bytes.value.decode('utf8') 
    
    return out_string, token_begin_end
 
def text_to_words_with_offsets(s):
    return text_to_token_with_offsets(s, blingfire.TextToWordsWithOffsets, ord(' '))
 
def text_to_sentences_and_offsets(s):
    return text_to_token_with_offsets(s, blingfire.TextToSentencesWithOffsets, ord('\n'))


def load_model(file_name):
    s_bytes = file_name.encode("utf-8")
    load_model_fn = blingfire.LoadModel
    load_model_fn.restype = c_void_p
    h = load_model_fn(c_char_p(s_bytes))
    return h


def free_model(h):
    free_model_fn = blingfire.FreeModel
    free_model_fn.argtypes = [c_void_p]
    free_model_fn(c_void_p(h))


def text_to_ids(h, s, max_len, unk = 0, no_padding = False):
    # get the UTF-8 bytes
    s_bytes = s.encode("utf-8")
    # allocate the output buffer
    o_bytes = (c_int32 * max_len)()
    o_bytes_count = len(o_bytes)
    # fill in the ids
    t_count = blingfire.TextToIds(c_void_p(h), c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), c_int(o_bytes_count), c_int(unk))
    out_count = min (o_bytes_count, t_count) if no_padding else o_bytes_count
    # return numpy array without copying
    return np.frombuffer(o_bytes, dtype=c_uint32, count = out_count)


def ids_to_text(h, ids, skip_special_tokens = True, output_buffer_size = None):
    # allocate the output buffer
    if output_buffer_size is None:
        output_buffer_size = len(ids) * 32
    # allocate the output buffer
    o_bytes = create_string_buffer(output_buffer_size)
    o_bytes_count = len(o_bytes)
    # compute the text from ids
    o_len = blingfire.IdsToText(c_void_p(h), c_void_p(ids.__array_interface__['data'][0]), len(ids), byref(o_bytes), c_int(o_bytes_count), c_bool(skip_special_tokens))
    # check if no error has happened
    if -1 == o_len or o_len > o_bytes_count:
        return ''
    # compute the unicode string from the UTF-8 bytes
    return o_bytes.value.decode('utf-8')


def utf8text_to_ids_with_offsets(h, s_bytes, max_len, unk = 0, no_padding = False):
    # allocate the output buffers
    o_bytes = (c_int32 * max_len)()
    o_bytes_starts = (c_int32 * max_len)()
    o_bytes_ends = (c_int32 * max_len)()
    o_bytes_count = len(o_bytes)
    # fill in the ids
    t_count = blingfire.TextToIdsWithOffsets(c_void_p(h), c_char_p(s_bytes), c_int(len(s_bytes)), byref(o_bytes), byref(o_bytes_starts), byref(o_bytes_ends), c_int(o_bytes_count), c_int(unk))
    out_count = min (o_bytes_count, t_count) if no_padding else o_bytes_count
    # return numpy array without copying
    return ( np.frombuffer(o_bytes, dtype=c_uint32, count = out_count), 
             np.frombuffer(o_bytes_starts, dtype=c_int32, count = out_count), 
             np.frombuffer(o_bytes_ends, dtype=c_int32, count = out_count) )


def change_settings_dummy_prefix(h, add_prefix):
    blingfire.SetNoDummyPrefix(c_void_p(h), c_int(not add_prefix))
