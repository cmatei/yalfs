#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <setjmp.h>
#include <assert.h>

#include "minime.h"

static void check_sanity();

/* these should be packed somewhere */

FILE *input_stream, *output_stream, *error_stream;

object nil; /* the empty list */
object the_empty_environment;
object the_user_environment;

object the_truth, the_falsity;

/* expression keyword symbols */
object _quote, _lambda, _if, _set, _begin, _cond, _and, _or;
object _case, _let, _letx, _letrec, _do, _delay, _quasiquote;

/* other syntactic keywords */
object _else, _implies, _define, _unquote, _unquote_splicing;

object end_of_file;

/*----------------------------------*/


static jmp_buf err_jump;
object error(char *msg, object o)
{
	fprintf(output_stream, "%s, object %p\n", msg, o);
	longjmp(err_jump, 1);
	return nil; /* not reached */
}

static int is_tagged(object exp, object tag)
{
	if (is_pair(exp) && (car(exp) == tag))
		return 1;

	return 0;
}

static int is_self_evaluating(object exp)
{
	return  is_boolean(exp) ||
		is_number(exp)  ||
		is_string(exp)  ||
		is_character(exp);
}


/* (quote exp) */
#define is_quoted(exp) is_tagged(exp, _quote)
#define text_of_quotation(exp) cadr(exp)

/* (set! var val) */
#define is_assignment(exp) is_tagged(exp, _set)
#define assignment_variable(exp) cadr(exp)
#define assignment_value(exp)    caddr(exp)

object lisp_eval(object exp, object env)
{
	/* self evaluating */
	if (is_self_evaluating(exp)) {
		return exp;
	}
	/* quote */
	else if (is_quoted(exp)) {
		return text_of_quotation(exp);
	}
	/* assignment */
	else if (is_assignment(exp)) {
		return assignment_value(exp);
	}


	return cons(make_fixnum(1),
		    cons(exp, nil));
}


void repl(object env)
{
	object exp, val;

	while ((exp = lisp_read(stdin)) != end_of_file) {
		val = lisp_eval(exp, env);

		fprintf(output_stream, "=> ");
		lisp_print(stdout, val);
		fprintf(output_stream, "\n");
	}
}

int main(int argc, char **argv)
{
	input_stream = stdin;
	output_stream = stdout;
	error_stream = stderr;

	runtime_init();

	if (setjmp(err_jump)) {
		printf("Sanity check failed!\n");
		exit(1);
	} else {
		check_sanity();
	}

restart:
	if (setjmp(err_jump)) {
		goto restart;
	}

	repl(nil);

	runtime_stats();

	return 0;
}


static void check_sanity()
{
	object o, o1;

	/* fixnums */
	assert(is_fixnum(make_fixnum(1234)));
	assert(is_fixnum(make_fixnum(-32768)));

	assert(!is_fixnum((object) 1));

	/* characters */
	assert(is_character(make_character('c')));
	assert(!is_character(make_fixnum(10)));

	/* pairs */
	assert(is_pair(cons(make_fixnum(10), make_fixnum(20))));

	assert(!is_pair(make_fixnum(1)));
	assert(!is_pair(make_character('f')));

	o = cons(make_fixnum(10), make_fixnum(-10));
	assert(car(o) == make_fixnum(10));
	assert(cdr(o) == make_fixnum(-10));

	o1 = cons(o, o);
	assert(car(o1) == o);
	assert(cdr(o1) == o);

	assert(caar(o1) == make_fixnum(10));
	assert(cdar(o1) == make_fixnum(-10));

	set_car(o1, o);
	set_cdr(o1, o1);

	assert(car(o1) == o);
	assert(cdr(o1) == o1);

	/* booleans */
	assert(is_false(the_falsity));
	assert(is_true(the_truth));

	assert(is_boolean(the_truth));
	assert(is_boolean(the_falsity));

	/* foreign pointers */
	assert(is_foreign_ptr(make_foreign_ptr((void *) 0xDEADBEEF)));
	assert(foreign_ptr_value(make_foreign_ptr((void *) 0xDEADBEEF)) == (void *) 0xDEADBEEF);

	/* strings */
	assert(string_length(make_string_c("foo", 3)) == 3);

	o = make_string_c("foobarbazooo", 12);
	assert(strncmp(string_value(o), "foobarbazooo", 12) == 0);

	/* symbols */
	assert(make_symbol("foo", 3) == make_symbol("foo", 3));

	assert(string_equal(symbol_string(make_symbol("foo", 3)),
			    make_string_c("foo", 3)));


#if SAFETY
	/* this should throw an error when safety is on */
//	car(make_fixnum(10));
#endif
}
