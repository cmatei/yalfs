/* xutil.c -- utility functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xutil.h"

void *
xmalloc(size_t size)
{
	void *p = malloc(size);
	
	if (p == NULL) {
		FATAL("malloc failed at %p\n", __builtin_return_address(0));
	}

	return p;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);

	if (p == NULL) {
		FATAL("realloc failed at %p\n", __builtin_return_address(0));
	}

	return p;
}

void
xfree(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}

char *
xstrdup(const char *s)
{
	char *d;

	if (s)
		d = strdup(s);
	else
		d = strdup("");

	if (d == NULL) {
		FATAL("strdup failed at %p\n", __builtin_return_address(0));
	}

	return d;
}


int 
xstrlen(const char *s)
{
	if (s == NULL)
		return 0;
	else
		return strlen(s);
}


