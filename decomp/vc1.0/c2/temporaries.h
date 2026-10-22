#ifndef TEMPORARIES_H
#define TEMPORARIES_H
/* Kind 19 descriptors are the short, non-hashable register variant. */
#pragma pack(1)
typedef struct TemporaryRegister {
    struct TemporaryRegister *next;
    unsigned number, bytes;
    signed char kind;
} TemporaryRegister;
#pragma pack()
extern TemporaryRegister *temporary_registers;
void reset_temporary_registers(void);
#endif
