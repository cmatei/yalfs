#ifndef __MINIME_H
#define __MINIME_H

typedef void *object;

typedef enum {
	T_NIL = 0, T_FIXNUM, T_CHARACTER, T_PAIR, T_BOOLEAN,
	T_EOF, T_PORT,
	T_STRING, T_SYMBOL,
} object_type;


extern object nil;			     /* the empty list */
extern object the_falsity, the_truth; 	     /* the boolean values */

extern object symbol_table;
extern object quote, unquote;
extern object end_of_file;

extern object error(char *msg, object o);


extern unsigned long heap_size;

#include "runtime.h"
#include "xutil.h"


extern object lisp_read();
extern object lisp_eval(object exp, object env);
extern void   lisp_print(object exp);




#endif
