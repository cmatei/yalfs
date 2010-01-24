/* environments.c -- deals with environments and their contents */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minime.h"

#define frame_variables(frame) car(frame)
#define frame_values(frame) cdr(frame)

#define enclosing_environment(env) cdr(env)
#define first_frame(env) car(env)

static object make_frame(object vars, object vals)
{
	return cons(vars, vals);
}

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

void set_variable_value(object var, object val, object env)
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

void define_variable(object var, object val, object env)
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
