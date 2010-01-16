#include <stdlib.h>
#include <stdio.h>
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

object symbol_table;
object quote, unquote;

object end_of_file;

/*----------------------------------*/


static jmp_buf err_jump;
object error(char *msg, object o)
{
	fprintf(output_stream, "%s, object %p\n", msg, o);
	longjmp(err_jump, 1);
	return nil; /* not reached */
}

object lisp_eval(object exp, object env)
{
	return exp;
}

static void lisp_print_pair(object pair)
{
	object car_obj, cdr_obj;

	car_obj = car(pair);
	cdr_obj = cdr(pair);

	lisp_print(car_obj);

	if (is_pair(cdr_obj)) {
		fprintf(output_stream, " ");
		lisp_print_pair(cdr_obj);
	} else if (is_null(cdr_obj)) {
		return;
	} else {
		fprintf(output_stream, " . ");
		lisp_print(cdr_obj);
	}
}

void lisp_print(object exp)
{
	char c;

	switch (type_of(exp)) {
	case T_NIL:
		fprintf(output_stream, "()");
		break;

	case T_FIXNUM:
		fprintf(output_stream, "%ld", fixnum_value(exp));
		break;

	case T_CHARACTER:
		c = character_value(exp);
		fprintf(output_stream, "#\\");
		switch (c) {
		case '\n':
			fprintf(output_stream, "newline");
			break;
		case ' ':
			fprintf(output_stream, "space");
			break;
		default:
			fprintf(output_stream, "%c", c);
		}
		break;

	case T_PAIR:
		fprintf(output_stream, "(");
		lisp_print_pair(exp);
		fprintf(output_stream, ")");
		break;

	case T_BOOLEAN:
		fprintf(output_stream, is_false(exp) ? "#f" : "#t");
		break;
	}
}

void repl(object env)
{
	object exp, val;

	do {
		exp = lisp_read(env);
		val = lisp_eval(exp, env);

		lisp_print(val);

		fprintf(output_stream, "\n");
	} while (exp != end_of_file);
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

	//repl(nil);

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

	/* booleans */
	assert(is_false(the_falsity));
	assert(is_true(the_truth));

	assert(is_boolean(the_truth));
	assert(is_boolean(the_falsity));

	/* foreign pointers */
	assert(is_foreign_ptr(make_foreign_ptr((void *) 0xDEADBEEF)));
	assert(foreign_ptr_value(make_foreign_ptr((void *) 0xDEADBEEF)) == (void *) 0xDEADBEEF);

#if SAFETY
	/* this should throw an error when safety is on */
//	car(make_fixnum(10));
#endif
}
