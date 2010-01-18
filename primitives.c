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

static inline int is_last_elt(object lst)
{
	return (cdr(lst) == nil);
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

#define number_fun(lname, cname, op)					\
object cname(object args)					        \
{									\
	object prec = nil;						\
	object arg;							\
									\
	while (!is_null(args)) {					\
		arg = car(args);					\
									\
		if (!is_number(arg))					\
			error("Expecting numbers -- " lname,	arg);	\
									\
		fprintf(stderr, "%ld\n", fixnum_value(arg));		\
		if ((prec != nil) &&					\
		    !(fixnum_value(prec) op fixnum_value(arg)))         \
			return the_falsity;				\
									\
		prec = arg;						\
		args = cdr(args);					\
	}								\
	return the_truth;						\
}

number_fun("=",  lisp_equal, ==)
number_fun("<",  lisp_increasing, <)
number_fun(">",  lisp_decreasing, >)
number_fun("<=", lisp_nondecreasing, <=)
number_fun(">=", lisp_nonincreasing, >=)

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
		error("Expecting a number -- odd?", args);

	return boolean((fixnum_value(car(args)) % 2) != 0);
}

object lisp_evenp(object args)
{
	check_args(1, args, "evenp?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- even?", args);

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
	long nargs = length(args);

	if (nargs < 1)
		error("Expecting at least 1 argument -- -", args);

	if (nargs > 1) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- -", args);

		initial = fixnum_value(car(args));
		args = cdr(args);
	}

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
	long nargs = length(args);

	if (nargs < 1)
		error("Expecting at least 1 argument -- /", args);

	if (nargs > 1) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- /", args);

		initial = fixnum_value(car(args));
		args = cdr(args);
	}

	while (!is_null(args)) {
		if (!is_fixnum(car(args)))
			error("Expecting numbers -- /", args);

		if (fixnum_value(car(args)) == 0)
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



/* Booleans */

object lisp_not(object args)
{
	check_args(1, args, "not");

        /* not returns #t if obj is false, and in scheme only #f is false */
	return (car(args) == the_falsity) ? the_truth : the_falsity;
}

object lisp_booleanp(object args)
{
	check_args(1, args, "boolean?");

	return boolean(((car(args) == the_truth) || (car(args) == the_falsity)));
}


/* Pairs and lists */

object lisp_pairp(object args)
{
	check_args(1, args, "pair?");

	return boolean(is_pair(car(args)));
}

object lisp_cons(object args)
{
	check_args(2, args, "cons");
	return cons(car(args), cadr(args));
}



#define pair_fun(X)					\
object lisp_##X (object args)			        \
{						        \
	check_args(1, args, #X );			\
	if (!is_pair(car(args)))			\
                error("Expecting a pair -- " #X, args); \
							\
	return  X(car(args));				\
}

pair_fun(car)
pair_fun(cdr)
pair_fun(caar)
pair_fun(cadr)
pair_fun(cdar)
pair_fun(cddr)
pair_fun(caaar)
pair_fun(caadr)
pair_fun(cadar)
pair_fun(caddr)
pair_fun(cdaar)
pair_fun(cdadr)
pair_fun(cddar)
pair_fun(cdddr)
pair_fun(caaaar)
pair_fun(caaadr)
pair_fun(caadar)
pair_fun(caaddr)
pair_fun(cadaar)
pair_fun(cadadr)
pair_fun(caddar)
pair_fun(cadddr)
pair_fun(cdaaar)
pair_fun(cdaadr)
pair_fun(cdadar)
pair_fun(cdaddr)
pair_fun(cddaar)
pair_fun(cddadr)
pair_fun(cdddar)
pair_fun(cddddr)

object lisp_set_car(object args)
{
	check_args(2, args, "set-car!");

	if (!is_pair(car(args)))
		error("Expecting a pair as first argument -- set-car!", args);

	return set_car(car(args), cadr(args));
}

object lisp_set_cdr(object args)
{
	check_args(2, args, "set-cdr!");

	if (!is_pair(car(args)))
		error("Expecting a pair as first argument -- set-cdr!", args);

	return set_cdr(car(args), cadr(args));
}

object lisp_nullp(object args)
{
	check_args(1, args, "null?");
	return boolean(is_null(car(args)));
}

object lisp_listp(object args)
{
	check_args(1, args, "list?");
	return boolean(is_list(car(args)));
}

object lisp_list(object args)
{
	object head = nil, tail = nil;

	while (!is_null(args)) {

		if (head == nil) {
			head = tail = cons(car(args), nil);
		} else {
			set_cdr(tail, cons(car(args), nil));
			tail = cdr(tail);
		}

		args = cdr(args);
	}

	return head;
}

object lisp_length(object args)
{
	check_args(1, args, "length");

	if (!is_list(car(args)))
		error("Object is not a proper list -- length", args);

	return make_fixnum(length(car(args)));
}

object lisp_append(object args)
{
	object head = nil, tail = nil, lst;

	if (is_null(args))
		return nil;

	while (!is_last_elt(args)) {

		if (!is_list(car(args)))
			error("Expecting lists -- append", car(args));

		lst = car(args);
		while (!is_null(lst)) {

			if (head == nil) {
				head = tail = cons(car(lst), nil);
			} else {
				set_cdr(tail, cons(car(lst), nil));
				tail = cdr(tail);
			}

			lst = cdr(lst);
		}

		args = cdr(args);
	}

	if (tail == nil)
		return car(args);
	else
		set_cdr(tail, car(args));

	return head;
}

object lisp_reverse(object args)
{
	object tail = nil;
	object lst;

	check_args(1, args, "reverse");
	if (!is_list(car(args)))
		error("Expecting a list -- reverse", args);

	if (is_null(car(args)))
		return nil;

	lst = car(args);

	while (!is_last_elt(lst)) {
		tail = cons(car(lst), tail);
		lst = cdr(lst);
	}

	return cons(car(lst), tail);
}

object lisp_list_tail(object args)
{
	object lst;
	long k;

	check_args(2, args, "list-tail");
	if (!is_list(car(args)))
		error("Expecting a list -- list-tail", car(args));

	if (!is_fixnum(cadr(args)))
		error("Expecting a number -- list-tail", cadr(args));

	k = fixnum_value(cadr(args));
	if (k < 0)
		error("Expecting an index integer -- list-tail", cadr(args));

	lst = car(args);
	while (k > 0) {
		if (is_null(lst))
			error("List too short -- list-tail", car(args));

		lst = cdr(lst);
		k--;
	}

	return lst;
}

object lisp_list_ref(object args)
{
	object lst;

	lst = lisp_list_tail(args);
	if (is_null(lst))
		error("List is empty -- list-ref", car(args));

	return car(lst);
}


object lisp_symbolp(object args)
{
	check_args(1, args, "symbol?");
	return boolean(is_symbol(car(args)));
}

object lisp_symbol_string(object args)
{
	check_args(1, args, "symbol->string");
	if (!is_symbol(car(args)))
		error("Object is not a symbol -- symbol->string", car(args));

	return symbol_string(car(args));
}

object lisp_string_symbol(object args)
{
	check_args(1, args, "string->symbol");
	if (!is_string(car(args)))
		error("Expecting a string -- string->symbol", car(args));

	return symbol(string_value(car(args)), string_length(car(args)));
}


object lisp_charp(object args)
{
	check_args(1, args, "char?");
	return boolean(is_character(car(args)));
}

#define identity(x) (x)
#define char_fun(lname, cname, operator, casefold)			\
object cname(object args)					        \
{									\
	object prec = nil;						\
									\
	while (!is_null(args)) {					\
		if (!is_character(car(args)))				\
			error("Expecting characters -- " lname,		\
			      car(args));				\
									\
		if (prec != nil) {					\
		        if ( casefold(character_value(prec)) operator	\
			     casefold(character_value(car(args))))	\
				return the_falsity;			\
									\
		}							\
									\
		prec = car(args);					\
		args = cdr(args);					\
	}								\
	return the_truth;						\
}

char_fun("char=?",  lisp_char_equal,               !=, identity)
char_fun("char<?",  lisp_char_increasing,          >=, identity)
char_fun("char>?",  lisp_char_decreasing,          <=, identity)
char_fun("char<=?", lisp_char_non_decreasing,       >, identity)
char_fun("char>=?", lisp_char_non_increasing,       <, identity)

char_fun("char-ci=?",  lisp_char_ci_equal,          !=, tolower)
char_fun("char-ci<?",  lisp_char_ci_increasing,     >=, tolower)
char_fun("char-ci>?",  lisp_char_ci_decreasing,     <=, tolower)
char_fun("char-ci<=?", lisp_char_ci_non_decreasing,  >, tolower)
char_fun("char-ci>=?", lisp_char_ci_non_increasing,  <, tolower)

#define char_type_fun(lname, cname, test)	            \
object cname(object args)			            \
{							    \
        check_args(1, args, lname);			    \
	if (!is_character(car(args)))			    \
		error("Expecting a character -- " lname,    \
		      car(args));			    \
							    \
	return boolean(test(character_value(car(args))));   \
}

char_type_fun("char-alphabetic?", lisp_char_alphabeticp, isalpha)
char_type_fun("char-numeric?",    lisp_char_numericp, isdigit)
char_type_fun("char-whitespace?", lisp_char_whitespacep, isspace)
char_type_fun("char-upper-case?", lisp_char_uppercasep, isupper)
char_type_fun("char-lower-case?", lisp_char_lowercasep, islower)

object lisp_char_integer(object args)
{
	check_args(1, args, "char->integer");
	if (!is_character(car(args)))
		error("Expecting a character -- char->integer", car(args));

	return make_fixnum(character_value(car(args)));
}

object lisp_integer_char(object args)
{
	check_args(1, args, "integer->char");
	if (!is_fixnum(car(args)))
		error("Expecting an integer -- integer->char", car(args));

	if (fixnum_value(car(args)) < 0 || fixnum_value(car(args)) > 255)
		error("Integer out of character range -- integer->char", car(args));

	return make_character(fixnum_value(car(args)));
}

object lisp_char_upcase(object args)
{
	check_args(1, args, "char-upcase");
	if (!is_character(car(args)))
		error("Expecting a character -- char-upcase", car(args));

	return make_character(toupper(character_value(car(args))));
}

object lisp_char_downcase(object args)
{
	check_args(1, args, "char-downcase");
	if (!is_character(car(args)))
		error("Expecting a character -- char-downcase", car(args));

	return make_character(tolower(character_value(car(args))));
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

#define pair_fun_def(X) { #X, lisp_##X }

static struct {
	char *name;
	primitive_proc proc;
} the_primitives[] = {

	/* Equivalence predicates */


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
	{ "even?",     lisp_evenp         },

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


	/* Pairs and lists */

	{ "cons", lisp_cons },
	{ "pair?", lisp_pairp },

	pair_fun_def(car),
	pair_fun_def(cdr),
	pair_fun_def(caar),
	pair_fun_def(cadr),
	pair_fun_def(cdar),
	pair_fun_def(cddr),
	pair_fun_def(caaar),
	pair_fun_def(caadr),
	pair_fun_def(cadar),
	pair_fun_def(caddr),
	pair_fun_def(cdaar),
	pair_fun_def(cdadr),
	pair_fun_def(cddar),
	pair_fun_def(cdddr),
	pair_fun_def(caaaar),
	pair_fun_def(caaadr),
	pair_fun_def(caadar),
	pair_fun_def(caaddr),
	pair_fun_def(cadaar),
	pair_fun_def(cadadr),
	pair_fun_def(caddar),
	pair_fun_def(cadddr),
	pair_fun_def(cdaaar),
	pair_fun_def(cdaadr),
	pair_fun_def(cdadar),
	pair_fun_def(cdaddr),
	pair_fun_def(cddaar),
	pair_fun_def(cddadr),
	pair_fun_def(cdddar),
	pair_fun_def(cddddr),

	{ "set-car!",  lisp_set_car       },
	{ "set-cdr!",  lisp_set_cdr       },

	{ "null?",     lisp_nullp         },
	{ "list?",     lisp_listp         },
	{ "list",      lisp_list          },
	{ "length",    lisp_length        },
	{ "append",    lisp_append        },
	{ "reverse",   lisp_reverse       },
	{ "list-tail", lisp_list_tail     },
	{ "list-ref",  lisp_list_ref      },
//	{ "memq",      lisp_memq          },
//	{ "memv",      lisp_memv          },
//	{ "member",    lisp_member        },
//	{ "assq",      lisp_assq          },
//	{ "assv",      lisp_assv          },
//	{ "assoc",     lisp_assoc         },


	/* Symbols */

	{ "symbol?",        lisp_symbolp       },
	{ "string->symbol", lisp_string_symbol },
	{ "symbol->string", lisp_symbol_string },


	/* Characters */

	{ "char?",            lisp_charp                  },

	{ "char=?",           lisp_char_equal             },
	{ "char<?",           lisp_char_increasing        },
	{ "char>?",           lisp_char_decreasing        },
	{ "char<=?",          lisp_char_non_decreasing    },
	{ "char>=?",          lisp_char_non_increasing    },

	{ "char-ci=?",        lisp_char_ci_equal          },
	{ "char-ci<?",        lisp_char_ci_increasing     },
	{ "char-ci>?",        lisp_char_ci_decreasing     },
	{ "char-ci<=?",       lisp_char_ci_non_decreasing },
	{ "char-ci>=?",       lisp_char_ci_non_increasing },

	{ "char-alphabetic?", lisp_char_alphabeticp       },
	{ "char-numeric?",    lisp_char_numericp          },
	{ "char-whitespace?", lisp_char_whitespacep       },
	{ "char-upper-case?", lisp_char_uppercasep        },
	{ "char-lower-case?", lisp_char_lowercasep        },

	{ "char->integer",    lisp_char_integer           },
	{ "integer->char",    lisp_integer_char           },

	{ "char-upcase",      lisp_char_upcase            },
	{ "char-downcase",    lisp_char_downcase          },



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
