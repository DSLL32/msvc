#ifndef PCH_RELOCATION_H
#define PCH_RELOCATION_H
#include "pch-format.h"
#include "options.h"
#include <recon.h>
extern unsigned * const pch_root_cells[149];
extern unsigned pch_encoded_pointer, pch_relocation_count;
typedef struct PchRelocationChain {
    unsigned head;
    unsigned short page, reserved;
} PchRelocationChain;
typedef char PchRelocationChainSizeMustBe8[(sizeof(PchRelocationChain) == 8) ? 1 : -1];
extern PchRelocationChain *pch_relocation_chains;
extern int pch_relocation_capacity;
Vector *__stdcall encode_pch_vector_values(Vector *vector);
Vector *__stdcall decode_pch_vector_values(Vector *vector);
#endif
