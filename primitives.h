#ifndef __PRIMITIVES_H
#define __PRIMITIVES_H

struct primitive {
	char *name;
	primitive_proc proc;
};

extern struct primitive the_primitives[];

/* C versions for the equality predicates */

extern int is_eq(object o1, object o2);
extern int is_eqv(object o1, object o2);
extern int is_equal(object o1, object o2);

extern int is_string_equal(object o1, object o2);


/* these functions raise an error if called */
extern object lisp_primitive_quote(object args);
extern object lisp_primitive_set(object args);
extern object lisp_primitive_define(object args);
extern object lisp_primitive_if(object args);
extern object lisp_primitive_lambda(object args);
extern object lisp_primitive_and(object args);
extern object lisp_primitive_or(object args);
extern object lisp_primitive_let(object args);
extern object lisp_primitive_letx(object args);
extern object lisp_primitive_letrec(object args);
extern object lisp_primitive_begin(object args);
extern object lisp_primitive_cond(object args);

extern object lisp_primitive_eval(object args);
extern object lisp_primitive_apply(object args);

extern object lisp_primitive_quasiquote(object args);

extern object lisp_primitive_timecall(object args);

extern object lisp_primitive_break(object args);
extern object lisp_primitive_pmacro(object args);
extern object lisp_primitive_macroexpand(object args);

#endif
