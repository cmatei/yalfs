#ifndef __EMACS_H
#define __EMACS_H

extern void emacs_prompt_for_command_expression(object port);

extern void emacs_error_decision(object port, object message);

extern void emacs_set_default_directory(object port);

extern void emacs_read_start(object port);
extern void emacs_read_finish(object port);

extern void emacs_write_result(object val, object port);

extern void emacs_ctrl_g_interrupt(object port);

extern void emacs_init();

#endif
