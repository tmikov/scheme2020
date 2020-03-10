#!/usr/bin/env python

from __future__ import print_function

tab = [[] for i in range(0, 256)]


def addSingleTag(tag, i):
    if not tab[i].count(tag):
        tab[i].append(tag)


def addTag(tag, i):
    if type(tag) is tuple:
        for t in tag:
            addTag(t, i)
    else:
        addSingleTag(tag, i)


def addRange(tag, a, b):
    if type(a) is str:
        a = ord(a)
    if type(b) is str:
        b = ord(b)
    for i in range(a, b + 1):
        addTag(tag, i)


def add(tag, *elems):
    for i in elems:
        if type(i) is str:
            i = ord(i)
        addTag(tag, i)


def genTable():
    for i in range(0, 256):
        print("  /* %3d, " % i, end="")
        if i < 32 or i >= 127:
            print("0x%02x" % i, end="")
        else:
            print("'%s' " % chr(i), end="")
        print(" */ ", end="")

        prev = False
        tab[i].sort()
        for j in tab[i]:
            if prev:
                print(" | ", end="")
            prev = True
            print(j, end="")
        if not prev:
            print(0, end="")
        print(",")


SUBSEQUENT = "CC::Subsequent"
DELIMITER = "CC::Delimiter"
WHITESPACE = ("CC::WhitespaceClass", DELIMITER)
UTF8 = "CC::UTF8Class"
PECULIAR_IDENT = "CC::PeculiarIdentClass"

DOT_SUBSEQUENT = "CC::DotSubsequent"
SIGN_SUBSEQUENT = ("CC::SignSubsequent", DOT_SUBSEQUENT)
INITIAL = ("CC::InitialClass", SIGN_SUBSEQUENT)

DIGIT = "CC::DigitClass"

addRange(INITIAL, 'a', 'z')
addRange(INITIAL, 'A', 'Z')
add(INITIAL, '!', '$', '%', '&', '*', '/', ':', '<', '=', '>', '?', '^', '_',
    '~', '@')

addRange(SUBSEQUENT, 'a', 'z')
addRange(SUBSEQUENT, 'A', 'Z')
add(SUBSEQUENT, '!', '$', '%', '&', '*', '/', ':', '<', '=', '>', '?', '^', '_',
    '~')
addRange(SUBSEQUENT, '0', '9')
add(SUBSEQUENT, '+', '-', '.', '@')

add(WHITESPACE, ' ', '\n', '\r', '\v', '\t')

add(PECULIAR_IDENT, '+', '-', '.')
add(SIGN_SUBSEQUENT, '+', '-', '@')
add(DOT_SUBSEQUENT, '.')

addRange(DIGIT, '0', '9')
add(DELIMITER, '|', '(', ')', '[', ']', '{', '}', '"', ';')

addRange(UTF8, 128, 255)

genTable()
