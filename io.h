#ifndef __IO_H
#define __IO_H

FILE *open_file(char *filename, char *mode);
int   close_file(FILE *file);

int peek_char(FILE *in);
int read_char(FILE *in);

int write_char(int c, FILE *out);

#endif
