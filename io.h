#ifndef __IO_H
#define __IO_H

/* Input */
extern object io_read(object port);
extern object io_read_char(object port);
extern object io_peek_char(object port);

/* Output */
extern void   io_write(object obj, object port);
extern void   io_display(object obj, object port);
extern void   io_newline(object port);
extern void   io_write_char(object chr, object port);

/* System interface */
extern object io_file_as_port(object filename, unsigned long port_type);
extern void   io_close_port(object port);
extern object io_load(object filename, object env);

#endif
