#ifndef RECON_SHIMS_H
#define RECON_SHIMS_H
#include <trace-abi.h>
/* VC5 expresses this convention implicitly on C++ members. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define RECON_THISCALL __attribute__((thiscall))
/* Use only for an independently proved 8-byte EDX:EAX return ABI. The
 * return object is addressable; sizeof checks never read/initialize its fields. */
#define RECON_RETURN8_TYPE(type) uint64_t
#define RECON_RETURN8(value) do { \
    uint64_t _recon_return8_bits; \
    typedef char _recon_return8_size[(sizeof(value)==8)?1:-1] __attribute__((unused)); \
    __builtin_memcpy(&_recon_return8_bits,&(value),8); \
    return _recon_return8_bits; \
} while (0)
#define RECON_ASSIGN_RETURN8(destination,call) do { \
    uint64_t _recon_return8_bits=(call); \
    typedef char _recon_return8_size[(sizeof(destination)==8)?1:-1] __attribute__((unused)); \
    __builtin_memcpy(&(destination),&_recon_return8_bits,8); \
} while (0)
#else
#define RECON_THISCALL
#define RECON_RETURN8_TYPE(type) type
#define RECON_RETURN8(value) return (value)
#define RECON_ASSIGN_RETURN8(destination,call) ((destination)=(call))
#endif
/* Native remaps are disabled in canonical Win32. Include last. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
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
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <signal.h>
#include <float.h>
#include <sys/timeb.h>
#include <win32.h>
#ifdef __cplusplus
extern "C" {
#endif

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
const ReconModuleSpec *recon_module_identity(HMODULE);
BOOL recon_module_thread_calls_disabled(HMODULE);

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
extern unsigned short *recon_pctype;
extern char *recon_pgmptr,*recon_aenvptr;
extern long recon_timezone;
extern int recon_daylight;
extern char *recon_tzname[2];
extern int recon_days[13],recon_fltused,recon_tmpoff;
extern int recon_adjust_fdiv;
typedef void (__cdecl *ReconInitializer)(void);
void recon_initterm(ReconInitializer *,ReconInitializer *);
struct _startupinfo;
struct _exception;
extern int recon_crt_app_type,recon_crt_newmode,recon_crt_argc;
extern char **recon_crt_argv,**recon_crt_environment,**recon_crt_initenv;
extern int (__cdecl *recon_crt_matherr)(struct _exception *);
void recon_crt_bind_arguments(int,char **,char **);
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
int recon_setvbuf(ShimFILE *,char *,int,size_t);
int recon_fgetc(ShimFILE *),recon_fputc(int,ShimFILE *);
char *recon_fgets(char *,int,ShimFILE *);
int recon_fputs(const char *,ShimFILE *),recon_puts(const char *);
int recon_fprintf(ShimFILE *,const char *,...),recon_printf(const char *,...);
int recon_vfprintf(ShimFILE *,const char *,va_list);
int recon_vprintf(const char *,va_list);
int recon_snprintf(char *,size_t,const char *,...);
int recon_mbtowc(WCHAR *,const char *,size_t);
int recon_sscanf(const char *,const char *,...);
void recon_qsort(void *,size_t,size_t,int (*)(const void *,const void *));
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
char *recon_mktemp(char *),*recon_tempnam(const char *,const char *);
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
size_t recon_wcslen(const WCHAR *);
WCHAR *recon_wcscat(WCHAR *,const WCHAR *);
WCHAR *recon_wcsdup(const WCHAR *);
long recon_wcstol(const WCHAR *,WCHAR **,int);
int recon_swprintf(WCHAR *,const WCHAR *,...);
int recon_iswctype(WCHAR,WCHAR);
WCHAR recon_towupper(WCHAR);
WCHAR *recon_wcscpy(WCHAR *,const WCHAR *),*recon_wcsncpy(WCHAR *,const WCHAR *,size_t);
unsigned recon_control87(unsigned,unsigned),recon_controlfp(unsigned,unsigned);
void recon_ftime(struct _timeb *);
typedef void (*ReconSignalHandler)(int);
ReconSignalHandler recon_signal(int,ReconSignalHandler);
int recon_isctype(int,int),recon_toupper(int),recon_tolower(int);
void recon_tzset(void),recon_endstdio(void),recon_rmtmp(void);
int recon_isindst(struct tm *);
int recon_mbsncmp(const unsigned char *,const unsigned char *,size_t);
int recon_mbsnbicmp(const unsigned char *,const unsigned char *,size_t);
unsigned char *recon_mbspbrk(const unsigned char *,const unsigned char *);
unsigned char *recon_mbsrchr(const unsigned char *,unsigned int);
unsigned char *recon_mbsupr(unsigned char *);
unsigned char *recon_mbsstr(const unsigned char *,const unsigned char *);
size_t recon_mbclen(const unsigned char *);
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
time_t recon_time(time_t *);
void recon_assert(void *,void *,unsigned);
void recon_exit(int);
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
void recon_unsupported(const char *);

int __cdecl recon_wsprintfA(LPSTR,LPCSTR,...);
typedef struct ReconModuleMemorySpec {
 const char *sha256; DWORD original_rva,size,original_image_base,original_va;
} ReconModuleMemorySpec;
void recon_register_module_memory(HMODULE,const ReconModuleMemorySpec *,void *,size_t);
void recon_load_module_resources(HMODULE,const char *);
void recon_load_system_profile(const char *,const char *);

#define RECON_WIN32_FUNCTIONS(X) \
 X(HANDLE,CreateFileA,(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)) \
 X(BOOL,CloseHandle,(HANDLE)) X(DWORD,GetLastError,(void)) X(void,SetLastError,(DWORD)) \
 X(DWORD,GetFileSize,(HANDLE,LPDWORD)) X(BOOL,FlushViewOfFile,(LPCVOID,DWORD)) \
 X(DWORD,GetFileType,(HANDLE)) X(DWORD,SetFilePointer,(HANDLE,LONG,LONG *,DWORD)) \
 X(BOOL,SetEndOfFile,(HANDLE)) X(BOOL,ReadFile,(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID)) \
 X(BOOL,WriteFile,(HANDLE,LPCVOID,DWORD,LPDWORD,LPVOID)) X(BOOL,FlushFileBuffers,(HANDLE)) \
 X(HANDLE,GetStdHandle,(DWORD)) X(BOOL,SetStdHandle,(DWORD,HANDLE)) \
 X(int,LoadStringA,(HINSTANCE,UINT,LPSTR,int)) \
 X(DWORD,FormatMessageA,(DWORD,LPCVOID,DWORD,DWORD,LPSTR,DWORD,va_list *)) \
 X(HMODULE,GetModuleHandleA,(LPCSTR)) X(HMODULE,LoadLibraryA,(LPCSTR)) X(BOOL,FreeLibrary,(HMODULE)) \
 X(FARPROC,GetProcAddress,(HMODULE,LPCSTR)) X(BOOL,DisableThreadLibraryCalls,(HMODULE)) \
 X(HANDLE,CreateFileMappingA,(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCSTR)) \
 X(LPVOID,MapViewOfFile,(HANDLE,DWORD,DWORD,DWORD,DWORD)) X(LPVOID,MapViewOfFileEx,(HANDLE,DWORD,DWORD,DWORD,DWORD,LPVOID)) X(BOOL,UnmapViewOfFile,(LPCVOID)) \
 X(BOOL,SetConsoleCtrlHandler,(BOOL (WINAPI *)(DWORD),BOOL)) \
 X(USHORT *,LdrProcessRelocationBlock,(LPVOID,DWORD,USHORT *,LONG)) \
 X(DWORD,SearchPathA,(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR *)) \
 X(DWORD,GetFullPathNameA,(LPCSTR,DWORD,LPSTR,LPSTR *)) \
 X(void,GetSystemInfo,(LPSYSTEM_INFO)) X(void,GlobalMemoryStatus,(LPMEMORYSTATUS)) \
 X(DWORD,GetTickCount,(void)) \
 X(DWORD,GetModuleFileNameA,(HMODULE,LPSTR,DWORD)) X(DWORD,GetVersion,(void)) \
 X(BOOL,SystemTimeToFileTime,(const SYSTEMTIME *,FILETIME *)) \
 X(BOOL,SetFileTime,(HANDLE,const FILETIME *,const FILETIME *,const FILETIME *)) \
 X(void,GetSystemTime,(SYSTEMTIME *)) X(void,ExitProcess,(UINT)) \
 X(LPVOID,VirtualAlloc,(LPVOID,DWORD,DWORD,DWORD)) X(BOOL,VirtualFree,(LPVOID,DWORD,DWORD)) \
 X(HLOCAL,LocalAlloc,(UINT,DWORD)) X(HLOCAL,LocalFree,(HLOCAL)) \
 X(HGLOBAL,GlobalAlloc,(UINT,DWORD)) X(HGLOBAL,GlobalFree,(HGLOBAL)) \
 X(int,MultiByteToWideChar,(UINT,DWORD,LPCSTR,int,LPWSTR,int)) \
 X(int,WideCharToMultiByte,(UINT,DWORD,LPCWSTR,int,LPSTR,int,LPCSTR,BOOL *)) \
 X(int,LCMapStringW,(DWORD,DWORD,LPCWSTR,int,LPWSTR,int)) \
 X(HANDLE,HeapCreate,(DWORD,DWORD,DWORD)) X(LPVOID,HeapAlloc,(HANDLE,DWORD,DWORD)) \
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
#define wcscmp recon_wcscmp
#define wcslen recon_wcslen
#define wcscat recon_wcscat
#define _wcsdup recon_wcsdup
#define wcstol recon_wcstol
#define swprintf recon_swprintf
#define iswctype recon_iswctype
#define towupper recon_towupper
#define wcscpy recon_wcscpy
#define wcsncpy recon_wcsncpy
#define _control87 recon_control87
#define _controlfp recon_controlfp
#define _ftime recon_ftime
#define signal recon_signal
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
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define fclose recon_fclose
#define fflush recon_fflush
#define fread recon_fread
#define fwrite recon_fwrite
#define fseek recon_fseek
#define ftell recon_ftell
#define setvbuf recon_setvbuf
#define fgetc recon_fgetc
#define fputc recon_fputc
#define fgets recon_fgets
#define fputs recon_fputs
#define puts recon_puts
#define fprintf recon_fprintf
#define printf recon_printf
#define vfprintf recon_vfprintf
#define vprintf recon_vprintf
#define _snprintf recon_snprintf
#define mbtowc recon_mbtowc
#define sscanf recon_sscanf
#define qsort recon_qsort
#define ungetc recon_ungetc
#define ferror recon_ferror
#define feof recon_feof
#define clearerr recon_clearerr
#define rewind recon_rewind
#define fileno recon_fileno
#define remove recon_remove
#define rename recon_rename
#define time(...) recon_time(__VA_ARGS__)
#define _onexit recon_onexit
#define __dllonexit recon_dllonexit
#define _assert recon_assert
#define exit recon_exit
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
#define _splitpath recon_splitpath
#define _makepath recon_makepath
#define _fullpath recon_fullpath
#define _getcwd recon_getcwd
#define _strupr recon_strupr
#define _strlwr recon_strlwr
#define _ultoa recon_ultoa
#define _itoa recon_itoa
#define _mktemp recon_mktemp
#define _tempnam recon_tempnam
#define _getpath recon_getpath
#define _searchenv recon_searchenv
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
#define _mbspbrk recon_mbspbrk
#define _mbsrchr recon_mbsrchr
#define _mbsupr recon_mbsupr
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
#define free recon_free
#define realloc recon_realloc
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
#define CreateFileA recon_CreateFileA
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
#define GetVersion recon_GetVersion
#define SystemTimeToFileTime recon_SystemTimeToFileTime
#define SetFileTime recon_SetFileTime
#define GetSystemTime recon_GetSystemTime
#define ExitProcess recon_ExitProcess
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
#define _IOFBF 0
#define _IOLBF 0x40
#define _IONBF 4
#endif /* !RECON_SHIM_IMPL */
#endif /* Linux native test build */
#endif
