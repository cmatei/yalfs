#ifndef __RUNTIME_H
#define __RUNTIME_H


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

#define EMPTY_LIST_TAG  0xFFUL
#define EMPTY_LIST_MASK 0xFFUL

static inline int is_null(object o)
{
	return (o == nil);
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

static inline object set_car(object pair, object o)
{
#if SAFETY
	if (!is_pair(pair))
		error("Object is not a pair -- SET-CAR!", pair);
#endif

	((object *)((unsigned long) o - PAIR_TAG))[0] = o;
	return o;			     /* r5rs return value is unspecified */
}

static inline object set_cdr(object pair, object o)
{
#if SAFETY
	if (!is_pair(pair))
		error("Object is not a pair -- SET-CDR!", pair);
#endif

	((object *)((unsigned long) o - PAIR_TAG))[1] = o;
	return o;			     /* r5rs return value is unspecified */
}

static inline unsigned long length(object o)
{
	unsigned long len = 0;

	if (o == nil)
		return 0;

#if SAFETY
	if (!is_pair(o))
		error("Object is not a pair -- LENGTH", o);
#endif

	while (!is_null(o)) {
		len++;
		o = cdr(o);
	}

	return len;
}


#define INDIRECT_TAG   3UL
#define INDIRECT_SHIFT 2UL
#define INDIRECT_MASK  3UL

static inline int is_indirect(object o)
{
	return (((unsigned long) o & INDIRECT_MASK) == INDIRECT_TAG);
}

#define STRING_TAG   2UL
#define STRING_MASK  3UL
#define STRING_SHIFT 2UL

static inline int is_string(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & STRING_MASK) == STRING_TAG);
}

extern object make_string(unsigned long length);
extern object make_string_c(char *str, unsigned long length);

static inline unsigned long string_length(object o)
{
	unsigned long indirect;

#if SAFETY
	if (!is_string(o))
		error("Object is not a string -- STRING-LENGTH", o);
#endif
	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return (indirect - STRING_TAG) >> STRING_SHIFT;
}

static inline char *string_value(object o)
{
#if SAFETY
	if (!is_string(o))
		error("Object is not a string -- STRING-VALUE", o);
#endif

	return (char *) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG) + 1);
}

static inline int string_equal(object s1, object s2)
{
#if SAFETY
	if (!is_string(s1))
		error("Object is not a string -- STRING=", s1);

	if (!is_string(s2))
		error("Object is not a string -- STRING=", s2);
#endif

	return ((string_length(s1) == string_length(s2)) &&
		(memcmp(string_value(s1), string_value(s2), string_length(s1)) == 0));
}

#define SYMBOL_TAG  0xBFUL
#define SYMBOL_MASK 0xFFUL

static inline int is_symbol(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & SYMBOL_MASK) == SYMBOL_TAG);
}

extern object make_symbol(char *str, unsigned long len);
extern object make_symbol_with_string(object o);

static inline object make_symbol_c(char *str)
{
	return make_symbol(str, strlen(str));
}

static inline object symbol_string(object o)
{
#if SAFETY
	if (!is_symbol(o))
		error("Object is not a symbol -- SYMBOL-STRING", o);
#endif

	return (object) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [1];
}

static inline int is_expression_keyword(object o)
{
	return  o == _quote || o == _lambda || o == _if     ||
		o == _set   || o == _begin  || o == _cond   ||
		o == _and   || o == _or     || o == _case   ||
		o == _let   || o == _letx   || o == _letrec ||
		o == _do    || o == _delay  || o == _quasiquote;
}

static inline int is_syntactic_keyword(object o)
{
	return  o == _else    || o == _implies || o == _define ||
		o == _unquote || o == _unquote_splicing ||
		is_expression_keyword(o);
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

#define FOREIGN_PTR_TAG  0x3FULL
#define FOREIGN_PTR_MASK 0xFFULL

static inline int is_foreign_ptr(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);

	return ((indirect & FOREIGN_PTR_MASK) == FOREIGN_PTR_TAG);
}

extern object make_foreign_ptr(void *ptr);

static inline void *foreign_ptr_value(object o)
{
#if SAFETY
	if (!is_foreign_ptr(o))
		error("Object is not a foreign pointer -- FOREIGN-PTR-VALUE", o);
#endif

	return (void *) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [1];
}

extern object_type type_of(object o);

extern void runtime_init();
extern void runtime_stats();


#endif

