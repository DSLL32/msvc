#ifndef WASM_H
#define WASM_H
#include <stdio.h>
#include <stddef.h>
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
enum { DO_NOTHING, ADD_USCORES, REMOVE_USCORES };
typedef struct File_Info {
    FILE *file[FILE_TYPES];
    char *fname[FILE_TYPES];
} File_Info;

extern File_Info AsmFiles;
extern global_options Options;
extern unsigned LineNumber;
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
void *CarveAlloc(carve_t cv);
void CarveDestroy(carve_t cv);
void CarveFree(carve_t cv, void *element);
void MemFree(void *memory);
void *MemAlloc(size_t size);
void *AsmAlloc(size_t size);
void Fatal(unsigned message, ...);
int InternalError(const char *file, unsigned line);
#ifdef __WATCOMC__
/* The original assertion helper terminates through exit(255). */
#pragma aux InternalError aborts;
#endif

void newBlk(cv_t *cv);
carve_t CarveCreate(size_t elm_size, size_t blk_size);
void ObjRecInit(void);
void ObjRecFini(void);
typedef struct obj_rec obj_rec;
/* Packed WOMP fixup, including logical reference consumed by CreateFixupRec. */
enum { FIX_LO_BYTE, FIX_OFFSET, FIX_BASE, FIX_POINTER, FIX_HI_BYTE,
    FIX_OFFSET386, FIX_POINTER386 };
#pragma pack(1)
typedef struct fixup {
    struct fixup *next;
    unsigned char loc_method : 3, self_relative : 1, loader_resolved : 1;
    unsigned long loc_offset;
    struct logref {
        unsigned char frame : 3, target : 3, is_secondary : 1;
        unsigned short frame_datum, target_datum;
        long target_offset;
    } lr;
} fixup;
#pragma pack()
typedef char fixup_size_check[sizeof(fixup) == 18 ? 1 : -1];
typedef char fixup_target_check[offsetof(fixup,lr.target_offset) == 14 ? 1 : -1];
fixup *FixNew(void);
typedef char fixup_offset_check[offsetof(fixup,loc_offset) == 5 ? 1 : -1];
#pragma pack(1)
struct linnum_data { unsigned short number; unsigned long offset; };
#pragma pack()
typedef char linnum_data_size_check[sizeof(struct linnum_data) == 6 ? 1 : -1];
#pragma pack(1)
typedef struct pubdef_data {
    unsigned short name;
    unsigned long offset;
    struct { unsigned short idx; } type;
} pubdef_data;
#pragma pack()
typedef char pubdef_data_size_check[sizeof(pubdef_data) == 8 ? 1 : -1];
/* Shared object-command values already used by GenMSOmfInit. */
enum {
    CMD_THEADR = 0x80,
    CMD_COMENT = 0x88,
    CMD_MODEND = 0x8a,
    CMD_EXTDEF = 0x8c,
    CMD_LNAMES = 0x96,
    CMD_SEGDEF = 0x98,
    CMD_GRPDEF = 0x9a,
    CMD_LEDATA = 0xa0,
    CMD_FIXUP = 0x9c,
    CMD_PUBDEF = 0x90,
    CMD_TYPDEF = 0x8e,
    CMD_LINNUM = 0x94,
    CMD_LIDATA = 0xa2,
    CMD_COMDEF = 0xb0,
    CMD_CEXTDF = 0xbc,
    CMD_STATIC_EXTDEF = 0xb4,
    CMD_STATIC_PUBDEF = 0xb6,
    CMD_STATIC_COMDEF = 0xb8,
    CMD_BAKPAT = 0xb2,
    CMD_COMDAT = 0xc2,
    CMD_LINSYM = 0xc4,
    CMD_ALIAS = 0xc6,
    CMD_NBKPAT = 0xc8,
    CMD_LLNAMES = 0xca
};
enum { F_SEG, F_GRP, F_EXT, F_ABS, F_LOC, F_TARG, F_NONE };
enum { T_SEGWD, T_GRPWD, T_EXTWD, T_ABSWD, T_SEG, T_GRP, T_EXT, T_ABS };
enum { FIX_GEN_INTEL, FIX_GEN_PHARLAP, FIX_GEN_MS386 };
size_t FixGenFix(fixup *fix, char *buffer, int type);
size_t FixGenLRef(struct logref *ref, char *buffer, int type);
char *putFrameDatum(char *p, unsigned char method, unsigned short datum);
char *putTargetDatum(char *p, unsigned char method, unsigned short datum);
char *putIndex(char *p, unsigned short value);
char *put16(char *p, unsigned short value);
char *put32(char *p, unsigned long value);
void FixKill(fixup *item);
void ObjDetachData(obj_rec *record);
obj_rec *ObjNewRec(unsigned char command);
void ObjKillRec(obj_rec *record);
void ObjAttachData(obj_rec *record, void *data, unsigned short length);
void ObjAllocData(obj_rec *record, unsigned short length);
void ObjPutName(obj_rec *record, const unsigned char *name, unsigned char length);
void ObjPutIndex(obj_rec *record, unsigned short value);
void write_flat_grp(void);
void ObjPut8(obj_rec *record, unsigned char value);
void ObjPut16(obj_rec *record, unsigned short value);
void ObjPut(obj_rec *record, const unsigned char *data, unsigned short length);
unsigned char *ObjGet(obj_rec *record, unsigned short length);
void write_record(obj_rec *record, unsigned char kill);
void check_need_32bit(obj_rec *record);
void write_linnum(void);
/* Full encountered SEGDEF layout. Other record variants are recovered
 * when their own consumers are reached. */
#pragma pack(1)
typedef struct physref { unsigned short frame; unsigned long offset; } physref;
struct obj_rec {
    obj_rec *next;
    unsigned short length, curoff;
    unsigned char *data;
    unsigned char command;
    unsigned char is_phar:1, is_32:1, free_data:1;
    union {
        struct {
            unsigned char main_module:1, start_addrs:1, is_logical:1;
            union logphys { struct logref log; physref phys; } ref;
        } modend;
        struct { unsigned short grp_idx, seg_idx, frame; } base;
        struct { unsigned char attr, class; } coment;
        struct { unsigned short first_idx, num_names; } lnames, extdef, comdef;
        struct { unsigned short idx; } grpdef;
        struct {
            unsigned short idx;
            unsigned char align:4, combine:4;
            unsigned char use_32:1, access_valid:1, access_attr:2;
            physref abs;
            unsigned long seg_length;
            unsigned short seg_name_idx, class_name_idx, ovl_name_idx;
        } segdef;
        struct { obj_rec *data_rec; fixup *fixup; } fixup;
        struct { unsigned short idx; unsigned long offset; } ledata;
        struct {
            union {
                struct { unsigned short grp_idx, seg_idx, frame; } base;
                struct { unsigned char flags; unsigned short public_name_idx; } linsym;
            } d;
            unsigned short num_lines;
            struct linnum_data *lines;
        } linnum;
        struct {
            struct { unsigned short grp_idx, seg_idx, frame; } base;
            unsigned short num_pubs;
            struct pubdef_data *pubs;
            unsigned char free_pubs:1, processed:1;
        } pubdef;
    } d;
};
#pragma pack()
typedef char modend_log_offset_check[offsetof(obj_rec,d.modend.ref.log) == 15 ? 1 : -1];
typedef char ledata_offset_check[offsetof(obj_rec,d.ledata.offset) == 16 ? 1 : -1];
typedef char segdef_record_size_check[sizeof(obj_rec) == 34 ? 1 : -1];
typedef char fixup_record_offset_check[offsetof(obj_rec,d.fixup.fixup) == 18 ? 1 : -1];
typedef char linnum_lines_offset_check[offsetof(obj_rec,d.linnum.lines) == 22 ? 1 : -1];
typedef char pubdef_pubs_offset_check[offsetof(obj_rec,d.pubdef.pubs) == 22 ? 1 : -1];
typedef char segdef_length_offset_check[offsetof(obj_rec,d.segdef.seg_length) == 24 ? 1 : -1];
typedef char segdef_class_offset_check[offsetof(obj_rec,d.segdef.class_name_idx) == 30 ? 1 : -1];
typedef struct pobj_state pobj_state;
typedef struct OBJ_WFILE OBJ_WFILE;
#pragma pack(1)
struct OBJ_WFILE {
    int fh;
    unsigned short length;
    size_t in_buf;
    unsigned char checksum;
    unsigned char in_rec : 1;
    char buffer[1];
};
#pragma pack()
typedef char objw_size_check[sizeof(OBJ_WFILE) == 13 ? 1 : -1];
#pragma pack(1)
struct pobj_state {
    unsigned char pass;
    OBJ_WFILE *file_out;
};
#pragma pack()
extern pobj_state pobjState;
OBJ_WFILE *ObjWriteOpen(const char *filename);
void DiskError(void);
void safeSeek(int handle, long offset, int mode);
void safeWrite(int handle, const char *data, size_t length);
unsigned char checkSum(const unsigned char *data, unsigned short length);
void ObjWriteRec(OBJ_WFILE *out, unsigned char command, unsigned short length, const char *data);
void DelErrFile(void);
typedef int (*pobj_filter)(obj_rec *objr, pobj_state *state);
extern pobj_filter jumpTable[];
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
void writeLinnumData(obj_rec *objr, OBJ_WFILE *out);
int writeLidata(obj_rec *objr, pobj_state *state);
int writeMisc32(obj_rec *objr, pobj_state *state);
int writeComdat(obj_rec *objr, pobj_state *state);
int writeLinsym(obj_rec *objr, pobj_state *state);
void PObjRegList(const pobj_list *list, size_t len);
void PObjUnRegList(const pobj_list *list, size_t len);
void GenMSOmfInit(void);
void objWFlushBuffer(OBJ_WFILE *out);
void ObjWBegRec(OBJ_WFILE *out, unsigned char command);
void ObjWrite8(OBJ_WFILE *out, unsigned char value);
void ObjWrite16(OBJ_WFILE *out, unsigned short value);
void ObjWrite32(OBJ_WFILE *out, unsigned long value);
void ObjWriteIndex(OBJ_WFILE *out, unsigned short value);
void ObjWrite(OBJ_WFILE *out, const char *data, size_t length);
void ObjWEndRec(OBJ_WFILE *out);
size_t FixGenRef(union logphys *ref, int is_logical, char *buffer, int type);
size_t FixGenPRef(physref *ref, char *buffer, int type);
void WompPrtMsg(unsigned message, ...);

/* LIDATA relocation adjustment node: actual8B owner. */
typedef struct lifix_munge {
    struct lifix_munge *next;
    unsigned short lower_bound;
    short delta;
} lifix;
typedef char lifix_size_check[sizeof(lifix) == 8 ? 1 : -1];
typedef struct lifix_list { lifix *head; } lifix_list;
short LifixDelta(lifix_list *list, unsigned short old_offset);
void LifixInit(lifix_list *list);
void LifixDestroy(lifix_list *list);
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
    void (*free)(void *);
};
long res_seek(int handle, long position, int where);
extern long FileShift;
extern struct WResRoutines WResRtns;
void WresRecordError(int status);
int ResOpenFileRO(const char *filename);
int OpenResFile(HANDLE_INFO *instance);
int CloseResFile(HANDLE_INFO *instance);
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
void __FreeResList(WResTypeNode *type);
void __FreeLangList(WResResNode *resource);
void WResFreeDir(WResDir dir);
int ResCloseFile(int handle);
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
void set_mem_type(char type);
void set_some_kinda_name(char token, char *name);
void get_fname(char *token, int type);
int InputQueueFile(char *filename);
struct file_list;
struct file_list *push_flist(char *name, unsigned char is_a_file);
FILE *open_file_in_include_path(char *name, char *fullpath);
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
extern char *IncludePath;
void AddStringToIncludePath(char *path);
int trademark(void);
void open_files(void);
typedef struct line_list { struct line_list *next; char *line; } line_list;
typedef struct input_queue {
    line_list *head, *tail;
    struct input_queue *next;
} input_queue;
extern input_queue *line_queue;
input_queue *PushLineQueue(void);
line_list *enqueue(void);
void PopLineQueue(void);
int main(int argc, char **argv);
void main_init(void);
void do_init_stuff(int argc, char **argv);
extern char *CodeBuffer;
extern unsigned BufSize;
void OutSelect(unsigned char starts);
void FlushCurrSeg(void);
void write_ledata(void);
extern int MacroLocalVarCounter;
extern char EndDirectiveFound, write_to_file, Modend;
extern signed char Use32, DefineProc;
struct fixup;
extern struct fixup *FixupListHead, *FixupListTail;
extern obj_rec *ModendRec;
extern signed char PhaseError;
extern unsigned long PassTotal[2];
/* Older packed layout: the32-bit index precedes16-bit sel_start. The expand
 * count at+14 is consumed by input_get;2002 reordered/widened these fields. */
#pragma pack(1)
typedef struct global_vars {
    unsigned code_seg;
    unsigned data_in_code : 1, dosseg : 1, : 30;
    unsigned sel_idx;
    unsigned short sel_start;
    unsigned expand_count;
    unsigned for_counter;
} global_vars;
#pragma pack()
typedef char globals_expand_check[offsetof(global_vars,expand_count) == 14 ? 1 : -1];
typedef char globals_for_check[offsetof(global_vars,for_counter) == 18 ? 1 : -1];
extern global_vars Globals;
typedef struct queuenode { void *next, *data; } queuenode;
typedef struct qdesc { void *head, *tail; } qdesc;
extern qdesc *LnameQueue;
extern qdesc *LinnumQueue;
long QCount(qdesc *queue);
extern qdesc *GlobalQueue;
extern qdesc *AliasQueue;
void *QDequeue(qdesc *queue);
void QInit(qdesc *queue);
void QEnqueue(qdesc *queue, void *item);
void IdxInit(void);
int LnameInsert(char *name);
void ModuleInit(void);
void FixInit(void);
void AssumeInit(void);
void write_init(void);
char *input_get(char *string);
char *ReadTextLine(char *string);
char *ReadPreparedLine(char *string);
/* Meaningful names for the two old helpers absent from the released source. */
void StripLineComment(char *string);
char *PrepareTextLine(char *string);
int get_instruction_position(char *string);
int Comment(int what_to_do, int index);
void prep_line_for_conditional_assembly(char *line);
char *join_multiline_cmds(char *line, int length);
char *ScanLine(char *string, int length);
int ExpandMacro(int token_count);
int ExpandProcString(int index);
int AsmParse(void);
void AsmLine(char *string);
void writepass1stuff(void);
void ObjWriteClose(OBJ_WFILE *obj);
int write_modend(void);
void reset_seg_len(void);
void PrepAnonLabels(void);
void AsmSymFini(void);
void FreeIncludePath(void);
void FixFini(void);
void WriteObjModule(void);
void PrintStats(void);
int MsgFini(void);
void main_fini(void);
void free_names(void);
void GenMSOmfFini(void);
void AsmShutDown(void);

#if defined(__WATCOMC__)
/* Watcom 10.0a intrinsic emits the required pending-entry trap inline. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#define WASM_PENDING() reconstruction_trap()
#else
/* Assembly is used only for an unreconstructed function's INT3. */
#define WASM_PENDING() __asm__ __volatile__("int $3")
#endif
void write_header(void);
void write_dosseg(void);
void write_lib(void);
void write_lnames(void);
void write_seg(void);
void write_grp(void);
void write_global(void);
void write_ext(void);
void write_comm(void);
int opsize(unsigned mem_type);
int get_number_of_bytes_for_size_in_commdef(unsigned long value);
void write_alias(void);
int write_pub(void);
void put_public_procs_in_public_table(void);
extern char **NameArray;
const char *NameGet(unsigned short handle);
void writeBase(obj_rec *record, OBJ_WFILE *out);
void write_export(void);
void write_end_of_pass1(void);
#endif
