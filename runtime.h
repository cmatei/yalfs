#ifndef __RUNTIME_H
#define __RUNTIME_H


typedef void *object;

extern void runtime_init();


#define FIXNUM_TAG   0UL
#define FIXNUM_SHIFT 2UL
#define FIXNUM_MASK  3UL

/* FIXME: fix this function if your compiler doesn't do arithmetic shifts */
static inline object make_fixnum(long number)
{
	return (object) ((number << FIXNUM_SHIFT) | FIXNUM_TAG);
}

static inline int is_fixnum(object o)
{
	return (((unsigned long) o & FIXNUM_MASK) == FIXNUM_TAG);
}

#define CHARACTER_TAG   1UL
#define CHARACTER_SHIFT 2UL
#define CHARACTER_MASK  3UL

static inline object make_character(char c)
{
	return (object) (((unsigned long) c << CHARACTER_SHIFT) | CHARACTER_TAG);
}

static inline int is_character(object o)
{
	return (((unsigned long) o & CHARACTER_MASK) == CHARACTER_TAG);
}

#define PAIR_TAG   2UL
#define PAIR_SHIFT 2UL
#define PAIR_MASK  3UL

extern object cons(object car_value, object cdr_value);

static inline int is_pair(object o)
{
	return (((unsigned long) o & PAIR_MASK) == PAIR_TAG);
}

#endif
