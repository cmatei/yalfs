/* symbols.c -- Symbol hashtable. Temporary solution */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"

#define SYMBOL_TABLE_BUCKETS 4099

static struct {
	unsigned long nbuckets;
	object *buckets;
} symbol_table;

/* djb hash */
static unsigned long symbol_string_hash(char *str, unsigned long len)
{
	unsigned long hash = 5381;
	int c;

	while (len--) {
		c = *str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

object symbol(char *str, unsigned long len)
{
	unsigned long bucket;
	object el, o;
	char *symstr;
	unsigned long symlen;

	bucket = symbol_string_hash(str, len) % symbol_table.nbuckets;
	el = symbol_table.buckets[bucket];

	while (!is_null(el)) {
		o = symbol_string(car(el));

		symstr = string_value(o);
		symlen = string_length(o);

		if ((len == symlen) && (memcmp(symstr, str, len) == 0))
			return car(el);

		el = cdr(el);
	}

	/* not there, intern now */
	o = make_string_c(str, len);
	el = make_symbol_with_string(o);
	symbol_table.buckets[bucket] = cons(el, symbol_table.buckets[bucket]);


	return el;
}


void symbol_table_init()
{
	unsigned long i;

	symbol_table.nbuckets = SYMBOL_TABLE_BUCKETS;
	symbol_table.buckets  = xmalloc(SYMBOL_TABLE_BUCKETS * sizeof(object));

	for (i = 0; i < SYMBOL_TABLE_BUCKETS; i++)
		symbol_table.buckets[i] = nil;
}

void symbol_table_stats()
{
	unsigned long i;
	unsigned long nel, total = 0;

	fprintf(stderr, "Symbol table: %lu buckets\n", symbol_table.nbuckets);

	for (i = 0; i < symbol_table.nbuckets; i++) {
		nel = length(symbol_table.buckets[i]);
		if (nel > 1) {
			fprintf(stderr, "    bucket %lu: %lu entries ", i, nel);
			lisp_print(stderr, symbol_table.buckets[i]);
			fprintf(stderr, "\n");
		}

		total += nel;
	}

	fprintf(stderr, "Number of symbols: %lu\n", total);
}
