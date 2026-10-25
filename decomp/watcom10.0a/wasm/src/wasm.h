#ifndef WASM_H
#define WASM_H
#include <stdio.h>
#include <recon.h>

/* Only the encountered nine-byte prefix is defined. The released source
 * supplies these names; expand the structure as additional fields are used. */
typedef struct global_options {
    unsigned char sign_value;
    unsigned char stop_at_end;
    unsigned char quiet;
    unsigned char banner_printed;
    unsigned char debug_flag;
    unsigned char naming_convention;
    unsigned char floating_point;
    unsigned char output_data_in_code_records;
    /* The target zero-extends this byte even for values above 127. */
    unsigned char error_count;
} global_options;

enum { ASM, ERR, OBJ, FILE_TYPES };
typedef struct File_Info {
    FILE *file[FILE_TYPES];
    char *fname[FILE_TYPES];
} File_Info;

extern File_Info AsmFiles;
extern global_options Options;
void ObjRecInit(void);
void GenMSOmfInit(void);
int main(int argc, char **argv);
void main_init(void);
void do_init_stuff(int argc, char **argv);
void WriteObjModule(void);
void PrintStats(void);
void MsgFini(void);
void main_fini(void);

#if defined(__WATCOMC__)
/* Watcom 10.0a intrinsic emits the required pending-entry trap inline. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#define WASM_PENDING() reconstruction_trap()
#else
/* Assembly is used only for an unreconstructed function's INT3. */
#define WASM_PENDING() __asm__ __volatile__("int $3")
#endif
#endif
