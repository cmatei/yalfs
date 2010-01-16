
CC		= gcc -Wall -g -O0
MAKEDEP		= gcc -MM

CFLAGS		+= -D_GNU_SOURCE -DSAFETY=1
INCLUDES	= -I.
LIBS		=

MINIME_SRC	= minime.c io.c runtime.c symbols.c xutil.c
MINIME_OBJ	= $(patsubst %.c,%.o,$(MINIME_SRC))

ALL_SRC		= $(MINIME_SRC)

all: minime

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

minime: $(MINIME_OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	-rm -f minime $(MINIME_OBJ)

tags:
	-etags *.[ch]

cscope:
	-scope -q -b -u *.[ch]

dep:	depend
depend:
	$(MAKEDEP) $(CFLAGS) $(INCLUDES) $(ALL_SRC) > .depends

.PHONY: clean cscope tags depend

include .depends
