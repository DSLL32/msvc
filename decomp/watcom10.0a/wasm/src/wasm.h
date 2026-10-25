#ifndef WASM_H
#define WASM_H
#include <stdio.h>
#include <recon.h>

/* Encountered prefix through allow_c_octals at37, packed as in1994.
 * Intervening fields retain their released source names and target defaults. */
#pragma pack(1)
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
    unsigned char warning_count, error_limit, warning_level, warning_error;
    char *build_target;
    char *code_class, *data_seg, *text_seg, *module_name;
    char *default_name_mangler;
    unsigned char allow_c_octals;
} global_options;
#pragma pack()

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
typedef struct HANDLE_INFO {
    int handle;
    const char *filename;
} HANDLE_INFO;
typedef int (*wres_open_fn)(const char *, int, ...);
struct WResRoutines {
    wres_open_fn open;
    int (*close)(int);
    int (*write)(int, const void *, unsigned);
    int (*read)(int, void *, unsigned);
    long (*seek)(int, long, int);
    long (*tell)(int);
    void *(*alloc)(size_t);
};
long res_seek(int handle, long position, int where);
extern long FileShift;
extern struct WResRoutines WResRtns;
void WresRecordError(int status);
int ResOpenFileRO(const char *filename);
int OpenResFile(HANDLE_INFO *instance);
int FindResources(HANDLE_INFO *instance);
typedef struct WResResNode WResResNode;
#pragma pack(1)
typedef struct WResIDName {
    unsigned char NumChars;
    char Name[1];
} WResIDName;
typedef struct WResID {
    unsigned char IsName;
    union { unsigned short Num; WResIDName Name; } ID;
} WResID;
typedef struct WResResInfo1 {
    unsigned short MemoryFlags;
    unsigned long Offset, Length;
    WResID ResName;
} WResResInfo1;
typedef struct WResResInfo {
    unsigned short NumResources;
    WResID ResName;
} WResResInfo;
typedef struct WResLangType { unsigned short lang; unsigned char sublang; } WResLangType;
typedef struct WResLangInfo {
    WResLangType lang;
    unsigned short MemoryFlags;
    unsigned long Offset, Length;
} WResLangInfo;
typedef struct WResLangNode {
    struct WResLangNode *Next, *Prev;
    void *data;
    WResLangInfo Info;
} WResLangNode;
struct WResResNode {
    WResResNode *Next, *Prev;
    WResLangNode *Head, *Tail;
    WResResInfo Info;
};
typedef struct WResTypeInfo {
    unsigned short NumResources;
    WResID TypeName;
} WResTypeInfo;
typedef struct WResTypeNode {
    struct WResTypeNode *Next, *Prev;
    WResResNode *Head, *Tail;
    WResTypeInfo Info;
} WResTypeNode;
typedef struct WResDirWindow {
    WResResNode *CurrRes;
    WResTypeNode *CurrType;
    WResLangNode *CurrLang;
} WResDirWindow;
#pragma pack()
#pragma pack(1)
typedef struct WResDirHead {
    unsigned short NumResources, NumTypes;
    WResTypeNode *Head, *Tail;
    unsigned short TargetOS;
} WResDirHead;
#pragma pack()
typedef WResDirHead *WResDir;
WResDir WResInitDir(void);
int WResReadDir(int handle, WResDir dir, int *duplicate_discarded);
void __FreeTypeList(WResDir dir);
int WResIsWResFile(int handle);
int ResReadUint32(unsigned long *value, int handle);
#pragma pack(1)
typedef struct WResHeader {
    unsigned long Magic[2], DirOffset;
    unsigned short NumResources, NumTypes, WResVer;
} WResHeader;
typedef struct WResExtHeader {
    unsigned short TargetOS, reserved[4];
} WResExtHeader;
#pragma pack()
int WResReadHeaderRecord(WResHeader *header, int handle);
int WResReadExtHeader(WResExtHeader *header, int handle);
int readTypeList(int handle, WResDir dir, unsigned short version);
int WResReadFixedTypeRecord(WResTypeInfo *info, int handle);
int WResIDExtraBytes(const WResID *id);
int WResIDCmp(const WResID *first, const WResID *second);
int WResIDNameCmp(const WResIDName *first, const WResIDName *second);
int WResReadExtraWResID(WResID *id, int handle);
void ResAddLLItemAtEnd(void **head, void **tail, void *newnode);
int readResList(int handle, WResTypeNode *type, unsigned short version);
int WResReadFixedResRecord1(WResResInfo1 *info, int handle);
int WResReadFixedResRecord(WResResInfo *info, int handle);
int readLangInfoList(int handle, WResResNode *resource);
int readWResDir(int handle, WResDir dir);
int readMResDir(int handle, WResDir dir, int *duplicate_discarded);
int InitResources(HANDLE_INFO *instance);
int MsgInit(const char *program);
void AsmInit(int cpu, int fpu, int use32);
void add_constant(char *string);
int isvalidident(char *id);
int StoreConstant(char *name, char *value, signed char redefine);
extern char StringBuf[256];
int AsmScan(char *string, char *stringbuf);
int createconstant(char *name, int value, int start, signed char redefine, unsigned char expand_early);
void do_envvar_cmdline(char *envvar, int level);
void parse_token(char *token, int nesting_level);
char *set_processor_type(char *input);
void set_mem_type(int type);
void set_some_kinda_name(int token, char *name);
void get_fname(char *token, int type);
void InputQueueFile(char *filename);
void AsmFree(void *memory);
void MsgPrintf1(int resourceid, const char *arg);
void parse_cmdline(int argc, char **argv);
void usage_msg(void);
void PrintfUsage(unsigned first_line);
void Wait_for_return(void);
void output(const unsigned char *text);
int MsgGet(int resourceid, char *buffer);
/* Actual offset+DS far pointer occupies EBX/ECX under Watcom register ABI.
 * The native shared profile erases __far only for proved flat-DS use. */
int LoadString(HANDLE_INFO *instance, unsigned id, char __far *buffer, int capacity);
void WResInitIDFromNum(long num, WResID *id);
WResDirWindow WResFindResource(const WResID *type, const WResID *name,
                              WResDir dir, WResLangType *lang);
WResLangInfo *WResGetLangInfo(WResDirWindow window);
WResTypeNode *__FindType(const WResID *type, WResDir dir);
WResResNode *__FindRes(const WResID *name, WResTypeNode *type);
WResLangNode *__FindLang(WResLangType *lang, WResResNode *res);
int GetString(WResLangInfo *res, HANDLE_INFO *instance, unsigned id,
              char __far *buffer, int capacity);
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
