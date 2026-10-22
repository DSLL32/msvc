#ifndef TEMPORARIES_H
#define TEMPORARIES_H
#include <recon.h>
/* Kind 19 descriptors are the short, non-hashable register variant. */
#pragma pack(1)
typedef struct TemporaryRegister {
    struct TemporaryRegister *next;
    unsigned number, bytes; /* number initially holds a reserved stack offset */
    signed char kind;
} TemporaryRegister;
#pragma pack()
extern TemporaryRegister *temporary_registers;
void reset_temporary_registers(void);
TemporaryRegister *__fastcall allocate_stack_temporary(unsigned bytes);
TemporaryRegister *__fastcall new_temporary_descriptor(unsigned bytes);
typedef char TemporaryRegisterSizeCheck[sizeof(TemporaryRegister) == 13 ? 1 : -1];
#endif
