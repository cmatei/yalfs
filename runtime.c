#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "runtime.h"
#include "xutil.h"

extern object error(char *msg, object o);

#define HEAP_SIZE (128 * 1024 * 1024)
unsigned long heap_size = HEAP_SIZE;

static unsigned long *heap, *freeptr;


static inline object make_the_empty_list()
{
	*freeptr = EMPTY_LIST_TAG;
	return (object) ((unsigned long) freeptr++ | INDIRECT_TAG);
}

static inline object make_boolean(int val)
{
	*freeptr = BOOLEAN_TAG | (val << BOOLEAN_SHIFT);
	return (object) ((unsigned long) freeptr++ | INDIRECT_TAG);
}

void runtime_init()
{
	if (posix_memalign((void **) &heap, sizeof(unsigned long), heap_size))
		FATAL("failed to allocate heap");

	freeptr = heap;

	/* make the empty list object */
	the_empty_list = make_the_empty_list();

	the_falsity = make_boolean(0);
	the_truth   = make_boolean(1);
}

void runtime_stats()
{
	printf("Used %zd heap bytes.\n", (freeptr - heap) * sizeof(unsigned long));
}


object cons(object car_value, object cdr_value)
{
	*freeptr++ = (unsigned long) car_value;
	*freeptr++ = (unsigned long) cdr_value;

	return (object) ((unsigned long) (freeptr - 2) | PAIR_TAG);
}

object safe_car(object o)
{
	if (!is_pair(o))
		return error("Object is not a pair -- CAR", o);

	return fast_car(o);
}

object safe_cdr(object o)
{
	if (!is_pair(o))
		return error("Object is not a pair -- CDR", o);

	return fast_cdr(o);
}

object_type type_of(object o)
{
	if (is_empty_list(o))
		return T_EMPTY_LIST;

	if (is_fixnum(o))
		return T_FIXNUM;

	if (is_character(o))
		return T_CHARACTER;

	if (is_pair(o))
		return T_PAIR;

	if (is_boolean(o))
		return T_BOOLEAN;

	error("Uknown object type -- TYPE-OF", o);
	return T_EMPTY_LIST; /* not reached */
}
