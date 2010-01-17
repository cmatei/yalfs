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

object nil;				     /* empty list */
object user_environment;		     /* user-initial-environment */

object the_truth, the_falsity;

/* expression keyword symbols */
object _quote, _lambda, _if, _set, _begin, _cond, _and, _or;
object _case, _let, _letx, _letrec, _do, _delay, _quasiquote;

/* other syntactic keywords */
object _else, _implies, _define, _unquote, _unquote_splicing;

object end_of_file;

/* quick hack for compound procedures */
object _interpreted;
object _break;

/*----------------------------------*/


static jmp_buf err_jump;
object error(char *msg, object o)
{
	fprintf(output_stream, "%s, object %p ", msg, o);

	if (is_symbol(o))
		lisp_print(output_stream, o);

	fprintf(output_stream, "\n");

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

/* environments */
object make_frame(object vars, object vals)
{
	return cons(vars, vals);
}

#define frame_variables(frame) car(frame)
#define frame_values(frame) cdr(frame)

static void add_binding_to_frame(object var, object val, object frame)
{
	set_car(frame, cons(var, car(frame)));
	set_cdr(frame, cons(val, cdr(frame)));
}

object extend_environment(object vars, object vals, object base_env)
{
	if (length(vars) == length(vals))
		return cons(make_frame(vars, vals), base_env);

	error("Extend environment has wrong number of args -- EXTEND-ENVIRONMENT", nil);
	return nil; /* not reached */
}

#define enclosing_environment(env) cdr(env)
#define first_frame(env) car(env)

object lookup_variable_value(object var, object env)
{
	object frame, vars, vals;

	while (env != nil) {
		frame = first_frame(env);

		for (vars = frame_variables(frame), vals = frame_values(frame);
		     !is_null(vars);
		     vars = cdr(vars), vals = cdr(vals)) {

			if (var == car(vars))
				return car(vals);
		}

		env = enclosing_environment(env);
	}

	error("Unbound variable", var);
	return nil; 			     /* not reached */
}

static void set_variable_value(object var, object val, object env)
{
	object frame, vars, vals;

	while (env != nil) {
		frame = first_frame(env);

		for (vars = frame_variables(frame), vals = frame_values(frame);
		     !is_null(vars);
		     vars = cdr(vars), vals = cdr(vals)) {

			if (var == car(vars)) {
				set_car(vals, val);
				return;
			}
		}

		env = enclosing_environment(env);
	}

	error("Unbound variable -- SET!", var);
}

static void define_variable(object var, object val, object env)
{
	object frame = first_frame(env);
	object vars, vals;

	for (vars = frame_variables(frame), vals = frame_values(frame);
	     !is_null(vars);
	     vars = cdr(vars), vals = cdr(vals)) {

		if (var == car(vars)) {
			set_car(vals, val);
			return;
		}
	}

	add_binding_to_frame(var, val, frame);
}

object setup_environment()
{
	object initial_env;

	initial_env = extend_environment(primitive_names(),
					 primitive_objects(),
					 nil);

	define_variable(make_symbol_c("true"), the_truth, initial_env);
	define_variable(make_symbol_c("false"), the_falsity, initial_env);
	define_variable(make_symbol_c("nil"), nil, initial_env);

	return initial_env;
}

/* syntax functions */

#define is_variable(exp) is_symbol(exp)

/* (quote exp) */
#define is_quoted(exp) is_tagged(exp, _quote)
#define text_of_quotation(exp) cadr(exp)

/* (set! var val) */
#define is_assignment(exp) is_tagged(exp, _set)
#define assignment_variable(exp) cadr(exp)
#define assignment_value(exp)    caddr(exp)

/* (define v e)
   (define (v p1 p2 p3 ..) body) */
#define is_definition(exp) is_tagged(exp, _define)

static object definition_variable(object exp)
{
	if (is_symbol(cadr(exp)))
	    return cadr(exp);

	return caadr(exp);
}

#define make_lambda(parameters, body) cons(_lambda, cons(parameters, body))

static object definition_value(object exp)
{
	if (is_symbol(cadr(exp)))
		return caddr(exp);
	else
		return make_lambda(cdadr(exp), cddr(exp));
}

#define is_if(exp) is_tagged(exp, _if)
#define if_predicate(exp) cadr(exp)
#define if_consequent(exp) caddr(exp)

static object if_alternate(object exp)
{
	if (!is_null(cdddr(exp)))
		return car(cdddr(exp));

	return the_falsity;
}

#define make_if(predicate, consequent, alternate) cons(_if, cons(predicate, cons(consequent, cons(alternate, nil))))

#define is_cond(exp) is_tagged(exp, _cond)

#define cond_clauses(exp) cdr(exp)

#define cond_predicate(clause) car(clause)
#define cond_actions(clause) cdr(clause)

#define is_cond_else_clause(clause) (cond_predicate(clause) == _else)

#define cond_to_ifs(exp) expand_cond_clauses(cond_clauses(exp))

#define is_begin(exp) is_tagged(exp, _begin)
#define begin_actions(exp) cdr(exp)
#define is_last_exp(exp) is_null(cdr(exp))
#define first_exp(seq) car(seq)
#define rest_exps(seq) cdr(seq)

#define make_begin(seq) cons(_begin, seq)

static object sequence_to_exp(object seq)
{
	return  is_null(seq) ? seq :
		is_last_exp(seq) ? first_exp(seq) :
		make_begin(seq);
}

static object expand_cond_clauses(object clauses)
{
	object first, rest;

	if (is_null(clauses))
		return the_falsity;

	first = car(clauses);
	rest  = cdr(clauses);

	if (is_cond_else_clause(first)) {
		if (is_null(rest)) {
			return sequence_to_exp(cond_actions(first));
		} else {
			error("ELSE clause is not last -- COND->IF", clauses);
		}
	} else {
		return make_if(cond_predicate(first),
			       sequence_to_exp(cond_actions(first)),
			       expand_cond_clauses(rest));
	}

	return nil;			     /* not reached */
}


#define is_lambda(exp) is_tagged(exp, _lambda)
#define lambda_parameters(exp) cadr(exp)
#define lambda_body(exp) cddr(exp)

/* a quick hack before I make a proper object */
#define make_procedure(parameters, body, environment) cons(_interpreted, cons(parameters, cons(body, cons(env, nil))))

#define is_interpreted(p) is_tagged(p, _interpreted)

#define procedure_parameters(p) cadr(p)
#define procedure_body(p) caddr(p)
#define procedure_environment(p) car(cdddr(p))


#define is_application(exp) is_pair(exp)

#define operator(exp) car(exp)
#define operands(exp) cdr(exp)

#define first_operand(exps) car(exps)
#define rest_operands(exps) cdr(exps)

static object list_of_values(object exps, object env)
{
	if (is_null(exps))
		return nil;

	return cons(lisp_eval(first_operand(exps), env),
		    list_of_values(rest_operands(exps), env));
}

static void stack_push(object *stackptr, object o)
{
	*stackptr = cons(o, *stackptr);
}

static object stack_pop(object *stackptr)
{
	object o = car(*stackptr);
	*stackptr = cdr(*stackptr);
	return o;
}

#define is_breakpoint(exp) is_tagged(exp, _break)

static void breakpoint()
{
	/* break here in gdb */
}

object lisp_eval(object exp, object env)
{
	object ret = nil;
	object tail_seq_stack = nil;
	object actions;
	object proc, args;

tail_call:

	/* self evaluating */
	if (is_self_evaluating(exp)) {
		ret = exp;
	}
	/* variables */
	else if (is_variable(exp)) {
		ret = lookup_variable_value(exp, env);
	}
	/* quote */
	else if (is_quoted(exp)) {
		ret = text_of_quotation(exp);
	}
	/* assignment */
	else if (is_assignment(exp)) {
		set_variable_value(assignment_variable(exp),
				   lisp_eval(assignment_value(exp), env),
				   env);
		ret = assignment_variable(exp);
	}
	/* definition */
	else if (is_definition(exp)) {
		define_variable(definition_variable(exp),
				lisp_eval(definition_value(exp), env),
				env);

		ret = definition_variable(exp);
	}
	/* if, tail recursive */
	else if (is_if(exp)) {
		if (is_true(lisp_eval(if_predicate(exp), env))) {
			exp = if_consequent(exp);
			goto tail_call;
		} else {
			exp = if_alternate(exp);
			goto tail_call;
		}
	}
	/* lambda */
	else if (is_lambda(exp)) {
		ret = make_procedure(lambda_parameters(exp),
				     lambda_body(exp),
				     env);
	}
	/* begin */
	else if (is_begin(exp)) {

		goto tail_sequence_entry;
	}
	/* cond */
	else if (is_cond(exp)) {
		exp = cond_to_ifs(exp);
		goto tail_call;
	}
	/* breakpoint hook */
	else if (is_breakpoint(exp)) {
		ret = nil;
		breakpoint();
	}
	/* application */
	else if (is_application(exp)) {

		goto apply;
	}
	else
		error("Unknown expression type -- EVAL", exp);

eval_exit:

	/* if the tail sequence stack is empty, we're done */
	if (is_null(tail_seq_stack))
		return ret;

//tail_sequence_continue:

	env    = stack_pop(&tail_seq_stack);
	actions = stack_pop(&tail_seq_stack);

	if (is_last_exp(actions)) {
		exp = first_exp(actions);
		goto tail_call;
	} else {
		stack_push(&tail_seq_stack, rest_exps(actions));
		stack_push(&tail_seq_stack, env);

		exp = first_exp(actions);
		goto tail_call;
	}
	error("This is not reached", nil);

tail_sequence_entry:

	actions = begin_actions(exp);

	if (is_null(actions)) {
		ret = nil;
	}
	else if (is_last_exp(actions)) {
		exp = first_exp(actions);
		goto tail_call;	     /* tail call to eval */
	}
	else {
		stack_push(&tail_seq_stack, rest_exps(actions));
		stack_push(&tail_seq_stack, env);

		exp = first_exp(actions);

		goto tail_call;
	}

	goto eval_exit;


apply:


	proc = lisp_eval(operator(exp), env);
	args = list_of_values(operands(exp), env);


	if (is_primitive(proc)) {
		ret = apply_primitive(proc, args);
		goto eval_exit;
	}
	else if (is_interpreted(proc)) {
		exp = sequence_to_exp(procedure_body(proc));
		env = extend_environment(procedure_parameters(proc),
					 args,
					 procedure_environment(proc));

		goto tail_call;
	}
	else
		error("Unknown procedure type -- APPLY", proc);

	return nil;
}

object lisp_apply(object procedure, object arguments)
{
	if (is_primitive(procedure)) {
		return apply_primitive(procedure, arguments);
	} else if (is_interpreted(procedure)) {
		return lisp_eval(
			/* transform to sequence */
			sequence_to_exp(procedure_body(procedure)),
			extend_environment(procedure_parameters(procedure),
					   arguments,
					   procedure_environment(procedure)));
	}



	error("Unknown procedure type -- APPLY", procedure);
	return nil;			     /* not reached */
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

	repl(user_environment);

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
