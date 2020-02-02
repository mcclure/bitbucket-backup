# Misc helper functions/classes

import codecs
import sys

# Is this python 3?
py3 = sys.version_info >= (3, 0)

# Take a path to a UTF-8 or UTF-16 file. Return an object to be used with utflines()
def utfOpen(path):
    with open(path, 'rb') as f:
        start = f.read(2) # Check first bytes for BOM
        utf16 = start.startswith(codecs.BOM_UTF16_BE) or start.startswith(codecs.BOM_UTF16_LE)
    return codecs.open(path, 'r', 'utf-16' if utf16 else 'utf-8-sig')

# Iterator that sweeps a file 1 character at a time
def fileChars(f):
    while True:
        ch = f.read(1)
        if ch:
            yield ch
        else:
            return

# String to unicode
def utf8string(s):
    if py3:
        return s
    return codecs.decode(s, 'utf-8')

# Unicode to bytes
def streamable(s):
    if py3:
        return s
    return codecs.encode(s, 'utf-8')

def unicodeJoin(joiner, ary):
    return joiner.join(unicode(v) for v in ary)

def quotedString(content):
    return u'"%s"' % (repr(content)[2:-1]) # FIXME: This only works because u'' and is not python3 compatible

# Switch statement adapted from http://code.activestate.com/recipes/410692/
# Used under the terms of the PSF license.

class switch(object):
    def __init__(self, value):
        self.value = value

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
    
    def match(self, arg):
        """Indicate whether or not to enter a case suite"""
        if self.value == arg:
            return True
        else:
            return False

# FIXME: Maybe this is not such a good thing.
class dynamicSwitch(object):
    def __init__(self, value, key):
        self.value = value
        self.key = key

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
    
    def match(self, arg):
        """Indicate whether or not to enter a case suite"""
        if getattr(self.value, self.key) == arg:
            return True
        else:
            return False
