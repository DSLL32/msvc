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
typedef struct carve_block {
    struct carve_block *next;
    char data[1];
} carve_block;
typedef struct carve_control {
    size_t elm_size, blk_size, blk_top;
    char *top_elm;
    carve_block *blk_list;
    void **free_list;
} cv_t;
typedef void *carve_t;
void *MemAlloc(size_t size);
void *AsmAlloc(size_t size);
void Fatal(unsigned message, ...);
void newBlk(cv_t *cv);
carve_t CarveCreate(size_t elm_size, size_t blk_size);
void ObjRecInit(void);
typedef struct obj_rec obj_rec;
typedef struct pobj_state pobj_state;
typedef int (*pobj_filter)(obj_rec *objr, pobj_state *state);
#pragma pack(1)
typedef struct pobj_list {
    unsigned char command, pass;
    pobj_filter func;
} pobj_list;
#pragma pack()
int writeTheadr(obj_rec *objr, pobj_state *state);
int writeComent(obj_rec *objr, pobj_state *state);
int writeModend(obj_rec *objr, pobj_state *state);
int writeMisc(obj_rec *objr, pobj_state *state);
int writeSegdef(obj_rec *objr, pobj_state *state);
int writeLedata(obj_rec *objr, pobj_state *state);
int writeFixup(obj_rec *objr, pobj_state *state);
int writePubdef(obj_rec *objr, pobj_state *state);
int writeLinnum(obj_rec *objr, pobj_state *state);
int writeLidata(obj_rec *objr, pobj_state *state);
int writeMisc32(obj_rec *objr, pobj_state *state);
int writeComdat(obj_rec *objr, pobj_state *state);
int writeLinsym(obj_rec *objr, pobj_state *state);
void PObjRegList(const pobj_list *list, size_t len);
void GenMSOmfInit(void);
int MsgInit(const char *program);
void AsmInit(int cpu, int fpu, int use32);
void add_constant(char *string);
void do_envvar_cmdline(char *envvar, int level);
void parse_cmdline(int argc, char **argv);
int set_build_target(void);
void get_os_include(void);
void AddStringToIncludePath(char *path);
int trademark(void);
void open_files(void);
void PushLineQueue(void);
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
