#ifndef __ENVIRONMENTS_H
#define __ENVIRONMENTS_H


extern void   define_variable(object var, object val, object env);
extern object lookup_variable_value(object var, object env);
extern void   set_variable_value(object var, object val, object env);

extern object extend_environment(object vars, object vals, object base_env);

#endif
