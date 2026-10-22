#ifndef REGISTER_VARIABLES_H
#define REGISTER_VARIABLES_H
/* The record's fields are recovered when its constructor is reached. */
typedef struct RegisterVariable RegisterVariable;
enum { REGISTER_VARIABLE_CLASSES = 3, REGISTER_VARIABLE_BUCKETS = 32 };
extern RegisterVariable *register_variable_buckets[REGISTER_VARIABLE_CLASSES][REGISTER_VARIABLE_BUCKETS];
void reset_global_register_map(void);
#endif
