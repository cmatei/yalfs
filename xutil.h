#ifndef __XUTIL_H
#define __XUTIL_H

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FATAL(fmt, args...) do { fprintf(stderr, fmt, ##args); exit(1); } while (0)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern void  xfree(void *ptr);

extern char *xstrdup(const char *s);

extern int   xstrlen(const char *s);

#ifdef __cplusplus
}
#endif

#endif
