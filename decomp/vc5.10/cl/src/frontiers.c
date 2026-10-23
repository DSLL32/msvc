/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

void show_usage(void) {
    __asm int 3

} /* 00402b8a */
void warning(unsigned id, ...) {
    __asm int 3

} /* 004053ba */
void fatal(unsigned id, ...) {
    __asm int 3

} /* 00405361 */
unsigned file_timestamp(Driver *state) { __asm int 3
    return 0;
}
Job *jobs_from_deferred(DeferredInput *inputs) { __asm int 3
    return NULL;
}
Job *next_jobs(Driver *state, PendingWork *pending) { __asm int 3
    return NULL;
}
Job *advance_jobs(Job *jobs) { __asm int 3
    return NULL;
}
void delete_temporary(int kind) { __asm int 3 }
int link_pass(Driver *state) { __asm int 3
    return 0;
}
void start_timer(void) { __asm int 3 }
void stop_timer(char *name) { __asm int 3 }
void driver_exit(int status) {
    __asm int 3

} /* 00404fda */

BOOL WINAPI console_control_handler(DWORD event)
{
    __asm int 3
    return FALSE;
}







void read_response(char *name) { __asm int 3 }


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






char *file_directory(char *out, char *name) { __asm int 3
    return NULL;
}
char *make_map_name(char *out, char *name) { __asm int 3
    return NULL;
}

void configure_assembly(Option *option) { __asm int 3 }

void enable_pass(Option *option) { __asm int 3 }

void toggle_pass(Option *option) { __asm int 3 }



void set_pass_suffix(Option *option) { __asm int 3 }

void set_pass_name(Option *option) { __asm int 3 }

void undefine_defaults(Option *option) { __asm int 3 }

void enable_dll(Option *option) { __asm int 3 }

int console_rows(void) { __asm int 3
    return 0;
}
