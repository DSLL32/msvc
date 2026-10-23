/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

void show_usage(void) {
    __asm int 3

} /* 00402b8a */
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
Input *create_input(char *name, int kind) { __asm int 3
    return NULL;
}


void option_conflict(unsigned id, Option *old, Option *replacement) { __asm int 3 }



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





Option *copy_option(Option *option) { __asm int 3
    return NULL;
}
Option *unlink_option(Option **head, Option *option) { __asm int 3
    return NULL;
}






char *file_directory(char *out, char *name) { __asm int 3
    return NULL;
}
char *make_map_name(char *out, char *name) { __asm int 3
    return NULL;
}

void configure_assembly(Option *option) { __asm int 3 }

void enable_pass(Option *option) { __asm int 3 }

void toggle_pass(Option *option) { __asm int 3 }

void disable_pass(Option *option) { __asm int 3 }


void set_pass_suffix(Option *option) { __asm int 3 }

void set_pass_name(Option *option) { __asm int 3 }

void undefine_defaults(Option *option) { __asm int 3 }

void enable_dll(Option *option) { __asm int 3 }

int console_rows(void) { __asm int 3
    return 0;
}
