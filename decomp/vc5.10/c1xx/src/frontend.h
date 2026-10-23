#ifndef C1XX_FRONTEND_H
#define C1XX_FRONTEND_H
#include "c1xx.h"
#include "output.h"
#include <recon.h>

enum { FRONTEND_STATE_BYTES = 8520, FRONTEND_INITIALIZERS = 43 };

/* Only encountered fields are modeled. Unmodeled original snapshot bytes
 * have no native representation and must not reach a semantic consumer. */
typedef struct FrontendData {
    int first_invocation;
} FrontendData;

/* A linked context stack saves the first two values in each stack frame. */
typedef struct FrontendContextStack {
    void *origin;
    unsigned position;
    void *top;
} FrontendContextStack;

typedef struct FrontendSourcePosition {
    void *origin;
    unsigned position;
} FrontendSourcePosition;

/* Frame contents remain unknown until a semantic consumer is reached. */
typedef struct DeferredWorkFrame DeferredWorkFrame;

/* Encountered semantic variants only; the original 32-byte tail is unmodeled. */
typedef union TokenValue {
    __int64 integer;
    void *pointer;
} TokenValue;

typedef struct SentinelToken {
    unsigned char kind;
    TokenValue value;
} SentinelToken;

/* Genuine thiscall POD: no implicit constructors, vtable or unknown fields. */
typedef struct CompilerPool {
    unsigned char *cursor;
    unsigned remaining;
    unsigned minimum_pages;
    struct CompilerArena *arena;
#ifdef __cplusplus
    CompilerPool *RECON_THISCALL Initialize(unsigned minimum_pages);
#endif
} CompilerPool;

typedef struct FrontendBss {
    FrontendData *saved_state;
    int cleanup_count;
    Initializer cleanup_callbacks[32];
    FrontendContextStack context_stack;
    FrontendContextStack saved_context_stack;
    FrontendContextStack nested_context_stacks[9];
    FrontendSourcePosition source_position;
    OutputBuffer output_buffers[FRONTEND_OUTPUT_BUFFERS];
    DeferredWorkFrame *deferred_work;
    TokenValue token_value;
    SentinelToken sentinel_token;
    unsigned long source_signature;
    CompilerPool pools[10];
    int type_ranks_initialized;
} FrontendBss;

#ifdef __cplusplus
extern "C" {
#endif
extern FrontendData frontend_data;
extern FrontendBss frontend_bss;
extern int restore_mode;
extern Initializer frontend_initializers[FRONTEND_INITIALIZERS];
void *checked_malloc(unsigned bytes);
int report_compiler_error(int number);
int report_compiler_diagnostic(int number, ...);
int register_cleanup(Initializer cleanup);
void initialize_deferred_work(void);
void destroy_deferred_work(void);
void initialize_frontend_24(void);
void initialize_frontend_25(void);
void initialize_frontend_26(void);
void initialize_frontend_27(void);
void initialize_frontend_28(void);
void initialize_frontend_29(void);
void initialize_frontend_30(void);
void initialize_frontend_31(void);
void initialize_frontend_32(void);
void initialize_frontend_33(void);
int initialize_type_ranks(void);
#ifdef __cplusplus
}
#endif
#endif
