#ifndef RECON_NATIVE_REMAPS_H
#define RECON_NATIVE_REMAPS_H
/* Consume host declarations before any token remapping. Project headers may
 * include this header before their caller has finished its own include list. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <signal.h>
#include <float.h>
#include <sys/timeb.h>
#include <win32.h>
#include <imagehlp.h>
#include <objbase.h>
#include <direct.h>
#ifdef RECON_BORLAND_400_DOS_TIME
#include <dos.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* BC4.0 BCC32 uses stdcall exception callbacks despite the later shipped
 * DOSEXCPT.H Win32 typedef. Setter itself is cdecl; dispatch is unsupported. */
typedef ULONG (WINAPI *ReconBorland400UserHandler)(EXCEPTION_RECORD *,void *,CONTEXT *,void *);
ReconBorland400UserHandler __cdecl recon_borland_400_SetUserHandler(ReconBorland400UserHandler);
void recon_borland_400_initialize_gp_handler(void);
void __cdecl __cxa_pure_virtual(void) __attribute__((noreturn));

#ifndef _ONEXIT_T_DEFINED
typedef int (__cdecl *_onexit_t)(void);
#define _ONEXIT_T_DEFINED
#endif
_onexit_t recon_onexit(_onexit_t);
_onexit_t recon_dllonexit(_onexit_t,_onexit_t **,_onexit_t **);

#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256
#define _P_WAIT 0
#define _P_NOWAIT 1
#define _P_OVERLAY 2

/* Native host metadata, verified against the captured original before launch.
 * Registration never opens or loads a PE image. Target DLLs retain DllMain. */
#define RECON_MODULE_PRIMARY 1
#define RECON_MODULE_NO_TLS 2
typedef struct ReconModuleSpec {
 const char *name,*path,*sha256;
 DWORD flags;
} ReconModuleSpec;
HMODULE recon_register_module(const ReconModuleSpec *);
/* Host-owned exact CreateProcessA input; bytes includes its final NUL. */
void recon_bind_command_line(char *,size_t);
const ReconModuleSpec *recon_module_identity(HMODULE);
BOOL recon_module_thread_calls_disabled(HMODULE);
/* Host-only, explicit dependency callbacks; no PE loader or target vertices. */
#define RECON_MODULE_EVENT_LOAD 1
#define RECON_MODULE_EVENT_FREE 2
#define RECON_MODULE_PROVIDER_REGISTERED 1
#define RECON_MODULE_PROVIDER_LOADING 2
#define RECON_MODULE_PROVIDER_LOADED 3
#define RECON_MODULE_PROVIDER_UNLOADING 4
#define RECON_MODULE_PROVIDER_RELEASED 5
typedef struct ReconModuleExport {
 const char *name; WORD ordinal; FARPROC address;
} ReconModuleExport;
typedef struct ReconModuleExportsSpec {
 const char *sha256; const ReconModuleExport *exports; unsigned count;
 void (__cdecl *lifecycle)(HMODULE,unsigned,void *); void *context;
} ReconModuleExportsSpec;
void recon_register_module_exports(HMODULE,const ReconModuleExportsSpec *);
unsigned recon_module_provider_state(HMODULE);

/* Ordinary static bundles are separate from fixture observer/lease providers.
 * Exports retain their actual native ABI; cast FARPROC at the typed call site.
 * Registration is startup-only. Loader operations are serialized thereafter. */
typedef BOOL (WINAPI *ReconDllEntry)(HINSTANCE,DWORD,LPVOID);
/* One native DLL-owned contiguous writable span, selected by the linker.
 * Configure after relocation and before any DLL initialization/constructors.
 * Caller guarantees readable/writable lifetime and excludes shared runtime state. */
typedef struct ReconModuleWritableRegion {
 void *address; size_t bytes;
} ReconModuleWritableRegion;
typedef struct ReconStaticModuleSpec {
 const ReconModuleExport *exports; unsigned count;
 ReconDllEntry entry;
 const HMODULE *dependencies; unsigned dependency_count;
 const ReconModuleWritableRegion *writable_region; /* NULL retains resident state */
} ReconStaticModuleSpec;
BOOL recon_configure_static_module(HMODULE,const ReconStaticModuleSpec *);
BOOL recon_attach_static_module(HMODULE);
void recon_detach_process_modules(void);
/* Bytes must be live immutable native spans; no PE image is loaded. */
BOOL recon_register_embedded_file(const char *,const void *,size_t);
void recon_load_module_resources_span(HMODULE,const void *,size_t);

typedef struct ShimFILE {
 char *_ptr; int _cnt; char *_base; int _flag,_file,_charbuf,_bufsiz; char *_tmpfname;
} ShimFILE;
typedef char ReconFileIs32[(sizeof(ShimFILE)==32)?1:-1];
/* glibc's convenience macros expand to host timespec members; VC1 has
 * three plain signed 32-bit seconds fields instead. Host shims use st_*tim. */
#undef st_atime
#undef st_mtime
#undef st_ctime
struct recon_stat {
 unsigned st_dev;
 unsigned short st_ino,st_mode;
 short st_nlink,st_uid,st_gid;
 unsigned st_rdev;
 long st_size,st_atime,st_mtime,st_ctime;
};
typedef char ReconStatIs36[(sizeof(struct recon_stat)==36)?1:-1];
typedef char ReconStatSizeAt20[(offsetof(struct recon_stat,st_size)==20)?1:-1];
typedef char ReconStatAtimeAt24[(offsetof(struct recon_stat,st_atime)==24)?1:-1];
typedef char ReconStatMtimeAt28[(offsetof(struct recon_stat,st_mtime)==28)?1:-1];
typedef char ReconStatCtimeAt32[(offsetof(struct recon_stat,st_ctime)==32)?1:-1];
extern ShimFILE recon_iob[20];
extern unsigned char recon_osfile[];
extern unsigned recon_nhandle;
extern int recon_fmode,recon_commode,recon_cflush,recon_umaskval;
extern unsigned long recon_doserrno,recon_lc_handle[6];
extern unsigned recon_lc_codepage;
extern int recon_mb_cur_max,recon_mb_codepage;
extern unsigned recon_mb_codepage_gate;
extern unsigned short *recon_pctype;
extern unsigned char recon_mbctype[257];
#define RECON_CRT_LOCALE_1252_EN_US 1
typedef struct ReconCrtLocaleProfile {
 const char *provider_sha256; unsigned kind;
 const unsigned char *mbctype; size_t mbctype_count;
 /* Includes EOF at element0; exposed _pctype points to element1. */
 const unsigned short *ctype; size_t ctype_count;
} ReconCrtLocaleProfile;
void recon_crt_apply_locale_profile(const ReconCrtLocaleProfile *);
/* Host enables the captured empty LC_CTYPE selection; only the actual target
 * call mutates locale state. Read-only inventory contains no captured address. */
typedef struct ReconCrtCtypeSelection {
 unsigned bound,calls;
 char reference_sha256[65];
 char *result;
 unsigned mb_calls;
} ReconCrtCtypeSelection;
void recon_crt_bind_empty_ctype_1252(const char *,const char *);
const ReconCrtCtypeSelection *recon_crt_ctype_selection_state(void);
char *recon_setlocale(int,const char *);
int __cdecl recon_setmbcp(int);
unsigned long __cdecl recon_mspdb_SigForPbCb(const void *,long,unsigned long);
void __cdecl recon_mspdb_TruncStFromSz(char *,const char *,size_t);
struct PDB;struct NameMap;
int __cdecl recon_mspdb_NameMap_open(struct PDB *,int,struct NameMap **);
int __cdecl recon_mspdb_PDB_OpenValidate(char *,char *,char *,DWORD,DWORD,LONG *,char *,struct PDB **);
extern char *recon_pgmptr,*recon_aenvptr;
extern long recon_timezone;
extern int recon_daylight;
extern char *recon_tzname[2];
extern int recon_days[13],recon_fltused,recon_tmpoff;
extern unsigned recon_tempnam_counter,recon_tempnam_prefix_length;
void __cdecl recon_adj_fdiv_r(void);
extern int recon_adjust_fdiv;
typedef void (__cdecl *ReconInitializer)(void);
void recon_initterm(ReconInitializer *,ReconInitializer *);
struct _startupinfo;
struct _exception;
extern int recon_crt_app_type,recon_crt_newmode,recon_crt_argc;
extern int recon_crt_arguments_bound; /* Read-only host observation. */
extern char **recon_crt_argv,**recon_crt_environment,**recon_crt_initenv;
extern int (__cdecl *recon_crt_matherr)(struct _exception *);
void recon_crt_bind_arguments(int,char **,char **);
/* Ordinary native process startup; owned bytes remain live through final DLL
 * detach. Does not run App startup, choose fmode/FP, or register module identity. */
typedef struct ReconNativeEntry {
 int argc; char **argv,**environment;
 const char *program_path,*current_directory,*command_line;
} ReconNativeEntry;
ReconNativeEntry *recon_native_entry_initialize(int,char **,char **);
/* Own the same ordinary inputs, but defer CRT argv/environment publication.
 * The root must call recon_crt_bind_arguments once with its live CRT view. */
ReconNativeEntry *recon_native_entry_prepare(int,char **,char **);
/* Explicit root-entry-only opt-in; do not set for general support/App objects.
 * The marked root must initialize, or prepare then bind, before App code. */
#define RECON_NATIVE_ENTRY_PROTOCOL_TEXT "recon-native-command-line-v1"
#ifdef RECON_NATIVE_ENTRY_PROTOCOL_V1
static const char recon_native_entry_protocol_v1[]
 __attribute__((used,section(".recon.entry"),aligned(1)))=RECON_NATIVE_ENTRY_PROTOCOL_TEXT;
#endif
/* Captured WCHAR16 process namespace; no Linux host controls are inherited. */
extern const WCHAR *recon_crt_os_environment;
extern size_t recon_crt_os_environment_units;
extern unsigned recon_crt_environment_updates;
typedef struct ReconCrtEnvironmentVector {
 char **vector; size_t count; unsigned borrowed,live;
} ReconCrtEnvironmentVector;
extern ReconCrtEnvironmentVector recon_crt_environment_vectors[3];
void recon_crt_bind_os_environment(const WCHAR *,size_t,const char *);
void recon_bind_process_os_environment(const WCHAR *,size_t,const char *);
#ifdef RECON_SHIM_IMPL
int recon_native_entry_active(void);
char **recon_native_entry_spawn_environment(void);
void recon_crt_validate_spawn_profile(void);
void recon_crt_validate_spawn_cp0_profile(void);
/* Read-only inventory of the existing configured native clock sequence. */
uint64_t recon_local_clock_call_count(void);
#endif

void recon_set_app_type(int);
int *recon_p_fmode(void),*recon_p_commode(void);
char ***recon_p_initenv(void);
void recon_getmainargs(int *,char ***,char ***,int,struct _startupinfo *);
void recon_setusermatherr(int (__cdecl *)(struct _exception *));
extern char *recon_decimal_point;
#ifndef SIGBREAK
#define SIGBREAK 21
#endif
#ifndef _UPPER
#define _UPPER 1
#define _LOWER 2
#define _DIGIT 4
#define _SPACE 8
#define _PUNCT 16
#define _CONTROL 32
#define _BLANK 64
#define _HEX 128
#define _ALPHA 0x103
#endif
#ifndef _IOREAD
#define _IOREAD 1
#define _IOWRT 2
#define _IORW 0x80
#define _IOMYBUF 8
#define _IOEOF 0x10
#define _IOERR 0x20
#define _IOSTRG 0x40
#endif

ShimFILE *recon_fopen(const char *,const char *);
ShimFILE *recon_fsopen(const char *,const char *,int);
char *recon_getenv(const char *);
int recon_putenv(const char *);
char *recon_host_getenv(const char *);
int *recon_errno(void);
char *recon_strerror(const char *);
ShimFILE *recon_freopen(const char *,const char *,ShimFILE *);
void recon_setbuf(ShimFILE *,char *);
int recon_flushall(void),recon_fcloseall(void),recon_flsbuf(int,ShimFILE *);
int recon_dup(int),recon_dup2(int,int);
int _fileno(ShimFILE *);
int recon_fclose(ShimFILE *),recon_fflush(ShimFILE *);
size_t recon_fread(void *,size_t,size_t,ShimFILE *);
size_t recon_fwrite(const void *,size_t,size_t,ShimFILE *);
int recon_fseek(ShimFILE *,long,int);
long recon_ftell(ShimFILE *);
typedef long ReconWatcomFilePosition;
typedef char ReconWatcomPositionIs4[(sizeof(ReconWatcomFilePosition)==4)?1:-1];
int __cdecl recon_watcom_fgetpos(ShimFILE *,ReconWatcomFilePosition *);
int __cdecl recon_watcom_fsetpos(ShimFILE *,const ReconWatcomFilePosition *);
int recon_setvbuf(ShimFILE *,char *,int,size_t);
int recon_fgetc(ShimFILE *),recon_fputc(int,ShimFILE *);
char *recon_fgets(char *,int,ShimFILE *);
int recon_fputs(const char *,ShimFILE *),recon_puts(const char *);
int recon_fprintf(ShimFILE *,const char *,...),recon_printf(const char *,...);
int recon_vfprintf(ShimFILE *,const char *,va_list);
int recon_vprintf(const char *,va_list);
int __cdecl recon_sprintf(char *,const char *,...);
int recon_snprintf(char *,size_t,const char *,...);
int recon_mbtowc(WCHAR *,const char *,size_t);
int recon_sscanf(const char *,const char *,...);
void recon_qsort(void *,size_t,size_t,int (*)(const void *,const void *));
void recon_watcom_qsort_1994(void *,size_t,size_t,int (*)(const void *,const void *));
int recon_ungetc(int,ShimFILE *),recon_ferror(ShimFILE *),recon_feof(ShimFILE *);
void recon_clearerr(ShimFILE *),recon_rewind(ShimFILE *);
int recon_fileno(ShimFILE *);
int recon_open(const char *,int,...),recon_sopen(const char *,int,int,...);
int recon_setmode(int,int);
ShimFILE *recon_openfile(const char *,const char *,int,ShimFILE *);
int recon_read(int,void *,unsigned),recon_write(int,const void *,unsigned),recon_close(int);
long recon_lseek(int,long,int),recon_tell(int),recon_filelength(int);
int recon_chsize(int,long),recon_access(const char *,int),recon_stat(const char *,struct recon_stat *);
int recon_alloc_osfhnd(void),recon_set_osfhnd(int,long),recon_free_osfhnd(int);
long recon_get_osfhandle(int);
void recon_dosmaperr(unsigned long),recon_getbuf(ShimFILE *),recon_freebuf(ShimFILE *);
int recon_flush(ShimFILE *),recon_filbuf(ShimFILE *);
int recon_remove(const char *),recon_unlink(const char *),recon_rename(const char *,const char *);
void recon_splitpath(const char *,char *,char *,char *,char *);
void recon_makepath(char *,const char *,const char *,const char *,const char *);
char *recon_fullpath(char *,const char *,size_t),*recon_strupr(char *),*recon_strlwr(char *);
char *recon_getcwd(char *,int);
char *recon_find(const char *);
char *recon_ultoa(unsigned long,char *,int),*recon_itoa(int,char *,int);
char *__cdecl recon_ltoa(long,char *,int);
char *__cdecl recon_gcvt(double,int,char *);
char *recon_mktemp(char *),*recon_tempnam(const char *,const char *);
char *__cdecl recon_tmpnam(char *);
typedef struct ReconMktempObservation {
    uint32_t magic,version,owner,count,argument,result;
    int32_t before_errno,after_errno;
    uint32_t before_doserrno,after_doserrno,before_last_error,after_last_error;
    char before[MAX_PATH],after[MAX_PATH];
} ReconMktempObservation;
extern ReconMktempObservation recon_mktemp_observation;
char *recon_strcpy(char *,const char *);
char *recon_getpath(const char *,char *,int);
void recon_searchenv(const char *,const char *,char *);
int recon_wcscmp(const WCHAR *,const WCHAR *);
int __cdecl recon_wcsncmp(const WCHAR *,const WCHAR *,size_t);
int __cdecl recon_wcsicmp(const WCHAR *,const WCHAR *);
WCHAR *recon_wcschr(const WCHAR *,WCHAR);
size_t recon_wcslen(const WCHAR *);
WCHAR *recon_wcscat(WCHAR *,const WCHAR *);
WCHAR *recon_wcsdup(const WCHAR *);
WCHAR *__cdecl recon_wcsupr(WCHAR *);
long recon_wcstol(const WCHAR *,WCHAR **,int);
int recon_swprintf(WCHAR *,const WCHAR *,...);
int recon_iswctype(WCHAR,WCHAR);
WCHAR recon_towupper(WCHAR);
WCHAR __cdecl recon_towlower(WCHAR);
WCHAR *recon_wcscpy(WCHAR *,const WCHAR *),*recon_wcsncpy(WCHAR *,const WCHAR *,size_t);
unsigned recon_control87(unsigned,unsigned),recon_controlfp(unsigned,unsigned);
void recon_apply_x87_empty_entry_profile(const char *,unsigned,unsigned,unsigned);
void recon_ftime(struct _timeb *);
typedef void (*ReconSignalHandler)(int);
ReconSignalHandler recon_signal(int,ReconSignalHandler);
ReconSignalHandler __cdecl recon_borland_400_signal(int,ReconSignalHandler);
struct date;struct time;
void __cdecl recon_borland_400_getdate(struct date *);
void __cdecl recon_borland_400_gettime(struct time *);
/* Explicit Watcom surfaces only: all remain unavailable until reached proof. */
int recon_watcom_heapchk(void),recon_watcom_nheapchk(void);
int __cdecl recon_watcom_open(const char *,int,...);
char *__cdecl recon_watcom_strerror(int);
long __cdecl recon_watcom_lseek(int,long,int);
long __cdecl recon_watcom_tell(int);
int __cdecl recon_watcom_setmode(int,int);
extern unsigned recon_watcom_amblksiz;
int __cdecl recon_watcom_memicmp(const void *,const void *,size_t);
struct ReconWatcomDirent;
struct ReconWatcomDirent *__cdecl recon_watcom_opendir(const char *);
struct ReconWatcomDirent *__cdecl recon_watcom_readdir(struct ReconWatcomDirent *);
int __cdecl recon_watcom_closedir(struct ReconWatcomDirent *);
char *__cdecl recon_watcom_getcmd(char *);
void *__cdecl recon_watcom_fill_dwords(void *,unsigned,unsigned);
double __cdecl recon_watcom_strtod(const char *,char **);
void __cdecl recon_watcom_zbuf2f(char *,double *);
double __cdecl recon_watcom_scale10v(double,int);
int __cdecl recon_watcom_sprintf_17e(char *,const char *,...);
char *__cdecl recon_watcom_strrev(char *);
void __cdecl recon_watcom_splitpath_dos(const char *,char *,char *,char *,char *);
void __cdecl recon_watcom_splitpath2(const char *,char *,char **,char **,char **,char **);
volatile int *__cdecl recon_watcom_errno(void);
extern char recon_watcom_os;
int __cdecl recon_watcom_getch(void);
char *__cdecl recon_watcom_utoa(unsigned int,char *,int);
char *__cdecl recon_watcom_ultoa(unsigned long,char *,int);
void *__cdecl recon_watcom_expand(void *,size_t);
int __cdecl recon_watcom_alloca_available(size_t);
unsigned int __cdecl recon_watcom_rotl(unsigned int,unsigned int);
int __cdecl recon_watcom_setvbuf(ShimFILE *,char *,int,size_t);
ReconSignalHandler recon_watcom_signal(int,ReconSignalHandler);
int recon_watcom_raise(int);
/* Host binding requires independently proved first-main-call/no-target-ctor
 * provenance. This is an empty target namespace check, not a glibc heap walk. */
typedef struct ReconWatcomFirstMainHeap {
 char reference_sha256[65];
 unsigned bound,clean,calls,target_activity;
} ReconWatcomFirstMainHeap;
extern ReconWatcomFirstMainHeap recon_watcom_first_main_heap;
void recon_bind_watcom_wlib_first_main_heap(const char *);
void *recon_watcom_malloc(size_t),*recon_watcom_calloc(size_t,size_t);
void *recon_watcom_realloc(void *,size_t);
void recon_watcom_free(void *);
char *recon_watcom_strdup(const char *);
int __cdecl recon_strcmp(const char *,const char *);
int recon_isctype(int,int),recon_toupper(int),recon_tolower(int);
void recon_tzset(void),recon_endstdio(void),recon_rmtmp(void);
int recon_isindst(struct tm *);
int recon_mbsncmp(const unsigned char *,const unsigned char *,size_t);
int recon_mbsnbicmp(const unsigned char *,const unsigned char *,size_t);
int __cdecl recon_mbsnbcmp(const unsigned char *,const unsigned char *,size_t);
int __cdecl recon_mbscmp(const unsigned char *,const unsigned char *);
unsigned char *__cdecl recon_mbstok(unsigned char *,const unsigned char *);
unsigned char *recon_mbspbrk(const unsigned char *,const unsigned char *);
unsigned char *recon_mbsrchr(const unsigned char *,unsigned int);
unsigned char *recon_mbsupr(unsigned char *);
unsigned char *__cdecl recon_mbslwr(unsigned char *);
unsigned int __cdecl recon_mbctolower(unsigned int);
unsigned char *recon_mbsstr(const unsigned char *,const unsigned char *);
size_t recon_mbclen(const unsigned char *);
size_t __cdecl recon_mbstrlen(const char *);
size_t recon_mbsspn(const unsigned char *,const unsigned char *);
unsigned char *recon_mbschr(const unsigned char *,unsigned int);
int recon_mbsicmp(const unsigned char *,const unsigned char *);
unsigned char *recon_mbsnbcpy(unsigned char *,const unsigned char *,size_t);
unsigned char *recon_mbsinc(const unsigned char *);
unsigned char *recon_mbsdec(const unsigned char *,const unsigned char *);
void recon_mbccpy(unsigned char *,const unsigned char *);
int recon_ismbcspace(unsigned int);
typedef struct ReconCppAllocation {
    void *base;
    size_t requested,rounded;
    uint64_t generation;
    int live;
    struct ReconCppAllocation *next;
} ReconCppAllocation;
extern ReconCppAllocation *recon_cpp_allocations;
void *recon_cpp_new(size_t);
void recon_cpp_delete(void *);
char *__cdecl recon_unDName(char *,const char *,int,void *(__cdecl *)(size_t),
                           void (__cdecl *)(void *),unsigned short);
void recon_free(void *);
void *recon_realloc(void *,size_t);
int recon_system(const char *);
int recon_spawnve(int,const char *,const char *const *,const char *const *);
int recon_spawnvpe(int,const char *,const char *const *,const char *const *);
int recon_spawnvp(int,const char *,const char *const *);
int recon_spawnv(int,const char *,const char *const *);
int recon_spawnl(int,const char *,const char *,...);
int recon_spawnlp(int,const char *,const char *,...);
int recon_cenvarg(const char *const *,const char *const *,char **,char **,const char *);
char *__cdecl recon_ctime(const time_t *);
time_t recon_time(time_t *);
void recon_assert(void *,void *,unsigned);
void recon_exit(int);
void __cdecl recon_perror(const char *);
void recon_record_terminal_status(uint32_t);
/* Resolve DOS paths with explicit RECON_DRIVE_X mappings (Z defaults to /).
 * No Wine prefix or executable discovery is performed. */
int recon_host_path(const char *,char *,size_t);
int recon_fstat(int,struct recon_stat *);
int recon_isatty(int);
/* Host-only scope guards shared by the CRT and Win32 file implementations. */
int recon_fsopen_restricted(HANDLE);
void recon_fsopen_check_path(const char *);
void recon_fsopen_check_mappings(void);
void recon_fsopen_check_read_mappings(void);
int recon_fsopen_read_alias(int);
void recon_unsupported(const char *);

int __cdecl recon_wsprintfA(LPSTR,LPCSTR,...);
typedef struct ReconModuleMemorySpec {
 const char *sha256; DWORD original_rva,size,original_image_base,original_va;
} ReconModuleMemorySpec;
/* Host-owned loaded ELF anchors; bind before module/case setup. Main-ELF
 * ownership remains the default when no application anchor is supplied. */
#define RECON_NATIVE_IMAGE_APPLICATION 1u
#define RECON_NATIVE_IMAGE_CASE_CALLBACKS 2u
void recon_register_native_image(const void *,unsigned);
/* Host setup, once: current native runner and copied immutable argv prefix
 * (1..64 strings, total storage <=64KiB). Child argv is runner, prefix, then
 * original argv. Used only for canonical self requests allowed by RSP1;
 * requires RECON_SELF_SPAWN=1. No broker does not authorize this route. */
void recon_set_native_self_command(const char *,const char *const *,size_t);
void recon_register_module_memory(HMODULE,const ReconModuleMemorySpec *,void *,size_t);
void recon_load_module_resources(HMODULE,const char *);
void recon_load_system_profile(const char *,const char *);
void recon_load_system_info_profile(const char *,const char *);

#define RECON_WIN32_FUNCTIONS(X) \
 X(HANDLE,CreateFileA,(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)) \
 X(BOOL,CopyFileA,(LPCSTR,LPCSTR,BOOL)) \
 X(BOOL,DeleteFileA,(LPCSTR)) \
 X(HANDLE,FindFirstFileA,(LPCSTR,LPWIN32_FIND_DATAA)) \
 X(BOOL,FindNextFileA,(HANDLE,LPWIN32_FIND_DATAA)) \
 X(BOOL,FindClose,(HANDLE)) \
 X(PIMAGE_NT_HEADERS,CheckSumMappedFile,(LPVOID,DWORD,LPDWORD,LPDWORD)) \
 X(DWORD,MapFileAndCheckSumA,(LPSTR,LPDWORD,LPDWORD)) \
 X(void,CoUninitialize,(void)) \
 X(BOOL,BindImageEx,(DWORD,LPSTR,LPSTR,LPSTR,PIMAGEHLP_STATUS_ROUTINE)) \
 X(BOOL,ReBaseImage,(LPSTR,LPSTR,BOOL,BOOL,BOOL,ULONG,ULONG *,ULONG *,ULONG *,ULONG *,ULONG)) \
 X(BOOL,CloseHandle,(HANDLE)) X(DWORD,GetLastError,(void)) X(void,SetLastError,(DWORD)) \
 X(DWORD,GetFileSize,(HANDLE,LPDWORD)) X(BOOL,FlushViewOfFile,(LPCVOID,DWORD)) \
 X(DWORD,GetFileType,(HANDLE)) X(DWORD,SetFilePointer,(HANDLE,LONG,LONG *,DWORD)) \
 X(BOOL,SetEndOfFile,(HANDLE)) X(BOOL,ReadFile,(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID)) \
 X(BOOL,WriteFile,(HANDLE,LPCVOID,DWORD,LPDWORD,LPVOID)) X(BOOL,FlushFileBuffers,(HANDLE)) \
 X(HANDLE,GetStdHandle,(DWORD)) X(BOOL,SetStdHandle,(DWORD,HANDLE)) \
 X(int,LoadStringA,(HINSTANCE,UINT,LPSTR,int)) \
 X(DWORD,FormatMessageA,(DWORD,LPCVOID,DWORD,DWORD,LPSTR,DWORD,va_list *)) \
 X(HMODULE,GetModuleHandleA,(LPCSTR)) X(HMODULE,LoadLibraryA,(LPCSTR)) X(BOOL,FreeLibrary,(HMODULE)) \
 X(DWORD,GetFileAttributesA,(LPCSTR)) \
 X(FARPROC,GetProcAddress,(HMODULE,LPCSTR)) X(BOOL,DisableThreadLibraryCalls,(HMODULE)) \
 X(HANDLE,CreateFileMappingA,(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCSTR)) \
 X(LPVOID,MapViewOfFile,(HANDLE,DWORD,DWORD,DWORD,DWORD)) X(LPVOID,MapViewOfFileEx,(HANDLE,DWORD,DWORD,DWORD,DWORD,LPVOID)) X(BOOL,UnmapViewOfFile,(LPCVOID)) \
 X(BOOL,SetConsoleCtrlHandler,(BOOL (WINAPI *)(DWORD),BOOL)) \
 X(USHORT *,LdrProcessRelocationBlock,(LPVOID,DWORD,USHORT *,LONG)) \
 X(DWORD,SearchPathA,(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR *)) \
 X(DWORD,GetFullPathNameA,(LPCSTR,DWORD,LPSTR,LPSTR *)) \
 X(void,GetSystemInfo,(LPSYSTEM_INFO)) X(void,GlobalMemoryStatus,(LPMEMORYSTATUS)) \
 X(DWORD,GetTickCount,(void)) \
 X(DWORD,GetModuleFileNameA,(HMODULE,LPSTR,DWORD)) X(LPSTR,GetCommandLineA,(void)) X(DWORD,GetVersion,(void)) \
 X(DWORD,GetTempPathA,(DWORD,LPSTR)) X(UINT,GetTempFileNameA,(LPCSTR,LPCSTR,UINT,LPSTR)) \
 X(BOOL,SetEnvironmentVariableA,(LPCSTR,LPCSTR)) \
 X(BOOL,SystemTimeToFileTime,(const SYSTEMTIME *,FILETIME *)) \
 X(BOOL,SetFileTime,(HANDLE,const FILETIME *,const FILETIME *,const FILETIME *)) \
 X(void,GetSystemTime,(SYSTEMTIME *)) X(void,ExitProcess,(UINT)) X(void,DebugBreak,(void)) \
 X(LPVOID,VirtualAlloc,(LPVOID,DWORD,DWORD,DWORD)) X(BOOL,VirtualFree,(LPVOID,DWORD,DWORD)) \
 X(HLOCAL,LocalAlloc,(UINT,DWORD)) X(HLOCAL,LocalFree,(HLOCAL)) \
 X(HGLOBAL,GlobalAlloc,(UINT,DWORD)) X(HGLOBAL,GlobalFree,(HGLOBAL)) \
 X(int,MultiByteToWideChar,(UINT,DWORD,LPCSTR,int,LPWSTR,int)) \
 X(int,WideCharToMultiByte,(UINT,DWORD,LPCWSTR,int,LPSTR,int,LPCSTR,BOOL *)) \
 X(int,LCMapStringW,(DWORD,DWORD,LPCWSTR,int,LPWSTR,int)) \
 X(HANDLE,HeapCreate,(DWORD,DWORD,DWORD)) X(LPVOID,HeapAlloc,(HANDLE,DWORD,DWORD)) \
 X(LPVOID,HeapReAlloc,(HANDLE,DWORD,LPVOID,DWORD)) \
 X(BOOL,HeapFree,(HANDLE,DWORD,LPVOID)) X(LRESULT,SendMessageA,(HWND,UINT,WPARAM,LPARAM)) \
 X(BOOL,HeapDestroy,(HANDLE)) \
 X(BOOL,IsDBCSLeadByteEx,(UINT,BYTE)) X(BOOL,IsValidCodePage,(UINT)) X(UINT,GetACP,(void)) X(LPSTR,CharNextA,(LPCSTR)) \
 X(UINT,GetConsoleCP,(void)) X(BOOL,GetCPInfo,(UINT,CPINFO *)) \
 X(UINT,SetHandleCount,(UINT)) \
 X(BOOL,GetConsoleScreenBufferInfo,(HANDLE,CONSOLE_SCREEN_BUFFER_INFO *)) \
 X(void,GetLocalTime,(SYSTEMTIME *)) \
 X(LONG,RtlAnsiStringToUnicodeString,(struct UnicodeString *,struct AnsiString *,BYTE)) \
 X(LONG,RtlUnicodeStringToAnsiString,(struct AnsiString *,struct UnicodeString *,BYTE))
#define RECON_DECLARE_WIN32(result,name,args) result WINAPI recon_##name args;
RECON_WIN32_FUNCTIONS(RECON_DECLARE_WIN32)
#undef RECON_DECLARE_WIN32

#ifdef __cplusplus
}
#endif

#ifndef RECON_SHIM_IMPL
/* Clang accepts the original MS inline-assembly trap bodies. Native fixtures
 * run ordinary paths; SEH faults must fail the native test, never be swallowed. */
#define __try if (1)
#define __except(filter) else if (0)
#define FILE ShimFILE
#define SigForPbCb recon_mspdb_SigForPbCb
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define TruncStFromSz recon_mspdb_TruncStFromSz
#endif
#ifdef RECON_BORLAND_400_USER_HANDLER
#define _SetUserHandler recon_borland_400_SetUserHandler
#endif
#ifdef RECON_BORLAND_400_DOS_TIME
/* Host time.h declarations were consumed before these target-only names. */
#define getdate recon_borland_400_getdate
#define gettime recon_borland_400_gettime
#endif
#define setlocale recon_setlocale
#define _setmbcp recon_setmbcp
#define _mbctype recon_mbctype
#define wcscmp recon_wcscmp
#define wcsncmp recon_wcsncmp
#define _wcsicmp recon_wcsicmp
#define wcschr recon_wcschr
#define wcslen recon_wcslen
#define wcscat recon_wcscat
#define _wcsdup recon_wcsdup
#define _wcsupr recon_wcsupr
#define wcstol recon_wcstol
#define swprintf recon_swprintf
#define iswctype recon_iswctype
#define towupper recon_towupper
#define towlower recon_towlower
#define wcscpy recon_wcscpy
#define wcsncpy recon_wcsncpy
#define _control87 recon_control87
#define _controlfp recon_controlfp
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define _adj_fdiv_r recon_adj_fdiv_r
#define _mbstrlen recon_mbstrlen
#endif
#define _ftime recon_ftime
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
#ifndef __far
/* Only the reviewed flat-DS native semantic ABI; no segmented storage. */
#define __far
#endif
#define _amblksiz recon_watcom_amblksiz
#define __OS recon_watcom_os
#define fpos_t ReconWatcomFilePosition
#define fgetpos recon_watcom_fgetpos
#define fsetpos recon_watcom_fsetpos
#define __get_errno_ptr recon_watcom_errno
#undef errno
#define errno (*recon_watcom_errno())
#undef ENOENT
#define ENOENT 1
#undef EMFILE
#define EMFILE 11
#define open recon_watcom_open
#ifdef RECON_WATCOM_STRERROR_ENOENT
#define strerror recon_watcom_strerror
#endif
#define lseek recon_watcom_lseek
#define tell recon_watcom_tell
#define setmode recon_watcom_setmode
#define memicmp recon_watcom_memicmp
#define opendir recon_watcom_opendir
#define readdir recon_watcom_readdir
#define closedir recon_watcom_closedir
#define getcmd recon_watcom_getcmd
#define __STOSD recon_watcom_fill_dwords
#define _rotl recon_watcom_rotl
#define _splitpath2 recon_watcom_splitpath2
#define getch recon_watcom_getch
#define utoa recon_watcom_utoa
#define ultoa recon_watcom_ultoa
#define _expand recon_watcom_expand
#define _nexpand recon_watcom_expand
#ifdef RECON_WATCOM_ALLOCA_400
#undef alloca
/* The builtin must remain in the actual caller; its storage lives until that
 * caller returns. Evaluate the size once, as an ordinary native expression. */
#define alloca(bytes) __extension__ ({ size_t recon_alloca_bytes=(bytes); \
    recon_watcom_alloca_available(recon_alloca_bytes) ? \
    __builtin_alloca(recon_alloca_bytes) : (void *)0; })
#endif
#define strtod recon_watcom_strtod
#define __ZBuf2F recon_watcom_zbuf2f
#define _Scale10V recon_watcom_scale10v
#ifdef RECON_WATCOM_SPRINTF_17E
#define sprintf recon_watcom_sprintf_17e
#endif
#define itoa recon_itoa
/* Source-level successful mtime use; no raw Watcom stat layout promise. */
#define stat recon_stat
#define fstat recon_fstat
#define isatty recon_isatty
#define stdaux (&recon_iob[3])
#define stdprn (&recon_iob[4])
#define _heapchk recon_watcom_heapchk
#define _nheapchk recon_watcom_nheapchk
#define signal recon_watcom_signal
#define raise recon_watcom_raise
#elif defined(RECON_BORLAND_400_SIGNAL)
#define signal recon_borland_400_signal
#else
#define signal recon_signal
#endif
#undef MB_CUR_MAX
#define MB_CUR_MAX recon_mb_cur_max
#undef stdin
#undef stdout
#undef stderr
#define stdin (&recon_iob[0])
#define stdout (&recon_iob[1])
#define stderr (&recon_iob[2])
#undef getc
#undef putc
#undef getchar
#undef putchar
#define getc recon_fgetc
#define putc recon_fputc
#define getchar() recon_fgetc(stdin)
#define putchar(c) recon_fputc((c),stdout)
#define fopen recon_fopen
#define _fsopen recon_fsopen
#define _isatty recon_isatty
#define getenv recon_getenv
#define _putenv recon_putenv
#define _errno recon_errno
#define _strerror recon_strerror
#define freopen recon_freopen
#define setbuf recon_setbuf
#define flushall recon_flushall
#define _flushall recon_flushall
#define _fcloseall recon_fcloseall
#define _flsbuf recon_flsbuf
#define _fileno recon_fileno
#define read recon_read
#define write recon_write
#define close recon_close
#define dup recon_dup
#define _dup recon_dup
#define dup2 recon_dup2
#define _dup2 recon_dup2
#define unlink recon_unlink
#define _unlink recon_unlink
#define mktemp recon_mktemp
#define strupr recon_strupr
#define strlwr recon_strlwr
#ifdef RECON_CRT_PROFILE_WATCOM_100A
#define strrev recon_watcom_strrev
#endif
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define fclose recon_fclose
#define fflush recon_fflush
#define fread recon_fread
#define fwrite recon_fwrite
#define fseek recon_fseek
#define ftell recon_ftell
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
#define setvbuf recon_watcom_setvbuf
#else
#define setvbuf recon_setvbuf
#endif
#define fgetc recon_fgetc
#define fputc recon_fputc
#define fgets recon_fgets
#define fputs recon_fputs
#define puts recon_puts
#define fprintf recon_fprintf
#define printf recon_printf
#define vfprintf recon_vfprintf
#define vprintf recon_vprintf
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define sprintf recon_sprintf
#endif
#define _snprintf recon_snprintf
#define mbtowc recon_mbtowc
#define sscanf recon_sscanf
#if defined(RECON_QSORT_WATCOM_1994)
#ifndef RECON_CRT_PROFILE_WATCOM_100A
#error Watcom1994 qsort requires the explicit Watcom CRT profile
#endif
#define qsort recon_watcom_qsort_1994
#else
#define qsort recon_qsort
#endif
#define ungetc recon_ungetc
#define ferror recon_ferror
#define feof recon_feof
#define clearerr recon_clearerr
#define rewind recon_rewind
#define fileno recon_fileno
#define remove recon_remove
#define rename recon_rename
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define ctime recon_ctime
#endif
#define time(...) recon_time(__VA_ARGS__)
#define _onexit recon_onexit
#define __dllonexit recon_dllonexit
#define _assert recon_assert
#define exit recon_exit
#define perror recon_perror
#define _open recon_open
#define _sopen recon_sopen
#define _setmode recon_setmode
#define _find recon_find
#define _initterm recon_initterm
#define _adjust_fdiv recon_adjust_fdiv
#define __set_app_type recon_set_app_type
#define __p__fmode recon_p_fmode
#define __p__commode recon_p_commode
#define __p___initenv recon_p_initenv
#define __getmainargs recon_getmainargs
#define __setusermatherr recon_setusermatherr
#define strcpy recon_strcpy
#define _openfile recon_openfile
#define _read recon_read
#define _write recon_write
#define _close recon_close
#define _lseek recon_lseek
#define _tell recon_tell
#define _filelength recon_filelength
#define _fstat recon_fstat
#define _chsize recon_chsize
#define access recon_access
#define _access recon_access
#define _stat recon_stat
#define _alloc_osfhnd recon_alloc_osfhnd
#define _set_osfhnd recon_set_osfhnd
#define _free_osfhnd recon_free_osfhnd
#define _get_osfhandle recon_get_osfhandle
#define _dosmaperr recon_dosmaperr
#define _getbuf recon_getbuf
#define _freebuf recon_freebuf
#define _flush recon_flush
#define _filbuf recon_filbuf
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_SPLITPATH_WATCOM_DOS)
#define _splitpath recon_watcom_splitpath_dos
#else
#define _splitpath recon_splitpath
#endif
#define _makepath recon_makepath
#define _fullpath recon_fullpath
#define _getcwd recon_getcwd
#define _strupr recon_strupr
#define _strlwr recon_strlwr
#define _ultoa recon_ultoa
#define _purecall __cxa_pure_virtual
#define _itoa recon_itoa
#define _ltoa recon_ltoa
#define _gcvt recon_gcvt
#define _mktemp recon_mktemp
#define _tempnam recon_tempnam
#define tmpnam recon_tmpnam
#define _getpath recon_getpath
#define _searchenv recon_searchenv
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define strcmp recon_strcmp
#endif
#define _isctype recon_isctype
#define _endstdio recon_endstdio
#define _rmtmp recon_rmtmp
#define _spawnv recon_spawnv
#define spawnv recon_spawnv
#define spawnvp recon_spawnvp
#define spawnve recon_spawnve
#define spawnvpe recon_spawnvpe
#define _mbsncmp recon_mbsncmp
#define _mbsnbicmp recon_mbsnbicmp
#define _mbsnbcmp recon_mbsnbcmp
#define _mbscmp recon_mbscmp
#ifdef RECON_CRT_PROFILE_MSVCRT_510
#define _mbstok recon_mbstok
#endif
#define _mbspbrk recon_mbspbrk
#define _mbsrchr recon_mbsrchr
#define _mbsupr recon_mbsupr
#define _mbslwr recon_mbslwr
#define _mbctolower recon_mbctolower
#define _mbsstr recon_mbsstr
#define _mbclen recon_mbclen
#define _mbsspn recon_mbsspn
#define _mbschr recon_mbschr
#define _mbsicmp recon_mbsicmp
#define _mbsnbcpy recon_mbsnbcpy
#define _mbsinc recon_mbsinc
#define _mbsdec recon_mbsdec
#define _mbccpy recon_mbccpy
#define _ismbcspace recon_ismbcspace
#define crt_operator_new recon_cpp_new
#define crt_operator_delete recon_cpp_delete
#define __unDName recon_unDName
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
#define malloc recon_watcom_malloc
#define calloc recon_watcom_calloc
#define free recon_watcom_free
#define realloc recon_watcom_realloc
#define strdup recon_watcom_strdup
#else
#define free recon_free
#define realloc recon_realloc
#endif
#define system recon_system
#define _spawnve recon_spawnve
#define _spawnvpe recon_spawnvpe
#define _spawnvp recon_spawnvp
#define _spawnl recon_spawnl
#define _spawnlp recon_spawnlp
#define _cenvarg recon_cenvarg
#define _isindst recon_isindst
#define _osfile recon_osfile
#define _osfhnd recon_osfhnd
#define _decimal_point recon_decimal_point
#define _atoldbl recon_atoldbl
#define __mtold12 recon_mtold12
#define __multtenpow12 recon_multtenpow12
#define _nhandle recon_nhandle
#define _fmode recon_fmode
#define _commode recon_commode
#define _cflush recon_cflush
#define _umaskval recon_umaskval
#define _doserrno recon_doserrno
#define _lc_handle recon_lc_handle
#define _lc_codepage recon_lc_codepage
#define _pctype recon_pctype
#define _pgmptr recon_pgmptr
#define _aenvptr recon_aenvptr
#define _timezone recon_timezone
#define _daylight recon_daylight
#define _tzname recon_tzname
#define _days recon_days
#define _fltused recon_fltused
#define _tmpoff recon_tmpoff
#define CopyFileA recon_CopyFileA
#define CreateFileA recon_CreateFileA
#define DeleteFileA recon_DeleteFileA
#define FindFirstFileA recon_FindFirstFileA
#define FindNextFileA recon_FindNextFileA
#define FindClose recon_FindClose
#define CheckSumMappedFile recon_CheckSumMappedFile
#define MapFileAndCheckSumA recon_MapFileAndCheckSumA
#define CoUninitialize recon_CoUninitialize
#define BindImageEx recon_BindImageEx
#define ReBaseImage recon_ReBaseImage
#define CloseHandle recon_CloseHandle
#define GetLastError recon_GetLastError
#define SetLastError recon_SetLastError
#define GetFileSize recon_GetFileSize
#define FlushViewOfFile recon_FlushViewOfFile
#define GetFileType recon_GetFileType
#define SetFilePointer recon_SetFilePointer
#define SetEndOfFile recon_SetEndOfFile
#define ReadFile recon_ReadFile
#define WriteFile recon_WriteFile
#define FlushFileBuffers recon_FlushFileBuffers
#define GetStdHandle recon_GetStdHandle
#define SetStdHandle recon_SetStdHandle
#define LoadStringA recon_LoadStringA
#define FormatMessageA recon_FormatMessageA
#define GetModuleHandleA recon_GetModuleHandleA
#define LoadLibraryA recon_LoadLibraryA
#define FreeLibrary recon_FreeLibrary
#define GetFileAttributesA recon_GetFileAttributesA
#define GetProcAddress recon_GetProcAddress
#define DisableThreadLibraryCalls recon_DisableThreadLibraryCalls
#define CreateFileMappingA recon_CreateFileMappingA
#define MapViewOfFile recon_MapViewOfFile
#define MapViewOfFileEx recon_MapViewOfFileEx
#define UnmapViewOfFile recon_UnmapViewOfFile
#define SetConsoleCtrlHandler recon_SetConsoleCtrlHandler
#define LdrProcessRelocationBlock recon_LdrProcessRelocationBlock
#define SearchPathA recon_SearchPathA
#define GetFullPathNameA recon_GetFullPathNameA
#define GetSystemInfo recon_GetSystemInfo
#define GlobalMemoryStatus recon_GlobalMemoryStatus
#define GetTickCount recon_GetTickCount
#define GetModuleFileNameA recon_GetModuleFileNameA
#define GetCommandLineA recon_GetCommandLineA
#define GetTempPathA recon_GetTempPathA
#define GetTempFileNameA recon_GetTempFileNameA
#define SetEnvironmentVariableA recon_SetEnvironmentVariableA
#define GetVersion recon_GetVersion
#define SystemTimeToFileTime recon_SystemTimeToFileTime
#define SetFileTime recon_SetFileTime
#define GetSystemTime recon_GetSystemTime
#define ExitProcess recon_ExitProcess
#define DebugBreak recon_DebugBreak
#define VirtualAlloc recon_VirtualAlloc
#define VirtualFree recon_VirtualFree
#define LocalAlloc recon_LocalAlloc
#define LocalFree recon_LocalFree
#define GlobalAlloc recon_GlobalAlloc
#define GlobalFree recon_GlobalFree
#define MultiByteToWideChar recon_MultiByteToWideChar
#define WideCharToMultiByte recon_WideCharToMultiByte
#define LCMapStringW recon_LCMapStringW
#define wsprintfA recon_wsprintfA
#define HeapCreate recon_HeapCreate
#define HeapAlloc recon_HeapAlloc
#define HeapReAlloc recon_HeapReAlloc
#define HeapFree recon_HeapFree
#define SendMessageA recon_SendMessageA
#define HeapDestroy recon_HeapDestroy
#define IsDBCSLeadByteEx recon_IsDBCSLeadByteEx
#define IsValidCodePage recon_IsValidCodePage
#define GetACP recon_GetACP
#define CharNextA recon_CharNextA
#define GetConsoleCP recon_GetConsoleCP
#define GetCPInfo recon_GetCPInfo
#define SetHandleCount recon_SetHandleCount
#define GetConsoleScreenBufferInfo recon_GetConsoleScreenBufferInfo
#define GetLocalTime recon_GetLocalTime
#define RtlAnsiStringToUnicodeString recon_RtlAnsiStringToUnicodeString
#define RtlUnicodeStringToAnsiString recon_RtlUnicodeStringToAnsiString
#undef toupper
#define toupper recon_toupper
#undef tolower
#define tolower recon_tolower
#define _strdup strdup
#define _stricmp strcasecmp
#define _strcmpi strcasecmp
#define _strnicmp strncasecmp
#define _iob recon_iob
#define __mb_cur_max recon_mb_cur_max
#define __tzset recon_tzset
#define _tzset recon_tzset
#undef _IOFBF
#undef _IOLBF
#undef _IONBF
#if defined(RECON_CRT_PROFILE_WATCOM_100A)
#define _IOFBF 0x0100
#define _IOLBF 0x0200
#define _IONBF 0x0400
#else
#define _IOFBF 0
#define _IOLBF 0x40
#define _IONBF 4
#endif
#endif /* !RECON_SHIM_IMPL */
#endif /* RECON_NATIVE_REMAPS_H */
