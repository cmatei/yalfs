#ifndef __RUNTIME_H
#define __RUNTIME_H

typedef void *object;

typedef enum {
	T_FIXNUM = 0, T_CHARACTER, T_PAIR,
	T_BOOLEAN, T_EMPTY_LIST,
} object_type;

extern void runtime_init();
extern void runtime_stats();
extern int  runtime_error_catch();

extern object the_empty_list;
extern object the_falsity;
extern object the_truth;

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

/* FIXME: fix this function if your compiler doesn't do arithmetic shifts */
static inline long fixnum_value(object o)
{
	return ((long) o >> FIXNUM_SHIFT);
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

static inline char character_value(object o)
{
	return (char) ((unsigned long) o >> CHARACTER_SHIFT);
}

#define PAIR_TAG   2UL
#define PAIR_MASK  3UL

extern object cons(object car_value, object cdr_value);

static inline int is_pair(object o)
{
	return (((unsigned long) o & PAIR_MASK) == PAIR_TAG);
}

extern object safe_car(object o);
extern object safe_cdr(object o);

static inline object fast_car(object o)
{
	return ((object *)((unsigned long) o - PAIR_TAG))[0];
}

static inline object fast_cdr(object o)
{
	return ((object *)((unsigned long) o - PAIR_TAG))[1];
}

#if (SAFETY == 0)

#define car fast_car
#define cdr fast_cdr

#else

#define car safe_car
#define cdr safe_cdr

#endif

#define caar(x) car(car((x)))
#define cadr(x) car(cdr((x)))
#define cdar(x) cdr(car((x)))
#define cddr(x) cdr(cdr((x)))


#define INDIRECT_TAG   3UL
#define INDIRECT_SHIFT 2UL
#define INDIRECT_MASK  3UL

static inline int is_indirect(object o)
{
	return (((unsigned long) o & INDIRECT_MASK) == INDIRECT_TAG);
}

#define EMPTY_LIST_TAG  0xFFUL
#define EMPTY_LIST_MASK 0xFFUL

static inline int is_empty_list(object o)
{
	return (o == the_empty_list);
}

#define BOOLEAN_TAG   0xEFULL
#define BOOLEAN_SHIFT 0x08ULL
#define BOOLEAN_MASK  0xFFULL

static inline int is_false(object o)
{
	return (o == the_falsity);
}

/* everything except the_falsity is true */
static inline int is_true(object o)
{
	return !is_false(o);
}

static inline int is_boolean(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);

	return ((indirect & BOOLEAN_MASK) == BOOLEAN_TAG);
}


extern object_type type_of(object o);


#endif

