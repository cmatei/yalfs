#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>

#include "minime.h"

#define HEAP_SIZE (128 * 1024 * 1024)
unsigned long heap_size = HEAP_SIZE;

static unsigned long *heap, *freeptr;

object make_the_empty_list()
{
	*freeptr++ = EMPTY_LIST_TAG;
	return (object) ((unsigned long) (freeptr - 1) | INDIRECT_TAG);
}

object make_the_eof()
{
	*freeptr++ = END_OF_FILE_TAG;
	return (object) ((unsigned long) (freeptr - 1) | INDIRECT_TAG);
}

object make_the_unspecified_value()
{
	*freeptr++ = UNSPECIFIED_VALUE_TAG;
	return (object) ((unsigned long) (freeptr - 1) | INDIRECT_TAG);
}

object make_port(FILE *in, unsigned long port_type)
{
	*freeptr++ = PORT_TAG;
	*freeptr++ = (port_type & PORT_TYPE_MASK);
	*freeptr++ = (unsigned long) in;

	return (object) ((unsigned long) (freeptr - 3) | INDIRECT_TAG);
}

object make_boolean(int val)
{
	*freeptr++ = BOOLEAN_TAG | (val << BOOLEAN_SHIFT);
	return (object) ((unsigned long) (freeptr - 1) | INDIRECT_TAG);
}

object make_foreign_ptr(void *ptr)
{
	*freeptr++ = FOREIGN_PTR_TAG;
	*freeptr++ = (unsigned long) ptr;

	return (object) ((unsigned long) (freeptr - 2) | INDIRECT_TAG);
}

object make_primitive(primitive_proc primitive)
{
	*freeptr++ = PRIMITIVE_PROC_TAG;
	*freeptr++ = (unsigned long) primitive;

	return (object) ((unsigned long) (freeptr - 2) | INDIRECT_TAG);
}

object make_procedure(object parameters, object body, object environment)
{
	*freeptr++ = PROCEDURE_TAG;
	*freeptr++ = (unsigned long) parameters;
	*freeptr++ = (unsigned long) body;
	*freeptr++ = (unsigned long) environment;

	return (object) ((unsigned long) (freeptr - 4) | INDIRECT_TAG);
}

object make_macro(object parameters, object body, object environment)
{
	*freeptr++ = MACRO_TAG;
	*freeptr++ = (unsigned long) parameters;
	*freeptr++ = (unsigned long) body;
	*freeptr++ = (unsigned long) environment;

	return (object) ((unsigned long) (freeptr - 4) | INDIRECT_TAG);
}

object make_string(unsigned long length)
{
	unsigned long words;

	*freeptr++ = STRING_TAG | (length << STRING_SHIFT);

	/* null-terminate */
	*((unsigned char *) freeptr + length) = 0;

	/* round length + 1 to full word */
	words = (length + sizeof(unsigned long)) / sizeof(unsigned long);
	freeptr += words;

	return (object) ((unsigned long) (freeptr - words - 1) | INDIRECT_TAG);
}

object make_string_buffer(char *str, unsigned long length)
{
	object o = make_string(length);
	memcpy(string_value(o), str, length);
	return o;
}

object make_string_c(char *str)
{
	return make_string_buffer(str, strlen(str));
}

object make_symbol(char *str, unsigned long len)
{
	return symbol(str, len);
}

object make_symbol_with_string(object o)
{
	*freeptr++ = SYMBOL_TAG;
	*freeptr++ = (unsigned long) o;

	return (object) ((unsigned long) (freeptr - 2) | INDIRECT_TAG);
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
		error("Object is not a pair -- CAR", o);

	return fast_car(o);
}

object safe_cdr(object o)
{
	if (!is_pair(o))
		error("Object is not a pair -- CDR", o);

	return fast_cdr(o);
}

object list(unsigned long elem, ...)
{
	unsigned long i;
	object head = nil, tail = nil;
	va_list ap;

	va_start(ap, elem);

	for (i = 0; i < elem; i++) {
		if (is_null(head)) {
			head = tail = cons(va_arg(ap, object), nil);
		} else {
			set_cdr(tail, cons(va_arg(ap, object), nil));
			tail = cdr(tail);
		}
	}

	va_end(ap);

	return head;
}

object_type type_of(object o)
{
	if (is_null(o))
		return T_NIL;

	if (is_symbol(o))
		return T_SYMBOL;

	if (is_fixnum(o))
		return T_FIXNUM;

	if (is_character(o))
		return T_CHARACTER;

	if (is_pair(o))
		return T_PAIR;

	if (is_string(o))
		return T_STRING;

	if (is_boolean(o))
		return T_BOOLEAN;

	if (is_foreign_ptr(o))
		return T_FOREIGN_PTR;

	if (is_primitive(o))
		return T_PRIMITIVE;

	if (is_procedure(o))
		return T_PROCEDURE;

	if (is_end_of_file(o))
		return T_EOF;

	if (is_port(o))
		return T_PORT;

	if (is_unspecified(o))
		return T_UNSPECIFIED;

	if (is_macro(o))
		return T_MACRO;

	error("Uknown object type -- TYPE-OF", o);
	return T_NIL; /* not reached */
}


void runtime_init()
{
	if (posix_memalign((void **) &heap, sizeof(unsigned long), heap_size))
		FATAL("failed to allocate heap");

	freeptr = heap;
}

void runtime_stats()
{
	fprintf(stderr, "Used %zd heap bytes.\n", (freeptr - heap) * sizeof(unsigned long));
	symbol_table_stats();
}

unsigned long runtime_current_heap_usage()
{
	return (freeptr - heap) * sizeof(unsigned long);
}

/* as a fixnum, this will wrap in about 3 days on 32 bit */
unsigned long runtime_current_timestamp()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
