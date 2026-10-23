/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

void show_usage(void) {
    __asm int 3

} /* 00402b8a */
void finish_options(void) {
    __asm int 3

} /* 00402933 */
void prepare_inputs(Driver *state) {
    __asm int 3

} /* 00402849 */
void prepare_environment(Driver *state) {
    __asm int 3

} /* 00402319 */
void show_help(void) {
    __asm int 3

} /* 004052a5 */
char * append_text(char *out, const char *text) {
    __asm int 3
    return NULL;
} /* 00404a80 */
void warning(unsigned id, ...) {
    __asm int 3

} /* 004053ba */
void fatal(unsigned id, ...) {
    __asm int 3

} /* 00405361 */
void prepare_execution(void) {
    __asm int 3

} /* 00405800 */
void run_driver(Driver *state) {
    __asm int 3

} /* 0040332c */
void driver_exit(int status) {
    __asm int 3

} /* 00404fda */

BOOL WINAPI console_control_handler(DWORD event)
{
    __asm int 3
    return FALSE;
}







void read_response(char *name) { __asm int 3 }
int classify_input(char *name) { __asm int 3
    return 0;
}
void add_input(char *name, int kind, int explicit_kind) { __asm int 3 }


void option_conflict(unsigned id, Option *old, Option *replacement) { __asm int 3 }


Option *define_macro(char **args, int *index) { __asm int 3
    return NULL;
}

Option *link_arguments(char **args, int *index) { __asm int 3
    return NULL;
}

Option *c_input(char **args, int *index) { __asm int 3
    return NULL;
}

Option *cpp_input(char **args, int *index) { __asm int 3
    return NULL;
}

Option *object_input(char **args, int *index) { __asm int 3
    return NULL;
}





char *append_escaped(char *out, char *text) { __asm int 3
    return NULL;
}


Option *copy_option(Option *option) { __asm int 3
    return NULL;
}
Option *unlink_option(Option **head, Option *option) { __asm int 3
    return NULL;
}


char *find_last_separator(char *text, char *separators) { __asm int 3
    return NULL;
}
