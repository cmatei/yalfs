#ifndef __MINIME_H
#define __MINIME_H

typedef void *object;

typedef enum {
	T_NIL = 0, T_BOOLEAN, T_FIXNUM, T_CHARACTER,
	T_STRING, T_SYMBOL, T_PAIR, T_PRIMITIVE, T_PROCEDURE,
	T_PORT, T_EOF, T_FOREIGN_PTR, T_UNSPECIFIED,

	T_MAX_TYPE
} object_type;


extern object nil;			     /* the empty list */
extern object unspecified;		     /* unspecified, the return value */
extern object the_falsity, the_truth; 	     /* the boolean values */
extern object end_of_file;		     /* the end-of-file object */

extern object empty_environment;	     /* the empty environment */
extern object null_environment;		     /* initial environment */
extern object interaction_environment;	     /* user initial environment */

extern object current_input_port;
extern object current_output_port;
extern object current_error_port;

#define undefined unspecified

/* expression keyword symbols */
extern object _quote, _lambda, _if, _set, _begin, _cond, _and, _or;
extern object _case, _let, _letx, _letrec, _do, _delay, _quasiquote;

/* other syntactic keywords */
extern object _else, _implies, _define, _unquote, _unquote_splicing;

extern object _ellipsis;

extern object _break;
extern object result_prompt;

extern unsigned long heap_size;
extern int emacs;
extern int error_is_unsafe;
extern void error(char *msg, object o);

#include "xutil.h"
#include "runtime.h"
#include "io.h"
#include "symbols.h"
#include "primitives.h"
#include "environments.h"
#include "emacs.h"

extern object lisp_read(FILE *in);
extern object lisp_eval(object exp, object env);
extern void   lisp_print(object exp, FILE *out);
extern void   lisp_display(object exp, FILE *out);

extern object lisp_repl(object input_port, object output_port, object env);

#endif
