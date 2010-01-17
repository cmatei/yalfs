#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"

#define boolean(c_int) ((c_int) ? the_truth : the_falsity)

static inline void check_args(long n, object args, char *name)
{
	char errbuf[64];
	unsigned long nargs;

	nargs = length(args);
	if (nargs != n) {
		snprintf(errbuf, 64,
			 "Expecting %ld arguments but was sent %lu -- %s",
			 n, nargs, name);
		error(errbuf, args);
	}
}

/* Numerical operations */

object lisp_integerp(object args)
{
	check_args(1, args, "integer?");

	return boolean(is_fixnum(car(args)));
}

object lisp_numberp(object args)
{
	return lisp_integerp(args);
}

object lisp_equal(object args)
{
	object prec = nil;

	while (!is_null(args)) {
		if ((prec != nil) && (car(args) != prec))
			return the_falsity;

		prec = car(args);
		args = cdr(args);
	}

	return the_truth;
}

object lisp_decreasing(object args)
{
	object prec = nil;

	while (!is_null(args)) {
		if ((prec != nil) && (fixnum_value(prec) >= fixnum_value(car(args))))
			return the_falsity;

		prec = car(args);
		args = cdr(args);
	}

	return the_truth;
}

object lisp_increasing(object args)
{
	object prec = nil;

	while (!is_null(args)) {
		if ((prec != nil) && (fixnum_value(prec) <= fixnum_value(car(args))))
			return the_falsity;

		prec = car(args);
		args = cdr(args);
	}
	return the_truth;
}

object lisp_nondecreasing(object args)
{
	object prec = nil;

	while (!is_null(args)) {
		if ((prec != nil) && (fixnum_value(prec) > fixnum_value(car(args))))
			return the_falsity;

		prec = car(args);
		args = cdr(args);
	}
	return the_truth;
}

object lisp_nonincreasing(object args)
{
	object prec = nil;

	while (!is_null(args)) {
		if ((prec != nil) && (fixnum_value(prec) < fixnum_value(car(args))))
			return the_falsity;

		prec = car(args);
		args = cdr(args);
	}
	return the_truth;
}

object lisp_zerop(object args)
{
	check_args(1, args, "zero?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- zero?", args);

	return boolean(fixnum_value(car(args)) == 0);
}

object lisp_positivep(object args)
{
	check_args(1, args, "positive?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- positive?", args);

	return boolean(fixnum_value(car(args)) > 0);
}

object lisp_negativep(object args)
{
	check_args(1, args, "negative?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- negative?", args);

	return boolean(fixnum_value(car(args)) < 0);
}

object lisp_oddp(object args)
{
	check_args(1, args, "oddp?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- oddp?", args);

	return boolean((fixnum_value(car(args)) % 2) == 1);
}

object lisp_evenp(object args)
{
	check_args(1, args, "evenp?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- evenp?", args);

	return boolean((fixnum_value(car(args)) % 2) == 0);
}

object lisp_max(object args)
{
	object max = nil;

	if (length(args) < 1)
		error("Expecting at least 1 argument -- max", args);

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- max", args);

		if ((max == nil) || (fixnum_value(max) < fixnum_value(car(args)))) {
			max = car(args);
		}

		args = cdr(args);
	}
	return max;
}

object lisp_min(object args)
{
	object min = nil;

	if (length(args) < 1)
		error("Expecting at least 1 argument -- min", args);

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- min", args);

		if ((min == nil) || (fixnum_value(min) > fixnum_value(car(args)))) {
			min = car(args);
		}

		args = cdr(args);
	}
	return min;
}


object lisp_plus(object args)
{
	long initial = 0;

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- +", args);

		initial += fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

object lisp_multiply(object args)
{
	long initial = 1;

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- *", args);

		initial *= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

object lisp_minus(object args)
{
	long initial = 0;

	if (length(args) < 1)
		error("Expecting at least 1 argument -- -", args);

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- -", args);

		initial -= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}


object lisp_divide(object args)
{
	long initial = 1;

	if (length(args) < 1)
		error("Expecting at least 1 argument -- /", args);

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- /", args);

		if (lisp_zerop(car(args)) == the_truth)
			error("Will not divide by zero -- /", args);

		initial /= fixnum_value(car(args));
		args = cdr(args);
	}

	return make_fixnum(initial);
}

object lisp_abs(object args)
{
	check_args(1, args, "abs");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- abs", args);

	return (fixnum_value(car(args)) < 0) ?
		make_fixnum(- fixnum_value(car(args))) :
		car(args);
}

/* not returns #t if obj is false, and in scheme only #f is false */
object lisp_not(object args)
{
	check_args(1, args, "not");

	return (car(args) == the_falsity) ? the_truth : the_falsity;
}

object lisp_booleanp(object args)
{
	check_args(1, args, "boolean?");

	return boolean(((car(args) == the_truth) || (car(args) == the_falsity)));
}



object lisp_cons(object args)
{
	check_args(2, args, "cons");
	return cons(car(args), cadr(args));
}



object lisp_eq(object args)
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

	/* Numerical operations */
	{ "number?",   lisp_numberp       },
	{ "integer?",  lisp_integerp      },

	{ "=",         lisp_equal         },
	{ "<",         lisp_increasing    },
	{ ">",         lisp_decreasing    },
	{ "<=",        lisp_nondecreasing },
	{ ">=",        lisp_nonincreasing },

	{ "zero?",     lisp_zerop         },
	{ "positive?", lisp_positivep     },
	{ "negative?", lisp_negativep     },
	{ "odd?",      lisp_oddp          },
	{ "evenp?",    lisp_evenp         },

	{ "max",       lisp_max           },
	{ "min",       lisp_min           },

	{ "+",         lisp_plus          },
	{ "*",         lisp_multiply      },
	{ "-",         lisp_minus         },
	{ "/",         lisp_divide        },

	{ "abs",       lisp_abs           },
//	{ "quotient",  lisp_quotient },
//	{ "remainder", lisp_remainder },
//	{ "modulo",    lisp_modulo },

//	{ "gcd",       lisp_gcd },
//	{ "lcm",       lisp_lcm },

//	{ "number->string", lisp_number_string },
//	{ "string->number", lisp_string_number },

	/* Booleans */
	{ "not", lisp_not },
	{ "boolean?", lisp_booleanp },

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
