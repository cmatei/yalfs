#ifndef __SYMBOLS_H
#define __SYMBOLS_H

extern object symbol(char *str, unsigned long len);

extern object gensym();

extern void symbol_table_init();
extern void symbol_table_stats();

#endif
