/* Link-driver operations that the recorded cases have not yet entered. */
#include "image.h"
void close_trace_file(void) { __asm { int 3 } }
void write_map(int kind,Image *image,const char *name) { __asm { int 3 } }
