
Object representation
=====================

We have:

Direct objects:

   fixnums
   characters
   pairs

Indirect objects:

   booleans
   symbols
   strings
   byte strings
   vectors

Direct Objects
--------------

These are objects that look like heap pointers, but aren't really.
The lowest two bits are a data type tag.

I've tagged fixnums with 00 so addition works without shifts.

Tag values:

00 - fixnum
01 - character (unicode in raw representation ?)
10 - pair
11 - indirect

Fixnums and characters are really their own value shifted
arithmetically to the left.

Pairs are represented as two consecutive words in the heap (the car
and cdr respectively). The pair object is a pointer to them, but due
to the tag, it's off by 2 bytes. Note that the heap is aligned at word
boundaries (4 bytes on 32 bit) so the pair actually points somewhere
in the middle of the car.

Everything else is an indirect object. Tagged as indirect, points to
a structure on the heap which has the actual type and a variable length.

Regarding characters, they're a bit waste of space, but at least they
allow for all unicode, including the astral planes :-)


Indirect objects
----------------

An indirect object has at least one word on the heap. This word may
contain an immediate value (e.g. booleans), or the length of the
object in words (not including this initial one).

The first bits (LSb) of this word are of course the type tag. The tag
is not fixed in bit length. The idea is that for certain types, where
we might need a large length field, we can use the remaining bits in a
fixnum-like manner.

x00 - byte strings
x01 - vectors
x10 - strings
x11 - the others


11111111 - empty list
01111111 - booleans
10111111 - symbols
00111111 - foreign pointer
11011111 - primitive procedure

