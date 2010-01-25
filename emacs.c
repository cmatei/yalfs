#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#include "minime.h"

/* this is modeled after mit-scheme-c/src/runtime/emacs.scm */

#define ESC 0x1B

static object emacs_eval_prompt;
static object escape;

static void transmit_modeline(object port, object prompt, int level)
{
	io_write_char(escape, port);
	io_write_char(make_character('p'), port);

	/* this should be prompt level */
	io_write_char(make_character('1'), port);
	io_write_char(make_character(' '), port);

	io_display(emacs_eval_prompt, port);
	io_write_char(escape, port);
}

void emacs_prompt_for_command_expression(object port)
{
	transmit_modeline(port, emacs_eval_prompt, 1);

	io_write_char(escape, port);
	io_write_char(make_character('R'), port);
}

/* this is where we might be entering a debugger */
void emacs_error_decision(object port, object message)
{
	io_write_char(escape, port);
	io_write_char(make_character('z'), port);
}

void emacs_set_default_directory(object port)
{
	char *cwd = get_current_dir_name();

	io_write_char(escape, port);
	io_write_char(make_character('w'), port);

	io_display(make_string_c(cwd, strlen(cwd)), port);

	io_write_char(escape, port);

	xfree(cwd);
}

void emacs_read_start(object port)
{
	io_write_char(escape, port);
	io_write_char(make_character('s'), port);
}

void emacs_read_finish(object port)
{
	io_write_char(escape, port);
	io_write_char(make_character('f'), port);
}



void emacs_write_result(object val, object port)
{
	io_write_char(escape, port);
	io_write_char(make_character('v'), port);

	io_write(val, port);

	io_write_char(escape, port);
}


void emacs_init()
{
	emacs_eval_prompt  = make_string_c("[Evaluator]", strlen("[Evaluator]"));

	escape = make_character(ESC);
}
