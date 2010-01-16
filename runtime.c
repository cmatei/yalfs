#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "minime.h"

#define HEAP_SIZE (128 * 1024 * 1024)
unsigned long heap_size = HEAP_SIZE;

static unsigned long *heap, *freeptr;

static inline object make_the_empty_list()
{
	*freeptr++ = EMPTY_LIST_TAG;
	return (object) ((unsigned long) (freeptr - 1) | INDIRECT_TAG);
}

static inline object make_boolean(int val)
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

object make_string(unsigned long length)
{
	unsigned long words;

	*freeptr++ = STRING_TAG | (length << STRING_SHIFT);

	/* round to full word */
	words = (length + sizeof(unsigned long) - 1) / sizeof(unsigned long);
	freeptr += words;

	return (object) ((unsigned long) (freeptr - words - 1) | INDIRECT_TAG);
}

object make_string_c(char *str, unsigned long length)
{
	object o = make_string(length);
	memcpy(string_value(o), str, length);
	return o;
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

	if (is_boolean(o))
		return T_BOOLEAN;

	if (is_foreign_ptr(o))
		return T_FOREIGN_PTR;

	if (is_string(o))
		return T_STRING;

	error("Uknown object type -- TYPE-OF", o);
	return T_NIL; /* not reached */
}

void runtime_init()
{
	if (posix_memalign((void **) &heap, sizeof(unsigned long), heap_size))
		FATAL("failed to allocate heap");

	freeptr = heap;

	/* make the empty list object */
	nil = make_the_empty_list();

	/* the booleans */
	the_falsity = make_boolean(0);
	the_truth   = make_boolean(1);

	/* uses nil */
	symbol_table_init();

	/* expression keywords */
	_quote            = make_symbol_c("quote");
	_lambda           = make_symbol_c("lambda");
	_if               = make_symbol_c("if");
	_set              = make_symbol_c("set!");
	_begin            = make_symbol_c("if");
	_cond             = make_symbol_c("cond");
	_and              = make_symbol_c("and");
	_or               = make_symbol_c("or");
	_case             = make_symbol_c("case");
	_let              = make_symbol_c("let");
	_letx             = make_symbol_c("let*");
	_letrec           = make_symbol_c("letrec");
	_do               = make_symbol_c("do");
	_quasiquote       = make_symbol_c("quasiquote");

        /* other syntactic keywords */
	_else             = make_symbol_c("else");
	_implies          = make_symbol_c("=>");
	_define           = make_symbol_c("define");
	_unquote          = make_symbol_c("unquote");
	_unquote_splicing = make_symbol_c("unquote-splicing");

}

void runtime_stats()
{
	printf("Used %zd heap bytes.\n", (freeptr - heap) * sizeof(unsigned long));

	symbol_table_stats();
}


