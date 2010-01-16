#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"


static object lisp_cons(object args)
{
	return cons(car(args), cadr(args));
}

static object lisp_plus(object args)
{
	long initial = 0;

	while (!is_null(args)) {
		initial += fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

static object lisp_minus(object args)
{
	long initial;

	if (length(args) < 2)
		error("Need at least two arguments -- -", args);

	initial = fixnum_value(car(args));
	args = cdr(args);

	while (!is_null(args)) {
		initial -= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

static object lisp_multiply(object args)
{
	long initial = 1;

	while (!is_null(args)) {
		initial *= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

static object lisp_divide(object args)
{
	long initial;

	if (length(args) < 2)
		error("Need at least two arguments -- /", args);

	initial = fixnum_value(car(args));
	args = cdr(args);

	while (!is_null(args)) {
		initial /= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

static object lisp_eq(object args)
{
	object initial;

	if (length(args) < 2)
		error("Need at least two arguments -- /", args);

	initial = car(args);
	args = cdr(args);

	while (!is_null(args)) {
		if (initial != car(args))
			return the_falsity;

		args = cdr(args);
	}

	return the_truth;
}


static struct {
	char *name;
	primitive_proc proc;
} the_primitives[] = {
	{ "cons", lisp_cons },

	{ "+", lisp_plus },
	{ "-", lisp_minus },
	{ "*", lisp_multiply },
	{ "/", lisp_divide },

	{ "eq?", lisp_eq },


	{ NULL, NULL }
};

object primitive_names()
{
	object names = nil;
	unsigned long i;

	for (i = 0; the_primitives[i].name; i++) {
		names = cons(make_symbol_c(the_primitives[i].name), names);
	}

	return names;
}

object primitive_objects()
{
	object prims = nil;
	unsigned long i;

	for (i = 0; the_primitives[i].name; i++) {
		prims = cons(make_primitive(the_primitives[i].proc), prims);
	}

	return prims;
}
