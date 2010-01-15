#ifndef __MINIME_H
#define __MINIME_H

extern object lisp_read(object env);
extern object lisp_eval(object exp, object env);
extern void   lisp_print(object exp);

extern object error(char *msg, object o);

#endif
