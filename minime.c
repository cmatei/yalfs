#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <getopt.h>

#include <setjmp.h>
#include <assert.h>

#include "minime.h"

/* these should be packed somewhere */

object nil;				     /* empty list */
object unspecified;			     /* unspecified object, for return values */

object empty_environment;		     /* the empty environment */
object null_environment;		     /* initial environment */
object interaction_environment;		     /* user initial environment */

object the_truth, the_falsity;

/* expression keyword symbols */
object _quote, _lambda, _if, _set, _begin, _cond, _and, _or;
object _case, _let, _letx, _letrec, _do, _delay, _quasiquote;

/* other syntactic keywords */
object _else, _implies, _define, _unquote, _unquote_splicing;

/* ... other useful symbols */
object _cons, _list, _append;
object _ellipsis;

object end_of_file;

object current_input_port;
object current_output_port;
object current_error_port;

object _break;
object result_prompt;

/*----------------------------------*/

int error_is_unsafe = 1;
int emacs = 0;

jmp_buf err_jump;

static int is_tagged(object exp, object tag)
{
	if (is_pair(exp) && (car(exp) == tag))
		return 1;

	return 0;
}

static int is_self_evaluating(object exp)
{
	return  is_null(exp)      ||
		is_boolean(exp)   ||
		is_number(exp)    ||
		is_string(exp)    ||
		is_vector(exp)    ||
		is_character(exp) ||
		is_unspecified(exp);	     /* not sure this leads to right behaviour */

	/* Re: vectors, 'Note that this is the external representation
	   of a vector, not an expression evaluating to a vector. Like
	   list constants, vector constants must be quoted.'

	   mit-scheme and mzscheme also happily eval #(a b c).
	   Leaving as is until I figure it out.
	*/
}

static int is_primitive_syntax(object proc, primitive_proc implementation)
{
	return is_primitive(proc) && primitive_implementation(proc) == implementation;
}

/* syntax functions */

#define is_variable(exp) is_symbol(exp)


/* (quote exp) */
#define is_quoted(exp) is_tagged(exp, _quote)

#define is_quotation(proc) is_primitive_syntax(proc, lisp_primitive_quote)
#define is_quasiquotation(proc) is_primitive_syntax(proc, lisp_primitive_quasiquote)

#define text_of_quotation(exp) cadr(exp)

/* (set! var val) */
#define is_assignment(proc) is_primitive_syntax(proc, lisp_primitive_set)
#define assignment_variable(exp) cadr(exp)
#define assignment_value(exp)    caddr(exp)

/* (define v e)
   (define (v p1 p2 p3 ..) body) */
#define is_definition(proc) is_primitive_syntax(proc, lisp_primitive_define)

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

#define is_if(proc) is_primitive_syntax(proc, lisp_primitive_if)
#define if_predicate(exp) cadr(exp)
#define if_consequent(exp) caddr(exp)

static object if_alternate(object exp)
{
	if (!is_null(cdddr(exp)))
		return car(cdddr(exp));

	return the_falsity;
}

#define make_if(predicate, consequent, alternate) cons(_if, cons(predicate, cons(consequent, cons(alternate, nil))))

#define is_cond(proc) is_primitive_syntax(proc, lisp_primitive_cond)

#define cond_clauses(exp) cdr(exp)

#define cond_predicate(clause) car(clause)
#define cond_actions(clause) cdr(clause)

#define is_cond_else_clause(clause) (cond_predicate(clause) == _else)

#define cond_to_ifs(exp) expand_cond_clauses(cond_clauses(exp))

#define is_begin(proc) is_primitive_syntax(proc, lisp_primitive_begin)

#define begin_actions(exp) cdr(exp)
#define is_last_exp(exp) is_null(cdr(exp))
#define first_exp(seq) car(seq)
#define rest_exps(seq) cdr(seq)

#define is_let(proc) is_primitive_syntax(proc, lisp_primitive_let)
#define is_letx(proc) is_primitive_syntax(proc, lisp_primitive_letx)
#define is_letrec(proc) is_primitive_syntax(proc, lisp_primitive_letrec)

#define let_bindings(exp) cadr(exp)
#define let_body(exp) cddr(exp)

#define binding_name(exp) car(exp)

static object binding_value(object exp)
{
	return is_null(cdr(exp)) ? unspecified : cadr(exp);
}

static object let_names(object exp)
{
	return is_null(exp) ? nil :
		cons( binding_name(first_exp(exp)), let_names(rest_exps(exp)));
}

static object let_values(object exp)
{
	return is_null(exp) ? nil :
		cons( binding_value(first_exp(exp)), let_values(rest_exps(exp)));
}

static object let_to_combination(object exp)
{
	object name = nil, bindings, body;
	object o;

	/* named let ? */
	if (is_symbol(cadr(exp))) {

		name     = cadr(exp);
		bindings = let_bindings(cdr(exp));
		body     = let_body(cdr(exp));
	} else {
		bindings = let_bindings(exp);
		body     = let_body(exp);
	}

	o = make_lambda(let_names(bindings), body);

	if (is_null(name))
		o = cons( o, let_values(bindings));
	else {

		/* (letrec ((name (lambda (<binding-names>) <body>)))
		       (name <binding-values>)) */
		o = list(3,
			 _letrec,
			 list(1, list(2, name, o)),
			 cons( name, let_values(bindings)));
	}

	return o;
}


static object letx_to_combination_rec(object bindings, object body)
{
	/* ((lambda (name) body) value) */
	if (is_last_exp(bindings))
		return list(2,
			    make_lambda(list(1, binding_name(first_exp(bindings))),
					body),
			    binding_value(first_exp(bindings)));

	/* ((lambda (name) ... recurse) value) */
	return list(2,
		    make_lambda( list(1, binding_name(first_exp(bindings)) ),
				 letx_to_combination_rec( rest_exps(bindings), body)),
		    binding_value(first_exp(bindings)));
}

static object letx_to_combination(object exp)
{
	if (is_null(let_bindings(exp)))
		return list(1, make_lambda(nil, let_body(exp)));

	return letx_to_combination_rec(let_bindings(exp), let_body(exp));
}


static object make_letrec_body(object bindings, object body)
{
	object head = nil, tail = nil;
	object setexp, lambda;

	while (!is_null(bindings)) {

		setexp = list(3, _set, binding_name(car(bindings)), binding_value(car(bindings)));

		if (is_null(head)) {
			head = tail = cons(setexp, nil);
		} else {
			set_cdr(tail, cons(setexp, nil));
			tail = cdr(tail);
		}

		bindings = cdr(bindings);
	}

	/* FIXME: I don't think I really need to wrap the body in a lambda */
	lambda = make_lambda(nil, body);

	if (is_null(head))
		return list(1, list(1, lambda));

	set_cdr(tail, list(1, list(1, lambda)));
	return head;
}

static object letrec_undefineds_for_bindings(object bindings)
{
	object head = nil, tail = nil;
	long i, len = length(bindings);

	for (i = 0; i < len; i++) {
		if (is_null(head)) {
			head = tail = cons(undefined, nil);
		} else {
			set_cdr(tail, cons(undefined, nil));
			tail = cdr(tail);
		}
	}

	return head;
}

static object letrec_to_combination(object exp)
{
	object bindings = let_bindings(exp);

	return cons( make_lambda( let_names(bindings),
				  make_letrec_body(bindings, let_body(exp))),
		     letrec_undefineds_for_bindings(bindings));
}


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


#define is_lambda(proc) is_primitive_syntax(proc, lisp_primitive_lambda)
#define lambda_parameters(exp) cadr(exp)
#define lambda_body(exp) cddr(exp)

#define is_and(proc) is_primitive_syntax(proc, lisp_primitive_and)
#define is_or(proc) is_primitive_syntax(proc, lisp_primitive_or)


#define is_application(exp) is_pair(exp)

#define operator(exp) car(exp)
#define operands(exp) cdr(exp)

#define first_operand(exps) car(exps)
#define rest_operands(exps) cdr(exps)

static object list_of_values(object exps, object env)
{
	if (is_null(exps))
		return nil;

	return cons( lisp_eval(first_operand(exps), env),
		     list_of_values(rest_operands(exps), env));
}

static object list_of_apply_values(object exps, object env)
{
	object o;

	if (is_null(exps))
		return nil;

	if (is_last_exp(exps)) {
		if (!is_list((o = lisp_eval(first_operand(exps), env))))
			error("Last argument must be a list -- apply", o);

		return o;
	}

	return cons( lisp_eval(first_operand(exps), env),
		     list_of_apply_values(rest_operands(exps), env));
}

/* Assuming vars is an improper list (we do), cons a fresh proper list
   with its contents and wrap the tail of *vals in a cons cell */
static void fixup_varargs(object *names, object *values)
{
	object vars = *names, vals = *values;
	object head = nil, tail = nil, vprec = nil;
	object o;

	// assert(!is_list(*vars));

	while (is_pair(vars)) {

		o = cons(car(vars), nil);

		if (head == nil) {
			head = tail = o;
		} else {
			set_cdr(tail, o);
			tail = cdr(tail);
		}

		/* skip over vals */
		if (is_null(vals))
			error("Insufficient fixed arguments", nil);

		vprec = vals;
		vals  = cdr(vals);

		vars = cdr(vars);
	}

	/* r5rs: there must be at least one formal before the period.
	   otoh, i've seen (define (main . args) ...) so maybe i
	   should allow it ?! */

	set_cdr(tail, cons(vars, nil));
	set_cdr(vprec, cons(vals, nil));

	*names = head;
}


#define is_eval(proc) is_primitive_syntax(proc, lisp_primitive_eval)
#define is_apply(proc) is_primitive_syntax(proc, lisp_primitive_apply)

#define is_timecall(proc) is_primitive_syntax(proc, lisp_primitive_timecall)
#define is_pmacro(proc) is_primitive_syntax(proc, lisp_primitive_pmacro)
#define is_macroexpand(proc) is_primitive_syntax(proc, lisp_primitive_macroexpand)

object maybe_unquote(object exp)
{
	if (is_quoted(exp))
		return text_of_quotation(exp);

	return exp;
}

/* quasiquotation */
int qq_is_constant(object exp)
{
	return is_pair(exp) ? is_quoted(exp) : !is_symbol(exp);
}

object qq_combine_parts(object left, object right, object exp, object env)
{
	object leval, reval;

	if (qq_is_constant(left) && qq_is_constant(right)) {
		leval = lisp_eval(left, env);
		reval = lisp_eval(right, env);

		if (is_eqv(leval, car(exp)) && is_eqv(reval, cdr(exp)))
			return list(2, _quote, exp);
		else
			return list(2, _quote, cons(leval, reval));
	}
	else if (is_null(right)) {
		return list(2, _list, left);
	}
	else if (is_pair(right) && car(right) == _list) {
		return cons(_list, cons(left, cdr(right)));
	}

	return list(3, _cons, left, right);
}

object qq_expand(object exp, unsigned long nesting, object env)
{
	if (!is_pair(exp)) {
		if (qq_is_constant(exp))
			return exp;
		else
			return list(2, _quote, exp);
	}
	else if (is_tagged(exp, _unquote) && length(exp) == 2) {
		if (nesting == 0)
			return cadr(exp);

		return qq_combine_parts( cons(_quote, cons(_quote, cons(_unquote, nil))),
					 qq_expand( cdr(exp), nesting - 1, env),
					 exp,
					 env);
	}
	else if (is_tagged(exp, _quasiquote) && length(exp) == 2) {

		return qq_combine_parts( cons(_quote, cons(_quote, cons(_quasiquote, nil))),
					 qq_expand( cdr(exp), nesting + 1, env),
					 exp,
					 env);
	}
	else if (is_pair(car(exp)) && caar(exp) == _unquote_splicing && length(car(exp)) == 2) {
		if (nesting == 0)
			return list(2, _append, cadr(car(exp)));

		return qq_combine_parts( qq_expand( car(exp), nesting - 1, env),
					 qq_expand( cdr(exp), nesting, env),
					 exp,
					 env);
	}

	return qq_combine_parts( qq_expand( car(exp), nesting, env),
				 qq_expand( cdr(exp), nesting, env),
				 exp,
				 env);
}

/* very dirty */
object macroexpand(object macro, object exp, object env)
{
	object menv;

	menv = extend_environment( list(1, make_symbol_c("exp")),
				  list(1, exp),
				  env);
//				  macro_environment(macro));

	return lisp_eval( macro_body(macro), menv);
}

void breakpoint()
{
}

#define is_breakpoint(proc) is_primitive_syntax(proc, lisp_primitive_break)

object lisp_eval(object exp, object env)
{
	object exps, val;
	object proc, args, vars;
	long nargs;

tail_call:

	/* self evaluating */
	if (is_self_evaluating(exp)) {
		return exp;
	}
	/* variables */
	else if (is_variable(exp)) {
		return lookup_variable_value(exp, env);
	}
	/* everything else must be application-like */
	else if (!is_pair(exp)) {
		error("Unknown expression type -- EVAL", exp);
	}

	/* language syntax, unless the symbols are bound to something else */

	proc = lisp_eval(operator(exp), env);

	/* quote */
	if (is_quotation(proc)) {
		return text_of_quotation(exp);
	}
	/* quasiquotation */
	else if (is_quasiquotation(proc)) {
		exp = qq_expand(cadr(exp), 0, env);
		goto tail_call;
	}
	/* assignment */
	else if (is_assignment(proc)) {
		set_variable_value(assignment_variable(exp),
				   lisp_eval(assignment_value(exp), env),
				   env);
		return assignment_variable(exp);
	}
	/* definition */
	else if (is_definition(proc)) {
		define_variable(definition_variable(exp),
				lisp_eval(definition_value(exp), env),
				env);

		return definition_variable(exp);
	}
	/* if, tail recursive */
	else if (is_if(proc)) {
		exp = is_true(lisp_eval(if_predicate(exp), env)) ?
			if_consequent(exp) :
			if_alternate(exp);

		goto tail_call;
	}
	/* lambda */
	else if (is_lambda(proc)) {
		return make_procedure(lambda_parameters(exp),
				     lambda_body(exp),
				     env);
	}
	/* and */
	else if (is_and(proc)) {
		exps = operands(exp);

		while (!is_null(exps)) {

			if (is_last_exp(exps)) {
				exp = first_exp(exps);
				goto tail_call;
			}

			if (lisp_eval(first_exp(exps), env) == the_falsity)
				return the_falsity;

			exps = rest_exps(exps);
		}

		return the_truth;
	}
	/* or */
	else if (is_or(proc)) {
		exps = operands(exp);

		while (!is_null(exps)) {

			if (is_last_exp(exps)) {
				exp = first_exp(exps);
				goto tail_call;
			}

			val = lisp_eval(first_exp(exps), env);
			if (val != the_falsity)
				return val;

			exps = rest_exps(exps);
		}

		return the_falsity;
	}
	/* let */
	else if (is_let(proc)) {
		exp = let_to_combination(exp);
		goto tail_call;
	}
	/* let* */
	else if (is_letx(proc)) {
		exp = letx_to_combination(exp);
		goto tail_call;
	}
	/* letrec */
	else if (is_letrec(proc)) {
		exp = letrec_to_combination(exp);
		goto tail_call;
	}
	/* begin */
	else if (is_begin(proc)) {

		exps = begin_actions(exp);

		while (!is_null(exps)) {
			/* tail call for last expression in a sequence */
			if (is_last_exp(exps)) {
				exp = first_exp(exps);
				goto tail_call;
			}

			lisp_eval(first_exp(exps), env);

			exps = rest_exps(exps);
		}

		return nil;
	}
	/* cond */
	else if (is_cond(proc)) {
		exp = cond_to_ifs(exp);
		goto tail_call;
	}
	/* eval */
	else if (is_eval(proc)) {
		nargs = length(operands(exp));

		if (nargs < 1)
			error("Expecting at least 1 argument -- EVAL", exp);

		if (nargs > 1)
			env = lisp_eval(cadr(operands(exp)), env);
//		else
//			env = interaction_environment;

		/* "Expression must be a valid Scheme expression represented as data ..." */
		exp = maybe_unquote(car(operands(exp)));

		goto tail_call;
	}
	/* apply */
	else if (is_apply(proc)) {

		if (length(operands(exp)) < 1)
			error("Expecting at least 1 argument -- APPLY", exp);

		proc = lisp_eval(car(operands(exp)), env);
		args = list_of_apply_values(cdr(operands(exp)), env);

		goto apply;
	}
	/* time-call */
	else if (is_timecall(proc)) {
		unsigned long h_start, h_end, t_start, t_end;

		h_start = runtime_current_heap_usage();
		t_start = runtime_current_timestamp();

		val = lisp_eval(car(operands(exp)), env);

		h_end = runtime_current_heap_usage();
		t_end = runtime_current_timestamp();

		return list(3, val, make_fixnum(t_end - t_start), make_fixnum(h_end - h_start));
	}
	/* break */
	else if (is_breakpoint(proc)) {
		breakpoint();
	}
	/* primitive macro */
	else if (is_pmacro(proc)) {
		return make_macro( cadr(exp),
				   cons( cons( _lambda,
					       cons( nil,
						     is_last_exp(cddr(exp)) ? cons(caddr(exp), nil) : cddr(exp))),
					 nil),
				   null_environment );
	}
	/* macroexpand */
	else if (is_macroexpand(proc)) {

		if (length(operands(exp)) != 1)
			error("Expecting 1 argument -- macroexpand", exp);

		val = car(car(operands(exp)));

		proc = lisp_eval(val, env);
		if (!is_macro(proc))
			error("Not a macro -- macroexpand", car(operands(exp)));

		val = macroexpand(proc, car(operands(exp)), env);
		return val;
	}
	/* macro */
	else if (is_macro(proc)) {
		exp = macroexpand(proc, exp, env);
		goto tail_call;
	}
	/* application */
	else {

		args = list_of_values(operands(exp), env);
		goto apply;
	}

	/* not reached */
	return nil;

apply:
	if (is_primitive(proc)) {
		return apply_primitive(proc, args);
	}
	else if (is_procedure(proc)) {

		vars = procedure_parameters(proc);

		/* If vars is an improper list, we're dealing with optional arguments.
		   It's safe to mutate args, but must not mutate vars */
		if (!is_list(vars)) {
			fixup_varargs(&vars, &args);
		}

		env = extend_environment(vars,
					 args,
					 procedure_environment(proc));

		exp = sequence_to_exp(procedure_body(proc));

		/* r5rs: the first argument passed to apply
		 * must be called via a tail call */
		goto tail_call;
	}
	else
		error("Unknown procedure type -- APPLY", proc);

	/* not reached */
	return nil;
}

object lisp_repl(object input_port, object output_port, object env)
{
	object exp, val = nil;

	while (1) {

		if (emacs && output_port != nil) {
			emacs_prompt_for_command_expression(output_port);
			emacs_read_start(output_port);
		}

		exp = io_read(input_port);
		if (exp == end_of_file)
			break;

		if (emacs && output_port != nil)
			emacs_read_finish(output_port);

		val = lisp_eval(exp, env);

		if (output_port != nil) {

			if (emacs) {
				emacs_write_result(val, output_port);
			} else {
				io_display(result_prompt, output_port);
				io_write(val, output_port);
				io_newline(output_port);
			}
		}
	}

	return val;
}


object library_file_path(char *name)
{
	char file_path[256];
	int len;

	len = snprintf(file_path, 256, "%s/lib/%s", STRINGIFY(INSTALLDIR), name);
	assert(len < 256);

	return make_string_c(file_path);
}

object setup_initial_environment(object baseenv)
{
	object initial_env;
	int i;

	initial_env = extend_environment(nil, nil, baseenv);

	for (i = 0; the_primitives[i].name != NULL; i++) {
		define_variable(make_symbol_c(the_primitives[i].name),
				make_primitive(the_primitives[i].proc),
				initial_env);
	}

	define_variable(make_symbol_c("true"), the_truth, initial_env);
	define_variable(make_symbol_c("false"), the_falsity, initial_env);
	define_variable(make_symbol_c("nil"), nil, initial_env);

	io_load(library_file_path("minime.scm"), initial_env);

	return initial_env;
}

void scheme_init()
{
	/* make the empty list object */
	nil = make_the_empty_list();
	end_of_file = make_the_eof();
	unspecified = make_the_unspecified_value();

	/* the booleans */
	the_falsity = make_boolean(0);
	the_truth   = make_boolean(1);

	/* uses nil */
	symbol_table_init();

	current_input_port  = make_port(stdin,  PORT_TYPE_INPUT);
	current_output_port = make_port(stdout, PORT_TYPE_OUTPUT);
//	current_error_port  = make_port(stderr, PORT_TYPE_OUTPUT);
	current_error_port  = current_output_port;

	result_prompt = make_string_c("=> ");

	/* expression keywords */
	_quote            = make_symbol_c("quote");
	_lambda           = make_symbol_c("lambda");
	_if               = make_symbol_c("if");
	_set              = make_symbol_c("set!");
	_begin            = make_symbol_c("begin");
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

	_cons             = make_symbol_c("cons");
	_list             = make_symbol_c("list");
	_append           = make_symbol_c("append");
	_ellipsis         = make_symbol_c("...");

	/* hacks */
	_break            = make_symbol_c("break");

	/* environments */
	empty_environment       = extend_environment(nil, nil, nil);
	null_environment        = setup_initial_environment(empty_environment);
	interaction_environment = extend_environment(nil, nil, null_environment);
}


static void parse_arguments(int argc, char **argv)
{
	struct option long_options[] = {
		{ "emacs",     no_argument,       NULL, 'e' },
		{ "heap-size", required_argument, NULL, 'h' },

		{ 0, 0, 0, 0 }
	};
	int opt;

	while ((opt = getopt_long_only(argc, argv, "eh:", long_options, NULL)) != -1) {
		switch (opt) {
		case 'e':
			emacs = 1;
			break;

		case 'h':
			heap_size = ((unsigned long) atoi(optarg)) * 1024 * 1024;
			break;

		default:
			fprintf(stderr, "Usage: minime [-emacs] [-heap-size MB]\n");
			exit(1);
		}
	}
}

int main(int argc, char **argv)
{

	parse_arguments(argc, argv);

	error_is_unsafe = 1;

	runtime_init();
	emacs_init();
	scheme_init();

	error_is_unsafe = 0;

restart:
	if (setjmp(err_jump))
		goto restart;

	if (emacs)
		emacs_set_default_directory(current_output_port);

	lisp_repl(current_input_port, current_output_port, interaction_environment);

	runtime_stats();

	return 0;
}


#if 0
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

}

#endif
