#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ucontext.h>

#include "runtime.h"
#include "xutil.h"



#define HEAP_SIZE (128 * 1024 * 1024)

unsigned long heap_size = HEAP_SIZE;

static unsigned long *heap, *freeptr;

void runtime_init()
{
	if (posix_memalign((void **) &heap, sizeof(unsigned long), heap_size))
		FATAL("failed to allocate heap");

	freeptr = heap;
}


object cons(object car_value, object cdr_value)
{
	*freeptr++ = (unsigned long) car_value;
	*freeptr++ = (unsigned long) cdr_value;

	return (object) ((unsigned long) (freeptr - 2) | PAIR_TAG);
}
