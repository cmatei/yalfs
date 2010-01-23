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

#define is_number is_fixnum

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

#define caaar(x) car(car(car(x)))
#define caadr(x) car(car(cdr(x)))
#define cadar(x) car(cdr(car(x)))
#define caddr(x) car(cdr(cdr(x)))
#define cdaar(x) cdr(car(car(x)))
#define cdadr(x) cdr(car(cdr(x)))
#define cddar(x) cdr(cdr(car(x)))
#define cdddr(x) cdr(cdr(cdr(x)))

#define caaaar(x) car(car(car(car(x))))
#define caaadr(x) car(car(car(cdr(x))))
#define caadar(x) car(car(cdr(car(x))))
#define caaddr(x) car(car(cdr(cdr(x))))
#define cadaar(x) car(cdr(car(car(x))))
#define cadadr(x) car(cdr(car(cdr(x))))
#define caddar(x) car(cdr(cdr(car(x))))
#define cadddr(x) car(cdr(cdr(cdr(x))))
#define cdaaar(x) cdr(car(car(car(x))))
#define cdaadr(x) cdr(car(car(cdr(x))))
#define cdadar(x) cdr(car(cdr(car(x))))
#define cdaddr(x) cdr(car(cdr(cdr(x))))
#define cddaar(x) cdr(cdr(car(car(x))))
#define cddadr(x) cdr(cdr(car(cdr(x))))
#define cdddar(x) cdr(cdr(cdr(car(x))))
#define cddddr(x) cdr(cdr(cdr(cdr(x))))

static inline object set_car(object pair, object o)
{
#if SAFETY
	if (!is_pair(pair))
		error("Object is not a pair -- set-car!", pair);
#endif

	((object *)((unsigned long) pair - PAIR_TAG))[0] = o;
	return o;			     /* r5rs return value is unspecified */
}

static inline object set_cdr(object pair, object o)
{
#if SAFETY
	if (!is_pair(pair))
		error("Object is not a pair -- set-cdr!", pair);
#endif

	((object *)((unsigned long) pair - PAIR_TAG))[1] = o;
	return o;			     /* r5rs return value is unspecified */
}

static inline unsigned long length(object o)
{
	unsigned long len = 0;

	/* the empty list has length 0, but is not a pair! */
	if (o == nil)
		return 0;

#if SAFETY
	if (!is_pair(o))
		error("Object is not a pair -- length", o);
#endif

	while (!is_null(o)) {
		len++;
		o = cdr(o);
	}

	return len;
}


/* This is where we detect cyclic lists, using the tortoise and hare
 * algorithm */
static inline int is_finite_list(object o, object *last_el_ptr)
{
	object rabbit;

	/* the hare moves twice as fast through the list. If the
	 * hare meets the tortoise, there is a loop. If the hare
	 * reaches the end, there is no loop.  o is our tortoise.
	 */
	rabbit =  o;
	while (is_pair(rabbit)) {
		rabbit = cdr(rabbit);
		o = cdr(o);

		if (!is_pair(rabbit))
			break;

		rabbit = cdr(rabbit);

		if (rabbit == o)
			return 0;
	}

	if (last_el_ptr != NULL)
		*last_el_ptr = rabbit;

	return 1;
}

/*
  "By definition, all lists have finite length and are terminated by the empty list".
  The empty list is a list (but not a pair).
*/
static inline int is_list(object o)
{
	object last_el;

	if (o == nil)
		return 1;

	if (!is_pair(o))
		return 0;

	if (is_finite_list(o, &last_el) && (last_el == nil))
		return 1;

	return 0;
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

#define BOOLEAN_TAG   0x7FULL
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

#define PRIMITIVE_PROC_TAG  0xDFUL
#define PRIMITIVE_PROC_MASK 0xFFUL

static inline int is_primitive(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & PRIMITIVE_PROC_MASK) == PRIMITIVE_PROC_TAG);
}

typedef object (*primitive_proc)(object);

extern object make_primitive(primitive_proc primitive);

static inline primitive_proc primitive_implementation(object o)
{
#if SAFETY
	if (!is_primitive(o))
		error("Object is not a primitive procedure -- PRIMITIVE-PROCEDURE", o);
#endif

	return (primitive_proc) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [1];
}

static inline object apply_primitive(object proc, object args)
{
	primitive_proc pproc;

#if SAFETY
	if (!is_primitive(proc))
		error("Object is not a primitive procedure -- APPLY-PRIMITIVE", proc);
#endif

	pproc = primitive_implementation(proc);
	return pproc(args);
}

#define PROCEDURE_TAG  0x5FUL
#define PROCEDURE_MASK 0xFFUL

static inline int is_procedure(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & PROCEDURE_MASK) == PROCEDURE_TAG);
}

extern object make_procedure(object parameters, object body, object environment);

static inline object procedure_parameters(object o)
{
#if SAFETY
	if (!is_procedure(o))
		error("Object is not a procedure -- APPLY", o);
#endif

	return (object) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [1];
}


static inline object procedure_body(object o)
{
#if SAFETY
	if (!is_procedure(o))
		error("Object is not a procedure -- APPLY", o);
#endif

	return (object) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [2];
}

static inline object procedure_environment(object o)
{
#if SAFETY
	if (!is_procedure(o))
		error("Object is not a procedure -- APPLY", o);
#endif

	return (object) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [3];
}

static inline int is_anykind_procedure(object o)
{
	return is_primitive(o) || is_procedure(o);
}

#define PORT_TAG  0x9FUL
#define PORT_MASK 0xFFUL

#define PORT_TYPE_INPUT  0UL
#define PORT_TYPE_OUTPUT 1UL
#define PORT_TYPE_MASK   0xFFUL

#define PORT_FLAGS_MASK   0xFFFF00UL
#define PORT_FLAG_CLOSED  0x000100UL

static inline int is_port(object o)
{
	unsigned long indirect;
	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & PORT_MASK) == PORT_TAG);
}

extern object make_port(FILE *in, unsigned long port_type);

static inline int is_input_port(object o)
{
#if SAFETY
	if (!is_port(o))
		error("Object is not a port -- input-port?", o);
#endif

	return PORT_TYPE_INPUT == (PORT_TYPE_MASK && ((unsigned long *) ((unsigned long) o - INDIRECT_TAG))[1]);
}

static inline int is_output_port(object o)
{
#if SAFETY
	if (!is_port(o))
		error("Object is not a port -- input-port?", o);
#endif

	return PORT_TYPE_OUTPUT == (PORT_TYPE_MASK && ((unsigned long *) ((unsigned long) o - INDIRECT_TAG))[1]);
}

/* unsafe */
static inline unsigned long get_port_flags(object o)
{
	return PORT_FLAGS_MASK && ((unsigned long *) ((unsigned long) o - INDIRECT_TAG))[1];
}

/* unsafe */
static inline void set_port_flags(object o, unsigned long flags)
{
	unsigned long *flagsptr = & ((unsigned long *) ((unsigned long) o - INDIRECT_TAG))[1];
	*flagsptr = (*flagsptr & ~PORT_FLAGS_MASK) | (flags & PORT_FLAGS_MASK);
}

/* unsafe */
static inline int is_port_closed(object o)
{
	return get_port_flags(o) & PORT_FLAG_CLOSED;
}

/* unsafe */
static inline void set_port_closed(object o)
{
	set_port_flags(o, PORT_FLAG_CLOSED | get_port_flags(o));
}

static inline FILE *port_implementation(object o)
{
#if SAFETY
	if (!is_port(o))
		error("Object is not a port -- port-implementation", o);
#endif

	return (FILE *) ((unsigned long *) ((unsigned long) o - INDIRECT_TAG)) [2];
}

#define END_OF_FILE_TAG  0x1FUL
#define END_OF_FILE_MASK 0xFFUL

static inline int is_end_of_file(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & END_OF_FILE_MASK) == END_OF_FILE_TAG);
}

#define UNSPECIFIED_VALUE_TAG  0xEFUL
#define UNSPECIFIED_VALUE_MASK 0xFFUL

static inline int is_unspecified(object o)
{
	unsigned long indirect;

	if (!is_indirect(o))
		return 0;

	indirect = *(unsigned long *) ((unsigned long) o - INDIRECT_TAG);
	return ((indirect & UNSPECIFIED_VALUE_MASK) == UNSPECIFIED_VALUE_TAG);
}

extern object_type type_of(object o);

extern void runtime_init();
extern void runtime_stats();


#endif

