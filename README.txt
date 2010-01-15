
Object representation

We have:

   fixnums
   characters
   pairs

   booleans
   symbols
   strings
   byte strings
   vectors

Fixnums are not allocated on the heap and are tagged.

conses are tagged and are a pointer to two words on the heap.

Everything else is an indirect object. Tagged as indirect, points to
a structure on the heap which has the type and a variable length.

An indirect object looks like:

type - 1 byte, MSB

The other bytes in the machine word depend on the type, but they
mostly mean "length". Except for booleans which have the immediate
value there and no length.


Tags are the lowest bits.

00 - fixnum
01 - ?! characters ?! unicode in raw representation ? utf8 for the BMP ?
10 - cell
11 - indirect


With raw unicode chars we need to deal with external formats, but
it is easier to do certain operations (indexing!).

ASCII doesn't sound too bad for a start :-)

I've tagged fixnums with 00 so addition works without shifts.

Cells should be aligned on at least 32 bit boundaries so the tag can
be nuked to access the object.

Indirect objects

The tag is placed in the MSB of the machine word, in the MSbits (so we
can reuse the rest).

Variable length tag:

0  - vectors (so they can be large)
10 - byte strings (so we can talk big to the outside world)
110 - strings

111.... - smaller objects

11100000 - empty list
11100001 - symbols
11100010 - booleans
