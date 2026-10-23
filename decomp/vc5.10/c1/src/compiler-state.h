#ifndef C1_COMPILER_STATE_H
#define C1_COMPILER_STATE_H
#include "c1.h"
#include <recon.h>

/* Only recovered semantic fields live here. Other historical bytes are
 * UNMODELED; neither zero defaults nor byte equivalence are assumed. */
typedef struct CompilerInitializedState {
    int first_call;
} CompilerInitializedState;
/* A context saves its current object/value together with a linked stack of
 * prior contexts. The domain of the value is not yet recovered. */
struct CompilerContextFrame;
typedef struct CompilerContextState {
    void *object;
    unsigned value;
    struct CompilerContextFrame *saved_frame;
} CompilerContextState;
typedef struct CompilerBss {
    CompilerInitializedState *saved_data;
    int cleanup_count;
    C1Initializer cleanup_callbacks[32];
    CompilerContextState context;
} CompilerBss;
extern CompilerInitializedState c1_data;
extern CompilerBss c1_bss;
extern int c1_reuse_state;
extern C1Initializer const c1_initialization_callbacks[34];
void *__cdecl allocate_compiler_memory(unsigned);
void __cdecl compiler_fatal(int);
void __cdecl initialize_state_12365(void);
void __cdecl initialize_state_12377(void);
void __cdecl initialize_state_12389(void);
void __cdecl initialize_state_123a4(void);
void __cdecl initialize_state_123a5(void);
void __cdecl initialize_state_123a6(void);
void __cdecl initialize_state_123a7(void);
void __cdecl initialize_state_1241c(void);
void __cdecl initialize_state_1246b(void);
void __cdecl initialize_state_124ba(void);
void __cdecl initialize_state_12509(void);
void __cdecl initialize_state_12558(void);
void __cdecl initialize_state_125a7(void);
void __cdecl initialize_state_125f6(void);
void __cdecl initialize_state_12645(void);
void __cdecl initialize_state_12646(void);
void __cdecl initialize_state_12663(void);
void __cdecl initialize_state_12664(void);
void __cdecl initialize_state_1266f(void);
void __cdecl initialize_state_126af(void);
void __cdecl initialize_state_126ef(void);
void __cdecl initialize_state_12712(void);
void __cdecl initialize_state_1271f(void);
void __cdecl initialize_state_1272c(void);
void __cdecl initialize_state_12739(void);
void __cdecl initialize_state_12746(void);
void __cdecl initialize_state_12753(void);
void __cdecl initialize_state_12760(void);
void __cdecl initialize_state_1276d(void);
void __cdecl initialize_state_1277a(void);
void __cdecl initialize_state_1277b(void);
void __cdecl initialize_state_1279c(void);
void __cdecl initialize_state_127a6(void);
#endif
