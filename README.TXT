
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
contain an immediate value (e.g. the two booleans), or the length of
the object in words (not including this initial one).

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
01011111 - interpreted procedure
10011111 - port
00011111 - end-of-file
11101111 - unspecified value
01101111 - macro

Syntactic Extensions (Macros)
=============================

There are only a few primitive expression types in R5RS: variables,
literal expressions, procedure calls, lambda, if, set! and define.
Everything else is derived syntax and can be defined using macros
(cond, let, and, or, etc). There are also no reserved keywords.

Implementing syntactic expressions (primitive or derived) straight
into the interpreter as special evaluation rules has the problem that
it forever "fixes" their meaning, and does not allow them to be
rebound.

My solution is to bind the identifiers used as syntactic keywords in
the initial environment as primitive procedures, which are however not
meant to be ever actually applied. Instead, when the evaluator sees
such a binding for an "application", it knows it must apply the
special evaluation rules.

When the evaluator sees an expression, it follows these rules:

 - is it a constant ? return it
 - is it a variable ? look it up in the environment
 - it must be an application-thingie then.
      evaluate the operator in the environment

      is it bound to primitive_if?
      	 evaluate if_predicate, then tail call for the consequent/alternate

      is it bound to primitive_cond?
         transform the expression to nested ifs, then tail call for them

      [...]

      is it a macro?
         TBD (macroexand then tail call to eval it)

      is it really an application?
      	 apply it


Lexical scoping should then take care of potential redefinitions.

What this also allows me is to actually use a proper scheme as
implementation language for the macro facilities.

