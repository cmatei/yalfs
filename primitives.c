#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"


static object lisp_cons(object args)
{
	return cons(car(args), cadr(args));
}

static object lisp_add1(object args)
{
	return nil;
}


static struct {
	char *name;
	primitive_proc proc;
} the_primitives[] = {
	{ "cons", lisp_cons },

	{ "add1", lisp_add1 },


	{ NULL, NULL }
};

object primitive_names()
{
	object names = nil;
	unsigned long i;

	for (i = 0; the_primitives[i].name; i++) {
		names = cons(make_symbol_c(the_primitives[i].name), names);
	}

	return names;
}

object primitive_objects()
{
	object prims = nil;
	unsigned long i;

	for (i = 0; the_primitives[i].name; i++) {
		prims = cons(make_primitive(the_primitives[i].proc), prims);
	}

	return prims;
}
