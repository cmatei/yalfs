#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <assert.h>

#include "minime.h"

#define unspecified nil
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



static int is_eq(object o1, object o2)
{
	return (o1 == o2);
}

static int is_eqv(object o1, object o2)
{
	/* In the current implementation, eqv? is the same as eq?
	   FIXME: This will need revisiting if I add floats as indirect
	   objects.
	 */

	return (o1 == o2);

	/* eqv? returns #t if:

	   - o1 and o2 are both #t or both #f
	   - o1 and o2 are both symbols and
	     (string=? (symbol->string o1)
	               (symbol->string o2)) => #t
	   - o1 and o2 are both numbers and =
	   - o1 and o2 are both characters and char=?
	   - o1 and o2 are both the empty list
	   - o1 and o2 are pairs, vectors or strings that denote the
	     same location in the store
	   - o1 and o2 are procedures whose location tags are equal

	*/
}

static int is_string_equal(object o1, object o2)
{
	if (o1 == o2)
		return 1;

	if (string_length(o1) != string_length(o2))
		return 0;

	if (memcmp(string_value(o1), string_value(o2), string_length(o1)) == 0)
		return 1;

	return 0;
}

static int is_equal(object o1, object o2)
{
	if (is_string(o1) && is_string(o2))
		return is_string_equal(o1, o2);

	if (is_pair(o1) && is_pair(o2))
		return  is_equal(car(o1), car(o2)) &&
			is_equal(cdr(o1), cdr(o2));

	return is_eqv(o1, o2);
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

#define number_fun(LISPNAME, CNAME, OP)                                 \
object CNAME(object args)                                               \
{                                                                       \
        object prec = nil;                                              \
        object arg;                                                     \
                                                                        \
        while (!is_null(args)) {                                        \
                arg = car(args);                                        \
                                                                        \
                if (!is_number(arg))                                    \
                        error("Expecting numbers -- " LISPNAME, arg);   \
                                                                        \
                if ((prec != nil) &&                                    \
                    !(fixnum_value(prec) OP fixnum_value(arg)))         \
                        return the_falsity;                             \
                                                                        \
                prec = arg;                                             \
                args = cdr(args);                                       \
        }                                                               \
        return the_truth;                                               \
}

number_fun("=",  lisp_number_equal,         ==)
number_fun("<",  lisp_number_increasing,     <)
number_fun(">",  lisp_number_decreasing,     >)
number_fun("<=", lisp_number_nondecreasing, <=)
number_fun(">=", lisp_number_nonincreasing, >=)

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

#define number_idiv_fun(LISPNAME, CNAME, OP)                            \
object CNAME(object args)                                               \
{                                                                       \
	long n1, n2;							\
									\
	check_args(2, args, LISPNAME);					\
	if (!is_fixnum(car(args)) || !is_fixnum(cadr(args)))		\
		error("Expecting 2 integer arguments --" LISPNAME,	\
		      args);						\
									\
	n1 = fixnum_value(car(args));					\
	n2 = fixnum_value(cadr(args));					\
									\
	if (n2 == 0)							\
		error("Will not divide by zero -- " LISPNAME, args);	\
									\
	return make_fixnum( OP );					\
}

/* there is some implementation-defined variation in C here ... */
number_idiv_fun("quotient",  lisp_quotient,  (n1 / n2))
number_idiv_fun("remainder", lisp_remainder, (n1 % n2))
number_idiv_fun("modulo",    lisp_modulo,    ((n1 % n2 + n2) % n2))


object lisp_number_string(object args)
{
	char buffer[64];
	long nargs, radix = 10;

	nargs = length(args);
	if (nargs < 1 || nargs > 2)
		error("Expecting 1 or 2 arguments -- number->string", args);

	if (!is_fixnum(car(args)))
		error("Expecting an integer -- number->string", car(args));

	if (nargs == 2) {
		if (!is_fixnum(cadr(args)))
			error("Expecting an integer radix -- number->string", cadr(args));

		radix = fixnum_value(cadr(args));
		if (radix != 10)
			error("Unsupported radix -- number->string", cadr(args));
	}

	snprintf(buffer, 64, "%ld", fixnum_value(car(args)));
	return make_string_c(buffer, strlen(buffer));
}

object lisp_string_number(object args)
{
	char *str, *endptr;
	long nargs, radix = 10;
	long val;

	nargs = length(args);
	if (nargs < 1 || nargs > 2)
		error("Expecting 1 or 2 arguments -- number->string", args);

	if (!is_string(car(args)))
		error("Expecting a string -- number->string", car(args));

	if (nargs == 2) {
		if (!is_fixnum(cadr(args)))
			error("Expecting an integer radix -- number->string", cadr(args));
		radix = fixnum_value(cadr(args));

		if (radix != 2 && radix != 8 && radix != 10 && radix != 16)
			error("Unsupported radix -- number->string", cadr(args));
	}

	str = xcalloc(1, string_length(car(args)) + 1);
	memcpy(str, string_value(car(args)), string_length(car(args)));

	/* yuck :-) */
	errno = 0;
	val = strtol(str, &endptr, radix);
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
	    || (errno != 0 && val == 0)
	    || (endptr == str)
	    || (*endptr != 0)) {
		xfree(str);
		return the_falsity;
	}
	xfree(str);

	return make_fixnum(val);
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
object lisp_##X (object args)				\
{							\
	check_args(1, args, #X );			\
	if (!is_pair(car(args)))			\
		error("Expecting a pair -- " #X, args); \
							\
	return	X(car(args));				\
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

	set_car(car(args), cadr(args));
	return unspecified;
}

object lisp_set_cdr(object args)
{
	check_args(2, args, "set-cdr!");

	if (!is_pair(car(args)))
		error("Expecting a pair as first argument -- set-cdr!", args);

	set_cdr(car(args), cadr(args));
	return unspecified;
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

#define member_fun(LISPNAME, CNAME, TESTFUN)			\
object CNAME(object args)		                        \
{								\
        object o, lst;						\
	check_args(2, args, LISPNAME);				\
							        \
	o = car(args);					        \
	lst = cadr(args);				        \
							        \
	if (!is_list(lst))					\
		error("Expecting a list -- " LISPNAME, lst);	\
								\
	while (!is_null(lst)) {					\
	        if ( TESTFUN(o, car(lst)) )			\
			return lst;				\
								\
		lst = cdr(lst);					\
	}							\
								\
	return the_falsity;					\
}

member_fun("memq",   lisp_memq,   is_eq)
member_fun("memv",   lisp_memv,   is_eqv)
member_fun("member", lisp_member, is_equal)

#define assoc_fun(LISPNAME, CNAME, TESTFUN)	                \
object CNAME(object args)                                       \
{								\
        object o, lst, pair;					\
	check_args(2, args, LISPNAME);				\
								\
	o = car(args);						\
	lst = cadr(args);					\
								\
	if (!is_list(lst))					\
		error("Expecting an alist -- " LISPNAME, lst);	\
								\
	while (!is_null(lst)) {					\
	        pair = car(lst);				\
		if (!is_pair(pair))				\
			error("Expecting an alist -- "		\
			      LISPNAME, cadr(args));		\
								\
		if ( TESTFUN(o, car(pair)) )			\
			return pair;				\
								\
		lst = cdr(lst);					\
	}							\
	return the_falsity;					\
}

assoc_fun("assq",  lisp_assq,     is_eq)
assoc_fun("assv",  lisp_assv,    is_eqv)
assoc_fun("assoc", lisp_assoc, is_equal)

object lisp_charp(object args)
{
	check_args(1, args, "char?");
	return boolean(is_character(car(args)));
}

#define identity(x) (x)
#define char_fun(LISPNAME, CNAME, OP, CASEFOLD)                         \
object CNAME(object args)                                               \
{                                                                       \
        object prec = nil, current;                                     \
        unsigned char p, c;                                             \
                                                                        \
        while (!is_null(args)) {                                        \
                current = car(args);                                    \
                if (!is_character(current))                             \
                        error("Expecting characters -- " LISPNAME,      \
                              current);                                 \
                                                                        \
                if (prec != nil) {                                      \
                        p = CASEFOLD( character_value(prec) );          \
                        c = CASEFOLD( character_value(current) );       \
                                                                        \
                        if (! (p OP c))                                 \
                                return the_falsity;                     \
                }                                                       \
                                                                        \
                prec = car(args);                                       \
                args = cdr(args);                                       \
        }                                                               \
        return the_truth;                                               \
}

char_fun("char=?",     lisp_char_equal,             ==, identity)
char_fun("char<?",     lisp_char_increasing,         <, identity)
char_fun("char>?",     lisp_char_decreasing,         >, identity)
char_fun("char<=?",    lisp_char_non_decreasing,    <=, identity)
char_fun("char>=?",    lisp_char_non_increasing,    >=, identity)

char_fun("char-ci=?",  lisp_char_ci_equal,          ==, tolower)
char_fun("char-ci<?",  lisp_char_ci_increasing,      <, tolower)
char_fun("char-ci>?",  lisp_char_ci_decreasing,      >, tolower)
char_fun("char-ci<=?", lisp_char_ci_non_decreasing, <=, tolower)
char_fun("char-ci>=?", lisp_char_ci_non_increasing, >=, tolower)

#define char_type_fun(LISPNAME, CNAME, TYPEFUN)                         \
object CNAME(object args)                                               \
{                                                                       \
        object arg;                                                     \
        check_args(1, args, LISPNAME);                                  \
        arg = car(args);                                                \
                                                                        \
        if (!is_character(arg))                                         \
                error("Expecting a character -- " LISPNAME, arg);       \
                                                                        \
        return boolean(TYPEFUN(character_value(arg)));                  \
}

char_type_fun("char-alphabetic?", lisp_char_alphabeticp, isalpha)
char_type_fun("char-numeric?",    lisp_char_numericp,    isdigit)
char_type_fun("char-whitespace?", lisp_char_whitespacep, isspace)
char_type_fun("char-upper-case?", lisp_char_uppercasep,  isupper)
char_type_fun("char-lower-case?", lisp_char_lowercasep,  islower)

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

object lisp_stringp(object args)
{
	check_args(1, args, "string?");
	return boolean(is_string(car(args)));
}

object lisp_make_string(object args)
{
	unsigned long nargs = length(args);
	object o;

	if (nargs < 1 || nargs > 2)
		error("Expecting 1 or 2 arguments -- make-string", car(args));

	if (!is_fixnum(car(args)) || fixnum_value(car(args)) < 0)
		error("Expecting a non-negative integer -- make-string", car(args));

	o = make_string((unsigned long) fixnum_value(car(args)));

	if (nargs == 2) {
		if (!is_character(cadr(args)))
			error("Expecting a character -- make-string", cadr(args));

		memset(string_value(o), character_value(cadr(args)), fixnum_value(car(args)));
	} else {
		memset(string_value(o), 0, fixnum_value(car(args)));
	}

	return o;
}

object lisp_string(object args)
{
	unsigned long nargs = length(args);
	object o;
	char *p;

	o = make_string(nargs);
	p = string_value(o);

	while (!is_null(args)) {
		if (!is_character(car(args)))
			error("Expecting characters -- string", car(args));

		*p++ = character_value(car(args));
		args = cdr(args);
	}

	return o;
}

object lisp_string_length(object args)
{
	check_args(1, args, "string-length");
	if (!is_string(car(args)))
		error("Expecting a string -- string-length", car(args));

	return make_fixnum(string_length(car(args)));
}

object lisp_string_ref(object args)
{
	object string, k;
	long pos;

	check_args(2, args, "string-ref");

	if (!is_string((string = car(args))))
		error("Expecting a string -- string-ref", string);

	if (!is_fixnum((k = cadr(args))))
		error("Expecting an integer -- string-ref", k);

	pos = fixnum_value(k);
	if (pos < 0 || pos >= string_length(string))
		error("Not a valid index -- string-ref", k);

	return make_character(* ((char *) string_value(string) + pos));

}

object lisp_string_set(object args)
{
	object string, k, chr;
	long pos;
	check_args(3, args, "string-set!");

	if (!is_string((string = car(args))))
		error("Expecting a string -- string-set!", string);

	if (!is_fixnum((k = cadr(args))))
		error("Expecting an integer -- string-set!", k);

	pos = fixnum_value(k);
	if (pos < 0 || pos >= string_length(string))
		error("Not a valid index -- string-set!", k);

	if (!is_character((chr = caddr(args))))
		error("Expecting a character -- string-set!", chr);

	*((char *) string_value(string) + pos) = character_value(chr);
	return unspecified;
}

#define identity(x) (x)
#define string_fun(LISPNAME, CNAME, OP, CASEFOLD)                       \
object CNAME(object args)                                               \
{                                                                       \
        object prec, current;                                           \
        unsigned long preclen, curlen, complen;                         \
        unsigned char *pptr, *cptr;                                     \
        unsigned char p, c;                                             \
        unsigned long i;                                                \
                                                                        \
        if (length(args) < 2)                                           \
                error("Expecting at least 2 arguments -- " LISPNAME,    \
                      args);                                            \
                                                                        \
        if (!is_string(prec = car(args)))                               \
                error("Expecting strings -- " LISPNAME, prec);          \
                                                                        \
        preclen = string_length(prec);                                  \
                                                                        \
        args = cdr(args);                                               \
        while (!is_null(args)) {                                        \
                                                                        \
                if (!is_string(current = car(args)))                    \
                        error("Expecting strings -- " LISPNAME,         \
                              current);                                 \
                                                                        \
                curlen = string_length(current);                        \
                                                                        \
                pptr = (unsigned char *) string_value(prec);            \
                cptr = (unsigned char *) string_value(current);         \
                                                                        \
                complen = MIN(preclen, curlen);                         \
                for (i = 0; i < complen; i++) {                         \
                        p = CASEFOLD( pptr[i] );                        \
                        c = CASEFOLD( cptr[i] );                        \
                                                                        \
                        if (p == c)                                     \
                                continue;                               \
                                                                        \
                        if (! (p OP c))                                 \
                                return the_falsity;                     \
                                                                        \
                        goto next_arg;                                  \
                }                                                       \
                                                                        \
                if (! (preclen OP curlen))                              \
                        return the_falsity;                             \
        next_arg:                                                       \
                                                                        \
                prec = current;                                         \
                args = cdr(args);                                       \
        }                                                               \
                                                                        \
        return the_truth;                                               \
}


string_fun("string=?",     lisp_string_equalp,           ==, identity)
string_fun("string<?",     lisp_string_increasing,        <, identity)
string_fun("string>?",     lisp_string_decreasing,        >, identity)
string_fun("string<=?",    lisp_string_nondecreasing,    <=, identity)
string_fun("string>=?",    lisp_string_nonincreasing,    >=, identity)

string_fun("string-ci=?",  lisp_string_ci_equalp,        ==, tolower)
string_fun("string-ci<?",  lisp_string_ci_increasing,     <, tolower)
string_fun("string-ci>?",  lisp_string_ci_decreasing,     >, tolower)
string_fun("string-ci<=?", lisp_string_ci_nondecreasing, <=, tolower)
string_fun("string-ci>=?", lisp_string_ci_nonincreasing, >=, tolower)


object lisp_substring(object args)
{
	object ret;
	long start, end;

	check_args(3, args, "substring");

	if (!is_string(car(args)))
		error("Expecting a string -- substring", car(args));

	if (!is_fixnum(cadr(args)))
		error("Expecting an integer start index -- substring", cadr(args));

	if (!is_fixnum(caddr(args)))
		error("Expecting an integer end index -- substring", caddr(args));

	start = fixnum_value(cadr(args));
	end   = fixnum_value(caddr(args));

	if (start < 0 || start > string_length(car(args)))
		error("Not a valid start index -- substring", cadr(args));

	if (end < start || end > string_length(car(args)))
		error("Not a valid end index -- substring", caddr(args));

	ret = make_string(end - start);

	memcpy(string_value(ret),
	       ((unsigned char *) string_value(car(args))) + start, (end - start));

	return ret;
}

object lisp_string_append(object args)
{
	object tmp = args, string;
	unsigned long len = 0;
	unsigned char *ptr;

	/* get total length first */
	while (!is_null(tmp)) {
		if (!is_string(car(tmp)))
			error("Expecting strings -- string-append", car(tmp));

		len += string_length(car(tmp));
		tmp = cdr(tmp);
	}

	string = make_string(len);
	ptr = (unsigned char *) string_value(string);

	/* copy contents */
	while (!is_null(args)) {
		tmp = car(args);

		memcpy(ptr, string_value(tmp), string_length(tmp));
		ptr += string_length(tmp);

		args = cdr(args);
	}

	return string;
}

object lisp_string_list(object args)
{
	object head = nil, tail = nil;
	unsigned char *ptr;
	unsigned long i;

	check_args(1, args, "string->list");
	if (!is_string(car(args)))
		error("Expecting a string -- string->list", car(args));

	ptr = (unsigned char *) string_value(car(args));
	for (i = 0; i < string_length(car(args)); i++) {
		if (head == nil) {
			head = cons(make_character(ptr[i]), nil);
			tail = head;
		} else {
			set_cdr(tail, cons(make_character(ptr[i]), nil));
			tail = cdr(tail);
		}
	}

	return head;
}

object lisp_list_string(object args)
{
	object string;
	unsigned char *ptr;

	check_args(1, args, "list->string");
	if (!is_list(car(args)))
		error("Expecting a list -- list->string", car(args));

	string = make_string(length(car(args)));
	ptr = (unsigned char *) string_value(string);

	args = car(args);
	while (!is_null(args)) {
		if (!is_character(car(args)))
			error("Expecting characters -- list->string", car(args));

		*ptr++ = character_value(car(args));

		args = cdr(args);
	}

	return string;
}

object lisp_string_copy(object args)
{
	object string;

	check_args(1, args, "string-copy");
	if (!is_string(car(args)))
		error("Expecting a string -- string-copy", car(args));

	string = make_string(string_length(car(args)));
	memcpy(string_value(string), string_value(car(args)),
	       string_length(car(args)));

	return string;
}

object lisp_string_fill(object args)
{
	check_args(2, args, "string-fill!");
	if (!is_string(car(args)))
		error("Expecting a string -- string-fill!", car(args));

	if (!is_character(cadr(args)))
		error("Expecting a character -- string-fill!", cadr(args));

	memset(string_value(car(args)),
	       (unsigned char) character_value(cadr(args)),
	       string_length(car(args)));

	return unspecified;
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

	return lisp_string_copy(cons(symbol_string(car(args)), nil));
}

object lisp_string_symbol(object args)
{
	check_args(1, args, "string->symbol");
	if (!is_string(car(args)))
		error("Expecting a string -- string->symbol", car(args));

	return symbol(string_value(car(args)), string_length(car(args)));
}

object lisp_eq(object args)
{
	object initial;

	if (length(args) < 2)
		error("Need at least two arguments -- /", args);

	initial = car(args);
	args = cdr(args);

	while (!is_null(args)) {
		if (!is_eq(initial, car(args)))
			return the_falsity;

		args = cdr(args);
	}

	return the_truth;
}


object lisp_eqv(object args)
{
	check_args(2, args, "eqv?");

	return boolean(is_eqv(car(args), cadr(args)));
}



object lisp_equalp(object args)
{
	check_args(2, args, "equal?");

	return boolean(is_equal(car(args), cadr(args)));

	/* Equal? recursively compares the contents of pairs, vectors,
	   and strings, applying eqv? on othe objects such as numbers
	   and symbols. A rule of thumb is that objects are equal? if
	   they print the same. Equal? may fail to terminate if its
	   arguments are circular data structures. */

}

object lisp_procedurep(object args)
{
	check_args(1, args, "procedure?");
	return boolean(is_anykind_procedure(car(args)));
}


object lisp_eofp(object args)
{
	check_args(1, args, "eof-object?");
	return boolean(is_end_of_file(car(args)));
}

object lisp_input_portp(object args)
{
	check_args(1, args, "input-port?");
	return boolean(is_input_port(car(args)));
}

object lisp_output_portp(object args)
{
	check_args(1, args, "output-port?");
	return boolean(is_output_port(car(args)));
}

object lisp_current_input_port(object args)
{
	check_args(0, args, "current-input-port");
	return current_input_port;
}

object lisp_current_output_port(object args)
{
	check_args(0, args, "current-output-port");
	return current_output_port;
}


#define pair_fun_def(X) { #X, lisp_##X }

static struct {
	char *name;
	primitive_proc proc;
} the_primitives[] = {

	/* Equivalence predicates */

	{ "eq?",       lisp_eq            },
	{ "eqv?",      lisp_eqv           },
	{ "equal?",    lisp_equalp        },

        /* Numerical operations */

	{ "number?",   lisp_numberp       },
	{ "integer?",  lisp_integerp      },

	{ "=",         lisp_number_equal         },
	{ "<",         lisp_number_increasing    },
	{ ">",         lisp_number_decreasing    },
	{ "<=",        lisp_number_nondecreasing },
	{ ">=",        lisp_number_nonincreasing },

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
	{ "quotient",  lisp_quotient },
	{ "remainder", lisp_remainder },
	{ "modulo",    lisp_modulo },

	/* math is hard, let's go shopping */
//	{ "gcd",       lisp_gcd },
//	{ "lcm",       lisp_lcm },

	{ "number->string", lisp_number_string },
	{ "string->number", lisp_string_number },


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
	{ "memq",      lisp_memq          },
	{ "memv",      lisp_memv          },
	{ "member",    lisp_member        },
	{ "assq",      lisp_assq          },
	{ "assv",      lisp_assv          },
	{ "assoc",     lisp_assoc         },


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


	/* Strings */

	{ "string?",       lisp_stringp                   },
	{ "make-string",   lisp_make_string               },
	{ "string",        lisp_string                    },
	{ "string-length", lisp_string_length             },
	{ "string-ref",    lisp_string_ref                },
	{ "string-set!",   lisp_string_set                },

	{ "string=?",      lisp_string_equalp             },
	{ "string<?",      lisp_string_increasing         },
	{ "string>?",      lisp_string_decreasing         },
	{ "string<=?",     lisp_string_nondecreasing      },
	{ "string>=?",     lisp_string_nonincreasing      },

	{ "string-ci=?",   lisp_string_ci_equalp          },
	{ "string-ci<?",   lisp_string_ci_increasing      },
	{ "string-ci>?",   lisp_string_ci_decreasing      },
	{ "string-ci<=?",  lisp_string_ci_nondecreasing   },
	{ "string-ci>=?",  lisp_string_ci_nonincreasing   },

	{ "substring",     lisp_substring                 },
	{ "string-append", lisp_string_append             },
	{ "string->list",  lisp_string_list               },
	{ "list->string",  lisp_list_string               },
	{ "string-copy",   lisp_string_copy               },
	{ "string-fill!",  lisp_string_fill               },


	/* Control features */

	{ "procedure?",    lisp_procedurep                },
	{ "apply",         lisp_apply                     },


	/* I/O */

	{ "eof-object?",   lisp_eofp                      },

	{ "input-port?",   lisp_input_portp               },
	{ "output-port?",  lisp_output_portp              },

	{ "current-input-port", lisp_current_input_port   },
	{ "current-output-port", lisp_current_output_port },


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
