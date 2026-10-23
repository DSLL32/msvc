#ifndef C1_COMPILER_STATE_H
#define C1_COMPILER_STATE_H
#include <stdio.h>
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
typedef void (__cdecl *C1BufferRelocator)(void *, unsigned char *, unsigned char *);
typedef struct CompilerOutputBuffer {
    unsigned char *data;
    unsigned char *cursor;
    unsigned capacity;
    C1BufferRelocator relocated;
    void *relocation_context;
    FILE *stream;
    int good;
    unsigned char mode;
    int file_descriptor;
} CompilerOutputBuffer;
/* Recovered numeric alternative only. Other original value-record members
 * remain UNMODELED and must be recovered before any semantic consumer. */
typedef struct CompilerValue {
    __int64 integer;
} CompilerValue;
typedef struct CompilerBss {
    CompilerInitializedState *saved_data;
    int cleanup_count;
    C1Initializer cleanup_callbacks[32];
    CompilerContextState context;
    CompilerContextState current_context;
    CompilerContextState contexts[6];
    CompilerOutputBuffer output_buffers[8];
    CompilerValue literal;
    CompilerValue saved_literal;
    unsigned char saved_literal_available;
    unsigned long pdb_signature;
} CompilerBss;
/* These session fields are outside the datafe snapshot and BSS reset.
 * Original offset0x10 is unmodeled; no provider object is dereferenced here. */
typedef struct CompilerPdbContext {
    struct PDB *database;
    struct NameMap *names;
    struct TPI *types;
    struct MREngine *minimal_rebuild;
    struct CompilerMreState *rebuild_state;
    int read_mode;
} CompilerPdbContext;
extern CompilerPdbContext c1_pdb_context;
extern CompilerInitializedState c1_data;
extern CompilerBss c1_bss;
extern int c1_reuse_state;
extern C1Initializer const c1_initialization_callbacks[34];
void *__cdecl allocate_compiler_memory(unsigned);
void __cdecl compiler_fatal(int);
int __cdecl compiler_error(int, ...);
int __cdecl initialize_output_buffer_123ae(void);
int __cdecl initialize_output_buffer_12423(void);
void __cdecl destroy_output_buffer_23939(void);
void __cdecl destroy_output_buffer_23940(void);
int __cdecl register_compiler_cleanup(C1Initializer);
int __cdecl initialize_pdb_context_12676(void);
void __cdecl destroy_pdb_context_238dc(void);
int __cdecl initialize_output_buffer_125fd(void);
void __cdecl destroy_output_buffer_238dd(void);
int __cdecl initialize_output_buffer_125ae(void);
void __cdecl destroy_output_buffer_23916(void);
int __cdecl initialize_output_buffer_1255f(void);
void __cdecl destroy_output_buffer_2391d(void);
int __cdecl initialize_output_buffer_12510(void);
void __cdecl destroy_output_buffer_23924(void);
int __cdecl initialize_output_buffer_124c1(void);
void __cdecl destroy_output_buffer_2392b(void);
int __cdecl initialize_output_buffer_12472(void);
void __cdecl destroy_output_buffer_23932(void);
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
