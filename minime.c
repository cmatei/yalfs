#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <assert.h>

#include "runtime.h"
#include "xutil.h"


static void check_sanity();

int main(int argc, char **argv)
{
	runtime_init();

	check_sanity();

	return 0;
}




static void check_sanity()
{
	assert(is_fixnum(make_fixnum(1234)));
	assert(is_fixnum(make_fixnum(-32768)));

	assert(!is_fixnum((object) 1));

	assert(is_character(make_character('c')));

	assert(!is_character(make_fixnum(10)));

	assert(is_pair(cons(make_fixnum(10),
			    make_fixnum(20))));

	assert(!is_pair(make_fixnum(1)));
	assert(!is_pair(make_character('f')));
}
