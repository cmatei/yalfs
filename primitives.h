#ifndef __PRIMITIVES_H
#define __PRIMITIVES_H

struct primitive {
	char *name;
	primitive_proc proc;
};

extern struct primitive the_primitives[];

#endif
