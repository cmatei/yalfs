
## temp
INSTALLDIR	= $(shell pwd)

CC		= gcc -Wall -g -O2
CFLAGS		= -DINSTALLDIR="$(INSTALLDIR)"
MAKEDEP		= gcc -MM

CFLAGS		+= -D_GNU_SOURCE -DSAFETY=1
#CFLAGS		+= -D_GNU_SOURCE
INCLUDES	= -I.
LIBS		=

MINIME_SRC	= minime.c environments.c io.c runtime.c symbols.c primitives.c emacs.c xutil.c
MINIME_OBJ	= $(patsubst %.c,%.o,$(MINIME_SRC))

ALL_SRC		= $(MINIME_SRC)

all: minime tests

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

minime: $(MINIME_OBJ)
	$(CC) -o $@ $^ $(LIBS)
	@echo

clean:
	-rm -f minime $(MINIME_OBJ)

tags:
	-etags *.[ch]

cscope:
	-scope -q -b -u *.[ch]

dep:	depend
depend:
	$(MAKEDEP) $(CFLAGS) $(INCLUDES) $(ALL_SRC) > .depends

tests:
	@cd tests && ./run-tests.pl

.PHONY: clean cscope tags depend tests

include .depends
