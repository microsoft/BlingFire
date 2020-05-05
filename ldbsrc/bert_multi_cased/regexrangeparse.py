#
# helper functions to deal with sets of characters in regexps
#

__max_unicode_char__ = 0x10ffff


#
# parse a range string into a dense array
#
def parse_range(s):

    def num_len(s, i):
        j = i
        while j < len(s) and ( \
                        (ord(s[j]) >= ord('0') and ord(s[j]) <= ord('9')) or \
                        (ord(s[j]) >= ord('a') and ord(s[j]) <= ord('f')) or \
                        (ord(s[j]) >= ord('A') and ord(s[j]) <= ord('F'))):
            j += 1
        return j - i

    res = [0] * (__max_unicode_char__ + 1)
    lens = len(s)
    start = -1
    num = -1
    minus = False
    i = 0
    while i < lens:
        if s[i] == '-':
            minus = True
            i += 1
        if i <= lens - 2 and s[i] == '\\' and s[i+1] == 'x':
            numLen = num_len(s, i + 2) + 2
            numStr = s[i+1:i+numLen]
            num = int("0" + numStr, 16)
            if num < 0 or num > __max_unicode_char__:
                raise ValueError("Cannot parse range, 1: %s" % s)
            if minus:
                if start != -1:
                    for j in range(start, num + 1):
                        res[j] = 1
                start = -1
                minus = False
            else:
                if start != -1:
                    res[start] = 1
                start = num
            i += numLen
        else:
            # TODO: add parsing for other cases
            raise ValueError("Non hex number or range encountered, Cannot parse range, 2: %s" % s)
            i += 1

    if start != -1:
        res[start] = 1

    return res


#
# serialize dense array as a range string
#
def serialize_range(r):

    def num2hex(i):
        h = hex(i)[2:]
        c = 4 - len(h)
        while c > 0:
            h = "0" + h
            c -= 1
        return '\\x' + h

    res = ""
    start = -1
    for i in range(__max_unicode_char__ + 1):
        if r[i] == 1 and start == -1:
            start = i
        elif r[i] == 0 and start != -1:
            if i - 1 != start:
                res += num2hex(start) + "-" + num2hex(i-1)
            else:
                res += num2hex(start)
            start = -1
    if start != -1:
        if __max_unicode_char__ != start:
            res += num2hex(start) + "-" + num2hex(__max_unicode_char__)
        else:
            res += num2hex(start)
    return res


#
# subtract one range from another
#
def subtract_range(r, minus_r):
    res = [0] * (__max_unicode_char__ + 1)
    for i in range(__max_unicode_char__ + 1):
        if minus_r[i] == 0 and r[i] == 1:
            res[i] = 1
    return res


#
# union two ranges
#
def union_range(r1, r2):
    res = [0] * (__max_unicode_char__ + 1)
    for i in range(__max_unicode_char__ + 1):
        if r1[i] == 1 or r2[i] == 1:
            res[i] = 1
    return res


#
# intersect two ranges
#
def intersect_range(r1, r2):
    res = [0] * (__max_unicode_char__ + 1)
    for i in range(__max_unicode_char__ + 1):
        if r1[i] == 1 and r2[i] == 1:
            res[i] = 1
    return res


#
# negate a range
#
def not_range(r):
    res = [0] * (__max_unicode_char__ + 1)
    for i in range(__max_unicode_char__ + 1):
        if r[i] == 0:
            res[i] = 1
    return res


#test = "\\x0041-\\x005a\\x0061-\\x007a\\x00aa\\x00b5\\x00ba\\x00c0-\\x00d6\\x00d8-\\x00f6\\x00f8-\\x0236\\x0250-\\x02c1\\x02c6-\\x02d1\\x02e0-\\x02e4\\x02ee\\x037a\\x0386\\x0388-\\x038a\\x038c"
#r = parse_range(test)
#s = serialize_range(r)
#print(s == test)
