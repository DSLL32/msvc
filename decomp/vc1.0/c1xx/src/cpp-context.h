#ifndef CPP_CONTEXT_H
#define CPP_CONTEXT_H
#include "inline-vector.h"
#include <recon.h>
extern InlineVector *cpp_context_stack;
void initialize_cpp_context_stack(void);
struct CppSymbol;
struct CppSymbol *__stdcall next_member_context(struct CppSymbol *context);
#endif
