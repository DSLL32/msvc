#ifndef C1XX_FRONTEND_H
#define C1XX_FRONTEND_H
#include "c1xx.h"
#include <recon.h>

enum { FRONTEND_STATE_BYTES = 8520, FRONTEND_INITIALIZERS = 43 };

/* Only encountered fields are modeled. Unmodeled original snapshot bytes
 * have no native representation and must not reach a semantic consumer. */
typedef struct FrontendData {
    int first_invocation;
} FrontendData;

/* A linked context stack saves the first two values in each stack frame. */
typedef struct FrontendContextStack {
    void *begin;
    void *end;
    void *top;
} FrontendContextStack;

typedef struct FrontendBss {
    FrontendData *saved_state;
    int cleanup_count;
    Initializer cleanup_callbacks[32];
    FrontendContextStack context_stack;
} FrontendBss;

extern FrontendData frontend_data;
extern FrontendBss frontend_bss;
extern int restore_mode;
extern Initializer frontend_initializers[FRONTEND_INITIALIZERS];
void *checked_malloc(unsigned bytes);
int report_compiler_error(int number);
#endif
