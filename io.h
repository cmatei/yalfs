#ifndef __IO_H
#define __IO_H

FILE *open_file(char *filename, char *mode);
int   close_file(FILE *file);

/* Input */
extern object io_read(object port);
extern object io_read_char(object port);
extern object io_peek_char(object port);

/* Output */
extern void   io_write(object obj, object port);
extern void   io_display(object obj, object port);
extern void   io_newline(object port);
extern void   io_write_char(object chr, object port);

#endif
