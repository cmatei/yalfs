#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

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

object impl_integerp(object args)
{
	check_args(1, args, "integer?");
	return boolean(is_fixnum(car(args)));
}

object impl_numberp(object args)
{
	return impl_integerp(args);
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

number_fun("=",  impl_number_equal,         ==)
number_fun("<",  impl_number_increasing,     <)
number_fun(">",  impl_number_decreasing,     >)
number_fun("<=", impl_number_nondecreasing, <=)
number_fun(">=", impl_number_nonincreasing, >=)

object impl_zerop(object args)
{
	check_args(1, args, "zero?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- zero?", args);

	return boolean(fixnum_value(car(args)) == 0);
}

object impl_positivep(object args)
{
	check_args(1, args, "positive?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- positive?", args);

	return boolean(fixnum_value(car(args)) > 0);
}

object impl_negativep(object args)
{
	check_args(1, args, "negative?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- negative?", args);

	return boolean(fixnum_value(car(args)) < 0);
}

object impl_oddp(object args)
{
	check_args(1, args, "oddp?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- odd?", args);

	return boolean((fixnum_value(car(args)) % 2) != 0);
}

object impl_evenp(object args)
{
	check_args(1, args, "evenp?");
	if (!is_fixnum(car(args)))
		error("Expecting a number -- even?", args);

	return boolean((fixnum_value(car(args)) % 2) == 0);
}

object impl_max(object args)
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

object impl_min(object args)
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


object impl_plus(object args)
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

object impl_multiply(object args)
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

object impl_minus(object args)
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


object impl_divide(object args)
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

object impl_abs(object args)
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
number_idiv_fun("quotient",  impl_quotient,  (n1 / n2))
number_idiv_fun("remainder", impl_remainder, (n1 % n2))
number_idiv_fun("modulo",    impl_modulo,    ((n1 % n2 + n2) % n2))


object impl_number_string(object args)
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

object impl_string_number(object args)
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

object impl_not(object args)
{
	check_args(1, args, "not");

	/* not returns #t if obj is false, and in scheme only #f is false */
	return (car(args) == the_falsity) ? the_truth : the_falsity;
}

object impl_booleanp(object args)
{
	check_args(1, args, "boolean?");

	return boolean(((car(args) == the_truth) || (car(args) == the_falsity)));
}


/* Pairs and lists */

object impl_pairp(object args)
{
	check_args(1, args, "pair?");

	return boolean(is_pair(car(args)));
}

object impl_cons(object args)
{
	check_args(2, args, "cons");
	return cons(car(args), cadr(args));
}



#define pair_fun(X)					\
object impl_##X (object args)				\
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

object impl_set_car(object args)
{
	check_args(2, args, "set-car!");

	if (!is_pair(car(args)))
		error("Expecting a pair as first argument -- set-car!", args);

	set_car(car(args), cadr(args));
	return unspecified;
}

object impl_set_cdr(object args)
{
	check_args(2, args, "set-cdr!");

	if (!is_pair(car(args)))
		error("Expecting a pair as first argument -- set-cdr!", args);

	set_cdr(car(args), cadr(args));
	return unspecified;
}

object impl_nullp(object args)
{
	check_args(1, args, "null?");
	return boolean(is_null(car(args)));
}

object impl_listp(object args)
{
	check_args(1, args, "list?");
	return boolean(is_list(car(args)));
}

object impl_list(object args)
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

object impl_length(object args)
{
	check_args(1, args, "length");

	if (!is_list(car(args)))
		error("Object is not a proper list -- length", args);

	return make_fixnum(length(car(args)));
}

object impl_append(object args)
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

object impl_reverse(object args)
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

object impl_list_tail(object args)
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

object impl_list_ref(object args)
{
	object lst;

	lst = impl_list_tail(args);
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

member_fun("memq",   impl_memq,   is_eq)
member_fun("memv",   impl_memv,   is_eqv)
member_fun("member", impl_member, is_equal)

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

assoc_fun("assq",  impl_assq,     is_eq)
assoc_fun("assv",  impl_assv,    is_eqv)
assoc_fun("assoc", impl_assoc, is_equal)

object impl_charp(object args)
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

char_fun("char=?",     impl_char_equal,             ==, identity)
char_fun("char<?",     impl_char_increasing,         <, identity)
char_fun("char>?",     impl_char_decreasing,         >, identity)
char_fun("char<=?",    impl_char_non_decreasing,    <=, identity)
char_fun("char>=?",    impl_char_non_increasing,    >=, identity)

char_fun("char-ci=?",  impl_char_ci_equal,          ==, tolower)
char_fun("char-ci<?",  impl_char_ci_increasing,      <, tolower)
char_fun("char-ci>?",  impl_char_ci_decreasing,      >, tolower)
char_fun("char-ci<=?", impl_char_ci_non_decreasing, <=, tolower)
char_fun("char-ci>=?", impl_char_ci_non_increasing, >=, tolower)

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

char_type_fun("char-alphabetic?", impl_char_alphabeticp, isalpha)
char_type_fun("char-numeric?",    impl_char_numericp,    isdigit)
char_type_fun("char-whitespace?", impl_char_whitespacep, isspace)
char_type_fun("char-upper-case?", impl_char_uppercasep,  isupper)
char_type_fun("char-lower-case?", impl_char_lowercasep,  islower)

object impl_char_integer(object args)
{
	check_args(1, args, "char->integer");
	if (!is_character(car(args)))
		error("Expecting a character -- char->integer", car(args));

	return make_fixnum(character_value(car(args)));
}

object impl_integer_char(object args)
{
	check_args(1, args, "integer->char");
	if (!is_fixnum(car(args)))
		error("Expecting an integer -- integer->char", car(args));

	if (fixnum_value(car(args)) < 0 || fixnum_value(car(args)) > 255)
		error("Integer out of character range -- integer->char", car(args));

	return make_character(fixnum_value(car(args)));
}

object impl_char_upcase(object args)
{
	check_args(1, args, "char-upcase");
	if (!is_character(car(args)))
		error("Expecting a character -- char-upcase", car(args));

	return make_character(toupper(character_value(car(args))));
}

object impl_char_downcase(object args)
{
	check_args(1, args, "char-downcase");
	if (!is_character(car(args)))
		error("Expecting a character -- char-downcase", car(args));

	return make_character(tolower(character_value(car(args))));
}

object impl_stringp(object args)
{
	check_args(1, args, "string?");
	return boolean(is_string(car(args)));
}

object impl_make_string(object args)
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

object impl_string(object args)
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

object impl_string_length(object args)
{
	check_args(1, args, "string-length");
	if (!is_string(car(args)))
		error("Expecting a string -- string-length", car(args));

	return make_fixnum(string_length(car(args)));
}

object impl_string_ref(object args)
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

object impl_string_set(object args)
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


string_fun("string=?",     impl_string_equalp,           ==, identity)
string_fun("string<?",     impl_string_increasing,        <, identity)
string_fun("string>?",     impl_string_decreasing,        >, identity)
string_fun("string<=?",    impl_string_nondecreasing,    <=, identity)
string_fun("string>=?",    impl_string_nonincreasing,    >=, identity)

string_fun("string-ci=?",  impl_string_ci_equalp,        ==, tolower)
string_fun("string-ci<?",  impl_string_ci_increasing,     <, tolower)
string_fun("string-ci>?",  impl_string_ci_decreasing,     >, tolower)
string_fun("string-ci<=?", impl_string_ci_nondecreasing, <=, tolower)
string_fun("string-ci>=?", impl_string_ci_nonincreasing, >=, tolower)


object impl_substring(object args)
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

object impl_string_append(object args)
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

object impl_string_list(object args)
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

object impl_list_string(object args)
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

object impl_string_copy(object args)
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

object impl_string_fill(object args)
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

object impl_symbolp(object args)
{
	check_args(1, args, "symbol?");
	return boolean(is_symbol(car(args)));
}

object impl_symbol_string(object args)
{
	check_args(1, args, "symbol->string");
	if (!is_symbol(car(args)))
		error("Object is not a symbol -- symbol->string", car(args));

	return impl_string_copy(cons(symbol_string(car(args)), nil));
}

object impl_string_symbol(object args)
{
	check_args(1, args, "string->symbol");
	if (!is_string(car(args)))
		error("Expecting a string -- string->symbol", car(args));

	return symbol(string_value(car(args)), string_length(car(args)));
}

object impl_eq(object args)
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


object impl_eqv(object args)
{
	check_args(2, args, "eqv?");

	return boolean(is_eqv(car(args), cadr(args)));
}



object impl_equalp(object args)
{
	check_args(2, args, "equal?");

	return boolean(is_equal(car(args), cadr(args)));

	/* Equal? recursively compares the contents of pairs, vectors,
	   and strings, applying eqv? on othe objects such as numbers
	   and symbols. A rule of thumb is that objects are equal? if
	   they print the same. Equal? may fail to terminate if its
	   arguments are circular data structures. */

}

object impl_procedurep(object args)
{
	check_args(1, args, "procedure?");
	return boolean(is_anykind_procedure(car(args)));
}

object impl_interaction_environment(object args)
{
	check_args(0, args, "interaction-environment");
	return user_environment;
}


object impl_eofp(object args)
{
	check_args(1, args, "eof-object?");
	return boolean(is_end_of_file(car(args)));
}

object impl_input_portp(object args)
{
	check_args(1, args, "input-port?");
	return boolean(is_input_port(car(args)));
}

object impl_output_portp(object args)
{
	check_args(1, args, "output-port?");
	return boolean(is_output_port(car(args)));
}

object impl_current_input_port(object args)
{
	check_args(0, args, "current-input-port");
	return current_input_port;
}

object impl_current_output_port(object args)
{
	check_args(0, args, "current-output-port");
	return current_output_port;
}

object impl_open_input_file(object args)
{
	check_args(1, args, "open-input-file");
	if (!is_string(car(args)))
		error("Expecting a string -- open-input-file", car(args));

	return io_file_as_port(car(args), PORT_TYPE_INPUT);
}

object impl_open_output_file(object args)
{
	check_args(1, args, "open-output-file");
	if (!is_string(car(args)))
		error("Expecting a string -- open-output-file", car(args));

	return io_file_as_port(car(args), PORT_TYPE_OUTPUT);
}

object impl_close_input_port(object args)
{
	object port;
	check_args(1, args, "close-input-port");

	port = car(args);
	if (!is_input_port(port))
		error("Expecting an input port -- close-input-port", port);

	io_close_port(port);

	return unspecified;
}

object impl_close_output_port(object args)
{
	object port;
	check_args(1, args, "close-output-port");

	port = car(args);
	if (!is_output_port(port))
		error("Expecting an output port -- close-output-port", port);

	io_close_port(port);

	return unspecified;
}

object impl_read(object args)
{
	object port = current_input_port;
	long nargs;

	nargs = length(args);
	if (nargs > 1)
		error("Expecting at most 1 argument -- read", args);

	if (nargs == 1)
		port = car(args);

	if (!is_input_port(port))
		error("Expecting an input port -- read", port);

	return io_read(port);
}

object impl_read_char(object args)
{
	object port = current_input_port;
	long nargs;

	nargs = length(args);
	if (nargs > 1)
		error("Expecting at most 1 argument -- read-char", args);

	if (nargs == 1)
		port = car(args);

	if (!is_input_port(port))
		error("Expecting an input port -- read-char", port);

	return io_read_char(port);
}

object impl_peek_char(object args)
{
	object port = current_input_port;
	long nargs;

	nargs = length(args);
	if (nargs > 1)
		error("Expecting at most 1 argument -- peek-char", args);

	if (nargs == 1)
		port = car(args);

	if (!is_input_port(port))
		error("Expecting an input port -- peek-char", port);

	return io_peek_char(port);
}

object impl_write(object args)
{
	object port = current_output_port;
	long nargs;

	nargs = length(args);
	if (nargs < 1 || nargs > 2)
		error("Expecting at least 1, at most 2 arguments -- write", args);

	if (nargs == 2)
		port = cadr(args);

	if (!is_output_port(port))
		error("Expecting an output port -- write", port);

	io_write(car(args), port);

	return unspecified;
}

object impl_display(object args)
{
	object port = current_output_port;
	long nargs;

	nargs = length(args);
	if (nargs < 1 || nargs > 2)
		error("Expecting at least 1, at most 2 arguments -- display", args);

	if (nargs == 2)
		port = cadr(args);

	if (!is_output_port(port))
		error("Expecting an output port -- display", port);

	io_display(car(args), port);

	return unspecified;
}

object impl_newline(object args)
{
	object port = current_output_port;
	long nargs;

	nargs = length(args);
	if (nargs > 1)
		error("Expecting at most 1 argument -- newline", args);

	if (nargs == 1)
		port = car(args);

	if (!is_output_port(port))
		error("Expecting an output port -- newline", port);

	io_newline(port);

	return unspecified;
}

object impl_write_char(object args)
{
	object port = current_output_port;
	long nargs;


	nargs = length(args);
	if (nargs < 1 || nargs > 2)
		error("Expecting at least 1, at most 2 arguments -- write-char", args);

	if (nargs == 2)
		port = cadr(args);

	if (!is_character(car(args)))
		error("Expecting a character -- write-char", car(args));

	if (!is_output_port(port))
		error("Expecting an output port -- write-char", port);

	io_write_char(car(args), port);

	return unspecified;
}

object impl_load(object args)
{
	check_args(1, args, "load");
	if (!is_string(car(args)))
		error("Expecting a string -- load", car(args));

	return io_load(car(args));
}

#define pair_fun_def(X) { #X, impl_##X }

static struct {
	char *name;
	primitive_proc proc;
} the_primitives[] = {

	/* Equivalence predicates */

	{ "eq?",       impl_eq            },
	{ "eqv?",      impl_eqv           },
	{ "equal?",    impl_equalp        },

        /* Numerical operations */

	{ "number?",   impl_numberp       },
	{ "integer?",  impl_integerp      },

	{ "=",         impl_number_equal         },
	{ "<",         impl_number_increasing    },
	{ ">",         impl_number_decreasing    },
	{ "<=",        impl_number_nondecreasing },
	{ ">=",        impl_number_nonincreasing },

	{ "zero?",     impl_zerop         },
	{ "positive?", impl_positivep     },
	{ "negative?", impl_negativep     },
	{ "odd?",      impl_oddp          },
	{ "even?",     impl_evenp         },

	{ "max",       impl_max           },
	{ "min",       impl_min           },

	{ "+",         impl_plus          },
	{ "*",         impl_multiply      },
	{ "-",         impl_minus         },
	{ "/",         impl_divide        },

	{ "abs",       impl_abs           },
	{ "quotient",  impl_quotient },
	{ "remainder", impl_remainder },
	{ "modulo",    impl_modulo },

	/* math is hard, let's go shopping */
//	{ "gcd",       impl_gcd },
//	{ "lcm",       impl_lcm },

	{ "number->string", impl_number_string },
	{ "string->number", impl_string_number },


	/* Booleans */

	{ "not", impl_not },
	{ "boolean?", impl_booleanp },


	/* Pairs and lists */

	{ "cons", impl_cons },
	{ "pair?", impl_pairp },

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

	{ "set-car!",  impl_set_car       },
	{ "set-cdr!",  impl_set_cdr       },

	{ "null?",     impl_nullp         },
	{ "list?",     impl_listp         },
	{ "list",      impl_list          },
	{ "length",    impl_length        },
	{ "append",    impl_append        },
	{ "reverse",   impl_reverse       },
	{ "list-tail", impl_list_tail     },
	{ "list-ref",  impl_list_ref      },
	{ "memq",      impl_memq          },
	{ "memv",      impl_memv          },
	{ "member",    impl_member        },
	{ "assq",      impl_assq          },
	{ "assv",      impl_assv          },
	{ "assoc",     impl_assoc         },


	/* Symbols */

	{ "symbol?",        impl_symbolp       },
	{ "string->symbol", impl_string_symbol },
	{ "symbol->string", impl_symbol_string },


	/* Characters */

	{ "char?",            impl_charp                  },

	{ "char=?",           impl_char_equal             },
	{ "char<?",           impl_char_increasing        },
	{ "char>?",           impl_char_decreasing        },
	{ "char<=?",          impl_char_non_decreasing    },
	{ "char>=?",          impl_char_non_increasing    },

	{ "char-ci=?",        impl_char_ci_equal          },
	{ "char-ci<?",        impl_char_ci_increasing     },
	{ "char-ci>?",        impl_char_ci_decreasing     },
	{ "char-ci<=?",       impl_char_ci_non_decreasing },
	{ "char-ci>=?",       impl_char_ci_non_increasing },

	{ "char-alphabetic?", impl_char_alphabeticp       },
	{ "char-numeric?",    impl_char_numericp          },
	{ "char-whitespace?", impl_char_whitespacep       },
	{ "char-upper-case?", impl_char_uppercasep        },
	{ "char-lower-case?", impl_char_lowercasep        },

	{ "char->integer",    impl_char_integer           },
	{ "integer->char",    impl_integer_char           },

	{ "char-upcase",      impl_char_upcase            },
	{ "char-downcase",    impl_char_downcase          },


	/* Strings */

	{ "string?",       impl_stringp                   },
	{ "make-string",   impl_make_string               },
	{ "string",        impl_string                    },
	{ "string-length", impl_string_length             },
	{ "string-ref",    impl_string_ref                },
	{ "string-set!",   impl_string_set                },

	{ "string=?",      impl_string_equalp             },
	{ "string<?",      impl_string_increasing         },
	{ "string>?",      impl_string_decreasing         },
	{ "string<=?",     impl_string_nondecreasing      },
	{ "string>=?",     impl_string_nonincreasing      },

	{ "string-ci=?",   impl_string_ci_equalp          },
	{ "string-ci<?",   impl_string_ci_increasing      },
	{ "string-ci>?",   impl_string_ci_decreasing      },
	{ "string-ci<=?",  impl_string_ci_nondecreasing   },
	{ "string-ci>=?",  impl_string_ci_nonincreasing   },

	{ "substring",     impl_substring                 },
	{ "string-append", impl_string_append             },
	{ "string->list",  impl_string_list               },
	{ "list->string",  impl_list_string               },
	{ "string-copy",   impl_string_copy               },
	{ "string-fill!",  impl_string_fill               },


	/* Control features */

	{ "procedure?",    impl_procedurep                },
	{ "apply",         lisp_apply                     },
//	{ "map",           impl_map                       },
//	{ "for-each",      impl_for_each                  },
//	{ "force",         impl_force                     },
//      { "delay",         impl_delay                     },
//	{ "eval",          impl_eval                      },

	{ "interaction-environment", impl_interaction_environment },

	/* I/O */

//	{ "call-with-input-file",  impl_call_w_input_file  },
//	{ "call-with-output-file", impl_call_w_output_file },

	{ "input-port?",   impl_input_portp               },
	{ "output-port?",  impl_output_portp              },

	{ "current-input-port",  impl_current_input_port  },
	{ "current-output-port", impl_current_output_port },

//	{ "with-input-from-file", impl_w_input_file       },
//	{ "with-output-to-file",  impl_w_output_file      },

	{ "open-input-file",     impl_open_input_file     },
	{ "open-output-file",    impl_open_output_file    },

	{ "close-input-port",    impl_close_input_port    },
	{ "close-output-port",   impl_close_output_port   },

	{ "read",          impl_read                      },
	{ "read-char",     impl_read_char                 },
	{ "peek-char",     impl_peek_char                 },

	{ "eof-object?",   impl_eofp                      },
//	{ "char-ready?",   impl_char_readyp               },

	{ "write",         impl_write                     },
	{ "display",       impl_display                   },
	{ "newline",       impl_newline                   },
	{ "write-char",    impl_write_char                },


	/* System interface */

	{ "load",          impl_load                      },

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
