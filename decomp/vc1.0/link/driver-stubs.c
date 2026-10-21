/* Link-driver operations that the recorded cases have not yet entered. */
#include "image.h"
int run_cvpack(ImageOptions *options) { __asm { int 3 } return 0; }
void close_all_files(void) { __asm { int 3 } }
void free_argument_names(void) { __asm { int 3 } }
void close_trace_file(void) { __asm { int 3 } }
void write_map(int kind,Image *image,const char *name) { __asm { int 3 } }
