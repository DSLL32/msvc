/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

char *option_signature(Driver *state) { __asm int 3
    return NULL;
}
int compare_option_strings(const void *left, const void *right) { __asm int 3
    return 0;
}
int option_matching_selector(Option *option, const char *selectors) { __asm int 3
    return 0;
}
Job *jobs_from_deferred(DeferredInput *inputs) { __asm int 3
    return NULL;
}
DeferredInput *deferred_from_jobs(Job *jobs) { __asm int 3
    return NULL;
}


BOOL WINAPI console_control_handler(DWORD event)
{
    __asm int 3
    return FALSE;
}
































void toggle_pass(Option *option) { __asm int 3 }










int console_rows(void) { __asm int 3
    return 0;
}
