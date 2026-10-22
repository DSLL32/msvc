#ifndef BACKEND_FLOW_H
#define BACKEND_FLOW_H
#include "functions.h"
typedef struct MachineInstruction MachineInstruction;
/* The function context is the kind-2 machine record marking its symbol. */
typedef struct MachineInstruction MachineContext;
typedef struct BackendSymbolList BackendSymbolList;
extern MachineInstruction *machine_instructions;
extern MachineContext *machine_context;
extern char *listing_source_filename;
extern unsigned frame_size, saved_register_mask;
#endif
