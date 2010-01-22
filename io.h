#ifndef __IO_H
#define __IO_H


FILE *open_input_file(char *filename);
FILE *open_output_file(char *filename);

int peek_char(FILE *in);
int read_char(FILE *in);

int write_char(FILE *out);

int close_file(FILE *file);



#endif
