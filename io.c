#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"

#define STRING_MIN_BUFFER         128
#define STRING_REALLOC_INCREMENT 1024

static int peek(FILE *in)
{
	int c = fgetc(in);
	ungetc(c, in);
	return c;
}

static void skip_whitespace(FILE *in)
{
	int c;
	while (isspace(c = fgetc(in)));
	ungetc(c, in);
}

static int is_delimiter(int c)
{
	return isspace(c) || c == EOF ||
		c == '('  || c == ')' ||
		c == '"'  || c == ';';
}

static int is_special_initial(int c)
{
	return strchr("!$%&*/:<=>?^_~", c) ? 1 : 0;
}

static int is_initial(int c)
{
	return isalpha(c) || is_special_initial(c);
}

static int is_special_subsequent(int c)
{
	return strchr("+-.@", c) ? 1 : 0;
}

static int is_subsequent(int c)
{
	return  is_initial(c) || isdigit(c) ||
		is_special_subsequent(c);
}

/* this doesn't read the peculiar identifiers, they are scanned in the
 * main reader body */
static object read_identifier(FILE *in)
{
	object o;
	int str_len = 0;
	int str_size = STRING_MIN_BUFFER;
	char *buffer;
	int c;

	buffer = xmalloc(str_size);

	c = tolower(fgetc(in));
	assert(is_initial(c));

	buffer[str_len++] = c;

	while (1) {
		c = fgetc(in);

		if (is_delimiter(c)) {
			ungetc(c, in);
			break;
		}

		if (!is_subsequent(c))
			error("Symbol has bad name -- READ", nil);

		/* we're a lower case scheme */
		buffer[str_len++] = tolower(c);
	}

	o = make_symbol(buffer, str_len);
	xfree(buffer);

	return o;
}

static void peek_expect_delimiter(FILE *in)
{
	if (!is_delimiter(peek(in)))
		error("Expecting delimiter -- READ", nil);
}

static void expect_string(FILE *in, char *str)
{
	int c;

	while (*str) {
		c = fgetc(in);
		if (tolower(c) != *str)
			error("Unexpected character -- READ", nil);

		str++;
	}
}

static object read_character(FILE *in)
{
	int c = fgetc(in);

	switch (c) {
	case EOF:
		error("Unexpected EOF -- READ", nil);
		break;

	case 's':
	case 'S':
		if (tolower(peek(in)) == 'p') {
			expect_string(in, "pace");
			peek_expect_delimiter(in);
			return make_character(' ');
		}
	        break;

	case 'n':
	case 'N':
		if (tolower(peek(in)) == 'e') {
			expect_string(in, "ewline");
			peek_expect_delimiter(in);
			return make_character('\n');
		}
	        break;
	}

	peek_expect_delimiter(in);
	return make_character(c);
}

static int is_digit_for_base(int c, int base)
{
	switch (base) {
	case 2:
		return (c == '0' || c == '1');
	case 8:
		return (c >= '0' && c <= '7');
	case 10:
		return isdigit(c);
	case 16:
		return isxdigit(c);
	}
	return 0;
}

static object read_string(FILE *in)
{
	object o;
	int str_len = 0;
	int str_size = STRING_MIN_BUFFER;
	char *buffer;
	int c, nextc;

	buffer = xmalloc(str_size);

	while (1) {
		c = fgetc(in);

		if (c == '"' || c == EOF)
			break;

		if (str_len == str_size) {
			str_size += STRING_REALLOC_INCREMENT;
			buffer = xrealloc(buffer, str_size);
		}

		if (c == '\\') {
			nextc = peek(in);

			if (nextc == '\\' || nextc == '"') {
				buffer[str_len++] = nextc;
				c = fgetc(in);
			}
			/* r5rs doesn't say what to do with the others */
			else if (nextc == 'n') {
				buffer[str_len++] = '\n';
				c = fgetc(in);
			}
			else {
				/* copy the '\\' */
				buffer[str_len++] = '\\';
			}
		}
		else {
			buffer[str_len++] = c;
		}
	}

	peek_expect_delimiter(in);
	o = make_string_c(buffer, str_len);
	xfree(buffer);

	return o;
}

static object read_number(FILE *in)
{
	int base = 10, exact = 1, sign = 1;
	int c;

	int at_prefix = 1;
	int radix_was_set = 0;
	int exactness_was_set = 0;

	int sign_was_set = 0;
	int digits_were_seen = 0;

	long number = 0;

	while (1) {
		c = tolower(fgetc(in));

		if (at_prefix && strchr("bodx", c)) {
			if (radix_was_set)
				error("Ill-formed number -- READ", nil);

			base  = (c == 'b') ? 2 :
				(c == 'o') ? 8 :
				(c == 'x') ? 16 : 10;

			radix_was_set = 1;

			if (peek(in) == '#')
				c = fgetc(in);
			else
				at_prefix = 0;

			continue;
		}

		if (at_prefix && (c == 'e' || c == 'i')) {
			if (exactness_was_set)
				error("Ill-formed number -- READ", nil);

			exact = (c == 'e') ? 1 : 0;
			exactness_was_set = 1;
			if (peek(in) == '#')
				c = fgetc(in);
			else
				at_prefix = 0;

			continue;
		}

		at_prefix = 0;

		if (c == '+' || c == '-') {
			if (sign_was_set)
				error("Ill-formed number -- READ", nil);

			sign = (c == '+') ? 1 : -1;
			sign_was_set = 1;

			continue;
		}

		if (is_digit_for_base(c, base)) {
			number = number * base + ((c >= 'a') ? c - 'a' + 10 : c - '0');
			digits_were_seen = 1;
			continue;
		}

		if (digits_were_seen && is_delimiter(c)) {
			ungetc(c, in);
			return make_fixnum(number);
		}

		error("Ill-formed number -- READ", nil);
	}
}

object read_pair(FILE *in)
{
	object the_car, the_cdr;
	int c;

	skip_whitespace(in);

	/* the empty list */
	c = fgetc(in);
	if (c == ')')
		return nil;
	ungetc(c, in);

	the_car = lisp_read(in);

	skip_whitespace(in);

	c = fgetc(in);
	/* improper list */
	if (c == '.') {
		c = fgetc(in);
		if (!isspace(c))
			error("Missing delimiter in improper list -- READ", nil);

		the_cdr = lisp_read(in);
		skip_whitespace(in);

		c = fgetc(in);
		if (c == ')')
			return cons(the_car, the_cdr);

		error("Missing parenthesis -- READ", nil);
	}
	else {
		ungetc(c, in);
		the_cdr = read_pair(in);
		return cons(the_car, the_cdr);
	}

	/* notreached */
	assert(0);
	return nil;
}

object lisp_read(FILE *in)
{
	int c;

	while ((c = fgetc(in)) != EOF) {
		/* atmosphere */
		if (isspace(c)) {
			continue;
		}
		else if (c == ';') {
			while (c != EOF && c != '\n')
				c = fgetc(in);
			continue;
		}
		/* characters, booleans or numbers with radix */
		else if (c == '#') {
			c = fgetc(in);

			switch (c) {
			/* number prefixes */
			case 'b':
			case 'B':
			case 'o':
			case 'O':
			case 'x':
			case 'X':
			case 'd':
			case 'D':
			case 'e':
			case 'i':
				ungetc(c, in);
				return read_number(in);

			/* booleans */
			case 't':
			case 'T':
				return the_truth;
			case 'f':
			case 'F':
				return the_falsity;

			/* characters */
			case '\\':
				return read_character(in);

			/* commented form, read and discard */
			case ';':
				lisp_read(in);
				continue;

			case '<':
				error("Object cannot be read back -- READ", nil);

			default:
				error("Unexpected character -- READ", nil);
			}
		}
		/* number */
		else if (isdigit(c) ||
			 ((c == '-') && isdigit(peek(in))) ||
			 ((c == '+') && isdigit(peek(in)))) {
			ungetc(c, in);
			return read_number(in);
		}
		/* string */
		else if (c == '"') {
			return read_string(in);
		}
		/* symbol */
		else if (is_initial(c)) {
			ungetc(c, in);
			return read_identifier(in);
		}
		/* peculiar identifier. FIXME for ... */
		else if (((c == '+') || c == '-') && is_delimiter(peek(in))) {
			return make_symbol_c((c == '+' ? "+" : "-"));
		}
		/* pair */
		else if (c == '(') {
			return read_pair(in);
		}
		/* quote */
		else if (c == '\'') {
			return cons(_quote, cons(lisp_read(in), nil));
		}

		else
			error("Unexpected character -- READ", nil);
	}

	return 0;
}




static void lisp_print_pair(FILE *out, object pair)
{
	object car_obj, cdr_obj;

	car_obj = car(pair);
	cdr_obj = cdr(pair);

	lisp_print(out, car_obj);

	if (is_pair(cdr_obj)) {
		fprintf(out, " ");
		lisp_print_pair(out, cdr_obj);
	} else if (is_null(cdr_obj)) {
		return;
	} else {
		fprintf(out, " . ");
		lisp_print(out, cdr_obj);
	}
}

void lisp_print(FILE *out, object exp)
{
	unsigned long i, len;
	char c;
	char *str;

	switch (type_of(exp)) {
	case T_NIL:
		fprintf(out, "()");
		break;

	case T_FIXNUM:
		fprintf(out, "%ld", fixnum_value(exp));
		break;

	case T_CHARACTER:
		c = character_value(exp);
		fprintf(out, "#\\");
		switch (c) {
		case '\n':
			fprintf(out, "newline");
			break;
		case ' ':
			fprintf(out, "space");
			break;
		default:
			fprintf(out, "%c", c);
		}
		break;

	case T_PAIR:
		if (is_finite_list(exp, NULL)) {
			fprintf(out, "(");
			lisp_print_pair(out, exp);
			fprintf(out, ")");
		} else {
			fprintf(out, "#<unprintable-structure>");
		}
		break;

	case T_BOOLEAN:
		fprintf(out, is_false(exp) ? "#f" : "#t");
		break;

	case T_STRING:
		fprintf(out, "\"");
		str = string_value(exp);
		len = string_length(exp);
		for (i = 0; i < len; i++) {
			switch (str[i]) {
			case '\n':
				fprintf(out, "\\n");
				break;
			case '"':
				fprintf(out, "\\\"");
				break;
			case '\\':
				fprintf(out, "\\\\");
				break;
			default:
				fprintf(out, "%c", str[i]);
			}
		}
		fprintf(out, "\"");
		break;

	case T_SYMBOL:
		str = string_value(symbol_string(exp));
		len = string_length(symbol_string(exp));
		for (i = 0; i < len; i++)
			fprintf(out, "%c", str[i]);
		break;

	case T_FOREIGN_PTR:
		fprintf(out, "#<foreign-pointer %p>", foreign_ptr_value(exp));
		break;

	case T_PRIMITIVE:
		fprintf(out, "#<primitive-procedure %p>", primitive_implementation(exp));
		break;

	case T_PROCEDURE:
		fprintf(out, "#<procedure ");
		lisp_print(out, procedure_parameters(exp));
		fprintf(out, ">");
		break;
	}
}
