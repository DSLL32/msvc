/* Object operations not yet entered by an accepted frontier. */
#include "object.h"
#include "comdat.h"
#include <recon.h>


void format_input_reference(char *display,const char *library,const char *file) { __asm { int 3 } }
void expand_hash(HashTable *table,void *context) { __asm { int 3 } }
void format_undefined_references(TextBuffer *buffer,External *external) { __asm { int 3 } }
