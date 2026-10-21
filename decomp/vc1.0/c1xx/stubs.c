/* Named, unaccepted reconstruction frontiers. */
#include "options.h"
void interrupt_compile(int signal_number) { __asm { int 3 } }
int guarded_compile(void) { __asm { int 3 } return 0; }
int compile_translation_unit(void) { __asm { int 3 } return 0; }
Vector *__stdcall new_vector(int capacity, int arena) { __asm { int 3 } return 0; }
int match_option(OptionRule *rules, char *argument, char *(*next)(void), int copy) { __asm { int 3 } return 0; }
char *next_argument(void) { __asm { int 3 } return 0; }
void initialize_japanese_characters(void) { __asm { int 3 } }
void finalize_options(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }
void clear_packing(PackingEntry *entry) { __asm { int 3 } }
