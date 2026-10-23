/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

unsigned file_timestamp(Driver *state) { __asm int 3
    return 0;
}
Job *jobs_from_deferred(DeferredInput *inputs) { __asm int 3
    return NULL;
}
DeferredInput *deferred_from_jobs(Job *jobs) { __asm int 3
    return NULL;
}
Job *advance_jobs(Job *jobs) { __asm int 3
    return NULL;
}

void start_timer(void) { __asm int 3 }
void stop_timer(char *name) { __asm int 3 }

BOOL WINAPI console_control_handler(DWORD event)
{
    __asm int 3
    return FALSE;
}




























char *file_directory(char *out, char *name) { __asm int 3
    return NULL;
}
char *make_map_name(char *out, char *name) { __asm int 3
    return NULL;
}




void toggle_pass(Option *option) { __asm int 3 }










int console_rows(void) { __asm int 3
    return 0;
}
