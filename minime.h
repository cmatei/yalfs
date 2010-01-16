#ifndef __MINIME_H
#define __MINIME_H

typedef void *object;

typedef enum {
	T_NIL = 0, T_FIXNUM, T_CHARACTER, T_PAIR, T_BOOLEAN,
	T_FOREIGN_PTR,
	T_PRIMITIVE,
//T_EOF, T_PORT,
	T_STRING, T_SYMBOL,
} object_type;


extern object nil;			     /* the empty list */
extern object the_falsity, the_truth; 	     /* the boolean values */

/* expression keyword symbols */
extern object _quote, _lambda, _if, _set, _begin, _cond, _and, _or;
extern object _case, _let, _letx, _letrec, _do, _delay, _quasiquote;

/* other syntactic keywords */
extern object _else, _implies, _define, _unquote, _unquote_splicing;

extern object end_of_file;



extern object error(char *msg, object o);

extern unsigned long heap_size;

#include "runtime.h"
#include "symbols.h"
#include "xutil.h"


extern object lisp_read(FILE *in);
extern object lisp_eval(object exp, object env);
extern void   lisp_print(FILE *out, object exp);




#endif
