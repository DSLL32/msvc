#ifndef WPP_CONTEXT_H
#define WPP_CONTEXT_H
#include "compiler.h"
#include <recon.h>
typedef struct nested_context NESTED_POST_CONTEXT;
struct nested_context {
    NESTED_POST_CONTEXT *next;
    void (*call_back)(NESTED_POST_CONTEXT *);
};
struct symbol;
void CtxFunction(struct symbol *symbol);
void CtxRegisterPostContext(NESTED_POST_CONTEXT *block);
#endif
