/* Unreconstructed function entries. Each body deliberately traps. */
#include "options.h"

/* Original 0x43ea0. */
void compiler_signal(int signal_number) { __asm { int 3 } }
/* Original 0x438d0. */
/* Original 0x43950. */
int compile_translation_unit(void) { __asm { int 3 } return 0; }

void initialize_japanese_characters(void) { __asm { int 3 } }
void fatal_error(unsigned code, ...) { __asm { int 3 } }





void clear_packing(PackingEntry *entry) { __asm { int 3 } }







void __stdcall grow_vector(Vector *vector) { __asm { int 3 } }

void compiler_warning(unsigned category, unsigned code, ...) { __asm { int 3 } }
