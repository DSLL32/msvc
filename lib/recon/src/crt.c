#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <elf.h>
#include <dirent.h>
#include <sys/mman.h>
#include <malloc.h>
#include "native-files.h"
#include "native-entry.h"

/* Reconstructed CRT overrides in a target take precedence over these fallbacks. */
#define WEAK __attribute__((weak))
/* The old CRT copies forward. CL also uses it to remove an escape byte with
 * strcpy(p,p+1); host vectorized strcpy need not preserve that behavior. */
WEAK char *recon_strcpy(char *destination,const char *source)
{
    char *result=destination;
    while ((*destination++=*source++)!=0) {}
    return result;
}
/* VC5 MSVCRT compares unsigned bytes and normalizes every mismatch. The
 * complete canonical leaf has no locale, error, allocation or FP effects. */
WEAK int __cdecl recon_strcmp(const char *left,const char *right)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    const unsigned char *a=(const unsigned char *)left,*b=(const unsigned char *)right;
    if (!a || !b) recon_unsupported("strcmp null input");
    while (*a==*b) {
        if (!*a) return 0;
        ++a;++b;
    }
    return *a<*b?-1:1;
#else
    (void)left;(void)right;recon_unsupported("strcmp requires MSVCRT510 profile");return 0;
#endif
}
/* The Win32 CRT writes a 16-bit wchar_t. The C locale maps all unsigned
 * bytes directly; source==NULL or n==0 is a no-op, not a host EILSEQ. */
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void mbtowc_1252_scope(WCHAR *,const char *,size_t,unsigned char);
#endif
WEAK int recon_mbtowc(WCHAR *destination,const char *source,size_t count)
{
    unsigned char value;
    if (!source || !count) return 0;
    value=(unsigned char)*source;
    if (value && recon_lc_handle[2]) {
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        mbtowc_1252_scope(destination,source,count,value);
#else
        recon_unsupported("mbtowc nondefault LC_CTYPE");
#endif
    }
    if (destination) *destination=value;
    return value?1:0;
}
ShimFILE recon_iob[20];
#ifdef RECON_CRT_PROFILE_WATCOM_100A
/* Native semantic state, separate from either CRT's private FILE flag layout. */
static char *watcom_stdout_line_buffer;
static size_t watcom_stdout_line_size;
#endif
#ifdef RECON_CRT_PROFILE_MSVCRT_510
/* Canonical provider .data points stdin at its static4096-byte buffer.
 * This is user-buffer ownership: fclose/freebuf must never heap-free it. */
static char recon_stdin_buffer[4096];
#endif
WEAK unsigned char recon_osfile[256];
WEAK unsigned recon_nhandle=256;
#ifndef RECON_DEFAULT_FMODE
#define RECON_DEFAULT_FMODE 0x4000
#endif
int recon_fmode=RECON_DEFAULT_FMODE,recon_commode,recon_cflush,recon_umaskval;
unsigned long recon_doserrno,recon_lc_handle[6];
unsigned recon_lc_codepage;
int recon_mb_cur_max=1,recon_mb_codepage;
/* Separate internal gate used by the canonical public _getmbcp. No public
 * getter/selection semantics are inferred merely from this observed value. */
unsigned recon_mb_codepage_gate;
static unsigned short character_flags[257];
unsigned short *recon_pctype=character_flags+1;
unsigned char recon_mbctype[257];
char *recon_pgmptr,*recon_aenvptr;
long recon_timezone;
int recon_daylight;
static char zone_names[2][32]={"PST","PDT"};
char *recon_tzname[2]={zone_names[0],zone_names[1]};
int recon_days[13]={-1,30,58,89,119,150,180,211,242,272,303,333,364};
int recon_fltused,recon_tmpoff;
WEAK int recon_adjust_fdiv;
int recon_crt_app_type,recon_crt_newmode,recon_crt_argc;
char **recon_crt_argv,**recon_crt_environment,**recon_crt_initenv;
const WCHAR *recon_crt_os_environment;
size_t recon_crt_os_environment_units;
unsigned recon_crt_environment_updates;
ReconCrtEnvironmentVector recon_crt_environment_vectors[3];
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int os_environment_bound;
static const WCHAR *pending_os_environment;
static size_t pending_os_environment_units;
static const WCHAR *initial_os_environment;
static WCHAR *link_os_environment;
static size_t initial_os_environment_units,link_os_environment_units,link_os_insertion;
static const char link_os_assignment[]="LINK_REPRO_NAME=\\deflib.rsp";
static int link_spawn_environment_valid(void);
static char **initial_crt_environment,**owned_crt_environment;
static char environment_update_name[256];
#endif

int (__cdecl *recon_crt_matherr)(struct _exception *);
int recon_crt_arguments_bound;
static int recon_crt_matherr_default;
struct _startupinfo { int newmode; };
WEAK long recon_osfhnd[256];
#define os_handles recon_osfhnd
char *recon_decimal_point=".";
static int pushed_byte[256];
/* Nonzero only for reviewed isolated binary _fsopen/_sopen leases.
 * Values are Win32 share flags plus one, not a general sharing emulator. */
DWORD recon_fsopen_shares[256];
static int fsopen_active;
int recon_fsopen_restricted(HANDLE handle)
{
    unsigned n;
    for (n=0;n<recon_nhandle;++n)
        if ((recon_osfile[n]&1) && recon_fsopen_shares[n] && (HANDLE)os_handles[n]==handle) return 1;
    return 0;
}
void recon_fsopen_check_path(const char *path)
{
    struct stat wanted,opened; unsigned n; int saved_errno=errno;
    if (!stat(path,&wanted)) for (n=0;n<recon_nhandle;++n)
        if ((recon_osfile[n]&1) && recon_fsopen_shares[n] &&
            !fstat((int)os_handles[n]-1,&opened) && wanted.st_dev==opened.st_dev && wanted.st_ino==opened.st_ino)
            recon_unsupported("CreateFile alias of restricted _fsopen file");
    errno=saved_errno;
}
static void fsopen_preflight(const char *name,int readonly_aliases)
{
    char path[PATH_MAX]; struct stat wanted,opened; DIR *directory;
    struct dirent *entry; int saved_errno=errno,fd; char *end;
    if (recon_host_path(name,path,sizeof(path))) recon_unsupported("_fsopen path translation");
    if (!stat(path,&wanted)) {
        if (!S_ISREG(wanted.st_mode)) recon_unsupported("_fsopen nonregular file");
        directory=opendir("/proc/self/fd");
        if (!directory) recon_unsupported("_fsopen descriptor inventory");
        while ((entry=readdir(directory))!=NULL) {
            fd=(int)strtol(entry->d_name,&end,10);
            if (*end || fd==dirfd(directory)) continue;
            if (!fstat(fd,&opened) && wanted.st_dev==opened.st_dev && wanted.st_ino==opened.st_ino &&
                (!readonly_aliases || !recon_fsopen_read_alias(fd)))
                recon_unsupported("_fsopen existing incompatible/unknown inode alias");
        }
        closedir(directory);
    } else if (errno!=ENOENT) recon_unsupported("_fsopen unavailable file identity");
    errno=saved_errno;
}

/* Encountered CP1252 tables. Each byte index uses character+1; index0 is
 * EOF. The mb-table keeps static identity even when the locale changes. */
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static unsigned short initial_ctype_510[257];
static int locale_profile_applied;
static ReconCrtCtypeSelection ctype_selection;
static unsigned short *locale_ctype_510;
static void msvcrt510_mb_flags(unsigned char table[257])
{
    static const unsigned char upper[]={138,140,142,159};
    static const unsigned char lower[]={131,154,156,158,170,181,186};
    unsigned c,n;
    memset(table,0,257);
    /* 10/20 are single-byte uppercase/lowercase flags; lead/trail04/08
     * remain clear throughout this encountered single-byte table. */
    for (c='A';c<='Z';++c) table[c+1]=0x10;
    for (c='a';c<='z';++c) table[c+1]=0x20;
    for (c=192;c<=222;++c) if (c!=215) table[c+1]=0x10;
    for (c=223;c<=255;++c) if (c!=247) table[c+1]=0x20;
    for (n=0;n<sizeof(upper);++n) table[upper[n]+1]=0x10;
    for (n=0;n<sizeof(lower);++n) table[lower[n]+1]=0x20;
}
static void msvcrt510_locale_ctype(const unsigned char mb[257],unsigned short table[257])
{
    enum {UPPER=1,LOWER=2,DIGIT=4,SPACE=8,PUNCT=16,CONTROL=32,
          BLANK=64,HEX=128,ALPHA=256,OBSERVED_NLS_0200=512};
    static const unsigned char punctuation[]={130,132,133,134,135,137,139,
        145,146,147,148,149,150,151,155,161,162,163,164,165,166,167,168,
        169,171,172,174,175,176,177,180,182,183,184,187,188,189,190,191,215,247};
    static const unsigned char controls[]={129,141,143,144,157};
    static const unsigned char numeric_punctuation[]={178,179,185};
    static const unsigned char lower_punctuation[]={170,181,186};
    unsigned c,n,bits;
    table[0]=0;
    for (c=0;c<256;++c) {
        /* 0200 is uniformly present in this captured reference NLS table.
         * Period WINNLS.H defines the other bits; no extra meaning is inferred. */
        bits=OBSERVED_NLS_0200;
        if (mb[c+1]&0x10) bits|=UPPER|ALPHA;
        if (mb[c+1]&0x20) bits|=LOWER|ALPHA;
        if (c<32 || c==127) bits|=CONTROL;
        if (c==32 || c==160 || (c>=9 && c<=13)) bits|=SPACE;
        if (c==32 || c==160 || c==9) bits|=BLANK;
        if (c>='0' && c<='9') bits|=DIGIT|HEX;
        if ((c>='A' && c<='F') || (c>='a' && c<='f')) bits|=HEX;
        if ((c>=33 && c<=47) || (c>=58 && c<=64) ||
            (c>=91 && c<=96) || (c>=123 && c<=126)) bits|=PUNCT;
        table[c+1]=(unsigned short)bits;
    }
    for (n=0;n<sizeof(punctuation);++n) table[punctuation[n]+1]|=PUNCT;
    for (n=0;n<sizeof(controls);++n) table[controls[n]+1]|=CONTROL;
    for (n=0;n<sizeof(numeric_punctuation);++n) table[numeric_punctuation[n]+1]|=DIGIT|PUNCT;
    for (n=0;n<sizeof(lower_punctuation);++n) table[lower_punctuation[n]+1]|=PUNCT;
    table[136+1]|=ALPHA; /* Encountered modifier letter without upper/lower. */
    table[173+1]|=PUNCT|CONTROL; /* Encountered soft-hyphen flags. */
}
#endif
void recon_crt_apply_locale_profile(const ReconCrtLocaleProfile *profile)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned char expected_mb[257]; unsigned short expected_ctype[257],*allocated;
    unsigned n; int saved=errno; unsigned long dos=recon_doserrno; DWORD error=recon_GetLastError();
    if (!profile || !profile->provider_sha256 ||
        strcmp(profile->provider_sha256,"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210") ||
        profile->kind!=RECON_CRT_LOCALE_1252_EN_US || !profile->mbctype || !profile->ctype ||
        profile->mbctype_count!=257 || profile->ctype_count!=257 || locale_profile_applied || ctype_selection.bound ||
        recon_mb_codepage || recon_lc_codepage || recon_mb_cur_max!=1 ||
        recon_pctype!=character_flags+1)
        recon_unsupported("locale profile identity/extent/initial state");
    for (n=0;n<6;++n) if (recon_lc_handle[n])
        recon_unsupported("locale profile nondefault initial handles");
    msvcrt510_mb_flags(expected_mb); msvcrt510_locale_ctype(expected_mb,expected_ctype);
    if (memcmp(recon_mbctype,expected_mb,sizeof(expected_mb)) ||
        memcmp(character_flags,initial_ctype_510,sizeof(character_flags)) ||
        memcmp(profile->mbctype,expected_mb,sizeof(expected_mb)) ||
        memcmp(profile->ctype,expected_ctype,sizeof(expected_ctype)))
        recon_unsupported("locale profile encountered table mismatch");
    allocated=malloc(sizeof(expected_ctype));
    if (!allocated) recon_unsupported("locale profile table allocation failure");
    memcpy(allocated,expected_ctype,sizeof(expected_ctype));
    /* Keep the old static table and the static mb-array alive. The changed
     * pctype allocation persists after the dependency's FreeLibrary. */
    locale_ctype_510=allocated;recon_pctype=allocated+1;
    recon_mb_codepage=1252; recon_lc_codepage=1252; recon_lc_handle[2]=1033;
    locale_profile_applied=1;
    errno=saved; recon_doserrno=dos; recon_SetLastError(error);
#else
    (void)profile;recon_unsupported("locale profile without explicit MSVCRT510 profile");
#endif
}


#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void isolated_ctype_initial_state(void)
{
    unsigned char mb[257];unsigned n;
    if (locale_profile_applied || recon_mb_codepage || recon_mb_codepage_gate ||
        recon_lc_codepage || recon_mb_cur_max!=1 || recon_pctype!=character_flags+1)
        recon_unsupported("isolated LC_CTYPE initial scalar/pointer state");
    for(n=0;n<6;++n) if(recon_lc_handle[n])
        recon_unsupported("isolated LC_CTYPE initial handles");
    msvcrt510_mb_flags(mb);
    if(memcmp(recon_mbctype,mb,sizeof(mb)) ||
       memcmp(character_flags,initial_ctype_510,sizeof(character_flags)))
        recon_unsupported("isolated LC_CTYPE initial tables");
}
#endif
void recon_crt_bind_empty_ctype_1252(const char *provider,const char *reference)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned n;
    if(!provider || strcmp(provider,"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210") ||
       !reference || strlen(reference)!=64 || ctype_selection.bound || ctype_selection.calls || ctype_selection.mb_calls)
        recon_unsupported("isolated LC_CTYPE binding identity/inventory");
    for(n=0;n<64;++n)
        if(!((reference[n]>='0' && reference[n]<='9') || (reference[n]>='a' && reference[n]<='f')))
            recon_unsupported("isolated LC_CTYPE reference SHA256");
    isolated_ctype_initial_state();
    memcpy(ctype_selection.reference_sha256,reference,65);ctype_selection.bound=1;
#else
    (void)provider;(void)reference;
    recon_unsupported("isolated LC_CTYPE requires MSVCRT510 profile");
#endif
}
const ReconCrtCtypeSelection *recon_crt_ctype_selection_state(void)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    return &ctype_selection;
#else
    static const ReconCrtCtypeSelection unbound;
    return &unbound;
#endif
}

/* Canonical VC5 MSVCRT _initterm: unsigned exclusive end, NULL slots skipped,
 * each callback pointer read when reached (earlier callbacks may change it). */
WEAK void recon_initterm(ReconInitializer *begin,ReconInitializer *end)
{
    while ((uintptr_t)begin<(uintptr_t)end) {
        if (*begin) (*begin)();
        ++begin;
    }
}

static int native_entry_deferred_binding;
void recon_crt_bind_arguments(int argc,char **argv,char **environment)
{
    int n;
    if (argc<0 || argc>4096 || !argv || !environment || argv[argc])
        recon_unsupported("invalid bound CRT arguments");
    for (n=0;n<argc;++n) if (!argv[n]) recon_unsupported("missing bound CRT argument");
    for (n=0;environment[n];++n) if (n==16384) recon_unsupported("bound CRT environment too large");
    if (recon_crt_arguments_bound &&
        (argc!=recon_crt_argc || argv!=recon_crt_argv || environment!=recon_crt_environment))
        recon_unsupported("CRT arguments rebound");
    recon_crt_argc=argc; recon_crt_argv=argv; recon_crt_environment=environment;
    recon_crt_arguments_bound=1;
    if(native_entry_deferred_binding) {
        recon_crt_initenv=environment;native_entry_deferred_binding=0;
    }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(pending_os_environment) {
        recon_crt_bind_os_environment(pending_os_environment,pending_os_environment_units,
            "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210");
        pending_os_environment=NULL;pending_os_environment_units=0;
    }
#endif
}

/* Ordinary process inputs have no fixture-provider identity. Keep the initial
 * OS block separate from the writable CRT vector, even with an empty envp. */
static ReconNativeEntry *native_entry_context;
int recon_native_entry_active(void) { return native_entry_context!=NULL; }
static const WCHAR native_cp1252_high[32]={
 0x20ac,0x81,0x201a,0x192,0x201e,0x2026,0x2020,0x2021,
 0x2c6,0x2030,0x160,0x2039,0x152,0x8d,0x17d,0x8f,
 0x90,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
 0x2dc,0x2122,0x161,0x203a,0x153,0x9d,0x17e,0x178
};
static unsigned native_environment_byte(WCHAR unit)
{
    unsigned n;
    if(unit<128 || (unit>=160 && unit<=255))return unit;
    for(n=0;n<32;++n)if(native_cp1252_high[n]==unit)return n+128;
    recon_unsupported("ordinary OS environment outside native ACP1252");return 0;
}
static char **native_copy_vector(char **input,unsigned limit,unsigned *count,size_t *bytes)
{
    size_t total=0,length;unsigned n;char **result,*cursor;
    if(!input)recon_unsupported("ordinary startup NULL vector");
    for(n=0;input[n];++n) {
        if(n==limit || (length=strnlen(input[n],32767))==32767 || total>32767-length-1)
            recon_unsupported("ordinary startup vector extent");
        total+=length+1;
    }
    result=malloc((n+1)*sizeof(*result)+total);
    if(!result)recon_unsupported("ordinary startup vector allocation");
    cursor=(char *)(result+n+1);
    for(n=0;input[n];++n) {result[n]=cursor;length=strlen(input[n])+1;memcpy(cursor,input[n],length);cursor+=length;}
    result[n]=NULL;*count=n;*bytes=total;return result;
}
static char *native_windows_path(const char *host)
{
    size_t n,length=strlen(host);char *result;
    if(!length || host[0]!='/' || length>32764)recon_unsupported("ordinary startup absolute path");
    result=malloc(length+3);if(!result)recon_unsupported("ordinary startup path allocation");
    result[0]='Z';result[1]=':';
    for(n=0;n<=length;++n) {
        unsigned char byte=(unsigned char)host[n];
        if(byte>=128 || (byte && byte<32))recon_unsupported("ordinary startup nonASCII path");
        result[n+2]=byte=='/'?'\\':byte;
    }
    return result;
}
/* Serialize direct POSIX argv losslessly; only a real Windows spawn supplies
 * a raw joined line to be tokenized (added by the separate process transport). */
static char *native_quote_arguments(int argc,char **argv)
{
    char *result=malloc(32767),*out=result;int i,quote;const char *p;size_t slashes,n;
    if(!result)recon_unsupported("ordinary command line allocation");
#define NATIVE_COMMAND_BYTE(ch) do {if(out-result>=32766)recon_unsupported("ordinary command line length");*out++=(ch);} while(0)
    for(i=0;i<argc;++i) {
        if(i)NATIVE_COMMAND_BYTE(' ');
        for(p=argv[i];*p;++p)if((unsigned char)*p>=128 || (!i && *p=='"'))
            recon_unsupported("ordinary command line ASCII/program quote");
        quote=!*argv[i] || strpbrk(argv[i]," \t\"")!=NULL;
        if(quote)NATIVE_COMMAND_BYTE('"');
        for(p=argv[i];*p;) {
            slashes=0;while(*p=='\\'){++slashes;++p;}
            n=slashes;
            if(*p=='"' || (!*p && quote))n*=2;
            while(n--)NATIVE_COMMAND_BYTE('\\');
            if(*p=='"')NATIVE_COMMAND_BYTE('\\');
            if(*p)NATIVE_COMMAND_BYTE(*p++);
        }
        if(quote)NATIVE_COMMAND_BYTE('"');
    }
    *out=0;
#undef NATIVE_COMMAND_BYTE
    return result;
}
char **recon_native_entry_spawn_environment(void)
{
    const WCHAR *block=recon_crt_os_environment;size_t units=recon_crt_os_environment_units,n,count=0;
    char **result,*bytes;
    if(!native_entry_context || !block || units<2 || units>32767 || block[units-1] || block[units-2])
        recon_unsupported("ordinary spawn OS environment extent");
    if(!block[0]) {
        if(units!=2)recon_unsupported("ordinary empty OS environment extent");
    } else for(n=0;n<units-1;++n) {
        if(!block[n]) {
            ++count;
            if(!block[n+1] && n!=units-2)recon_unsupported("ordinary OS environment early terminator");
        } else (void)native_environment_byte(block[n]);
    }
    result=malloc((count+1)*sizeof(*result)+units);
    if(!result)recon_unsupported("ordinary spawn environment allocation");
    bytes=(char *)(result+count+1);count=0;
    if(block[0])result[count++]=bytes;
    for(n=0;n<units;++n) {
        bytes[n]=(char)native_environment_byte(block[n]);
        if(!block[n] && n<units-2)result[count++]=bytes+n+1;
    }
    result[count]=NULL;return result;
}
static char *native_take_command_line(void)
{
    static const char expected[]="/memfd:" RECON_NATIVE_COMMAND_MEMFD " (deleted)";
    char name[128],*line;struct stat info;ssize_t length,got;size_t offset=0,n;
    int seals,failed=0;
    if(fcntl(RECON_NATIVE_COMMAND_FD,F_GETFD)<0)return NULL;
    length=readlink("/proc/self/fd/198",name,sizeof(name)-1);
    if(length<0 || length>=(ssize_t)sizeof(name)-1)return NULL;
    name[length]=0;if(strcmp(name,expected))return NULL; /* unrelated fd: leave it owned by caller */
    seals=fcntl(RECON_NATIVE_COMMAND_FD,F_GET_SEALS);
    if(fstat(RECON_NATIVE_COMMAND_FD,&info) || !S_ISREG(info.st_mode) || info.st_size<2 || info.st_size>32767 ||
       seals<0 || (seals&(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))!=(F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))
        failed=1;
    line=failed?NULL:malloc(info.st_size);
    if(!failed && !line)failed=1;
    while(!failed && offset<(size_t)info.st_size) {
        got=pread(RECON_NATIVE_COMMAND_FD,line+offset,info.st_size-offset,offset);
        if(got<0 && errno==EINTR)continue;
        if(got<=0){failed=1;break;}offset+=got;
    }
    if(close(RECON_NATIVE_COMMAND_FD))failed=1;
    if(failed)recon_unsupported("invalid native command-line transport");
    if(line[info.st_size-1] || !line[0])recon_unsupported("native command-line terminator");
    for(n=0;n<(size_t)info.st_size-1;++n)
        if(!line[n] || (unsigned char)line[n]>=128 || ((unsigned char)line[n]<32 && line[n]!='\t'))
            recon_unsupported("native command-line ASCII extent");
    return line;
}
/* Old single-byte CRT argument grammar: argv0 has quote grouping; later
 * arguments use even/odd backslashes before quotes. No shell expansion. */
static char **native_parse_command_line(const char *line,int *argc)
{
    size_t bytes=strlen(line)+1,slashes,n;char **arguments,*out;const char *p=line;
    int quoted=0,count=0,copy;
    arguments=malloc(4097*sizeof(*arguments)+bytes);
    if(!arguments)recon_unsupported("native command-line argument allocation");
    out=(char *)(arguments+4097);arguments[count++]=out;
    if(*p=='"') {
        ++p;while(*p && *p!='"')*out++=*p++;
        if(*p=='"')++p;
    } else while(*p && (unsigned char)*p>' ')*out++=*p++;
    *out++=0;
    for(;;) {
        while(*p==' ' || *p=='\t')++p;
        if(!*p)break;
        if(count==4096)recon_unsupported("native command-line argument count");
        arguments[count++]=out;quoted=0;
        for(;;) {
            slashes=0;copy=1;while(*p=='\\'){++slashes;++p;}
            if(*p=='"') {
                if(!(slashes&1)) {
                    if(quoted && p[1]=='"')++p;
                    else copy=0;
                    /* The reached old CRT toggles even for doubled quotes. */
                    quoted=!quoted;
                }
                slashes/=2;
            }
            for(n=0;n<slashes;++n)*out++='\\';
            if(!*p || (!quoted && (*p==' ' || *p=='\t')))break;
            if(copy)*out++=*p;
            ++p;
        }
        *out++=0;
    }
    arguments[count]=NULL;*argc=count;return arguments;
}
static ReconNativeEntry *native_entry_inputs(int argc,char **argv,char **environment,int bind_crt)
{
    ReconNativeEntry *entry;char program[PATH_MAX],*cwd,*raw_environment,*cursor,*raw_command;
    char **arguments,**variables;WCHAR *os_block;ssize_t length;
    unsigned count,n;size_t argument_bytes,environment_bytes,units,k;
    if(native_entry_context || recon_crt_arguments_bound || recon_crt_os_environment ||
       argc<1 || argc>4096 || !argv || argv[argc] || recon_crt_environment_updates)
        recon_unsupported("ordinary startup state/arguments");
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(pending_os_environment || os_environment_bound)
        recon_unsupported("ordinary startup mixed with fixture OS binding");
#endif
    raw_command=native_take_command_line();
    if(raw_command)arguments=native_parse_command_line(raw_command,&argc);
    else {
        arguments=native_copy_vector(argv,4096,&count,&argument_bytes);
        if(count!=(unsigned)argc)recon_unsupported("ordinary startup argument count");
    }
    variables=native_copy_vector(environment?environment:environ,16384,&count,&environment_bytes);
    units=environment_bytes+1;if(!count)units=2;
    if(units>32767)recon_unsupported("ordinary startup environment length");
    os_block=calloc(units,sizeof(*os_block));raw_environment=calloc(units,1);
    entry=calloc(1,sizeof(*entry));
    if(!os_block || !raw_environment || !entry)recon_unsupported("ordinary startup allocation");
    cursor=raw_environment;
    for(n=0;n<count;++n) {size_t bytes=strlen(variables[n])+1;memcpy(cursor,variables[n],bytes);cursor+=bytes;}
    for(k=0;k<units;++k) {unsigned char byte=raw_environment[k];os_block[k]=byte>=128 && byte<160?native_cp1252_high[byte-128]:byte;}
    length=readlink("/proc/self/exe",program,sizeof(program)-1);
    if(length<=0 || length==(ssize_t)sizeof(program)-1 || !(cwd=getcwd(NULL,0)))
        recon_unsupported("ordinary startup actual executable/cwd");
    program[length]=0;entry->program_path=native_windows_path(program);
    entry->current_directory=native_windows_path(cwd);free(cwd);
    entry->command_line=raw_command?raw_command:native_quote_arguments(argc,arguments);
    entry->argc=argc;entry->argv=arguments;entry->environment=variables;
    native_entry_context=entry;
    if(bind_crt) {recon_crt_bind_arguments(argc,arguments,variables);recon_crt_initenv=variables;}
    else native_entry_deferred_binding=1;
    recon_crt_os_environment=os_block;recon_crt_os_environment_units=units;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    /* These are the actual owned startup namespaces, not a fixture provider
     * binding. Preserve the CRT snapshot while an API derives the OS block. */
    initial_os_environment=os_block;initial_os_environment_units=units;
    initial_crt_environment=variables;
    recon_crt_environment_vectors[0].vector=variables;
    recon_crt_environment_vectors[0].count=count;
    recon_crt_environment_vectors[0].borrowed=1;
    recon_crt_environment_vectors[0].live=1;os_environment_bound=1;
#endif
    recon_pgmptr=(char *)entry->program_path;recon_aenvptr=raw_environment;
    recon_bind_command_line((char *)entry->command_line,strlen(entry->command_line)+1);
    /* Target fresh-process errors are independent of Linux setup syscalls. */
    errno=0;recon_doserrno=0;recon_SetLastError(0);
    return entry;
}

ReconNativeEntry *recon_native_entry_initialize(int argc,char **argv,char **environment)
{ return native_entry_inputs(argc,argv,environment,1); }
ReconNativeEntry *recon_native_entry_prepare(int argc,char **argv,char **environment)
{ return native_entry_inputs(argc,argv,environment,0); }

/* Host controls and application environment are separate namespaces. The
 * canonical getenv returns a borrowed pointer into its mutable environment. */
static void ascii_encountered_locale_scope(void);
char *recon_host_getenv(const char *name) { return getenv(name); }
WEAK int *recon_errno(void) { return &errno; }
WEAK char *recon_getenv(const char *name)
{
    char **slot; size_t length,i; unsigned char a,b;
    if (!recon_crt_arguments_bound) return getenv(name);
    if (!name) return NULL;
    ascii_encountered_locale_scope();
    length=strlen(name);
    for (i=0;i<length;++i) if ((unsigned char)name[i]>=128)
        recon_unsupported("bound getenv nonASCII name");
    for (slot=recon_crt_environment;*slot;++slot) {
        if (strlen(*slot)<=length || (*slot)[length]!='=') continue;
        for (i=0;i<length;++i) {
            a=(unsigned char)(*slot)[i]; b=(unsigned char)name[i];
            if (a>=128 || b>=128) recon_unsupported("bound getenv nonASCII name");
            if (a>='A' && a<='Z') a+='a'-'A';
            if (b>='A' && b<='Z') b+='a'-'A';
            if (a!=b) break;
        }
        if (i==length) {
            if (recon_lc_codepage) {
                const unsigned char *value=(const unsigned char *)*slot+length+1;
                for (;*value;++value) if (*value>=128)
                    recon_unsupported("bound getenv nonASCII encountered-profile value");
            }
            return *slot+length+1;
        }
    }
    return NULL;
}

WEAK void recon_set_app_type(int type) { recon_crt_app_type=type; }
WEAK int *recon_p_fmode(void) { return &recon_fmode; }
WEAK int *recon_p_commode(void) { return &recon_commode; }
WEAK char ***recon_p_initenv(void) { return &recon_crt_initenv; }

WEAK void recon_getmainargs(int *argc,char ***argv,char ***environment,int wildcard,
                            struct _startupinfo *information)
{
    if (!recon_crt_arguments_bound) recon_unsupported("unbound CRT arguments");
    if (wildcard) recon_unsupported("__getmainargs wildcard expansion");
    if (!argc || !argv || !environment || !information) recon_unsupported("invalid __getmainargs output");
    recon_crt_newmode=information->newmode;
    *argc=recon_crt_argc; *argv=recon_crt_argv; *environment=recon_crt_environment;
}

WEAK void recon_setusermatherr(int (__cdecl *callback)(struct _exception *))
{
    recon_crt_matherr_default=0;
    recon_crt_matherr=callback;
}

/* Host proof metadata, not target heap headers. Reused numeric addresses
 * acquire new generations; pointer-only delete cannot distinguish an old
 * dangling alias from a fresh owner at the identical address (the ABA limit). */
ReconCppAllocation *recon_cpp_allocations;
static uint64_t cpp_allocation_generation;
static ReconCppAllocation *live_cpp_allocation(const void *base)
{
    ReconCppAllocation *record;
    for(record=recon_cpp_allocations;record;record=record->next)
        if(record->live && record->base==base)return record;
    return NULL;
}
/* Canonical MSVCRT's C++ new forces _nh_malloc's handler mode to1.
 * Current consumers prove the provider handler remains NULL. Preserve this
 * distinct import boundary; unobserved heap-failure/handler paths fail closed. */
WEAK void *recon_cpp_new(size_t size)
{
    void *result; int saved_errno=errno;
    DWORD saved_error=recon_GetLastError();
    size_t requested=size;
    ReconCppAllocation *record;
    if (size>0xffffffe0UL) return NULL;
    if (!size) size=1;
    size=(size+15)&~(size_t)15;
    result=malloc(size);
    if (!result) recon_unsupported("operator new allocation failure/new-handler path");
    record=malloc(sizeof(*record));
    if(!record || cpp_allocation_generation==UINT64_MAX)
        recon_unsupported("operator new ownership bookkeeping failure");
    record->base=result;record->requested=requested;record->rounded=size;
    record->generation=++cpp_allocation_generation;record->live=1;
    record->next=recon_cpp_allocations;recon_cpp_allocations=record;
    recon_SetLastError(saved_error);errno=saved_errno;
    return result;
}
WEAK void recon_free(void *pointer)
{
    ReconCppAllocation *record=live_cpp_allocation(pointer);
    if(record)record->live=0;
    free(pointer);
}
WEAK void *recon_realloc(void *pointer,size_t size)
{
    ReconCppAllocation *record=live_cpp_allocation(pointer);
    void *result;
    if(record && !size)recon_unsupported("unreviewed tracked-new realloc zero size");
    result=realloc(pointer,size);
    /* Success transfers ownership to realloc even when the base is unchanged.
     * Failure retains the original live allocation. */
    if(result && record)record->live=0;
    return result;
}
WEAK void recon_cpp_delete(void *pointer)
{
    ReconCppAllocation *record;
    int saved_errno;
    DWORD saved_error;
    /* Canonical scalar delete delegates to free, whose NULL path returns
     * without entering the heap. No ownership or public state changes. */
    if(!pointer)return;
    record=live_cpp_allocation(pointer);
    saved_errno=errno;
    saved_error=recon_GetLastError();
    if(!record)recon_unsupported("operator delete without live new ownership");
    record->live=0;free(pointer);
    recon_SetLastError(saved_error);errno=saved_errno;
}

/* Binary rb/deny-write and wb/exclusive files. Explicit510 additionally
 * accepts tracked prior readonly cache handles/views; other aliases deny.
 * HANDLE inheritance remains the explicitly bounded no-child native model. */
WEAK ShimFILE *recon_fsopen(const char *path,const char *mode,int sharing)
{
    int n,saved_errno=errno,readonly_aliases=0; ShimFILE *result;
    if (!path || !mode || !((!strcmp(mode,"rb") && sharing==0x20) ||
        (!strcmp(mode,"wb") && sharing==0x10))) recon_unsupported("unreviewed _fsopen mode/share");
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    readonly_aliases=!strcmp(mode,"rb") && sharing==0x20;
#endif
    if(readonly_aliases)recon_fsopen_check_read_mappings();
    else recon_fsopen_check_mappings();
    fsopen_preflight(path,readonly_aliases);
    for (n=3;n<20;++n) if (!recon_iob[n]._flag) break;
    if (n==20 || fsopen_active) recon_unsupported("_fsopen stream exhaustion/reentry");
    fsopen_active=1;
    result=recon_openfile(path,mode,sharing,&recon_iob[n]);
    fsopen_active=0;
    if (!result) recon_unsupported("_fsopen open failure");
    recon_fsopen_shares[result->_file]=(sharing==0x20?FILE_SHARE_READ:0)+1;
    errno=saved_errno;
    return result;
}

WEAK void recon_assert(void *expression,void *filename,unsigned line)
{
    (void)expression; (void)filename; (void)line;
    recon_unsupported("_assert without reviewed diagnostic/abort contract");
}

void recon_unsupported(const char *operation)
{
    dprintf(2,"recon: unsupported native operation: %s\n",operation);
    _exit(125);
}

/* Opt-in host-only terminal receipt. Fork/exec children inherit the descriptor
 * but cannot publish into their parent's PID-owned slot. */
static uint32_t *terminal_status;
static void __attribute__((constructor(102))) initialize_terminal_status(void)
{
    const char *value=getenv("RECON_STATUS_FD"); char *end; long fd;
    struct stat st; int saved=errno,flags;
    if (!value) return;
    errno=0; fd=strtol(value,&end,10);
    if (!*value || *end || errno || fd<0 || fd>INT_MAX)
        recon_unsupported("invalid RECON_STATUS_FD");
    flags=fcntl((int)fd,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)!=O_RDWR || fstat((int)fd,&st)<0 || !S_ISREG(st.st_mode) || st.st_size!=24)
        recon_unsupported("terminal status descriptor");
    terminal_status=mmap(NULL,24,PROT_READ|PROT_WRITE,MAP_SHARED,(int)fd,0);
    if (terminal_status==MAP_FAILED) recon_unsupported("terminal status mapping");
    if (!terminal_status[0] && !terminal_status[1] && !terminal_status[2] &&
        !terminal_status[3] && !terminal_status[4] && !terminal_status[5]) {
        terminal_status[0]=0x31535452; terminal_status[1]=1; terminal_status[2]=(uint32_t)getpid();
    } else if (terminal_status[0]!=0x31535452 || terminal_status[1]!=1 || !terminal_status[2] ||
               terminal_status[3]>1 || terminal_status[5]) recon_unsupported("terminal status header");
    errno=saved;
}
void recon_record_terminal_status(uint32_t code)
{
    if (!terminal_status || terminal_status[2]!=(uint32_t)getpid()) return;
    if (terminal_status[3]) recon_unsupported("duplicate terminal status");
    terminal_status[4]=code; __sync_synchronize(); terminal_status[3]=1;
}

static int existing_spelling(char *path,size_t capacity)
{
    char original[PATH_MAX],resolved[PATH_MAX],*part,*save;
    size_t used=0;
    if (strlen(path)>=sizeof(original)) { errno=ENAMETOOLONG; return -1; }
    strcpy(original,path);
    if (path[0]=='/') resolved[used++]='/';
    resolved[used]=0;
    for (part=strtok_r(original,"/",&save);part;part=strtok_r(NULL,"/",&save)) {
        struct stat st;
        size_t prefix=used,length=strlen(part);
        DIR *directory; struct dirent *entry; char match[NAME_MAX+1]; int found=0;
        if (used && resolved[used-1]!='/') resolved[used++]='/';
        if (used+length+1>sizeof(resolved)) { errno=ENAMETOOLONG; return -1; }
        strcpy(resolved+used,part);
        if (lstat(resolved,&st)<0 && errno==ENOENT) {
            char parent[PATH_MAX];
            memcpy(parent,resolved,prefix); parent[prefix]=0;
            directory=opendir(prefix?parent:".");
            if (directory) {
                while ((entry=readdir(directory))!=NULL) if (!strcasecmp(entry->d_name,part)) {
                    if (found) { closedir(directory); errno=EEXIST; return -1; }
                    strcpy(match,entry->d_name); found=1;
                }
                closedir(directory);
                if (found) { strcpy(resolved+used,match); length=strlen(match); }
            }
        }
        used+=length;
    }
    if (used+1>capacity) { errno=ENAMETOOLONG; return -1; }
    strcpy(path,resolved);
    return 0;
}

int recon_host_path(const char *name,char *output,size_t capacity)
{
    char variable[]="RECON_DRIVE_Z";
    const char *root="",*rest=name;
    size_t n=0;
    if (!name || !capacity) { errno=EINVAL; return -1; }
    if (!strncasecmp(name,"\\\\.\\pipe\\",9)) {
        root=getenv("RECON_NAMED_PIPE_ROOT");
        if (!root || !*root) { errno=ENOENT; return -1; }
        rest=name+9;
        if (!*rest || strchr(rest,'/') || strchr(rest,'\\')) { errno=EINVAL; return -1; }
        if (strlen(root)+2>=capacity) { errno=ENAMETOOLONG; return -1; }
        strcpy(output,root); n=strlen(root);
        if (n && output[n-1]!='/') output[n++]='/';
    } else if (name[0] && name[1]==':') {
        unsigned char drive=(unsigned char)toupper((unsigned char)name[0]);
        if (drive<'A' || drive>'Z') { errno=EINVAL; return -1; }
        variable[12]=(char)drive;
        root=recon_native_entry_active()?NULL:getenv(variable);
        if (recon_native_entry_active() && drive!='Z') {errno=ENOTSUP;return -1;}
        if (!root && drive=='Z') root="/";
        if (!root) { errno=ENOENT; return -1; }
        rest=name+2;
        /* Drive-relative paths need per-drive CWD state, not a guessed root. */
        if (*rest && *rest!='/' && *rest!='\\') { errno=ENOTSUP; return -1; }
        if (strlen(root)+2>=capacity) { errno=ENAMETOOLONG; return -1; }
        strcpy(output,root); n=strlen(root);
        if (n && output[n-1]!='/') output[n++]='/';
        while (*rest=='/' || *rest=='\\') ++rest;
    }
    while (*rest) {
        if (n+1>=capacity) { errno=ENAMETOOLONG; return -1; }
        output[n++]=*rest=='\\'?'/':*rest; ++rest;
    }
    output[n]=0;
    /* Resolve existing components with Windows' case-insensitive spelling;
     * leave nonexistent output names as requested by the application. */
    return existing_spelling(output,capacity);
}

void recon_dosmaperr(unsigned long code)
{
    recon_doserrno=code;
    switch (code) {
    case 2: case 3: errno=ENOENT; break;
    case 5: case 32: case 33: errno=EACCES; break;
    case 6: errno=EBADF; break;
    case 131: errno=EINVAL; break;
    case 8: errno=ENOMEM; break;
    case 80: case 183: errno=EEXIST; break;
    case 87: errno=EINVAL; break;
    case 112: errno=ENOSPC; break;
    default: errno=EIO; break;
    }
}
int recon_alloc_osfhnd(void)
{
    unsigned n;
    for (n=0;n<recon_nhandle;++n) if (!(recon_osfile[n]&1)) { os_handles[n]=-1; recon_fsopen_shares[n]=0; return n; }
    errno=EMFILE; return -1;
}
int recon_set_osfhnd(int fd,long handle)
{
    if ((unsigned)fd>=recon_nhandle || os_handles[fd]!=-1) { errno=EBADF; recon_doserrno=0; return -1; }
    if (fd<3) recon_SetStdHandle((DWORD)(-10-fd),(HANDLE)handle);
    os_handles[fd]=handle; return 0;
}
int recon_free_osfhnd(int fd)
{
    if ((unsigned)fd>=recon_nhandle || !(recon_osfile[fd]&1) || os_handles[fd]==-1) {
        errno=EBADF; recon_doserrno=0; return -1;
    }
    if (fd<3) recon_SetStdHandle((DWORD)(-10-fd),NULL);
    os_handles[fd]=-1; recon_fsopen_shares[fd]=0; return 0;
}
long recon_get_osfhandle(int fd)
{
    if ((unsigned)fd>=recon_nhandle || !(recon_osfile[fd]&1)) { errno=EBADF; return -1; }
    return os_handles[fd];
}
static int valid_descriptor(int fd)
{
    if ((unsigned)fd<recon_nhandle && (recon_osfile[fd]&1)) return 1;
    errno=EBADF; return 0;
}
/* __find in VC1 _SETARGV.OBJ keeps one search and one 580-byte result buffer.
 * Its callers observe only the filename at byte44. Windows '?' can consume no
 * character at a dot/end; a final pattern dot also matches an extensionless name.
 * Keep this boundary here rather than reproducing CRT-internal Win32 calls. */
static char *find_buffer;
static char **find_names;
static size_t find_count,find_index;
static int find_handle;

int recon_native_find_match(const char *pattern,const char *name)
{
    while (*pattern) {
        if (*pattern=='*') {
            const char *end=name+strlen(name),*dot;
            ++pattern;
            if (!*pattern) return 1;
            if (*pattern=='.' && (dot=strrchr(name,'.'))!=NULL) end=dot;
            for (;;) {
                if (recon_native_find_match(pattern,end)) return 1;
                if (end==name) break;
                --end;
            }
            return 0;
        }
        if (*pattern=='?') {
            if (*name && *name!='.') ++name;
        } else if (*pattern=='.' && !*name) {
            /* A DOS dot can be absent at the end of a filename. */
        } else {
            if (!*name || toupper((unsigned char)*pattern)!=toupper((unsigned char)*name)) return 0;
            ++name;
        }
        ++pattern;
    }
    return !*name;
}

static int find_compare(const void *left,const void *right)
{
    const char *a=*(const char *const *)left,*b=*(const char *const *)right;
    int result=strcasecmp(a,b);
    return result?result:strcmp(a,b);
}

static void find_close(void)
{
    size_t n;
    for (n=0;n<find_count;++n) free(find_names[n]);
    free(find_names); find_names=NULL; find_count=find_index=0;
    if (find_handle!=1) recon_SetLastError(ERROR_INVALID_HANDLE);
    find_handle=0;
}

WEAK char *recon_find(const char *pattern)
{
    int saved_errno=errno;
    if (pattern) {
        char directory[PATH_MAX],host[PATH_MAX],*slash,*backslash;
        const char *mask;
        DIR *stream;
        struct dirent *entry;
        if (pattern[0] && pattern[1]==':' && pattern[2]!='/' && pattern[2]!='\\')
            recon_unsupported("_find drive-relative path");
        if (!find_buffer) {
            find_buffer=malloc(580);
            if (!find_buffer) { recon_SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
        }
        if (find_handle) find_close();
        if (strlen(pattern)>=sizeof(directory)) {
            recon_SetLastError(ERROR_PATH_NOT_FOUND); find_handle=-1; errno=saved_errno; return NULL;
        }
        strcpy(directory,pattern);
        slash=strrchr(directory,'/'); backslash=strrchr(directory,'\\');
        if (backslash && (!slash || backslash>slash)) slash=backslash;
        if (slash) {
            mask=pattern+(slash-directory)+1;
            if (slash==directory) slash[1]=0;
            else *slash=0;
        } else { strcpy(directory,"."); mask=pattern; }
        if (recon_host_path(directory,host,sizeof(host))<0 || !(stream=opendir(host))) {
            recon_SetLastError(errno==EACCES?ERROR_ACCESS_DENIED:ERROR_PATH_NOT_FOUND);
            find_handle=-1; errno=saved_errno; return NULL;
        }
        while ((entry=readdir(stream))!=NULL) if (recon_native_find_match(mask,entry->d_name)) {
            char **grown;
            if (strlen(entry->d_name)>=260) recon_unsupported("_find filename exceeds MAX_PATH");
            grown=realloc(find_names,(find_count+1)*sizeof(*find_names));
            if (!grown) recon_unsupported("_find enumeration allocation");
            find_names=grown;
            find_names[find_count]=strdup(entry->d_name);
            if (!find_names[find_count]) recon_unsupported("_find filename allocation");
            ++find_count;
        }
        closedir(stream);
        if (!find_count) {
            free(find_names); find_names=NULL;
            recon_SetLastError(ERROR_FILE_NOT_FOUND); find_handle=-1; errno=saved_errno; return NULL;
        }
        qsort(find_names,find_count,sizeof(*find_names),find_compare);
        find_handle=1;
    } else if (find_handle!=1) {
        find_close(); errno=saved_errno; return NULL;
    }
    if (find_index==find_count) {
        recon_SetLastError(ERROR_NO_MORE_FILES); /* FindClose preserves it. */
        find_close(); errno=saved_errno; return NULL;
    }
    strcpy(find_buffer+44,find_names[find_index++]);
    errno=saved_errno;
    return find_buffer+44;
}
/* VC1 __setmode changes only the descriptor's text bit, without flushing
 * a FILE or resetting its input state. Descriptor errors precede mode errors. */
/* Canonical _isatty is an unsigned descriptor-bound check followed by the
 * FDEV mask, even for a closed slot. Invalid input returns0 without errors. */
WEAK int recon_isatty(int fd)
{
    return (unsigned)fd<recon_nhandle ? recon_osfile[fd]&0x40 : 0;
}
WEAK int recon_setmode(int fd,int mode)
{
    unsigned char previous;
    if (!valid_descriptor(fd)) return -1;
    previous=recon_osfile[fd];
    if (mode==0x8000) recon_osfile[fd]&=~0x80;
    else if (mode==0x4000) recon_osfile[fd]|=0x80;
    else { errno=EINVAL; return -1; }
    return previous&0x80?0x4000:0x8000;
}
WEAK int recon_sopen(const char *name,int mode,int sharing,...)
{
    DWORD access,disposition;
    HANDLE handle;
    int fd,restricted=0;
    DWORD share=FILE_SHARE_READ|FILE_SHARE_WRITE;
    if (!fsopen_active && (mode&0x80)) {
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        char path[PATH_MAX];struct stat existing;int permission,saved_errno=errno;va_list args;
        if(mode!=0x8182 || sharing!=0x10)
            recon_unsupported("unreviewed noinherit _sopen tuple");
        va_start(args,sharing);permission=va_arg(args,int);va_end(args);
        if(permission!=0x180)
            recon_unsupported("unreviewed noinherit _sopen permission");
        recon_fsopen_check_mappings();
        if(recon_host_path(name,path,sizeof(path)) || !lstat(path,&existing) || errno!=ENOENT)
            recon_unsupported("noinherit _sopen requires absent path");
        /* An isolated single-process lease reuses the existing inode guard;
         * later aliases, dup and mappings remain explicit unsupported paths. */
        fsopen_preflight(name,0);errno=saved_errno;share=0;restricted=1;
#else
        recon_unsupported("noinherit _sopen requires MSVCRT510 profile");
#endif
    }
    if (fsopen_active) share=sharing==0x20?FILE_SHARE_READ:0;
    access=(mode&3)==2?GENERIC_READ|GENERIC_WRITE:(mode&1)?GENERIC_WRITE:GENERIC_READ;
    disposition=(mode&0x500)==0x500?CREATE_NEW:(mode&0x300)==0x300?CREATE_ALWAYS:
        mode&0x100?OPEN_ALWAYS:mode&0x200?TRUNCATE_EXISTING:OPEN_EXISTING;
    handle=recon_CreateFileA(name,access,share,NULL,disposition,FILE_ATTRIBUTE_NORMAL,NULL);
    if (handle==INVALID_HANDLE_VALUE) { recon_dosmaperr(recon_GetLastError()); return -1; }
    fd=recon_alloc_osfhnd();
    if (fd<0) { recon_CloseHandle(handle); return -1; }
    if (recon_set_osfhnd(fd,(long)handle)) recon_unsupported("sopen descriptor ownership");
    recon_osfile[fd]=1|((mode&0x8000)?0:(mode&0x4000 || recon_fmode!=0x8000)?0x80:0);
    if (mode&8) recon_osfile[fd]|=0x20;
    if (restricted) {recon_osfile[fd]|=0x10;recon_fsopen_shares[fd]=1;}
    pushed_byte[fd]=-1;
    return fd;
}
WEAK int recon_open(const char *name,int mode,...)
{
    int permission=0666; va_list args;
    if (mode&0x100) { va_start(args,mode); permission=va_arg(args,int); va_end(args); }
    return recon_sopen(name,mode,0x40,permission);
}
WEAK long recon_lseek(int fd,long offset,int origin)
{
    DWORD p;
    if (!valid_descriptor(fd)) return -1;
    recon_SetLastError(0);
    p=recon_SetFilePointer((HANDLE)os_handles[fd],offset,NULL,origin);
    if (p==0xffffffffUL && recon_GetLastError()) { recon_dosmaperr(recon_GetLastError()); return -1; }
    pushed_byte[fd]=-1; recon_osfile[fd]&=~2; return p;
}
long recon_tell(int fd) { return recon_lseek(fd,0,SEEK_CUR); }
long recon_filelength(int fd)
{
    long old=recon_tell(fd),end;
    if (old<0) return -1;
    end=recon_lseek(fd,0,SEEK_END); recon_lseek(fd,old,SEEK_SET); return end;
}
int recon_chsize(int fd,long length)
{
    long old=recon_tell(fd); int result;
    if (old<0 || recon_lseek(fd,length,SEEK_SET)<0) return -1;
    result=recon_SetEndOfFile((HANDLE)os_handles[fd])?0:-1;
    if (result) recon_dosmaperr(recon_GetLastError());
    recon_lseek(fd,old,SEEK_SET); return result;
}
int recon_close(int fd)
{
    DWORD error=0;
    if (!valid_descriptor(fd)) return -1;
    /* Startup may intentionally give stdout and stderr the same OS HANDLE. */
    if (!((fd==1 || fd==2) && os_handles[1]==os_handles[2])) {
        if (!recon_CloseHandle((HANDLE)os_handles[fd])) error=recon_GetLastError();
    }
    recon_free_osfhnd(fd);
    if (error) { recon_dosmaperr(error); return -1; }
    recon_osfile[fd]=0; pushed_byte[fd]=-1; return 0;
}
static int read_raw(int fd,void *buffer,unsigned count)
{
    DWORD got;
    if (!recon_ReadFile((HANDLE)os_handles[fd],buffer,count,&got,NULL)) {
        recon_dosmaperr(recon_GetLastError()); return -1;
    }
    return got;
}
int recon_read(int fd,void *buffer,unsigned count)
{
    unsigned char *raw=buffer,next;
    unsigned consumed=0,produced=0,available=0;
    int got;
    if (!valid_descriptor(fd)) return -1;
    if (!count || (recon_osfile[fd]&2)) return 0;
    if (pushed_byte[fd]>=0) {
        raw[available++]=pushed_byte[fd]; pushed_byte[fd]=-1;
    }
    got=read_raw(fd,raw+available,count-available);
    if (got<0) return available?(int)available:-1;
    available+=got;
    if (!(recon_osfile[fd]&0x80)) return available;
    if (available && raw[0]=='\n') recon_osfile[fd]|=4;
    else recon_osfile[fd]&=~4;
    /* VC1 bounds a text read by RAW bytes, then compacts CRLF in place.
     * Filling count translated bytes breaks the CRT's ftell buffer accounting. */
    while (consumed<available) {
        unsigned char c=raw[consumed++];
        if (c==0x1a) {
            if (!(recon_osfile[fd]&0x40)) recon_osfile[fd]|=2;
            break;
        }
        if (c!='\r') { raw[produced++]=c; continue; }
        if (consumed<available) {
            if (raw[consumed]=='\n') { ++consumed; c='\n'; }
            raw[produced++]=c; continue;
        }
        got=read_raw(fd,&next,1);
        if (got!=1) { raw[produced++]='\r'; continue; }
        if (recon_osfile[fd]&0x48) {
            if (next=='\n') raw[produced++]='\n';
            else { raw[produced++]='\r'; pushed_byte[fd]=next; }
        } else if (!produced && next=='\n') raw[produced++]='\n';
        else {
            recon_lseek(fd,-1,SEEK_CUR);
            if (next!='\n') raw[produced++]='\r';
        }
    }
    return produced;
}
static int write_raw(int fd,const void *buffer,unsigned count)
{
    unsigned done=0; DWORD bytes;
    while (done<count) {
        if (!recon_WriteFile((HANDLE)os_handles[fd],(const char *)buffer+done,count-done,&bytes,NULL)) {
            recon_dosmaperr(recon_GetLastError()); return -1;
        }
        if (!bytes) { errno=ENOSPC; return -1; }
        done+=bytes;
    }
    return done;
}
int recon_write(int fd,const void *buffer,unsigned count)
{
    char translated[4096]; unsigned n=0,i;
    if (!valid_descriptor(fd)) return -1;
    if (recon_osfile[fd]&0x20) recon_lseek(fd,0,SEEK_END);
    if (!(recon_osfile[fd]&0x80)) return write_raw(fd,buffer,count);
    for (i=0;i<count;++i) {
        if (n+2>sizeof(translated)) { if (write_raw(fd,translated,n)<0) return -1; n=0; }
        if (((const char *)buffer)[i]=='\n') translated[n++]='\r';
        translated[n++]=((const char *)buffer)[i];
    }
    if (n && write_raw(fd,translated,n)<0) return -1;
    return count;
}
/* Observed VC1 default-C perror: direct text lowio output, not FILE I/O.
 * Only the proved ENOENT message and successful regular stderr route apply. */
WEAK void __cdecl recon_perror(const char *prefix)
{
#if defined(RECON_CRT_PROFILE_MSVCRT_510) || defined(RECON_CRT_PROFILE_WATCOM_100A)
    (void)prefix;recon_unsupported("unreviewed perror CRT profile");
#else
    const char *cursor;size_t length=0;unsigned n;long handle;int flags;
    struct stat info;
    if (errno!=ENOENT || recon_doserrno!=3 || recon_GetLastError()!=3 ||
        recon_mb_codepage || recon_mb_codepage_gate || recon_lc_codepage ||
        recon_mb_cur_max!=1 || recon_pctype!=character_flags+1)
        recon_unsupported("perror message/error/default-C state");
    for (n=0;n<6;++n) if(recon_lc_handle[n])
        recon_unsupported("perror locale handles");
    if (recon_nhandle<=2 || recon_osfile[2]!=0x81 ||
        (handle=os_handles[2])<=0 || handle>=65536)
        recon_unsupported("perror registered text stderr");
    flags=fcntl((int)handle-1,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)==O_RDONLY || (flags&O_APPEND) ||
        fstat((int)handle-1,&info) || !S_ISREG(info.st_mode))
        recon_unsupported("perror writable regular stderr");
    if (prefix) for(cursor=prefix;*cursor;++cursor) {
        if ((unsigned char)*cursor>=128 || ++length>INT_MAX)
            recon_unsupported("perror ASCII prefix span");
    }
    if (length && (recon_write(2,prefix,(unsigned)length)!=(int)length ||
                   recon_write(2,": ",2)!=2))
        recon_unsupported("perror prefix write failure");
    if (recon_write(2,"No such file or directory",25)!=25 || recon_write(2,"\n",1)!=1)
        recon_unsupported("perror message write failure");
    /* The existing successful regular-file lowio route preserves these slots;
     * do not reseed them after writes or accept an unproved failure path. */
    if (errno!=ENOENT || recon_doserrno!=3 || recon_GetLastError()!=3)
        recon_unsupported("perror lowio error-state change");
#endif
}
int recon_access(const char *name,int mode)
{
    char path[PATH_MAX],parent[PATH_MAX],*separator;
    struct stat information;
    int saved=errno,failure;
    unsigned long code;
    {struct stat64 info;int matched=recon_native_path_probe(name,&info);
     if(matched) {
        if(matched<0)return -1;
        if(mode&~6) {errno=EINVAL;return -1;}
        if(mode&2) {recon_dosmaperr(ERROR_ACCESS_DENIED);recon_SetLastError(ERROR_ACCESS_DENIED);return -1;}
        errno=saved;return 0;
     }}
    /* Other modes retain the legacy, uncharacterized host path. The proved
     * existence/read checks use GetFileAttributes semantics, not Unix access
     * permissions, and keep CRT errno independent from path-translation IO. */
    if (mode!=0 && mode!=4)
        return recon_host_path(name,path,sizeof(path))<0?-1:access(path,mode);
    if (recon_host_path(name,path,sizeof(path))<0)
        recon_unsupported("_access path translation");
    if (!stat(path,&information)) {
        if (!S_ISREG(information.st_mode) && !S_ISDIR(information.st_mode))
            recon_unsupported("_access unreviewed file kind");
        errno=saved;
        return 0;
    }
    failure=errno;
    if (failure!=ENOENT && failure!=ENOTDIR)
        recon_unsupported("_access unreviewed attribute failure");
    strcpy(parent,path);
    separator=strrchr(parent,'/');
    if (!separator) strcpy(parent,".");
    else if (separator==parent) separator[1]=0;
    else *separator=0;
    if (!stat(parent,&information))
        code=S_ISDIR(information.st_mode)?2:3;
    else {
        failure=errno;
        if (failure!=ENOENT && failure!=ENOTDIR)
            recon_unsupported("_access unreviewed parent failure");
        code=3;
    }
    recon_SetLastError(code);
    recon_dosmaperr(code);
    return -1;
}
int recon_stat(const char *name,struct recon_stat *out)
{
    struct stat64 st;
    char path[PATH_MAX];
    const char *drive=name,*extension,*canonical;
    unsigned device=25,permissions=0400;
    if (strpbrk(name,"*?") || (name[0] && name[1]==':' && !name[2])) {
        errno=ENOENT; recon_doserrno=2; return -1;
    }
    {int matched=recon_native_path_probe(name,&st);
     if(matched<0)return -1;
     if(!matched) {
        if(recon_host_path(name,path,sizeof(path))<0)return -1;
        if(stat64(path,&st)<0) {
            if(errno==ENOENT || errno==ENOTDIR) {errno=ENOENT;recon_doserrno=2;}
            return -1;
        }
     }}
    canonical=getenv("RECON_CANONICAL_CWD");
    if (!(drive[0] && drive[1]==':') && canonical) drive=canonical;
    if (drive[0] && drive[1]==':' && isalpha((unsigned char)drive[0]))
        device=toupper((unsigned char)drive[0])-'A';
    if (st.st_mode&0222) permissions|=0200;
    extension=strrchr(name,'.');
    if (S_ISDIR(st.st_mode) || (extension &&
        (!strcasecmp(extension,".exe") || !strcasecmp(extension,".cmd") ||
         !strcasecmp(extension,".bat") || !strcasecmp(extension,".com"))))
        permissions|=0100;
    permissions|=(permissions>>3)|(permissions>>6);
    /* The old CRT leaves the alignment bytes at14..15 untouched. */
    out->st_dev=out->st_rdev=device;
    out->st_ino=0; out->st_uid=out->st_gid=0; out->st_nlink=1;
    out->st_mode=(S_ISDIR(st.st_mode)?0040000:0100000)|permissions;
    out->st_size=S_ISDIR(st.st_mode)?0:(long)(uint32_t)st.st_size;
    out->st_atime=st.st_atim.tv_sec<0 || st.st_atim.tv_sec>INT32_MAX?-1:(long)st.st_atim.tv_sec;
    out->st_mtime=st.st_mtim.tv_sec<0 || st.st_mtim.tv_sec>INT32_MAX?-1:(long)st.st_mtim.tv_sec;
    /* Linux Wine's directory information uses mtime as creation time, not
     * Unix inode-change time. This is the original capture environment. */
    out->st_ctime=out->st_mtime;
    return 0;
}
/* VC1 FSTAT.OBJ uses descriptor identity for device/pipe st_dev, but zero
 * for disk files. It preserves the two alignment bytes at offsets14..15. */
WEAK int recon_fstat(int descriptor,struct recon_stat *out)
{
    struct stat64 info; int saved_errno=errno,fd; unsigned mode;
    DWORD saved_error=recon_GetLastError();
    if ((unsigned)descriptor>=recon_nhandle) {errno=EBADF;return -1;}
    fd=(int)os_handles[descriptor]-1;
    if (os_handles[descriptor]<=0 || fstat64(fd,&info)) {
        recon_SetLastError(ERROR_INVALID_HANDLE);errno=EBADF;return -1;
    }
    if (!out) recon_unsupported("_fstat NULL destination");
    if (S_ISCHR(info.st_mode)) mode=0020000;
    else if (S_ISFIFO(info.st_mode)) mode=0010000;
    else if (S_ISREG(info.st_mode)) mode=0100000;
    else {recon_unsupported("_fstat unreviewed file type");return -1;}
    out->st_ino=0;out->st_uid=out->st_gid=0;out->st_nlink=1;
    if (mode==0100000) {
        out->st_mode=mode|((info.st_mode&0222)?0666:0444);
        out->st_dev=out->st_rdev=0;
        out->st_size=(long)(uint32_t)info.st_size;
        out->st_atime=info.st_atim.tv_sec<0 || info.st_atim.tv_sec>INT32_MAX?-1:(long)info.st_atim.tv_sec;
        out->st_mtime=info.st_mtim.tv_sec<0 || info.st_mtim.tv_sec>INT32_MAX?-1:(long)info.st_mtim.tv_sec;
        /* The proved Wine environment exposes modification as creation. */
        out->st_ctime=out->st_mtime;
    } else {
        out->st_mode=mode;out->st_dev=out->st_rdev=descriptor;
        out->st_size=out->st_atime=out->st_mtime=out->st_ctime=0;
    }
    errno=saved_errno;recon_SetLastError(saved_error);return 0;
}
int recon_remove(const char *name)
{
    char path[PATH_MAX];struct stat64 info;
    if(recon_native_path_probe(name,&info)>0) {recon_dosmaperr(ERROR_ACCESS_DENIED);return -1;}
    return recon_host_path(name,path,sizeof(path))<0?-1:remove(path);
}
int recon_rename(const char *from,const char *to)
{
    char a[PATH_MAX],b[PATH_MAX];struct stat64 info;
    if(recon_native_path_probe(from,&info)>0 || recon_native_path_probe(to,&info)>0) {
        recon_dosmaperr(ERROR_ACCESS_DENIED);return -1;
    }
    if (recon_host_path(from,a,sizeof(a))<0 || recon_host_path(to,b,sizeof(b))<0) return -1;
    return rename(a,b);
}

WEAK void recon_getbuf(ShimFILE *s)
{
    s->_base=malloc(4096);
    if (s->_base) { s->_flag|=8; s->_bufsiz=4096; }
    else { s->_base=(char *)&s->_charbuf; s->_flag|=4; s->_bufsiz=2; }
    s->_ptr=s->_base; s->_cnt=0;
}
WEAK void recon_freebuf(ShimFILE *s)
{
    if (!(s->_flag&0x83) || !(s->_flag&8)) return;
    free(s->_base);
    s->_base=s->_ptr=NULL; s->_cnt=0;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    s->_flag&=~0x408;
#else
    s->_flag&=~8;
#endif
}
int recon_flush(ShimFILE *s)
{
    int size,result=0;
    if ((s->_flag&3)==2 && (s->_flag&0x108) && s->_base && s->_ptr>s->_base) {
        size=s->_ptr-s->_base;
        if (recon_write(s->_file,s->_base,size)!=size) { s->_flag|=0x20; result=EOF; }
        else if (s->_flag&0x80) s->_flag&=~2;
    }
    s->_ptr=s->_base; s->_cnt=0; return result;
}
int recon_fflush(ShimFILE *s)
{
    int i,result=0;
    if (s) return recon_flush(s);
    for (i=0;i<20;++i) if (recon_iob[i]._flag&2)
        if (recon_flush(&recon_iob[i])) result=EOF;
    return result;
}
WEAK int recon_filbuf(ShimFILE *s)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (!(s->_flag&0x83) || (s->_flag&0x40)) return EOF;
#endif
    if (s->_flag&2) { s->_flag|=0x20; return EOF; }
    s->_flag|=1;
    if (!s->_base) recon_getbuf(s);
    s->_ptr=s->_base; s->_cnt=recon_read(s->_file,s->_base,s->_bufsiz);
    if (s->_cnt<=0) { s->_flag|=s->_cnt?0x20:0x10; s->_cnt=0; return EOF; }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (s->_bufsiz==512 && (s->_flag&8) && !(s->_flag&0x400)) s->_bufsiz=4096;
#elif !defined(RECON_CRT_PROFILE_WATCOM_100A)
    if (s->_bufsiz==512 && s->_flag==9) {
        unsigned n;
        /* The reached old-CRT seek temporarily shrinks an owned4096 buffer.
         * Only restore capacity for an established registered MYBUF owner;
         * a genuine512 setvbuf allocation must retain its smaller capacity. */
        for (n=0;n<20;++n) if (s==&recon_iob[n]) {
            if (malloc_usable_size(s->_base)>=4096) s->_bufsiz=4096;
            break;
        }
    }
#endif
    --s->_cnt; return (unsigned char)*s->_ptr++;
}
int recon_fgetc(ShimFILE *s)
{
    if (--s->_cnt>=0) return (unsigned char)*s->_ptr++;
    return recon_filbuf(s);
}
int recon_fputc(int c,ShimFILE *s)
{
    unsigned char byte=c;
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if(s==&recon_iob[1] && watcom_stdout_line_buffer &&
       (s->_base!=watcom_stdout_line_buffer || s->_bufsiz!=(int)watcom_stdout_line_size ||
        !(s->_flag&0x100) || s->_file!=1))
        recon_unsupported("Watcom line-buffer ownership/state change");
#endif
    if (!(s->_flag&2)) {
        if ((s->_flag&1) && !(s->_flag&0x80)) { s->_flag|=0x20; return EOF; }
        s->_flag=(s->_flag&~1)|2; s->_cnt=0; s->_ptr=s->_base;
    }
#ifndef RECON_CRT_PROFILE_MSVCRT_510
    if (s->_base && !s->_bufsiz && !(s->_flag&4)) {
        unsigned n;int registered=0,pending;
        for(n=0;n<20;++n) if(s==&recon_iob[n]) { registered=1;break; }
        if (!registered || s->_flag!=0x0a || (unsigned)s->_file>=recon_nhandle ||
            !(recon_osfile[s->_file]&1) || os_handles[s->_file]==-1 ||
            !((s->_ptr==s->_base && !s->_cnt) ||
              (s->_ptr==s->_base+1 && s->_cnt==-1)))
            recon_unsupported("zero-capacity buffered FILE identity/ownership/state");
        /* MYBUF on a registered CRT stream is an existing malloc-ownership
         * invariant. Measure only that established allocation; this is not
         * a validity test for arbitrary pointers or forged FILE objects. */
        if (malloc_usable_size(s->_base)<1)
            recon_unsupported("zero-capacity buffered FILE backing");
        pending=s->_ptr==s->_base?0:1;
        if (pending && recon_write(s->_file,s->_base,1)!=1)
            recon_unsupported("zero-capacity buffered FILE write failure");
        s->_ptr=s->_base+1;s->_cnt=-1;*s->_base=(char)byte;
        return byte;
    }
#endif
    if ((s->_flag&4) || (s->_base && s->_bufsiz<=0))
        return recon_write(s->_file,&byte,1)==1?byte:EOF;
    if (!s->_base) recon_getbuf(s);
    if (s->_ptr==s->_base+s->_bufsiz && recon_flush(s)) return EOF;
    s->_flag|=2;
    *s->_ptr++=byte;
    /* _cnt is observable to the reconstructed CRT. */
    s->_cnt=s->_bufsiz-(s->_ptr-s->_base);
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if(s==&recon_iob[1] && watcom_stdout_line_buffer && byte=='\n' && recon_flush(s)) return EOF;
#endif
    return byte;
}
char *recon_fgets(char *dest,int size,ShimFILE *s)
{
    int n=0,c;
    if (size<=0) return NULL;
    while (n<size-1) { c=recon_fgetc(s); if (c==EOF) break; dest[n++]=c; if (c=='\n') break; }
    if (!n && size!=1) return NULL;
    dest[n]=0; return dest;
}
WEAK size_t recon_fread(void *p,size_t size,size_t count,ShimFILE *s)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    size_t total=size*count,remaining=total,buffer_size,amount;
    unsigned char *output=p;int got;
    if (!total) return 0;
    if (!p || !s || s->_bufsiz<0 || s->_cnt<0)
        recon_unsupported("unreviewed VC5 fread buffer/argument state");
    buffer_size=s->_flag&0x10c?(size_t)s->_bufsiz:4096;
    while (remaining) {
        if ((s->_flag&0x10c) && s->_cnt) {
            amount=remaining<(size_t)s->_cnt?remaining:(size_t)s->_cnt;
            memcpy(output,s->_ptr,amount);
            s->_ptr+=amount;s->_cnt-=(int)amount;
            output+=amount;remaining-=amount;
        } else if (remaining>=buffer_size) {
            /* Canonical fread bypasses FILE storage for whole-sized reads.
             * A short read can be followed by another direct attempt at EOF;
             * only _filbuf restores the temporary512-byte capacity to4096. */
            amount=buffer_size?remaining-remaining%buffer_size:remaining;
            got=recon_read(s->_file,output,(unsigned)amount);
            if (got<=0) { s->_flag|=got?0x20:0x10;break; }
            output+=got;remaining-=(size_t)got;
        } else {
            got=recon_filbuf(s);
            if (got==EOF) break;
            *output++=(unsigned char)got;--remaining;
            buffer_size=(size_t)s->_bufsiz;
        }
    }
    return (total-remaining)/size;
#else
    size_t n,total=size*count; int c;
    if (!size) return 0;
    for (n=0;n<total;++n) { c=recon_fgetc(s); if (c==EOF) break; ((char *)p)[n]=c; }
    return n/size;
#endif
}

size_t recon_fwrite(const void *p,size_t size,size_t count,ShimFILE *s)
{
    size_t n,total=size*count;
    if (!size) return 0;
    for (n=0;n<total;++n) if (recon_fputc(((const unsigned char *)p)[n],s)==EOF) break;
    return n/size;
}
int recon_fputs(const char *text,ShimFILE *s) { size_t n=strlen(text); return recon_fwrite(text,1,n,s)==n?0:EOF; }
int recon_puts(const char *text) { return recon_fputs(text,&recon_iob[1])==EOF?EOF:recon_fputc('\n',&recon_iob[1]); }
/* The reviewed VC1 plain finite %g converter is integer based and leaves
 * all x87 environment/register bytes unchanged. Isolate host formatting only
 * for this literal-plus-one-conversion scope; other formats keep their path. */
static int format_is_plain_g(const char *format)
{
    unsigned conversions=0;
    while (*format) {
        if (*format++!='%') continue;
        if (*format=='%') { ++format;continue; }
        if (*format=='L') ++format;
        if (*format!='g') return 0;
        ++format;++conversions;
    }
    return conversions==1;
}
static int format_plain_g(char **output,const char *format,va_list args)
{
    unsigned char environment[108] __attribute__((aligned(16)));
    union { double number; uint32_t words[2]; } value;
    unsigned short control;
    char number[64],*exponent,*out,*start;
    const char *conversion=format;
    long double extended;
    size_t length;
    int count,legacy_long=0;
    if (recon_lc_handle[2] || recon_lc_codepage)
        recon_unsupported("finite %g outside default-C locale");
    /* fnsave resets the scratch stack. Preserve incoming precision/rounding
     * while host libc works, then restore every original x87 register/field. */
    __asm__ volatile("fnsave %0" : "=m"(environment));
    memcpy(&control,environment,sizeof(control));
    if (control!=0x027f && control!=0x037f)
        recon_unsupported("unreviewed finite %g x87 control");
    __asm__ volatile("fldcw %0" : : "m"(control));
    while (*conversion) {
        if (*conversion++!='%') continue;
        if (*conversion=='%') { ++conversion;continue; }
        legacy_long=*conversion=='L';break;
    }
    if (legacy_long) {
        /* Original VC1 long double is8 bytes. Native C's explicit promotion
         * transports that same double value in a12-byte vararg object. */
        extended=va_arg(args,long double);value.number=(double)extended;
        if ((long double)value.number!=extended)
            recon_unsupported("unreviewed genuine80-bit %Lg value");
    } else value.number=va_arg(args,double);
    if ((value.words[1]&0x7ff00000u)==0x7ff00000u)
        recon_unsupported("unreviewed nonfinite %g");
    count=snprintf(number,sizeof(number),"%g",value.number);
    if (count<0 || count>=(int)sizeof(number)-1)
        recon_unsupported("finite %g formatting failure");
    /* Original dtold's zero path drops the sign; old CRT exponents have
     * at least three digits. These spelling differences are directly probed. */
    if (!(value.words[0] || (value.words[1]&0x7fffffffu))) strcpy(number,"0");
    exponent=strchr(number,'e');
    if (exponent && strlen(exponent+1)==3) {
        memmove(exponent+3,exponent+2,3);exponent[2]='0';
    }
    length=strlen(format)+strlen(number)+1;
    start=malloc(length);
    if (!start) recon_unsupported("finite %g allocation failure");
    out=start;
    while (*format) {
        if (*format!='%') *out++=*format++;
        else if (format[1]=='%') { *out++='%';format+=2; }
        else { length=strlen(number);memcpy(out,number,length);out+=length;format+=format[1]=='L'?3:2; }
    }
    *out=0;*output=start;count=(int)(out-start);
    __asm__ volatile("frstor %0" : : "m"(environment));
    return count;
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int format_has_wide_string(const char *format)
{
    const char *p=format;
    while (*p) {
        if (*p++!='%') continue;
        while (*p && !strchr("diouxXfFeEgGaAcspnCS%",*p)) ++p;
        if (*p=='S') return 1;
        if (*p) ++p;
    }
    return 0;
}
static void append_formatted_bytes(char **buffer,size_t *used,size_t *capacity,
                                   const char *text,size_t length)
{
    size_t needed,next;
    char *grown;
    if (length>(size_t)INT_MAX-*used) recon_unsupported("wide printf count overflow");
    needed=*used+length+1;
    if (needed>*capacity) {
        next=*capacity?*capacity:128;
        while (next<needed) {
            if (next>(size_t)INT_MAX/2) { next=needed;break; }
            next*=2;
        }
        grown=realloc(*buffer,next);
        if (!grown) recon_unsupported("wide printf allocation failure");
        *buffer=grown;*capacity=next;
    }
    memcpy(*buffer+*used,text,length);*used+=length;(*buffer)[*used]=0;
}
static int format_wide_ascii(char **output,const char *format,va_list args)
{
    char *buffer=NULL,byte,number[9];
    size_t used=0,capacity=0;
    const WCHAR *wide;
    unsigned value;
    int count;
    if (recon_lc_handle[2] || recon_lc_codepage)
        recon_unsupported("wide printf outside default-C locale");
    while (*format) {
        if ((unsigned char)*format>=128) recon_unsupported("nonASCII wide printf format");
        if (*format!='%') {
            append_formatted_bytes(&buffer,&used,&capacity,format++,1);continue;
        }
        ++format;
        if (*format=='S') {
            wide=va_arg(args,const WCHAR *);
            if (!wide) recon_unsupported("NULL wide printf argument");
            while (*wide) {
                if (*wide>=128) recon_unsupported("nonASCII WCHAR16 printf argument");
                byte=(char)*wide++;append_formatted_bytes(&buffer,&used,&capacity,&byte,1);
            }
            ++format;
        } else if (*format=='c') {
            value=(unsigned)va_arg(args,int);
            if (value>=128) recon_unsupported("nonASCII wide printf char argument");
            byte=(char)value;append_formatted_bytes(&buffer,&used,&capacity,&byte,1);++format;
        } else if (!strncmp(format,".4x",3)) {
            value=va_arg(args,unsigned);count=snprintf(number,sizeof(number),"%.4x",value);
            if (count<0 || count>8) recon_unsupported("wide printf hex formatting");
            append_formatted_bytes(&buffer,&used,&capacity,number,count);format+=3;
        } else if (*format=='%') {
            append_formatted_bytes(&buffer,&used,&capacity,"%",1);++format;
        } else recon_unsupported("unreviewed mixed WCHAR16 printf grammar");
    }
    if (!buffer) { buffer=malloc(1);if(!buffer)recon_unsupported("wide printf empty allocation");buffer[0]=0; }
    *output=buffer;return (int)used;
}
#endif
static int format_has_pointer(const char *format)
{
    const char *p=format;
    while (*p) {
        if (*p++!='%') continue;
        if (*p=='%') { ++p;continue; }
        /* A new percent starts a separate conversion even after malformed
         * input. Do not let an invalid prefix hide a later pointer from the
         * strict whole-format validator. */
        while (*p && *p!='%') {
            if (*p=='p') return 1;
            if (strchr("diouxXfFeEgGaAcsnCS",*p)) { ++p;break; }
            ++p;
        }
    }
    return 0;
}
static int format_pointer_ascii(char **output,const char *format,va_list args)
{
    const char *p;char number[8],*text,*out;uint32_t value;
    unsigned conversions=0,n;size_t length=strlen(format);
    int saved_errno=errno,shift;unsigned long saved_dos=recon_doserrno;
    DWORD saved_error=recon_GetLastError();
    if (recon_mb_codepage || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("pointer printf outside default-C locale");
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("pointer printf locale handle");
    /* Validate the entire grammar before consuming the single DWORD pointer. */
    for(p=format;*p;++p){
        if((unsigned char)*p>=128)recon_unsupported("nonASCII pointer printf format");
        if(*p!='%')continue;
        ++p;
        if(*p=='%')continue;
        if(*p!='p' || ++conversions!=1)
            recon_unsupported("unreviewed pointer printf grammar");
    }
    if(conversions!=1 || length>(size_t)INT_MAX-9)
        recon_unsupported("pointer printf conversion/count scope");
    value=(uint32_t)(uintptr_t)va_arg(args,void *);
    for(n=0,shift=28;shift>=0;++n,shift-=4)number[n]="0123456789ABCDEF"[(value>>shift)&15];
    text=malloc(length+9);
    if(!text)recon_unsupported("pointer printf allocation failure");
    for(out=text,p=format;*p;++p){
        if(*p!='%')*out++=*p;
        else if(*++p=='%')*out++='%';
        else{memcpy(out,number,8);out+=8;}
    }
    *out=0;*output=text;
    errno=saved_errno;recon_doserrno=saved_dos;recon_SetLastError(saved_error);
    return (int)(out-text);
}
int recon_vfprintf(ShimFILE *s,const char *format,va_list args)
{
    char *text,*copy=NULL; size_t length=strlen(format),percent=0;
    int count,result;
    if (format_is_plain_g(format)) count=format_plain_g(&text,format,args);
    else if (format_has_pointer(format)) count=format_pointer_ascii(&text,format,args);
    else
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (format_has_wide_string(format)) count=format_wide_ascii(&text,format,args);
    else
#endif
    {
        /* VC1 prints the valid prefix of LINK's formats ending in a lone '%'.
         * glibc rejects the whole conversion. Preserve literal %% pairs. */
        while (percent<length && format[length-percent-1]=='%') ++percent;
        if (percent&1) {
            copy=strdup(format); if (!copy) return -1;
            copy[length-1]=0; format=copy;
        }
        count=vasprintf(&text,format,args);
    }
    free(copy);
    if (count<0) return -1;
    result=recon_fwrite(text,1,count,s)==(unsigned)count?count:-1; free(text); return result;
}
int recon_vprintf(const char *format,va_list args)
{
    return recon_vfprintf(&recon_iob[1],format,args);
}
int recon_fprintf(ShimFILE *s,const char *format,...)
{
    va_list args; int n; va_start(args,format); n=recon_vfprintf(s,format,args); va_end(args); return n;
}
int recon_printf(const char *format,...)
{
    va_list args; int n; va_start(args,format); n=recon_vfprintf(&recon_iob[1],format,args); va_end(args); return n;
}
#ifdef RECON_BORLAND_400_STDOUT
static char *borland_stdout_buffer;
static void borland_stdout_state(ShimFILE *stream)
{
    uintptr_t base=(uintptr_t)stream->_base,position=(uintptr_t)stream->_ptr;
    if(!borland_stdout_buffer) {
        if(stream->_flag!=2 || stream->_base || stream->_ptr || stream->_cnt || stream->_bufsiz)
            recon_unsupported("Borland400 printf fresh stdout state");
    } else if(stream->_flag!=0x0a || stream->_base!=borland_stdout_buffer ||
              stream->_bufsiz!=512 || position<base || position>base+512 ||
              (position==base?stream->_cnt!=0:stream->_cnt!=512-(int)(position-base)))
        recon_unsupported("Borland400 printf owned stdout state");
}
#endif
/* The reached printf grammar is only %s. Original VPRINTER.C stages80 bytes,
 * then FPUTN.C preflushes a whole chunk against the512-byte stream capacity.
 * Reuse native FILE allocation/flush/text-write machinery; no message data or
 * case-specific output lengths occur here. Success assumes writable regular
 * redirected stdout; other formatting, stream roles and failures stay denied. */
int __cdecl recon_borland_400_printf(const char *format,...)
{
#ifdef RECON_BORLAND_400_STDOUT
    ShimFILE *stream=&recon_iob[1];const char *text;va_list arguments;
    size_t length,offset,chunk,pending;struct stat info;int descriptor,flags;
    int saved=errno,*target=recon_errno(),saved_target=*target;unsigned long dos=recon_doserrno;
    DWORD error=recon_GetLastError();uintptr_t address=(uintptr_t)format;
    if(address<4096 || address>UINTPTR_MAX-3 || memcmp(format,"%s",3))
        recon_unsupported("Borland400 printf exact string grammar");
    va_start(arguments,format);text=va_arg(arguments,const char *);va_end(arguments);
    address=(uintptr_t)text;
    if(address<4096 || address>UINTPTR_MAX-65536)
        recon_unsupported("Borland400 printf caller-readable string span");
    length=strnlen(text,65536);
    if(length==65536)recon_unsupported("Borland400 printf bounded NUL string");
    if(stream->_file!=1 || stream->_tmpfname || recon_nhandle<=1 || recon_osfile[1]!=0x81 ||
       os_handles[1]<=0 || os_handles[1]>(long)INT_MAX)
        recon_unsupported("Borland400 printf regular text stdout role");
    descriptor=(int)os_handles[1]-1;
    if(fstat(descriptor,&info) || !S_ISREG(info.st_mode) ||
       (flags=fcntl(descriptor,F_GETFL))<0 || (flags&O_ACCMODE)==O_RDONLY)
        recon_unsupported("Borland400 printf writable regular stdout");
    borland_stdout_state(stream);
    if(borland_stdout_buffer && address<(uintptr_t)borland_stdout_buffer+512 &&
       (uintptr_t)borland_stdout_buffer<address+length+1)
        recon_unsupported("Borland400 printf disjoint input/stdout buffer");
    if(!borland_stdout_buffer) {
        if(recon_setvbuf(stream,NULL,0,512))recon_unsupported("Borland400 printf buffer allocation");
        borland_stdout_buffer=stream->_base;
    }
    for(offset=0;offset<length;offset+=chunk) {
        chunk=length-offset;if(chunk>80)chunk=80;
        pending=(size_t)(stream->_ptr-stream->_base);
        if(pending+chunk>512 && recon_flush(stream))
            recon_unsupported("Borland400 printf stream flush failure");
        if(recon_fwrite(text+offset,1,chunk,stream)!=chunk)
            recon_unsupported("Borland400 printf stream write failure");
    }
    errno=saved;*target=saved_target;recon_doserrno=dos;recon_SetLastError(error);
    return (int)length;
#else
    (void)format;recon_unsupported("Borland400 printf requires explicit opt-in");
    __builtin_unreachable();
#endif
}
void __cdecl recon_borland_400_quick_exit(int code)
{
#ifdef RECON_BORLAND_400_STDOUT
    /* Borland EXIT.C quick1 skips atexit, pragma-exit routines and stdio flush.
     * Publishing the actual status is native host plumbing, not CRT cleanup. */
    recon_record_terminal_status((uint32_t)code);_exit(code);
#else
    (void)code;recon_unsupported("Borland400 quick exit requires explicit opt-in");
    __builtin_unreachable();
#endif
}

WEAK int __cdecl recon_sprintf(char *output,const char *format,...)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    const char *cursor,*start;size_t format_length,name_length,length,n;
    const WCHAR *filename;char number[12],*destination;int line,result;va_list arguments;
    uintptr_t out=(uintptr_t)output,source=(uintptr_t)format,wide;
    if(source<4096 || source>UINTPTR_MAX-4096 || out<4096)
        recon_unsupported("MSVCRT510 sprintf format/output extent");
    format_length=strnlen(format,4096);
    if(format_length==4096)recon_unsupported("MSVCRT510 sprintf format length");
    if(!strcmp(format,"%ws(%d) : ")) {
        if(out>UINTPTR_MAX-273 || recon_mb_codepage || recon_mb_codepage_gate ||
           recon_mb_cur_max!=1 || recon_lc_codepage)
            recon_unsupported("MSVCRT510 diagnostic sprintf default-C/output span");
        for(n=0;n<6;++n)if(recon_lc_handle[n])
            recon_unsupported("MSVCRT510 diagnostic sprintf locale handles");
        va_start(arguments,format);filename=va_arg(arguments,const WCHAR *);
        line=va_arg(arguments,int);va_end(arguments);wide=(uintptr_t)filename;
        if(wide<4096 || (wide&1) || wide>UINTPTR_MAX-257*sizeof(WCHAR))
            recon_unsupported("MSVCRT510 diagnostic sprintf filename span");
        for(name_length=0;name_length<257 && filename[name_length];++name_length)
            if(filename[name_length]>=128)recon_unsupported("MSVCRT510 diagnostic sprintf ASCII WCHAR filename");
        if(name_length==257)recon_unsupported("MSVCRT510 diagnostic sprintf filename length");
        recon_itoa(line,number,10);length=name_length+strlen(number)+5;
        if((out<source+format_length+1 && source<out+length+1) ||
           (out<wide+(name_length+1)*sizeof(WCHAR) && wide<out+length+1))
            recon_unsupported("MSVCRT510 diagnostic sprintf output/input overlap");
        destination=output;
        for(n=0;n<name_length;++n)*destination++=(char)filename[n];
        *destination++='(';memcpy(destination,number,strlen(number));destination+=strlen(number);
        memcpy(destination,") : ",5);return (int)length;
    }
    /* Ordinary narrow conversions already used the host sprintf backend.
     * Keep that behavior, but never pass Windows WCHAR16 forms to it. */
    for(cursor=format;*cursor;) {
        if(*cursor++!='%')continue;
        if(*cursor=='%'){++cursor;continue;}
        start=cursor;
        while(*cursor && *cursor!='%' && !strchr("diouxXfFeEgGaAcCsSpn",*cursor))++cursor;
        if(!*cursor)break;
        if(*cursor=='%')continue;
        if(*cursor=='S' || *cursor=='C' ||
           ((*cursor=='s' || *cursor=='c') &&
            (memchr(start,'w',(size_t)(cursor-start)) || memchr(start,'l',(size_t)(cursor-start)))))
            recon_unsupported("unreviewed Windows wide sprintf conversion");
        ++cursor;
    }
    va_start(arguments,format);result=vsprintf(output,format,arguments);va_end(arguments);return result;
#else
    (void)output;(void)format;
    recon_unsupported("sprintf typed boundary requires explicit510 profile");return 0;
#endif
}
/* VC1 _snprintf permits an exact fit without a NUL; overflow returns -1.
 * Host formatting is safe for the encountered ordinary conversions. A %n
 * after a failed write must not execute, unlike host snprintf, so reject that
 * unsupported conversion before any formatting or caller-memory writes. */
WEAK int recon_snprintf(char *buffer,size_t capacity,const char *format,...)
{
    const char *cursor;
    char *text,*copy=NULL;
    size_t length,percent=0,used;
    va_list arguments;
    int result;
    for (cursor=format;*cursor;++cursor) if (*cursor=='%') {
        ++cursor;
        while (*cursor && !strchr("diouxXeEfFgGaAcCsSpn%",*cursor)) ++cursor;
        if (*cursor=='n') recon_unsupported("_snprintf %n");
        if (!*cursor) break;
    }
    length=strlen(format);
    while (percent<length && format[length-percent-1]=='%') ++percent;
    if (percent&1) {
        copy=strdup(format);
        if (!copy) return -1;
        copy[length-1]=0; format=copy;
    }
    va_start(arguments,format);
    result=vasprintf(&text,format,arguments);
    va_end(arguments);
    free(copy);
    if (result<0) return -1;
    used=(size_t)result<capacity?(size_t)result:capacity;
    if (used) memcpy(buffer,text,used);
    if ((size_t)result<capacity) buffer[result]=0;
    free(text);
    return (size_t)result>capacity?-1:result;
}
WEAK ShimFILE *recon_openfile(const char *name,const char *mode,int sharing,ShimFILE *s)
{
    int flags,fd;
    if (*mode=='r') flags=0;
    else if (*mode=='w') flags=0x301;
    else if (*mode=='a') flags=0x109;
    else { errno=EINVAL; return NULL; }
    if (strchr(mode,'+')) flags=(flags&~3)|2;
    if (strchr(mode,'b')) flags|=0x8000;
    else if (strchr(mode,'t')) flags|=0x4000;
    fd=recon_sopen(name,flags,sharing,0666); if (fd<0) return NULL;
    /* Both encountered CRT families leave these two fields untouched when a
     * closed FILE slot is reused. Successful open resets the remaining fields,
     * including tmpfname; a retained capacity does not imply a live buffer. */
    { int retained_size=s->_bufsiz,retained_character=s->_charbuf;
      memset(s,0,sizeof(*s)); s->_bufsiz=retained_size; s->_charbuf=retained_character; }
    s->_file=fd;
    s->_flag=strchr(mode,'+')?0x80:*mode=='r'?1:2; return s;
}
ShimFILE *recon_fopen(const char *name,const char *mode)
{
    int n;
    for (n=3;n<20;++n) if (!recon_iob[n]._flag) {
        ShimFILE *stream=&recon_iob[n];
        /* Canonical getstream prepares a free slot before attempting open.
         * charbuf, bufsiz and tmpfname survive selection, including failure. */
        stream->_ptr=stream->_base=NULL;stream->_cnt=stream->_flag=0;stream->_file=-1;
        return recon_openfile(name,mode,0x40,stream);
    }
    errno=EMFILE; return NULL;
}
WEAK int recon_fclose(ShimFILE *s)
{
    int result=EOF;
    if (!(s->_flag&0x40) && (s->_flag&0x83)) {
        result=recon_flush(s); recon_freebuf(s);
        if (recon_close(s->_file)<0) result=EOF;
        else if (s->_tmpfname) free(s->_tmpfname);
    }
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if(s==&recon_iob[1]) { watcom_stdout_line_buffer=NULL;watcom_stdout_line_size=0; }
#endif
    s->_flag=0; return result;
}
int __cdecl recon_watcom_setvbuf(ShimFILE *stream,char *buffer,int mode,size_t size)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    if(stream!=&recon_iob[1] || !buffer || mode!=0x0200 || (size!=128 && size!=256) ||
       (uintptr_t)buffer>UINTPTR_MAX-size || stream->_file!=1 || stream->_tmpfname ||
       recon_nhandle<=1 || os_handles[1]<=0)
        recon_unsupported("Watcom supplied stdout line-buffer domain");
    if((recon_osfile[1]&~8)!=0x81) {
        int saved=errno,is_terminal;
        if(!recon_native_entry_active() || recon_osfile[1]!=0xc1 || size!=128 || watcom_stdout_line_buffer)
            recon_unsupported("Watcom canonical stdout terminal line-buffer domain");
        is_terminal=isatty((int)os_handles[1]-1);errno=saved;
        if(!is_terminal)recon_unsupported("Watcom stdout terminal identity");
    }
    if(watcom_stdout_line_buffer) {
        if(size!=256 || watcom_stdout_line_size!=256 || stream->_flag!=0x102 ||
           stream->_base!=watcom_stdout_line_buffer || stream->_ptr!=stream->_base ||
           stream->_cnt || stream->_bufsiz!=256 ||
           ((uintptr_t)buffer<(uintptr_t)watcom_stdout_line_buffer+256 &&
            (uintptr_t)watcom_stdout_line_buffer<(uintptr_t)buffer+256))
            recon_unsupported("Watcom stdout replacement outside empty256B userbuffer");
    } else if(stream->_flag!=2 || stream->_base || stream->_ptr || stream->_cnt || stream->_bufsiz)
        recon_unsupported("Watcom supplied stdout initial state");
    /* The caller owns both sizes. The observed empty256B replacement neither
     * flushes nor frees its old permanent buffer; ownership stays with App.
     * Existing fwrite/printf paths reach fputc and the normal lowio flush. */
    stream->_base=stream->_ptr=buffer;stream->_bufsiz=size;stream->_cnt=0;
    stream->_flag|=0x100;watcom_stdout_line_buffer=buffer;watcom_stdout_line_size=size;
    return 0;
#else
    (void)stream;(void)buffer;(void)mode;(void)size;
    recon_unsupported("Watcom setvbuf requires explicit profile");return -1;
#endif
}
WEAK int recon_setvbuf(ShimFILE *s,char *buffer,int mode,size_t size)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    int saved_errno=errno; DWORD saved_error=recon_GetLastError();
    if (mode!=4 && (size<2 || size>0x7fffffffU || (mode!=0 && mode!=0x40))) return -1;
    size&=~(size_t)1;
    if (recon_flush(s)) recon_unsupported("MSVCRT510 setvbuf flush failure");
    recon_freebuf(s); s->_flag&=~0x3d0c;
    if (mode==4) { s->_base=(char *)&s->_charbuf; s->_bufsiz=2; s->_flag|=4; }
    else {
        s->_base=buffer?buffer:malloc(size);
        if (!s->_base) recon_unsupported("MSVCRT510 setvbuf allocation failure");
        s->_bufsiz=size; s->_flag|=buffer?0x500:0x408;
    }
    s->_ptr=s->_base; s->_cnt=0;
    errno=saved_errno; recon_SetLastError(saved_error); return 0;
#else
    recon_flush(s); recon_freebuf(s);
    if (mode==4) { s->_base=(char *)&s->_charbuf; s->_bufsiz=2; s->_flag|=4; }
    else if (!size || (mode!=0 && mode!=0x40)) return -1;
    else { s->_base=buffer?buffer:malloc(size); if (!s->_base) return -1; s->_bufsiz=size; s->_flag|=buffer?0x100:8; }
    s->_ptr=s->_base; s->_cnt=0; return 0;
#endif
}
WEAK long recon_ftell(ShimFILE *s)
{
    long position=recon_tell(s->_file);
    if (position<0) return -1;
    if (s->_flag&1) return position-s->_cnt;
    return position+(s->_base?s->_ptr-s->_base:0);
}
WEAK int recon_fseek(ShimFILE *s,long offset,int origin)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (!(s->_flag&0x83) || (origin!=SEEK_SET && origin!=SEEK_CUR && origin!=SEEK_END)) {
        errno=EINVAL;return -1;
    }
    s->_flag&=~0x10;
    if (origin==SEEK_CUR) { offset+=recon_ftell(s);origin=SEEK_SET; }
    recon_flush(s);
    if (s->_flag&0x80) s->_flag&=~3;
    else if ((s->_flag&1) && (s->_flag&8) && !(s->_flag&0x400)) s->_bufsiz=512;
#else
    if (origin==SEEK_CUR) { offset+=recon_ftell(s); origin=SEEK_SET; }
    if (recon_flush(s)) return -1;
    s->_flag&=~0x10;
#endif
    return recon_lseek(s->_file,offset,origin)<0?-1:0;
}
int recon_ungetc(int c,ShimFILE *s)
{
    if (c==EOF || !s->_base || s->_ptr==s->_base) return EOF;
    *--s->_ptr=c; ++s->_cnt; s->_flag&=~0x10; return (unsigned char)c;
}
int recon_ferror(ShimFILE *s) { return !!(s->_flag&0x20); }
int recon_feof(ShimFILE *s) { return !!(s->_flag&0x10); }
void recon_clearerr(ShimFILE *s) { s->_flag&=~0x30; }
void recon_rewind(ShimFILE *s) { recon_fseek(s,0,SEEK_SET); recon_clearerr(s); }
int recon_fileno(ShimFILE *s) { return s->_file; }
/* One host registration per callback retains ordering with ordinary atexit. */
typedef struct OnexitCallback {
    struct OnexitCallback *next;
    _onexit_t callback;
} OnexitCallback;
static OnexitCallback *onexit_callbacks;
static int onexit_running;
static void run_onexit_callback(void)
{
    OnexitCallback *entry=onexit_callbacks; _onexit_t callback;
    if (!entry || onexit_running) recon_unsupported("unreviewed nested _onexit dispatch");
    onexit_callbacks=entry->next; callback=entry->callback;
    munmap(entry,sizeof(*entry));
    onexit_running=1; (void)callback(); onexit_running=0;
}
WEAK _onexit_t recon_onexit(_onexit_t callback)
{
    OnexitCallback *entry; int saved_errno=errno;
    if (!callback || onexit_running) recon_unsupported("unreviewed _onexit NULL/nested registration");
    entry=mmap(NULL,sizeof(*entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (entry==MAP_FAILED) recon_unsupported("_onexit registration allocation failure");
    entry->callback=callback; entry->next=onexit_callbacks;
    if (atexit(run_onexit_callback)) recon_unsupported("_onexit host registration failure");
    onexit_callbacks=entry; errno=saved_errno;
    return callback;
}
WEAK _onexit_t recon_dllonexit(_onexit_t callback,_onexit_t **begin,_onexit_t **end)
{
    (void)callback; (void)begin; (void)end;
    recon_unsupported("unreviewed __dllonexit"); return NULL;
}

void recon_endstdio(void) { recon_fflush(NULL); }
void recon_rmtmp(void) { }
void recon_exit(int code)
{
    if (onexit_running) recon_unsupported("unreviewed exit from _onexit callback");
    recon_record_terminal_status((uint32_t)code);
    exit(code);
}

void recon_splitpath(const char *path,char *drive,char *directory,char *name,char *extension)
{
    const char *start=path,*slash=NULL,*dot=NULL,*p;
    if (drive) *drive=0;
    if (path[0] && path[1]==':') { if (drive) { drive[0]=path[0]; drive[1]=':'; drive[2]=0; } start+=2; }
    for (p=start;*p;++p) { if (*p=='/' || *p=='\\') { slash=p; dot=NULL; } else if (*p=='.') dot=p; }
    if (directory) { size_t n=slash?slash+1-start:0; memcpy(directory,start,n); directory[n]=0; }
    start=slash?slash+1:start;
    if (name) { size_t n=dot?(size_t)(dot-start):strlen(start); memcpy(name,start,n); name[n]=0; }
    if (extension) strcpy(extension,dot?dot:"");
}
void recon_makepath(char *out,const char *drive,const char *dir,const char *name,const char *ext)
{
    size_t n; *out=0;
    if (drive && *drive) { out[0]=*drive; out[1]=':'; out[2]=0; }
    if (dir && *dir) { strcat(out,dir); n=strlen(out); if (out[n-1]!='/' && out[n-1]!='\\') strcat(out,"\\"); }
    if (name) strcat(out,name);
    if (ext && *ext) { if (*ext!='.') strcat(out,"."); strcat(out,ext); }
}
char *recon_fullpath(char *out,const char *name,size_t capacity)
{
    const char *canonical=recon_native_entry_active()?NULL:getenv("RECON_CANONICAL_CWD"),*input=name&&*name?name:".";
    char host[PATH_MAX],absolute[PATH_MAX],normalized[PATH_MAX],check[PATH_MAX],cwd[PATH_MAX];
    char *part,*save,*result=out,drive='Z'; size_t len=0;
    int saved_errno=errno;
    if (canonical) {
        if (!canonical[0] || canonical[1]!=':' ||
            (canonical[2]!='\\' && canonical[2]!='/') ||
            recon_host_path(canonical,host,sizeof(host))<0 || !realpath(host,check) ||
            !getcwd(cwd,sizeof(cwd)) || strcmp(check,cwd)) { errno=EINVAL; return NULL; }
    }
    if (input[0] && input[1]==':') {
        if (recon_host_path(input,host,sizeof(host))<0) return NULL;
        drive=(char)toupper((unsigned char)input[0]);
        if (strlen(input+2)>=sizeof(absolute)) { errno=ENAMETOOLONG; return NULL; }
        strcpy(absolute,input+2);
    } else if (canonical && input[0]!='/') {
        drive=(char)toupper((unsigned char)canonical[0]);
        if (input[0]=='\\') {
            if (strlen(input)>=sizeof(absolute)) { errno=ENAMETOOLONG; return NULL; }
            strcpy(absolute,input);
        } else {
            if (strlen(canonical+2)+strlen(input)+2>sizeof(absolute)) { errno=ENAMETOOLONG; return NULL; }
            strcpy(absolute,canonical+2); strcat(absolute,"\\"); strcat(absolute,input);
        }
    } else {
        if (recon_host_path(input,host,sizeof(host))<0) return NULL;
        if (host[0]=='/') strcpy(absolute,host);
        else {
            if (!getcwd(absolute,sizeof(absolute))) return NULL;
            if (strlen(absolute)+strlen(host)+2>sizeof(absolute)) { errno=ENAMETOOLONG; return NULL; }
            strcat(absolute,"/"); strcat(absolute,host);
        }
    }
    normalized[0]=0;
    for (part=strtok_r(absolute,"/\\",&save);part;part=strtok_r(NULL,"/\\",&save)) {
        if (!strcmp(part,".")) continue;
        if (!strcmp(part,"..")) {
            while (len && normalized[len-1]!='\\') --len;
            if (len) --len;
            normalized[len]=0; continue;
        }
        normalized[len++]='\\'; strcpy(normalized+len,part); len+=strlen(part);
    }
    if (!len) { normalized[len++]='\\'; normalized[len]=0; }
    if (!result) { capacity=len+3; result=malloc(capacity); if (!result) return NULL; }
    if (capacity<len+3) { errno=ERANGE; return NULL; }
    result[0]=drive; result[1]=':'; strcpy(result+2,normalized); errno=saved_errno; return result;
}
/* VC1 _getcwd delegates to _getdcwd(0,...). A NULL destination allocates
   at least the requested capacity, even when that capacity is zero/negative;
   caller-owned storage is left untouched on ERANGE. */
char *recon_getcwd(char *out,int capacity)
{
    char cwd[PATH_MAX];
    size_t required,allocation;
    if (!recon_fullpath(cwd,".",sizeof(cwd))) return NULL;
    required=strlen(cwd)+1;
    if (out) {
        if (capacity<0 || (size_t)capacity<required) { errno=ERANGE; return NULL; }
    } else {
        allocation=capacity>0 && (size_t)capacity>required?(size_t)capacity:required;
        out=malloc(allocation);
        if (!out) return NULL;
    }
    memcpy(out,cwd,required);
    return out;
}
char *recon_strupr(char *s) { char *p; for (p=s;*p;++p) *p=toupper((unsigned char)*p); return s; }
char *recon_strlwr(char *s) { char *p; for (p=s;*p;++p) *p=tolower((unsigned char)*p); return s; }
char *recon_ultoa(unsigned long value,char *out,int radix)
{
    char b[33]; unsigned n=0,i;
    if (radix<2 || radix>36) { errno=EINVAL; *out=0; return out; }
    do { b[n++]="0123456789abcdefghijklmnopqrstuvwxyz"[value%radix]; value/=radix; } while (value);
    for (i=0;i<n;++i) out[i]=b[n-i-1];
    out[n]=0;
    return out;
}
char *recon_itoa(int n,char *s,int radix) { if (n<0 && radix==10) { *s='-'; recon_ultoa(0U-(unsigned)n,s+1,radix); } else recon_ultoa((unsigned)n,s,radix); return s; }
char *__cdecl recon_ltoa(long value,char *output,int radix)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    (void)value;(void)output;(void)radix;
    recon_unsupported("unreviewed Watcom _ltoa");return NULL;
#else
    if(radix!=10 || !output) recon_unsupported("_ltoa decimal output domain");
    return recon_itoa((int)value,output,10);
#endif
}
#ifdef __clang__
/* At O0 Clang otherwise emits an unused x87 load for this double formal.
 * Its native target attribute keeps the bit-only arm free of FP effects. */
__attribute__((target("no-x87")))
#endif
char *__cdecl recon_gcvt(double value,int digits,char *output)
{
#if !defined(RECON_CRT_PROFILE_MSVCRT_510) && !defined(RECON_CRT_PROFILE_WATCOM_100A)
    uint64_t bits,significand;unsigned exponent,negative,n,magnitude;int integer;
    size_t length;
    if(digits!=16 || (uintptr_t)output<4096 || (uintptr_t)output>UINTPTR_MAX-13 ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("VC1 gcvt integral output/default-C domain");
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("VC1 gcvt default-C handles");
    /* Decode the binary64 argument without changing host FP state. This
     * finite arm needs no decimal rounding: every admitted integer is exact. */
    memcpy(&bits,&value,sizeof(bits));negative=(unsigned)(bits>>63);
    exponent=(unsigned)((bits>>52)&0x7ff);significand=bits&UINT64_C(0xfffffffffffff);
    if(!exponent && !significand && !negative)magnitude=0;
    else {
        if(exponent<1023 || exponent>1054)
            recon_unsupported("VC1 gcvt nonintegral/nonfinite/range");
        exponent-=1023;significand|=UINT64_C(0x10000000000000);
        if(significand&((UINT64_C(1)<<(52-exponent))-1))
            recon_unsupported("VC1 gcvt fractional argument");
        magnitude=(unsigned)(significand>>(52-exponent));
        if(magnitude>(negative?0x80000000U:0x7fffffffU))
            recon_unsupported("VC1 gcvt signed32 range");
    }
    integer=negative?(magnitude==0x80000000U?INT_MIN:-(int)magnitude):(int)magnitude;
    recon_itoa(integer,output,10);length=strlen(output);
    /* The identified old GCVT trims zeroes after the decimal point but
     * retains the point itself, so integral 7.0 is spelled "7.". */
    output[length]='.';output[length+1]=0;return output;
#else
    (void)value;(void)digits;(void)output;
    recon_unsupported("unreviewed profile _gcvt");return NULL;
#endif
}
ReconMktempObservation recon_mktemp_observation;
static unsigned mktemp_process_owner;
static void mktemp_expected_state(const char *text,unsigned values[3])
{
    unsigned n; char *end; unsigned long number; int saved_errno=errno;
    if (!text) recon_unsupported("strict mktemp missing prestate");
    for (n=0;n<3;++n) {
        if (*text<'0' || *text>'9') recon_unsupported("strict mktemp invalid prestate");
        errno=0;number=strtoul(text,&end,10);
        if (errno || number>UINT_MAX || (n<2?*end!=',':*end!=0))
            recon_unsupported("strict mktemp invalid prestate");
        values[n]=(unsigned)number; text=end+1;
    }
    errno=saved_errno;
}
static char *mktemp_strict(char *pattern,const char *expected,const char *suffix)
{
    ReconMktempObservation *record=&recon_mktemp_observation;
    char path[PATH_MAX],parent[PATH_MAX],*slash,*end;
    const char *setting=getenv("RECON_MKTEMP_FD");
    struct stat info; unsigned state[3]; size_t length,i; long fd;
    int before_errno=errno; DWORD before_last=recon_GetLastError();
    unsigned before_dos=recon_doserrno;
    if ((unsigned)getpid()!=mktemp_process_owner || record->count)
        recon_unsupported("strict mktemp repeated/child invocation");
    if (!pattern || !expected || !suffix || strcmp(pattern,expected))
        recon_unsupported("strict mktemp unknown template");
    length=strlen(pattern);
    if (length<6 || length>=MAX_PATH || strcmp(pattern+length-6,"XXXXXX") ||
        (length>6 && pattern[length-7]=='X') || strlen(suffix)!=6)
        recon_unsupported("strict mktemp invalid template/suffix");
    for (i=0;i<6;++i) if (!((suffix[i]>='a' && suffix[i]<='z') ||
        (suffix[i]>='A' && suffix[i]<='Z') || (suffix[i]>='0' && suffix[i]<='9')))
        recon_unsupported("strict mktemp nonASCII suffix");
    mktemp_expected_state(getenv("RECON_MKTEMP_PRESTATE"),state);
    if ((unsigned)before_errno!=state[0] || before_dos!=state[1] || before_last!=state[2])
        recon_unsupported("strict mktemp entry state mismatch");
    memset(record,0,sizeof(*record)); record->magic=0x31544d52;record->version=1;
    record->owner=getpid();record->count=1;record->argument=(uintptr_t)pattern;
    record->before_errno=before_errno;record->before_doserrno=before_dos;
    record->before_last_error=before_last;strcpy(record->before,pattern);
    memcpy(pattern+length-6,suffix,6);
    if (recon_host_path(pattern,path,sizeof(path))) recon_unsupported("strict mktemp invalid path");
    strcpy(parent,path);slash=strrchr(parent,'/');
    if (slash) {if(slash==parent)slash[1]=0;else *slash=0;} else strcpy(parent,".");
    if (stat(parent,&info) || !S_ISDIR(info.st_mode)) recon_unsupported("strict mktemp missing parent");
    if (!lstat(path,&info) || errno!=ENOENT) recon_unsupported("strict mktemp collision/unknown access failure");
    /* Canonical _access calls GetFileAttributesA, then _dosmaperr. For this
     * observed missing file in an existing directory, both Win32 errors are2.
     * _mktemp restores its saved errno after the failed existence check. */
    recon_SetLastError(ERROR_FILE_NOT_FOUND);recon_doserrno=ERROR_FILE_NOT_FOUND;errno=before_errno;
    record->result=(uintptr_t)pattern;record->after_errno=errno;
    record->after_doserrno=recon_doserrno;record->after_last_error=recon_GetLastError();
    strcpy(record->after,pattern);
    if (setting) {
        if (*setting<'0' || *setting>'9') recon_unsupported("strict mktemp invalid receipt fd");
        fd=strtol(setting,&end,10);
        if (*end || fd<3 || fd>INT_MAX || fstat(fd,&info) || !S_ISREG(info.st_mode) ||
            info.st_size!=sizeof(*record) || pwrite(fd,record,sizeof(*record),0)!=sizeof(*record))
            recon_unsupported("strict mktemp receipt write");
    }
    errno=before_errno; return pattern;
}
char *recon_mktemp(char *pattern)
{
    const char *fixed=getenv("RECON_MKTEMP_SUFFIX"),*expected=getenv("RECON_MKTEMP_TEMPLATE");
    size_t length,i; unsigned pid=(unsigned)getpid();
    if (expected) return mktemp_strict(pattern,expected,fixed);
    if (getenv("RECON_MKTEMP_PRESTATE") || getenv("RECON_MKTEMP_FD"))
        recon_unsupported("mktemp receipt/prestate without strict template");
    length=strlen(pattern);
    int saved_errno=errno,collision; char letter=0;
    if (length<6 || strcmp(pattern+length-6,"XXXXXX") ||
        (length>6 && pattern[length-7]=='X')) return NULL;
    if (fixed) {
        if (strlen(fixed)!=6) recon_unsupported("RECON_MKTEMP_SUFFIX requires six alphanumeric characters");
        for (i=0;i<6;++i) if (!isalnum((unsigned char)fixed[i]))
            recon_unsupported("invalid RECON_MKTEMP_SUFFIX");
        memcpy(pattern+length-6,fixed,6);
    } else for (i=0;i<6;++i) { pattern[length-1-i]=(char)('0'+pid%10); pid/=10; }
    for (;;) {
        collision=recon_access(pattern,0)==0 || errno==EACCES;
        if (!collision) { errno=saved_errno; return pattern; }
        if (letter=='z') { errno=saved_errno; return NULL; }
        letter=letter?(char)(letter+1):'a'; pattern[length-6]=letter;
    }
}
unsigned recon_tempnam_counter=1,recon_tempnam_prefix_length;
static char *tempnam_libc100(const char *dir,const char *prefix)
{
    const char *base=recon_getenv("TMP"),*cursor;
    char host[PATH_MAX],digits[10],*path; struct stat information;
    size_t length,prefix_length,used,n=0; unsigned value; int saved;
    if (recon_mb_codepage || recon_lc_codepage || recon_lc_handle[2] || recon_mb_cur_max!=1)
        recon_unsupported("libc100 tempnam unreviewed locale");
    if (!dir || strcmp(dir,"\\") || !prefix || !*prefix || !base || !*base)
        recon_unsupported("libc100 tempnam unreviewed directory/prefix/TMP");
    prefix_length=strlen(prefix); length=strlen(base);
    if (prefix_length>5 || length<3 || length>=MAX_PATH ||
        !((base[0]>='A' && base[0]<='Z') || (base[0]>='a' && base[0]<='z')) ||
        base[1]!=':' || (base[2]!='\\' && base[2]!='/'))
        recon_unsupported("libc100 tempnam unreviewed prefix/absolute TMP");
    for (cursor=prefix;*cursor;++cursor)
        if ((unsigned char)*cursor>=128 || strchr("\\/:;\"*?",*cursor))
            recon_unsupported("libc100 tempnam nonASCII/path prefix");
    for (cursor=base;*cursor;++cursor)
        if ((unsigned char)*cursor>=128 || strchr(";\"*?",*cursor))
            recon_unsupported("libc100 tempnam nonASCII/wildcard TMP");
    if (recon_access(base,0)) recon_unsupported("libc100 tempnam unavailable TMP");
    saved=errno;
    if (recon_host_path(base,host,sizeof(host)) || stat(host,&information) || !S_ISDIR(information.st_mode))
        recon_unsupported("libc100 tempnam TMP is not an existing directory");
    errno=saved;
    path=malloc(length+prefix_length+8);
    if (!path) recon_unsupported("libc100 tempnam allocation failure");
    errno=saved;
    memcpy(path,base,length); used=length;
    if (path[used-1]!='\\' && path[used-1]!='/') path[used++]='\\';
    memcpy(path+used,prefix,prefix_length); used+=prefix_length;
    /* These are the proved initialized counter/prefix-length globals, not a
     * fixture filename seed. The provider resets when prefix length grows. */
    if (prefix_length>recon_tempnam_prefix_length) recon_tempnam_counter=1;
    recon_tempnam_prefix_length=prefix_length;
    if (recon_tempnam_counter==UINT_MAX)
        recon_unsupported("libc100 tempnam counter overflow");
    value=++recon_tempnam_counter;
    do { digits[n++]=(char)('0'+value%10); value/=10; } while (value);
    if (prefix_length+n>8 || n>6) recon_unsupported("libc100 tempnam basename overflow");
    while (n) path[used++]=digits[--n];
    path[used]=0;
    if (!recon_access(path,0)) recon_unsupported("libc100 tempnam candidate collision");
    if (errno!=ENOENT || recon_doserrno!=2 || recon_GetLastError()!=2)
        recon_unsupported("libc100 tempnam unreviewed candidate failure");
    /* The original direct access leaves errno/DOS/LastError2. It neither
     * creates the candidate nor restores errno like the separate mktemp API. */
    return path;
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static char *tempnam510_current_directory(const char *dir,const char *prefix)
{
    const char *base;char candidate[254],*path;
    struct stat information;size_t prefix_bytes=0,n;
    int saved=errno;unsigned long dos=recon_doserrno;DWORD error=recon_GetLastError();
    if(!recon_crt_arguments_bound || !recon_crt_environment || dir ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("MSVCRT510 tempnam arguments/namespace");
    /* Shipped MSVCRT copies the full optional prefix, not just five bytes.
     * Its directory length + prefix length + 12 <= MAX_PATH guard admits247
     * prefix bytes for TMP="."; six native unique suffix bytes still fit. */
    if(prefix) {
        if((uintptr_t)prefix<4096 || (uintptr_t)prefix>UINTPTR_MAX-248)
            recon_unsupported("MSVCRT510 tempnam prefix span");
        prefix_bytes=strnlen(prefix,248);
        if(prefix_bytes==248)recon_unsupported("MSVCRT510 tempnam prefix length");
        for(n=0;n<prefix_bytes;++n)
            if((unsigned char)prefix[n]<32 || (unsigned char)prefix[n]>=127 ||
               strchr("\\/:*?\"<>|",prefix[n]))
                recon_unsupported("MSVCRT510 tempnam ASCII leaf prefix");
    }
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("MSVCRT510 tempnam default-C handles");
    base=recon_getenv("TMP");
    if(!base || strcmp(base,".") || lstat(".",&information) || !S_ISDIR(information.st_mode))
        recon_unsupported("MSVCRT510 tempnam target TMP current directory");
    if(prefix_bytes)memcpy(candidate,prefix,prefix_bytes);
    memcpy(candidate+prefix_bytes,"XXXXXX",7);
    /* Names are internal open/remove identities. Native uniqueness is real;
     * no original spelling/counter is injected, and no file is reserved here. */
    if(mktemp(candidate)!=candidate || !candidate[0])
        recon_unsupported("MSVCRT510 tempnam native unique-name failure");
    for(n=prefix_bytes;n<prefix_bytes+6;++n)if(!((candidate[n]>='A' && candidate[n]<='Z') ||
        (candidate[n]>='a' && candidate[n]<='z') || (candidate[n]>='0' && candidate[n]<='9')))
        recon_unsupported("MSVCRT510 tempnam native leaf spelling");
    path=malloc(prefix_bytes+9);if(!path)recon_unsupported("MSVCRT510 tempnam result allocation");
    path[0]='.';path[1]='\\';memcpy(path+2,candidate,prefix_bytes+7);
    if(recon_access(path,0)!=-1 || errno!=ENOENT || recon_doserrno!=2 ||
       recon_GetLastError()!=ERROR_FILE_NOT_FOUND)
        recon_unsupported("MSVCRT510 tempnam genuine absent leaf");
    errno=saved;recon_doserrno=dos;recon_SetLastError(error);return path;
}
#endif
char *recon_tempnam(const char *dir,const char *prefix)
{
    const char *profile=getenv("RECON_TEMPNAM_PROFILE");
    if (profile) {
        if (strcmp(profile,"libc100")) recon_unsupported("unknown tempnam CRT profile");
        return tempnam_libc100(dir,prefix);
    }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    return tempnam510_current_directory(dir,prefix);
#else
    char *path; const char *base=dir?dir:getenv("TMP");
    if (!base) base=".";
    if (asprintf(&path,"%s\\%sXXXXXX",base,prefix?prefix:"")<0) return NULL;
    if (!recon_mktemp(path)) { free(path); return NULL; } return path;
#endif
}
char *__cdecl recon_tmpnam(char *buffer)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char candidate[]="RCXXXXXX";unsigned n;
    int saved_errno=errno,target_errno=*recon_watcom_errno();
    unsigned long saved_dos=recon_doserrno;DWORD saved_error=recon_GetLastError();
    if(!buffer || (uintptr_t)buffer>UINTPTR_MAX-13)
        recon_unsupported("Watcom tmpnam caller13B storage");
    /* This qualified name is internal, never name-sensitive, and deleted by
     * the caller. Native unique identity is sufficient; no historical PID or
     * AAA/A..P naming sequence is reproduced. mktemp creates no file. */
    if(mktemp(candidate)!=candidate || !candidate[0])
        recon_unsupported("Watcom tmpnam native unique-name failure");
    for(n=2;n<8;++n)
        if(!((candidate[n]>='A' && candidate[n]<='Z') ||
             (candidate[n]>='a' && candidate[n]<='z') ||
             (candidate[n]>='0' && candidate[n]<='9')))
            recon_unsupported("Watcom tmpnam native leaf spelling");
    if(recon_access(candidate,0)!=-1 || errno!=ENOENT ||
       recon_doserrno!=2 || recon_GetLastError()!=ERROR_FILE_NOT_FOUND)
        recon_unsupported("Watcom tmpnam requires genuine absent leaf");
    memcpy(buffer,candidate,sizeof(candidate));
    *recon_watcom_errno()=target_errno;errno=saved_errno;
    recon_doserrno=saved_dos;recon_SetLastError(saved_error);return buffer;
#else
    (void)buffer;recon_unsupported("tmpnam requires qualified Watcom profile");return NULL;
#endif
}
char *recon_getpath(const char *path,char *out,int capacity)
{
    int n=0,quoted=0;
    while (*path==';') ++path;
    if (!*path) return NULL;
    while (*path && (*path!=';' || quoted)) {
        if (*path=='"') quoted=!quoted;
        else if (n+1<capacity) out[n++]=*path;
        ++path;
    }
    out[n]=0; return (char *)(*path?path+1:path);
}
int recon_isctype(int c,int flags) { return c>=-1 && c<256?(recon_pctype[c]&flags):0; }
/* Canonical WCHAR16 comparison returns the code-unit difference, not its sign.
 * A zero count is valid without readable pointers. Other spans must be valid. */
WEAK int __cdecl recon_wcsncmp(const WCHAR *left,const WCHAR *right,size_t count)
{
    unsigned a,b;
    if (!count) return 0;
    if (!left || !right) recon_unsupported("wcsncmp null positive-count span");
    do {
        a=*left++;b=*right++;
        if (a!=b) return (int)a-(int)b;
        if (!a) return 0;
    } while (--count);
    return 0;
}
int recon_wcscmp(const WCHAR *a,const WCHAR *b)
{
    while (*a && *a==*b) { ++a; ++b; }
    return *a<*b?-1:*a>*b;
}
WEAK int recon_toupper(int c) { return toupper(c); }
WEAK int recon_tolower(int c) { return tolower(c); }
WEAK void recon_tzset(void) { tzset(); recon_timezone=timezone; recon_daylight=daylight; }
WEAK int recon_isindst(struct tm *tm) { return tm->tm_isdst>0; }
/* MSVCRT's asctime path writes two decimal day digits. Keep the established
 * native host calendar/TZ policy; do not expose its space-padded day spelling.
 * Returned storage is per-thread and overwritten by the next successful call;
 * sharing that storage with other calendar APIs is not qualified here. */
char *__cdecl recon_ctime(const time_t *clock)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    static __thread char result[26];
    char text[26];int saved_errno=errno,*target_errno=recon_errno();
    int saved_target=*target_errno;unsigned long saved_dos=recon_doserrno;
    DWORD saved_error=recon_GetLastError();
    typedef char TimeWidth[sizeof(time_t)==4?1:-1];
    (void)sizeof(TimeWidth);
    if(!clock || (uintptr_t)clock>UINTPTR_MAX-sizeof(*clock))
        recon_unsupported("ctime input span");
    /* The shipped localtime returns NULL for negative signed32 timestamps. */
    if(*clock<0)return NULL;
    if(!ctime_r(clock,text) || text[24]!='\n' || text[25] ||
       text[3]!=' ' || text[7]!=' ' || text[10]!=' ' ||
       text[13]!=':' || text[16]!=':' || text[19]!=' ' ||
       (text[8]!=' ' && (text[8]<'0' || text[8]>'3')) ||
       text[9]<'0' || text[9]>'9')
        recon_unsupported("ctime native calendar result");
    if(text[8]==' ')text[8]='0';
    memcpy(result,text,sizeof(result));
    errno=saved_errno;*target_errno=saved_target;recon_doserrno=saved_dos;
    recon_SetLastError(saved_error);return result;
#else
    (void)clock;recon_unsupported("ctime explicit MSVCRT510 profile");return NULL;
#endif
}
time_t recon_time(time_t *out)
{
    const char *fixed=getenv("RECON_FIXED_TIME"); time_t value=fixed?(time_t)strtoll(fixed,NULL,10):time(NULL);
    if (out) *out=value;
    return value;
}
int recon_spawnvp(int mode,const char *file,const char *const *argv) { return recon_spawnvpe(mode,file,argv,NULL); }
static int spawn_list(int search,int mode,const char *file,const char *first,va_list args)
{
    const char *argv[256]; unsigned n=0; argv[n++]=first;
    while (n<256 && (argv[n]=va_arg(args,const char *))!=NULL) ++n;
    if (n==256) { errno=E2BIG; return -1; }
    return search?recon_spawnvp(mode,file,argv):recon_spawnve(mode,file,argv,NULL);
}
int recon_spawnl(int mode,const char *file,const char *first,...)
{
    va_list args; int n; va_start(args,first); n=spawn_list(0,mode,file,first,args); va_end(args); return n;
}
int recon_spawnlp(int mode,const char *file,const char *first,...)
{
    va_list args; int n; va_start(args,first); n=spawn_list(1,mode,file,first,args); va_end(args); return n;
}
__attribute__((constructor)) static void initialize_crt(void)
{
    int i; FILE *command; char program[PATH_MAX]; size_t bytes,total=1; char **e,*cursor;
    mktemp_process_owner=getpid();
    for (i=0;i<256;++i) pushed_byte[i]=-1;
    for (i=0;i<3;++i) {
        os_handles[i]=(long)recon_GetStdHandle((DWORD)(-10-i)); recon_osfile[i]=0x81;
        if (recon_GetFileType((HANDLE)os_handles[i])==FILE_TYPE_PIPE) recon_osfile[i]|=8;
        else if (recon_GetFileType((HANDLE)os_handles[i])==FILE_TYPE_CHAR) recon_osfile[i]|=0x40;
        recon_iob[i]._file=i; recon_iob[i]._flag=i?2:1;
    }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    recon_iob[0]._ptr=recon_iob[0]._base=recon_stdin_buffer;
    recon_iob[0]._flag=0x101;recon_iob[0]._bufsiz=sizeof(recon_stdin_buffer);
#endif
    /* VC1 buffers redirected stderr just like stdout. Abrupt traps leave
     * pending bytes in FILE; normal exit/fflush emits them. */
    if (recon_osfile[2]&0x40) recon_iob[2]._flag|=4;
    for (i=0;i<256;++i) {
        unsigned bits=0;
        if (isupper(i)) bits|=1;
        if (islower(i)) bits|=2;
        if (isdigit(i)) bits|=4;
        if (isspace(i)) bits|=8;
        if (ispunct(i)) bits|=16;
        if (iscntrl(i)) bits|=32;
        if (i==' ') bits|=64;
        if (isxdigit(i)) bits|=128;
        recon_pctype[i]=bits;
    }
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    memcpy(initial_ctype_510,character_flags,sizeof(character_flags));
    msvcrt510_mb_flags(recon_mbctype);
#endif
    if (getenv("RECON_PROGRAM_PATH")) recon_pgmptr=strdup(getenv("RECON_PROGRAM_PATH"));
    else {
        command=fopen("/proc/self/cmdline","rb");
        bytes=command?fread(program,1,sizeof(program)-1,command):0;
        if (command) fclose(command);
        program[bytes]=0;
        recon_pgmptr=strdup(bytes?program:"LINK.EXE");
    }
    for (e=environ;*e;++e) total+=strlen(*e)+1;
    recon_aenvptr=malloc(total); if (!recon_aenvptr) _exit(125);
    cursor=recon_aenvptr;
    for (e=environ;*e;++e) { strcpy(cursor,*e); cursor+=strlen(*e)+1; }
    *cursor=0;
    atexit(recon_endstdio);
}

/* The VC1 scanner accumulates integer conversions modulo 2^32. glibc's
 * strtol-based scanner saturates overflowing long fields in COFF archives.
 * Implement the conversion families actually used by LINK, without changing
 * the reconstructed archive reader or adding terminators to its raw records. */
int recon_sscanf(const char *input,const char *format,...)
{
    const char *start=input;
    va_list args;
    int assigned=0,converted=0;
    va_start(args,format);
    while (*format) {
        int suppress=0,width=INT_MAX,length=0,base=10,negative=0,digits=0;
        unsigned long value=0;
        char conversion;
        if (isspace((unsigned char)*format)) {
            while (isspace((unsigned char)*format)) ++format;
            while (isspace((unsigned char)*input)) ++input;
            continue;
        }
        if (*format!='%') {
            if (*input!=*format) break;
            ++input; ++format; continue;
        }
        ++format;
        if (*format=='%') { if (*input!='%') break; ++input; ++format; continue; }
        if (*format=='*') { suppress=1; ++format; }
        if (isdigit((unsigned char)*format)) {
            width=0;
            while (isdigit((unsigned char)*format)) width=width*10+*format++-'0';
        }
        if (*format=='h') { length=-1; ++format; }
        else if (*format=='l') { length=1; ++format; }
        conversion=*format++;
        if (conversion=='n') {
            if (!suppress) *va_arg(args,int *)=input-start;
            ++converted; continue;
        }
        if (conversion!='c') while (isspace((unsigned char)*input)) ++input;
        if (!*input) { if (!converted) assigned=EOF; break; }
        if (conversion=='s' || conversion=='c') {
            char *output=suppress?NULL:va_arg(args,char *);
            int n=0;
            if (conversion=='c' && width==INT_MAX) width=1;
            while (n<width && *input && (conversion=='c' || !isspace((unsigned char)*input))) {
                if (output) output[n]=*input;
                ++n; ++input;
            }
            if (!n) break;
            if (output) { if (conversion=='s') output[n]=0; ++assigned; }
            ++converted; continue;
        }
        if (!strchr("diuoxX",conversion)) recon_unsupported("sscanf conversion");
        if (*input=='+' || *input=='-') { negative=*input=='-'; ++input; --width; }
        if (conversion=='o') base=8;
        else if (conversion=='x' || conversion=='X') base=16;
        else if (conversion=='i') base=0;
        if ((base==0 || base==16) && width>2 && input[0]=='0' && (input[1]=='x' || input[1]=='X') && isxdigit((unsigned char)input[2])) {
            base=16; input+=2; width-=2;
        } else if (!base) base=*input=='0'?8:10;
        while (width>0 && *input) {
            unsigned char c=*input; unsigned digit;
            if (c>='0' && c<='9') digit=c-'0';
            else if (c>='a' && c<='f') digit=c-'a'+10;
            else if (c>='A' && c<='F') digit=c-'A'+10;
            else break;
            if (digit>=(unsigned)base) break;
            value=value*base+digit; ++input; --width; ++digits;
        }
        if (!digits) break;
        if (negative) value=0UL-value;
        if (!suppress) {
            if (length<0) *va_arg(args,unsigned short *)=(unsigned short)value;
            else if (length>0) *va_arg(args,unsigned long *)=value;
            else *va_arg(args,unsigned *)=value;
            ++assigned;
        }
        ++converted;
    }
    va_end(args);
    return assigned;
}

static void sort_swap(unsigned char *a,unsigned char *b,size_t width)
{
    if (a!=b) while (width--) { unsigned char c=*a; *a++=*b; *b++=c; }
}
/* The identified 1994 Watcom variant predates the later hybrid qsort.
 * Its forward partition and equal-tail skip give visible equal-key ordering.
 * Opt-in is separate from the general Watcom CRT profile. */
void recon_watcom_qsort_1994(void *array,size_t count,size_t width,
                           int (*compare)(const void *,const void *))
{
#if defined(RECON_CRT_PROFILE_WATCOM_100A) && defined(RECON_QSORT_WATCOM_1994)
    unsigned char *base=array,*pending_base[32];size_t pending_count[32];
    unsigned pending=0;
    if(count<2)return;
    if(!array || !compare || !width || count>SIZE_MAX/width ||
       (uintptr_t)array>UINTPTR_MAX-count*width)
        recon_unsupported("Watcom1994 qsort array/span/width/comparator");
    for(;;) {
        while(count>1) {
            size_t scan,partition=0,last_different=0,left_count,right_count;
            unsigned char *right;
            if(count==2) {
                if(compare(base,base+width)>0)sort_swap(base,base+width,width);
                break;
            }
            sort_swap(base,base+(count/2)*width,width);
            for(scan=1;scan<count;++scan) {
                int order=compare(base+scan*width,base);
                if(order<=0) {
                    ++partition;
                    if(scan!=partition)sort_swap(base+scan*width,base+partition*width,width);
                }
                if(order)last_different=partition;
            }
            if(partition==count-1 && !last_different)break;
            if(partition)sort_swap(base,base+partition*width,width);
            right_count=count-partition-1;
            left_count=partition==last_different?partition:last_different+1;
            right=base+(partition+1)*width;
            if(pending==32)recon_unsupported("Watcom1994 qsort pending ranges");
            /* Save the larger range and sort the smaller in this frame. */
            if(left_count<right_count) {
                pending_base[pending]=right;pending_count[pending++]=right_count;
                count=left_count;
            } else {
                pending_base[pending]=base;pending_count[pending++]=left_count;
                base=right;count=right_count;
            }
        }
        if(!pending)break;
        --pending;base=pending_base[pending];count=pending_count[pending];
    }
#else
    (void)array;(void)count;(void)width;(void)compare;
    recon_unsupported("Watcom1994 qsort requires explicit variant/profile");
#endif
}

/* VC1 QSORT.OBJ: midpoint pivot, <=8-element maximum-selection shortsort.
 * This tie ordering is observable in LINK's serialized import tables.
 * Verified against LINK.EXE [0x519d0,0x51b40) and shortsort [0x51b40,0x51ba0).
 * Host qsort need not preserve the ordering of equal keys. */
void recon_qsort(void *array,size_t count,size_t width,int (*compare)(const void *,const void *))
{
    unsigned char *base=array;
    size_t lo=0,hi,low_stack[32],high_stack[32];
    unsigned stack=0;
    if (count<2 || !width) return;
    hi=count-1;
    for (;;) {
        if (hi-lo+1<=8) {
            size_t end;
            for (end=hi;end>lo;--end) {
                size_t max=lo,p;
                for (p=lo+1;p<=end;++p)
                    if (compare(base+p*width,base+max*width)>0) max=p;
                sort_swap(base+max*width,base+end*width,width);
            }
        } else {
            size_t left=lo,right=hi+1;
            sort_swap(base+(lo+(hi-lo+1)/2)*width,base+lo*width,width);
            for (;;) {
                do ++left; while (left<=hi && compare(base+left*width,base+lo*width)<=0);
                do --right; while (right>lo && compare(base+right*width,base+lo*width)>=0);
                if (right<left) break;
                sort_swap(base+left*width,base+right*width,width);
            }
            sort_swap(base+lo*width,base+right*width,width);
            /* Push the larger side, iterate the smaller: bounded stack. The
             * original compares byte distances, including its minus-one byte. */
            if ((long)((right-lo)*width)-1 >= (long)(hi*width)-(long)(left*width)) {
                if (lo+1<right) { low_stack[stack]=lo; high_stack[stack++]=right-1; }
                if (left<hi) { lo=left; continue; }
            } else {
                if (left<hi) { low_stack[stack]=left; high_stack[stack++]=hi; }
                if (lo+1<right) { hi=right-1; continue; }
            }
        }
        if (!stack) break;
        --stack; lo=low_stack[stack]; hi=high_stack[stack];
    }
}

/* CRT variants supplied by a target override these native fallbacks. */
ShimFILE *recon_freopen(const char *name,const char *mode,ShimFILE *stream)
{
    if ((stream->_flag&0x83) && recon_fclose(stream)==EOF) return NULL;
    stream->_base=stream->_ptr=NULL; stream->_cnt=stream->_flag=0;
    return recon_openfile(name,mode,0x40,stream);
}
void recon_setbuf(ShimFILE *stream,char *buffer)
{
    recon_setvbuf(stream,buffer,buffer?0:4,buffer?512:0);
}
int recon_flushall(void)
{
    int n,count=0;
    for (n=0;n<20;++n) if (recon_iob[n]._flag) {
        if (recon_flush(&recon_iob[n])) return EOF;
        ++count;
    }
    return count;
}
int recon_fcloseall(void)
{
    int n,count=0;
    for (n=3;n<20;++n) if (recon_iob[n]._flag) {
        if (recon_fclose(&recon_iob[n])) return EOF;
        ++count;
    }
    return count;
}
int recon_flsbuf(int byte,ShimFILE *stream) { return recon_fputc(byte,stream); }
int _fileno(ShimFILE *stream) { return recon_fileno(stream); }
int recon_spawnv(int mode,const char *file,const char *const *argv)
{
    return recon_spawnve(mode,file,argv,NULL);
}
int recon_dup(int source)
{
    int dest,fd;
    if (!valid_descriptor(source)) return -1;
    if (recon_fsopen_restricted((HANDLE)os_handles[source])) recon_unsupported("dup restricted _fsopen file");
    dest=recon_alloc_osfhnd(); if (dest<0) return -1;
    fd=dup((int)os_handles[source]-1);
    if (fd<0) { recon_osfile[dest]=0; return -1; }
    if (recon_set_osfhnd(dest,fd+1)) recon_unsupported("dup descriptor ownership");
    recon_osfile[dest]=recon_osfile[source];
    return dest;
}
int recon_dup2(int source,int dest)
{
    int host,old;
    if (!valid_descriptor(source)) return -1;
    if (dest<0 || (unsigned)dest>=recon_nhandle) { errno=EBADF; return -1; }
    if (recon_fsopen_restricted((HANDLE)os_handles[source]) ||
        ((recon_osfile[dest]&1) && recon_fsopen_restricted((HANDLE)os_handles[dest])))
        recon_unsupported("dup2 restricted _fsopen file");
    if (source==dest) return 0;
    old=(recon_osfile[dest]&1)?(int)os_handles[dest]-1:-1;
    if (dest<3) {
        host=dup2((int)os_handles[source]-1,dest);
        if (host<0) return -1;
        if (old>=3 && old!=host) close(old);
    } else {
        host=dup((int)os_handles[source]-1); if (host<0) return -1;
        if (old>=0) close(old);
    }
    os_handles[dest]=host+1; recon_osfile[dest]=recon_osfile[source];
    if (dest<3) recon_SetStdHandle((DWORD)(-10-dest),(HANDLE)(long)(host+1));
    return 0;
}
static ReconSignalHandler break_handler;
static void dispatch_break(int signo)
{
    (void)signo;
    if (break_handler && break_handler!=SIG_IGN && break_handler!=SIG_DFL)
        break_handler(SIGBREAK);
}
ReconSignalHandler recon_signal(int signo,ReconSignalHandler handler)
{
    ReconSignalHandler old;
    if (signo!=SIGBREAK) return signal(signo,handler);
    old=break_handler;
    if (signal(SIGQUIT,handler==SIG_IGN?SIG_IGN:handler==SIG_DFL?SIG_DFL:dispatch_break)==SIG_ERR)
        return SIG_ERR;
    break_handler=handler;
    return old;
}

WEAK int recon_unlink(const char *name)
{
    char path[PATH_MAX],parent[PATH_MAX],*separator;
    struct stat information;
    int saved=errno,failure;
    unsigned long dos=recon_doserrno,code;
    DWORD error=recon_GetLastError();
    if (!name || !*name || strpbrk(name,"*?") ||
        recon_host_path(name,path,sizeof(path))<0)
        recon_unsupported("_unlink path translation");
    if (!lstat(path,&information)) {
        if (!S_ISREG(information.st_mode))
            recon_unsupported("_unlink unreviewed file kind");
        if (!unlink(path)) {
            errno=saved;recon_doserrno=dos;recon_SetLastError(error);
            return 0;
        }
    }
    failure=errno;
    if (failure!=ENOENT && failure!=ENOTDIR)
        recon_unsupported("_unlink unreviewed deletion failure");
    /* DeleteFileA distinguishes a missing leaf from a missing or non-directory
     * parent. Only those two observed failures enter the CRT error mapper. */
    strcpy(parent,path);separator=strrchr(parent,'/');
    if (!separator) strcpy(parent,".");
    else if (separator==parent) separator[1]=0;
    else *separator=0;
    if (!stat(parent,&information)) code=S_ISDIR(information.st_mode)?2:3;
    else {
        failure=errno;
        if (failure!=ENOENT && failure!=ENOTDIR)
            recon_unsupported("_unlink unreviewed parent failure");
        code=3;
    }
    recon_SetLastError(code);recon_dosmaperr(code);
    return -1;
}

/* Canonical default-C multibyte state: only encountered ASCII is supported. */
static void ascii_multibyte_scope(void)
{
    if (recon_mb_codepage || recon_mb_cur_max!=1 || recon_lc_codepage || recon_lc_handle[2])
        recon_unsupported("unreviewed multibyte codepage/locale");
}
/* Only the separately probed ASCII APIs below accept the encountered1252
 * state. Other conversions/searches retain their default-C guard. */
static void ascii_encountered_locale_scope(void)
{
    if (!recon_mb_codepage && !recon_lc_codepage && !recon_lc_handle[2] && recon_mb_cur_max==1) return;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if (locale_profile_applied && recon_mb_codepage==1252 && recon_lc_codepage==1252 &&
        recon_mb_cur_max==1 && recon_pctype==locale_ctype_510+1) {
        unsigned char mb[257];unsigned short ctype[257];unsigned n;
        for(n=0;n<6;++n) if(recon_lc_handle[n]!=(n==2?1033:0))
            recon_unsupported("ASCII API encountered locale handles");
        msvcrt510_mb_flags(mb);msvcrt510_locale_ctype(mb,ctype);
        if(memcmp(recon_mbctype,mb,sizeof(mb)) || memcmp(recon_pctype-1,ctype,sizeof(ctype)))
            recon_unsupported("ASCII API encountered locale tables");
        return;
    }
#endif
    recon_unsupported("ASCII API unreviewed codepage/locale/profile");
}
/* The encountered diagnostic owner supplies a readable2048-byte local span.
 * For ASCII in the validated SBCS state, character and byte counts coincide.
 * The bound/wrap checks do not establish allocation ownership. */
size_t __cdecl recon_mbstrlen(const char *text)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    size_t length;unsigned char value;
    if((uintptr_t)text<4096 || (uintptr_t)text>UINTPTR_MAX-2048)
        recon_unsupported("_mbstrlen caller-readable ASCII span");
    if(recon_mb_codepage_gate || recon_mb_cur_max!=1)
        recon_unsupported("_mbstrlen multibyte gate/width");
    ascii_encountered_locale_scope();
    for(length=0;length<2048;++length) {
        value=(unsigned char)text[length];
        if(!value)return length;
        if(value>=128)recon_unsupported("_mbstrlen nonASCII input");
    }
    recon_unsupported("_mbstrlen missing bounded terminator");return 0;
#else
    (void)text;recon_unsupported("unreviewed _mbstrlen");return 0;
#endif
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
/* Reached isolated CP1252/SBCS ASCII conversion. Caller owns the one readable
 * source byte and any two writable output bytes; span checks do not prove that. */
static void mbtowc_1252_scope(WCHAR *destination,const char *source,size_t count,unsigned char value)
{
    uintptr_t out=(uintptr_t)destination,in=(uintptr_t)source;
    if(count!=1 || value>=128 || ctype_selection.bound!=1 || ctype_selection.calls!=1 ||
       ctype_selection.mb_calls!=1 || recon_mb_codepage_gate ||
       !ctype_selection.result || strcmp(ctype_selection.result,"English_United States.1252"))
        recon_unsupported("mbtowc isolated1252 ASCII selection/domain");
    ascii_encountered_locale_scope();
    if(in>UINTPTR_MAX-1 || (destination && ((out&1) || out>UINTPTR_MAX-2 ||
       (out<in+1 && in<out+2))))
        recon_unsupported("mbtowc isolated1252 disjoint WCHAR16 output span");
}
#endif
/* Internal native process-provider gate, not a target CRT API. */
void recon_crt_validate_spawn_profile(void)
{
    if (!recon_crt_arguments_bound || !recon_crt_os_environment ||
        recon_mb_codepage!=1252 || recon_lc_codepage!=1252)
        recon_unsupported("spawn profile target namespace/locale");
    ascii_encountered_locale_scope();
}
/* Separately observed LINK-only child: initial static default-C tables and
 * borrowed namespaces, without the C1 locale/environment transition. */
void recon_crt_validate_spawn_cp0_profile(void)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned char mb[257];unsigned n;
    if (!recon_crt_arguments_bound || !os_environment_bound ||
        !link_spawn_environment_valid() ||
        recon_crt_environment_updates || locale_profile_applied ||
        recon_mb_codepage || recon_lc_codepage || recon_mb_cur_max!=1 ||
        recon_pctype!=character_flags+1)
        recon_unsupported("CP0 spawn target namespace/locale");
    for(n=0;n<6;++n) if(recon_lc_handle[n])
        recon_unsupported("CP0 spawn locale handles");
    msvcrt510_mb_flags(mb);
    if(memcmp(recon_mbctype,mb,sizeof(mb)) ||
        memcmp(character_flags,initial_ctype_510,sizeof(initial_ctype_510)))
        recon_unsupported("CP0 spawn initial tables");
#else
    recon_unsupported("CP0 spawn requires MSVCRT510 profile");
#endif
}
WEAK int recon_mbsncmp(const unsigned char *left,const unsigned char *right,size_t count)
{
    unsigned char a,b;
    if (!count) return 0;
    ascii_encountered_locale_scope();
    if (!left || !right) recon_unsupported("null _mbsncmp input");
    while (count--) {
        a=*left++; b=*right++;
        if (a>127 || b>127) recon_unsupported("non-ASCII _mbsncmp input");
        if (a!=b) return a<b?-1:1;
        if (!a) return 0;
    }
    return 0;
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
/* Same-thread caller keeps the current line alive until exhausted/reset.
 * This finite native cursor is not a model of Windows' private PTD storage. */
static unsigned char *mbstok_cursor,*mbstok_line_end;
static int mbstok_line_bound;
#endif
WEAK unsigned char *__cdecl recon_mbstok(unsigned char *text,const unsigned char *delimiters)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    size_t length,start,end,n;
    uintptr_t input,separator=(uintptr_t)delimiters;
    if(!delimiters || separator<4096 || separator>UINTPTR_MAX-3 ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 ||
       recon_lc_codepage || locale_profile_applied || ctype_selection.bound)
        recon_unsupported("_mbstok ASCII input/default-C scope");
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("_mbstok default-C locale handles");
    if(delimiters[0]!=' ' || delimiters[1]!='\t' || delimiters[2])
        recon_unsupported("_mbstok exact space/tab delimiter scope");
    if(text) {
        input=(uintptr_t)text;
        if(input<4096 || input>UINTPTR_MAX-4096)
            recon_unsupported("_mbstok fresh line pointer span");
        for(length=0;length<4096 && text[length];++length)
            if(text[length]>=128)recon_unsupported("_mbstok nonASCII line");
        if(length==4096)recon_unsupported("_mbstok fresh line bound");
    } else {
        if(!mbstok_line_bound)recon_unsupported("_mbstok unbound continuation");
        if(!mbstok_cursor)return NULL;
        text=mbstok_cursor;input=(uintptr_t)text;
        if(input>(uintptr_t)mbstok_line_end || (uintptr_t)mbstok_line_end-input>=4096)
            recon_unsupported("_mbstok continuation saved extent");
        length=(uintptr_t)mbstok_line_end-input;
        for(n=0;n<length;++n)if(!text[n] || text[n]>=128)
            recon_unsupported("_mbstok continuation changed ASCII extent");
        if(text[length])recon_unsupported("_mbstok continuation changed terminator");
    }
    if(input<separator+3 && separator<input+length+1)
        recon_unsupported("_mbstok delimiter overlaps line");
    /* The same-thread caller owns the live line. Validate before any token
     * write; only a qualified fresh call establishes/reset this continuation. */
    mbstok_line_end=text+length;mbstok_line_bound=1;
    start=0;while(start<length && (text[start]==' ' || text[start]=='\t'))++start;
    if(start==length){mbstok_cursor=NULL;return NULL;}
    end=start;while(end<length && text[end]!=' ' && text[end]!='\t')++end;
    if(end<length){text[end]=0;mbstok_cursor=text+end+1;}
    else mbstok_cursor=NULL;
    return text+start;
#else
    (void)text;(void)delimiters;
    recon_unsupported("unreviewed MSVCRT _mbstok");return NULL;
#endif
}
WEAK unsigned char *recon_mbsinc(const unsigned char *text)
{
    ascii_encountered_locale_scope();
    if (!text || *text>127) recon_unsupported("non-ASCII/null _mbsinc input");
    return (unsigned char *)text+1;
}
WEAK unsigned char *recon_mbsdec(const unsigned char *start,const unsigned char *current)
{
    ascii_multibyte_scope();
    if (!start || !current) recon_unsupported("null _mbsdec input");
    if ((uintptr_t)current<=(uintptr_t)start) return NULL;
    if (current[-1]>127) recon_unsupported("non-ASCII _mbsdec input");
    return (unsigned char *)current-1;
}
WEAK void recon_mbccpy(unsigned char *destination,const unsigned char *source)
{
    ascii_encountered_locale_scope();
    if (!destination || !source || *source>127) recon_unsupported("non-ASCII/null _mbccpy input");
    *destination=*source;
}
WEAK int recon_ismbcspace(unsigned int value)
{
    ascii_encountered_locale_scope();
    if (value>127) recon_unsupported("non-ASCII _ismbcspace input");
    return value==' ' || (value>='\t' && value<='\r')?8:0;
}

WEAK unsigned char *recon_mbsrchr(const unsigned char *text,unsigned int value)
{
    const unsigned char *last=NULL; unsigned char byte;
    ascii_multibyte_scope();
    if (!text || value>127) recon_unsupported("non-ASCII/null _mbsrchr argument");
    do {
        byte=*text;
        if (byte>127) recon_unsupported("non-ASCII _mbsrchr text");
        if (byte==value) last=text;
        ++text;
    } while (byte);
    return (unsigned char *)last;
}
WEAK int recon_mbsnbicmp(const unsigned char *left,const unsigned char *right,size_t bytes)
{
    unsigned char a,b;
    if (!bytes) return 0;
    ascii_multibyte_scope();
    if (!left || !right) recon_unsupported("null _mbsnbicmp input");
    while (bytes--) {
        a=*left++; b=*right++;
        if (a>127 || b>127) recon_unsupported("non-ASCII _mbsnbicmp input");
        if (a>='A' && a<='Z') a+='a'-'A';
        if (b>='A' && b<='Z') b+='a'-'A';
        if (a!=b) return a<b?-1:1;
        if (!a) return 0;
    }
    return 0;
}
WEAK unsigned char *recon_mbsstr(const unsigned char *text,const unsigned char *needle)
{
    const unsigned char *cursor;
    ascii_multibyte_scope();
    if (!text || !needle) recon_unsupported("_mbsstr null argument");
    for (cursor=text;*cursor;++cursor)
        if (*cursor>=128) recon_unsupported("_mbsstr nonASCII text");
    for (cursor=needle;*cursor;++cursor)
        if (*cursor>=128) recon_unsupported("_mbsstr nonASCII needle");
    /* The canonical CP0 branch delegates to ordinary strstr. Its result is
     * borrowed from text, including text itself for an empty needle. */
    return (unsigned char *)strstr((const char *)text,(const char *)needle);
}
/* Canonical byte-domain CP1252 mapping after the independently observed
 * isolated LC_CTYPE and SBCS selection. Wide values retain their boundary. */
WEAK unsigned int __cdecl recon_mbctolower(unsigned int character)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(character>255 || ctype_selection.bound!=1 || ctype_selection.calls!=1 ||
       ctype_selection.mb_calls!=1 || !locale_profile_applied ||
       !locale_ctype_510 || recon_pctype!=locale_ctype_510+1 ||
       recon_mb_codepage!=1252 || recon_mb_codepage_gate ||
       recon_lc_codepage!=1252 || recon_mb_cur_max!=1 ||
       !ctype_selection.result || strcmp(ctype_selection.result,"English_United States.1252") ||
       memcmp(character_flags,initial_ctype_510,sizeof(character_flags)))
        recon_unsupported("unreviewed _mbctolower state/domain");
    ascii_encountered_locale_scope();
    /* The whole canonical <=255 leaf only reads byte flags/case mapping;
     * integer operations preserve incoming errors and floating-point state. */
    if((character>='A' && character<='Z') ||
       (character>=192 && character<=214) ||
       (character>=216 && character<=222)) return character+32;
    switch(character) {
    case 138:return 154;
    case 140:return 156;
    case 142:return 158;
    case 159:return 255;
    default:return character;
    }
#else
    (void)character;recon_unsupported("unreviewed _mbctolower profile");return 0;
#endif
}
WEAK unsigned char *recon_mbsupr(unsigned char *text)
{ (void)text; recon_unsupported("unreviewed _mbsupr"); return NULL; }
/* Observed default-C ASCII span, with no concurrent target locale mutation.
 * Original lock25/critical-section allocation is provider-private: its live
 * initialization and bytes were not observed and are not reproduced here. */
WEAK unsigned char *__cdecl recon_mbslwr(unsigned char *text)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned char *cursor;
    if (!text || ctype_selection.bound || ctype_selection.calls || ctype_selection.mb_calls ||
        errno!=2 || recon_doserrno!=2 || recon_GetLastError())
        recon_unsupported("_mbslwr default-C span/error state");
    isolated_ctype_initial_state();
    /* Reject the complete unsupported span before changing its first byte. */
    for (cursor=text;*cursor;++cursor)
        if (*cursor>=128) recon_unsupported("_mbslwr nonASCII span");
    for (cursor=text;*cursor;++cursor)
        if (*cursor>='A' && *cursor<='Z') *cursor=(unsigned char)(*cursor+('a'-'A'));
    return text;
#else
    (void)text;recon_unsupported("_mbslwr requires MSVCRT510 profile");return NULL;
#endif
}
WEAK unsigned char *recon_mbspbrk(const unsigned char *text,const unsigned char *accept)
{
    const unsigned char *cursor,*set;
    ascii_multibyte_scope();
    if (!text || !accept) recon_unsupported("NULL _mbspbrk argument");
    for (set=accept;*set;++set)
        if (*set>=128) recon_unsupported("nonASCII _mbspbrk set");
    for (cursor=text;*cursor;++cursor) {
        if (*cursor>=128) recon_unsupported("nonASCII _mbspbrk text");
        if (strchr((const char *)accept,*cursor)) return (unsigned char *)cursor;
    }
    return NULL;
}
WEAK unsigned char *recon_mbschr(const unsigned char *text,unsigned int value)
{
    unsigned char byte;
    ascii_encountered_locale_scope();
    if (!text || value>127) recon_unsupported("non-ASCII/null _mbschr argument");
    do {
        byte=*text;
        if (byte>127) recon_unsupported("non-ASCII _mbschr text");
        if (byte==value) return (unsigned char *)text;
        ++text;
    } while (byte);
    return NULL;
}

WEAK int recon_mbsicmp(const unsigned char *left,const unsigned char *right)
{
    unsigned char a,b;
    ascii_multibyte_scope();
    if (!left || !right) recon_unsupported("null _mbsicmp input");
    for (;;) {
        a=*left++;b=*right++;
        if (a>127 || b>127) recon_unsupported("nonASCII _mbsicmp input");
        if (a>='A' && a<='Z') a+='a'-'A';
        if (b>='A' && b<='Z') b+='a'-'A';
        if (a!=b) return a<b?-1:1;
        if (!a) return 0;
    }
}
int __cdecl recon_mbsnbcmp(const unsigned char *left,const unsigned char *right,size_t bytes)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    (void)left;(void)right;(void)bytes;
    recon_unsupported("_mbsnbcmp Microsoft profile required");return 0;
#else
    size_t index;unsigned char a,b;
    ascii_multibyte_scope();
    if(recon_mb_codepage_gate) recon_unsupported("_mbsnbcmp multibyte gate");
    if(!bytes) return 0;
    if(!left || !right) recon_unsupported("_mbsnbcmp NULL input");
    for(index=0;index<bytes;++index) {
        if(index>=UINTPTR_MAX-(uintptr_t)left || index>=UINTPTR_MAX-(uintptr_t)right)
            recon_unsupported("_mbsnbcmp wrapped input");
        a=left[index];b=right[index];
        if(a>=128 || b>=128) recon_unsupported("_mbsnbcmp nonASCII compared units");
        if(a!=b) return a<b?-1:1;
        if(!a) return 0;
    }
    return 0;
#endif
}
int __cdecl recon_mbscmp(const unsigned char *left,const unsigned char *right)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    /* The canonical gate-zero branch is a case-sensitive unsigned compare.
     * Reuse the guarded ASCII loop through its first mismatch or NUL. */
    return recon_mbsnbcmp(left,right,(size_t)-1);
#else
    (void)left;(void)right;
    recon_unsupported("_mbscmp MSVCRT510 profile required");return 0;
#endif
}
WEAK unsigned char *recon_mbsnbcpy(unsigned char *out,const unsigned char *text,size_t bytes)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    (void)out;(void)text;(void)bytes;
    recon_unsupported("_mbsnbcpy Microsoft profile required");return NULL;
#else
    size_t copied=0,read_bytes;uintptr_t destination=(uintptr_t)out,source=(uintptr_t)text;
    ascii_multibyte_scope();
    if (recon_mb_codepage_gate) recon_unsupported("_mbsnbcpy multibyte gate");
    if (!bytes) return out;
    if (!out || !text || bytes>UINTPTR_MAX-destination)
        recon_unsupported("_mbsnbcpy NULL/wrapped span");
    /* The canonical gate-zero path delegates to strncpy. Validate only its
     * bounded read span, before any writes; text[bytes] need not exist. */
    while (copied<bytes) {
        if (copied>=UINTPTR_MAX-source)
            recon_unsupported("_mbsnbcpy wrapped source");
        if (!text[copied]) break;
        if (text[copied]>=128) recon_unsupported("_mbsnbcpy nonASCII span");
        ++copied;
    }
    read_bytes=copied<bytes?copied+1:bytes;
    if (destination<source?bytes>source-destination:read_bytes>destination-source)
        recon_unsupported("_mbsnbcpy overlapping spans");
    if (copied) memcpy(out,text,copied);
    if (copied<bytes) memset(out+copied,0,bytes-copied);
    return out;
#endif
}

/* LINK's private exported MSVCRT boundary: callback types and stack arguments
 * are known, but no demangling/allocation behavior has been qualified. */
char *__cdecl recon_unDName(char *output,const char *name,int capacity,
                           void *(__cdecl *allocate)(size_t),
                           void (__cdecl *release)(void *),unsigned short flags)
{
    (void)output;(void)name;(void)capacity;(void)allocate;(void)release;(void)flags;
    recon_unsupported("unreviewed __unDName");return NULL;
}

int __cdecl recon_wcsicmp(const WCHAR *left,const WCHAR *right)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned a,b;
    ascii_multibyte_scope();
    if(!left || !right) recon_unsupported("_wcsicmp NULL argument");
    for(;;) {
        a=*left++;b=*right++;
        if(a>=128 || b>=128) recon_unsupported("_wcsicmp nonASCII comparison prefix");
        if(a>='A' && a<='Z') a+='a'-'A';
        if(b>='A' && b<='Z') b+='a'-'A';
        if(a!=b || !a) return (int)a-(int)b;
    }
#else
    (void)left;(void)right;
    recon_unsupported("unreviewed _wcsicmp");return 0;
#endif
}
WEAK WCHAR *recon_wcscat(WCHAR *out,const WCHAR *text)
{
    WCHAR *end=out;
    if (!out || !text) recon_unsupported("UTF16 wcscat NULL argument");
    while (*end) ++end;
    while ((*end++=*text++)!=0) {}
    return out;
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int format_apstudio_macro(WCHAR *out,const WCHAR *format,va_list args,int reference)
{
    static const WCHAR expected[]={'%','c','%','s','%','c','%','s','%','c','%','d','%','c','%','c','%','c',0};
    static const WCHAR reference_format[]={' ','%','c','"','%','s','%','c','%','s','%','c','%','d','"',0};
    const WCHAR *name,*text,*layout=reference?reference_format:expected;WCHAR *cursor=out;
    size_t pattern_bytes=reference?sizeof(reference_format):sizeof(expected);
    int first,second,third,line,fourth=0,flag=0,last=0;
    size_t name_length,text_length,number_length,total,n;
    uintptr_t destination=(uintptr_t)out,pattern=(uintptr_t)format;
    char number[12];
    if(destination<4096 || pattern<4096 || (destination&1) || (pattern&1) ||
       destination>UINTPTR_MAX-4096 || pattern>UINTPTR_MAX-pattern_bytes ||
       (destination<pattern+pattern_bytes && pattern<destination+4096) ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 ||
       locale_profile_applied || ctype_selection.bound)
        recon_unsupported("APSTUDIO swprintf output/format/default-C scope");
    for(n=0;n<6;++n)if(recon_lc_handle[n])
        recon_unsupported("APSTUDIO swprintf locale handles");
    for(n=0;n<pattern_bytes/sizeof(WCHAR);++n)
        if(format[n]!=layout[n])recon_unsupported("APSTUDIO swprintf exact format");
    first=va_arg(args,int);name=va_arg(args,const WCHAR *);
    second=va_arg(args,int);text=va_arg(args,const WCHAR *);
    third=va_arg(args,int);line=va_arg(args,int);
    if(!reference){fourth=va_arg(args,int);flag=va_arg(args,int);last=va_arg(args,int);}
    if(first!=(reference?0xe001:0xe000) || second!=0xe002 || third!=0xe002 ||
       (!reference && (fourth!=0xe002 || last!=0xe002 ||
        ((unsigned)flag!=0xffffff80U && (unsigned)flag!=0xffffff81U))))
        recon_unsupported("APSTUDIO swprintf marker/flag domain");
    if((uintptr_t)name<4096 || (uintptr_t)text<4096 ||
       ((uintptr_t)name&1) || ((uintptr_t)text&1) ||
       (uintptr_t)name>UINTPTR_MAX-4096 || (uintptr_t)text>UINTPTR_MAX-4096)
        recon_unsupported("APSTUDIO swprintf string spans");
    for(name_length=0;name_length<2048 && name[name_length];++name_length)
        if(name[name_length]>=128)recon_unsupported("APSTUDIO swprintf ASCII name");
    for(text_length=0;text_length<2048 && text[text_length];++text_length)
        if(text[text_length]>=128)recon_unsupported("APSTUDIO swprintf ASCII text");
    if(name_length==2048 || text_length==2048 ||
       (destination<(uintptr_t)name+(name_length+1)*2 && (uintptr_t)name<destination+4096) ||
       (destination<(uintptr_t)text+(text_length+1)*2 && (uintptr_t)text<destination+4096))
        recon_unsupported("APSTUDIO swprintf string bound/output overlap");
    recon_itoa(line,number,10);number_length=strlen(number);
    total=name_length+text_length+number_length+6;
    if(total>=2048)recon_unsupported("APSTUDIO swprintf caller capacity");
    /* The exact caller owns2048 WCHARs. Every source/extent is validated before
     * output; %c keeps the low16 WCHAR of the genuine promoted int argument. */
    if(reference)*cursor++=' ';
    *cursor++=(WCHAR)first;
    if(reference)*cursor++='"';
    for(n=0;n<name_length;++n)*cursor++=name[n];
    *cursor++=(WCHAR)second;
    for(n=0;n<text_length;++n)*cursor++=text[n];
    *cursor++=(WCHAR)third;
    for(n=0;n<number_length;++n)*cursor++=(unsigned char)number[n];
    if(reference)*cursor++='"';
    else {*cursor++=(WCHAR)fourth;*cursor++=(WCHAR)flag;*cursor++=(WCHAR)last;}
    *cursor=0;
    return (int)total;
}
#endif
WEAK int recon_swprintf(WCHAR *out,const WCHAR *format,...)
{
    static const WCHAR line_prefix[]={'#','l','i','n','e',' ','%','d'};
    const WCHAR *suffix;
    WCHAR digits[10],*end=out;
    unsigned prefix_length=6;
    unsigned value,n=0,i;
    int number;
    va_list args;
    if (!out || !format || recon_lc_handle[2] || recon_lc_codepage)
        recon_unsupported("legacy UTF16 swprintf outside default-C scope");
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(((uintptr_t)format&1) || (uintptr_t)format>UINTPTR_MAX-4)
        recon_unsupported("legacy swprintf format selection span");
    if(format[0]=='%') {
        va_start(args,format);number=format_apstudio_macro(out,format,args,0);va_end(args);
        return number;
    }
    if(format[0]==' ') {
        va_start(args,format);number=format_apstudio_macro(out,format,args,1);va_end(args);
        return number;
    }
    if(format[0]=='#' && format[1]=='p') {
        static const WCHAR pragma_format[]={'#','p','r','a','g','m','a',' ','c','o','d','e','_','p','a','g','e',' ','%','d','\r','\n',0};
        uintptr_t source=(uintptr_t)format,destination=(uintptr_t)out;
        /* Caller owns at least32 WCHARs; the actual RC scratch is1024B.
         * The old #line path retains its previously qualified contract. */
        if((source&1) || (destination&1) || source>UINTPTR_MAX-sizeof(pragma_format) ||
           destination>UINTPTR_MAX-64 || (destination<source+sizeof(pragma_format) && source<destination+64) ||
           recon_mb_codepage || recon_mb_cur_max!=1)
            recon_unsupported("pragma swprintf span/default-C scope");
        for(i=0;i<sizeof(pragma_format)/sizeof(pragma_format[0]);++i)
            if(format[i]!=pragma_format[i])recon_unsupported("pragma swprintf exact format");
        prefix_length=18;suffix=format+20;
    } else
#endif
    {
        for (i=0;i<sizeof(line_prefix)/sizeof(line_prefix[0]);++i)
            if (format[i]!=line_prefix[i])
                recon_unsupported("unreviewed legacy UTF16 swprintf format");
        if (!((format[8]==' ' && !format[9]) ||
              (format[8]=='\r' && format[9]=='\n' && !format[10])))
            recon_unsupported("unreviewed legacy UTF16 swprintf suffix");
        suffix=format+8;
    }
    va_start(args,format);number=va_arg(args,int);va_end(args);
    for (i=0;i<prefix_length;++i) *end++=format[i];
    value=(unsigned)number;
    if (number<0) { *end++='-';value=0u-value; }
    do { digits[n++]=(WCHAR)('0'+value%10);value/=10; } while (value);
    while (n) *end++=digits[--n];
    while (*suffix) *end++=*suffix++;
    *end=0;
    return (int)(end-out);
}
WEAK int recon_iswctype(WCHAR character,WCHAR mask)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(mask==128 || mask==0x107) {
        if(character>=128 || recon_lc_handle[2] || recon_lc_codepage ||
           recon_mb_codepage || recon_mb_cur_max!=1)
            recon_unsupported("iswctype mask outside MSVCRT510 default-C ASCII scope");
        /* Exact pristine wide-table AND: ASCII letters do not carry bit100. */
        if(mask==0x107) {
            if(character>='A' && character<='Z')return 1;
            if(character>='a' && character<='z')return 2;
            return character>='0' && character<='9'?4:0;
        }
        return ((character>='0' && character<='9') ||
                (character>='A' && character<='F') ||
                (character>='a' && character<='f'))?128:0;
    }
#endif
    if (character>=128 || (mask!=4 && mask!=8) || recon_lc_handle[2] || recon_lc_codepage)
        recon_unsupported("iswctype outside default-C ASCII digit/space scope");
    if (mask==4) return character>='0' && character<='9'?4:0;
    return character==' ' || (character>='\t' && character<='\r')?8:0;
}

WEAK WCHAR __cdecl recon_towlower(WCHAR character)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    /* The authenticated provider's default-C path is integer-only and reads
     * no character table. Its caller consumes AX, not unspecified upper EAX. */
    if (character>=128 || recon_lc_handle[2] || recon_lc_codepage ||
        recon_mb_codepage || recon_mb_cur_max!=1)
        recon_unsupported("towlower outside MSVCRT510 default-C ASCII scope");
    return character>='A' && character<='Z'?character+('a'-'A'):character;
#else
    (void)character;
    recon_unsupported("unreviewed towlower");
    return 0;
#endif
}
WEAK WCHAR recon_towupper(WCHAR character)
{
    if (character>=128 || recon_lc_handle[2] || recon_lc_codepage)
        recon_unsupported("towupper outside default-C ASCII scope");
    return character>='a' && character<='z'?character-('a'-'A'):character;
}

WEAK size_t recon_mbclen(const unsigned char *text)
{
    ascii_encountered_locale_scope();
    if (!text || *text>=128) recon_unsupported("NULL/nonASCII _mbclen input");
    return 1;
}
WEAK size_t recon_mbsspn(const unsigned char *text,const unsigned char *accept)
{
    const unsigned char *cursor,*set;
    ascii_encountered_locale_scope();
    if (!text || !accept) recon_unsupported("NULL _mbsspn argument");
    for (set=accept;*set;++set)
        if (*set>=128) recon_unsupported("nonASCII _mbsspn set");
    for (cursor=text;*cursor;++cursor) {
        if (*cursor>=128) recon_unsupported("nonASCII _mbsspn text");
        if (!strchr((const char *)accept,*cursor)) break;
    }
    return (size_t)(cursor-text);
}

/* Only the reached SBCS selection from the already established LC_CTYPE
 * is supported. Other codepages never forward to a host locale operation. */
WEAK int __cdecl recon_setmbcp(int codepage)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned char mb[257];unsigned short table[257];unsigned n;DWORD error;
    if(codepage!=-4 || ctype_selection.bound!=1 || ctype_selection.calls!=1 ||
       ctype_selection.mb_calls || !locale_profile_applied ||
       !locale_ctype_510 || recon_pctype!=locale_ctype_510+1 ||
       recon_lc_codepage!=1252 || recon_mb_codepage || recon_mb_codepage_gate ||
       recon_mb_cur_max!=1 || !ctype_selection.result ||
       strcmp(ctype_selection.result,"English_United States.1252"))
        recon_unsupported("unreviewed _setmbcp selection");
    error=recon_GetLastError();
    if(errno || recon_doserrno || (error!=0 && error!=203))
        recon_unsupported("SBCS locale codepage entry errors");
    for(n=0;n<6;++n) if(recon_lc_handle[n]!=(n==2?1033:0))
        recon_unsupported("SBCS locale codepage handles");
    msvcrt510_mb_flags(mb);msvcrt510_locale_ctype(mb,table);
    if(memcmp(recon_mbctype,mb,sizeof(mb)) ||
       memcmp(recon_pctype-1,table,sizeof(table)) ||
       memcmp(character_flags,initial_ctype_510,sizeof(character_flags)))
        recon_unsupported("SBCS locale codepage tables");
    /* The existing static SBCS byte flags need no regeneration; no lead/trail
     * gate is enabled. The public _getmbcp representation remains distinct. */
    recon_mb_codepage=(int)recon_lc_codepage;++ctype_selection.mb_calls;
    return 0;
#else
    (void)codepage;
    recon_unsupported("unreviewed _setmbcp selection");return -1;
#endif
}

/* Empty LC_CTYPE selection has one explicit captured native-host profile.
 * Other selections never forward Microsoft category numbers to host libc. */
WEAK char *recon_setlocale(int category,const char *locale)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    unsigned short table[257],*allocated;char *result;
    int saved=errno;unsigned long dos=recon_doserrno;DWORD error=recon_GetLastError();
    if(ctype_selection.bound!=1 || ctype_selection.calls || ctype_selection.mb_calls || category!=2 || !locale || *locale)
        recon_unsupported("unreviewed setlocale selection");
    /* Independently captured C1XX and C1 calls preserve these exact tuples;
     * no other entry-error behavior is inferred. */
    if(saved || dos || (error!=203 && error!=0))
        recon_unsupported("isolated LC_CTYPE entry errors");
    isolated_ctype_initial_state();
    msvcrt510_locale_ctype(recon_mbctype,table);
    allocated=malloc(sizeof(table));
    result=malloc(sizeof("English_United States.1252"));
    if(!allocated || !result) recon_unsupported("isolated LC_CTYPE allocation failure");
    memcpy(allocated,table,sizeof(table));
    strcpy(result,"English_United States.1252");
    /* Native ownership is explicit. Original proof covers initialized spans
     * and their writable lifetime, without inferring original heap extents. */
    locale_ctype_510=allocated;recon_pctype=allocated+1;
    recon_lc_codepage=1252;recon_lc_handle[2]=1033;
    locale_profile_applied=1;ctype_selection.result=result;ctype_selection.calls=1;
    errno=saved;recon_doserrno=dos;recon_SetLastError(error);
    return result;
#else
    (void)category;(void)locale;
    recon_unsupported("unreviewed setlocale selection");return NULL;
#endif
}

WEAK WCHAR *recon_wcschr(const WCHAR *text,WCHAR character)
{
    if (!text) recon_unsupported("NULL UTF16 wcschr input");
    for (;;) {
        if (*text==character) return (WCHAR *)text;
        if (!*text) return NULL;
        ++text;
    }
}

WEAK WCHAR *__cdecl recon_wcsupr(WCHAR *text)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    WCHAR *p;
    if (!text || recon_lc_handle[2] || recon_lc_codepage ||
        recon_mb_codepage || recon_mb_cur_max!=1)
        recon_unsupported("_wcsupr outside MSVCRT510 default-C ASCII scope");
    /* Validate before any writes, including when a later code unit is outside
     * this supported subset. The provider's default path needs no tables. */
    for(p=text;*p;++p) if(*p>=128)
        recon_unsupported("_wcsupr nonASCII span");
    for(p=text;*p;++p) if(*p>='a' && *p<='z') *p-='a'-'A';
    return text;
#else
    (void)text;
    recon_unsupported("unreviewed _wcsupr");
    return NULL;
#endif
}
WEAK WCHAR *recon_wcsdup(const WCHAR *text)
{
    (void)text;recon_unsupported("unreviewed UTF16 _wcsdup");return NULL;
}
WEAK long recon_wcstol(const WCHAR *text,WCHAR **end,int base)
{
    const WCHAR *cursor;
    unsigned long value=0,limit;
    unsigned digit;
    int negative=0,overflow=0;
    ascii_multibyte_scope();
    if (!text || end || base!=10) recon_unsupported("unreviewed UTF16 wcstol arguments");
    for (cursor=text;*cursor;++cursor)
        if (*cursor>=128) recon_unsupported("nonASCII UTF16 wcstol text");
    cursor=text;
    while (*cursor==' ' || (*cursor>='\t' && *cursor<='\r')) ++cursor;
    if (*cursor=='-' || *cursor=='+') { negative=*cursor=='-';++cursor; }
    limit=negative?0x80000000UL:0x7fffffffUL;
    while (*cursor>='0' && *cursor<='9') {
        digit=*cursor++-'0';
        if (value>limit/10 || (value==limit/10 && digit>limit%10)) overflow=1;
        else if (!overflow) value=value*10+digit;
    }
    if (overflow) { errno=ERANGE;return negative?LONG_MIN:LONG_MAX; }
    if (negative) return value==0x80000000UL?LONG_MIN:-(long)value;
    return (long)value;
}

#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void validate_os_environment(const WCHAR *block,size_t units)
{
    size_t n;
    if (!block || units<3 || units>65536 || !block[0] || block[units-1] || block[units-2])
        recon_unsupported("target OS environment extent/terminator");
    for (n=0;n<units;++n) {
        if (block[n]>=128) recon_unsupported("target OS environment nonASCII units");
        if (n+1<units && !block[n] && !block[n+1] && n!=units-2)
            recon_unsupported("target OS environment early terminator");
    }
}
static size_t target_environment_count(char **vector)
{
    size_t n,k;
    if (!vector) recon_unsupported("unbound target CRT environment");
    for (n=0;vector[n];++n) {
        if (n==16384) recon_unsupported("target CRT environment count");
        for (k=0;vector[n][k];++k)
            if (k==32768 || (unsigned char)vector[n][k]>=128)
                recon_unsupported("target CRT environment nonASCII/length");
    }
    return n;
}
static int ascii_environment_byte_equal(unsigned a,unsigned b)
{
    if (a>='A' && a<='Z') a+='a'-'A';
    if (b>='A' && b<='Z') b+='a'-'A';
    return a==b;
}
static int target_environment_has_name(const char *name,size_t length)
{
    char **entry; const char *equals; size_t n;
    const WCHAR *wide;
    for (entry=recon_crt_environment;*entry;++entry) {
        equals=strchr(*entry,'=');
        if (!equals || (size_t)(equals-*entry)!=length) continue;
        for (n=0;n<length && ascii_environment_byte_equal((unsigned char)(*entry)[n],(unsigned char)name[n]);++n) {}
        if (n==length) return 1;
    }
    for (wide=recon_crt_os_environment;*wide;) {
        for (n=0;wide[n] && wide[n]!='=';++n) {}
        if (n==length && wide[n]=='=') {
            for (n=0;n<length && ascii_environment_byte_equal(wide[n],(unsigned char)name[n]);++n) {}
            if (n==length) return 1;
        }
        while (*wide) ++wide;
        ++wide;
    }
    return 0;
}
#endif
/* Only the derived LINK OS namespace may supplement the original CP0 block.
 * Private metadata is checked again before the ordinary spawn consumes it. */
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int link_spawn_environment_valid(void)
{
    size_t n,length=sizeof(link_os_assignment);
    if(!link_os_environment)return recon_crt_os_environment==initial_os_environment;
    if(recon_crt_os_environment!=link_os_environment ||
       recon_crt_os_environment_units!=link_os_environment_units ||
       link_os_environment_units!=initial_os_environment_units+length ||
       link_os_insertion>initial_os_environment_units-1)
        return 0;
    validate_os_environment(initial_os_environment,initial_os_environment_units);
    validate_os_environment(link_os_environment,link_os_environment_units);
    if(memcmp(link_os_environment,initial_os_environment,link_os_insertion*sizeof(WCHAR)))return 0;
    for(n=0;n<length;++n)if(link_os_environment[link_os_insertion+n]!=(unsigned char)link_os_assignment[n])return 0;
    return !memcmp(link_os_environment+link_os_insertion+length,initial_os_environment+link_os_insertion,
        (initial_os_environment_units-link_os_insertion)*sizeof(WCHAR));
}
#endif
/* Internal Win32 boundary. It deliberately changes neither the CRT snapshot
 * nor Linux host environ; the synchronous child inherits the actual OS block. */
BOOL recon_crt_set_link_repro_environment(LPCSTR name,LPCSTR value)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    size_t offset=0,insertion,n,length=sizeof(link_os_assignment),units;
    WCHAR *derived;int saved=errno;unsigned long dos=recon_doserrno;DWORD error=recon_GetLastError();
    if(!name || !value || strcmp(name,"LINK_REPRO_NAME") || strcmp(value,"\\deflib.rsp") ||
       !os_environment_bound || link_os_environment || recon_crt_environment_updates ||
       recon_crt_environment!=initial_crt_environment || recon_crt_os_environment!=initial_os_environment)
        recon_unsupported("SetEnvironmentVariableA outside absent LINK OS insertion");
    recon_crt_validate_spawn_cp0_profile();
    target_environment_count(recon_crt_environment);
    validate_os_environment(initial_os_environment,initial_os_environment_units);
    if(target_environment_has_name(name,strlen(name)))
        recon_unsupported("SetEnvironmentVariableA existing target name");
    units=initial_os_environment_units+length;
    if(units>32767)recon_unsupported("SetEnvironmentVariableA derived block size");
    insertion=initial_os_environment_units-1;
    /* Preserve every existing entry and its relative order. Place the new
     * assignment before the first greater ASCII case-insensitive name. */
    while(initial_os_environment[offset]) {
        for(n=0;initial_os_environment[offset+n] && initial_os_environment[offset+n]!='=' && name[n];++n) {
            unsigned a=initial_os_environment[offset+n],b=(unsigned char)name[n];
            if(a>='a' && a<='z')a-='a'-'A';if(b>='a' && b<='z')b-='a'-'A';
            if(a!=b)break;
        }
        {
            unsigned a=initial_os_environment[offset+n],b=(unsigned char)name[n];
            if(a=='=')a=0;if(a>='a' && a<='z')a-='a'-'A';if(b>='a' && b<='z')b-='a'-'A';
            if(a>b){insertion=offset;break;}
        }
        while(initial_os_environment[offset])++offset;++offset;
    }
    derived=malloc(units*sizeof(*derived));
    if(!derived)recon_unsupported("SetEnvironmentVariableA owned block allocation");
    memcpy(derived,initial_os_environment,insertion*sizeof(*derived));
    for(n=0;n<length;++n)derived[insertion+n]=(unsigned char)link_os_assignment[n];
    memcpy(derived+insertion+length,initial_os_environment+insertion,
        (initial_os_environment_units-insertion)*sizeof(*derived));
    link_os_environment=derived;link_os_environment_units=units;link_os_insertion=insertion;
    recon_crt_os_environment=derived;recon_crt_os_environment_units=units;
    if(!link_spawn_environment_valid())recon_unsupported("SetEnvironmentVariableA derived block validation");
    errno=saved;recon_doserrno=dos;recon_SetLastError(error);return TRUE;
#else
    (void)name;(void)value;recon_unsupported("SetEnvironmentVariableA requires explicit510 profile");return FALSE;
#endif
}

void recon_bind_process_os_environment(const WCHAR *block,size_t units,const char *provider_sha256)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(recon_crt_arguments_bound || os_environment_bound || pending_os_environment ||
       recon_crt_environment_updates || !provider_sha256 ||
       strcmp(provider_sha256,"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210"))
        recon_unsupported("process OS environment binding state/provider");
    if(!block || ((uintptr_t)block&1) || units>32767 ||
       (uintptr_t)block>UINTPTR_MAX-units*sizeof(*block))
        recon_unsupported("process OS environment binding extent");
    validate_os_environment(block,units);
    /* Only the actual launch block is borrowed for process lifetime. The
     * provider string is consumed now; the strict binder uses its known literal. */
    pending_os_environment=block;pending_os_environment_units=units;
#else
    (void)block;(void)units;(void)provider_sha256;
    recon_unsupported("process OS environment requires explicit510 profile");
#endif
}

void recon_crt_bind_os_environment(const WCHAR *block,size_t units,const char *provider_sha256)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    size_t count;
    if (recon_native_entry_active() || !provider_sha256 || strcmp(provider_sha256,"e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210") ||
        !recon_crt_arguments_bound || os_environment_bound || recon_crt_environment_updates)
        recon_unsupported("target OS environment provider/binding state");
    validate_os_environment(block,units);count=target_environment_count(recon_crt_environment);
    recon_crt_os_environment=initial_os_environment=block;
    recon_crt_os_environment_units=units;initial_os_environment_units=units;
    initial_crt_environment=recon_crt_environment;
    recon_crt_environment_vectors[0].vector=initial_crt_environment;
    recon_crt_environment_vectors[0].count=count;
    recon_crt_environment_vectors[0].borrowed=1;recon_crt_environment_vectors[0].live=1;
    os_environment_bound=1;
#else
    (void)block;(void)units;(void)provider_sha256;
    recon_unsupported("target OS environment without explicit MSVCRT510 profile");
#endif
}
WEAK int recon_putenv(const char *assignment)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    const char *equals; size_t length,name_length,count,n,units;
    char **vector,*inserted; WCHAR *os_block;
    int saved=errno; unsigned long dos=recon_doserrno; DWORD error=recon_GetLastError();
    if (!os_environment_bound || recon_crt_environment_updates>=2 || !assignment ||
        saved!=2 || dos!=2 || error!=2 || recon_crt_initenv!=initial_crt_environment ||
        recon_crt_os_environment!=initial_os_environment)
        recon_unsupported("_putenv unreviewed binding/phase/error/initial alias");
    equals=strchr(assignment,'=');length=strlen(assignment);
    if (!equals || equals==assignment || length>32768 || (name_length=equals-assignment)>255)
        recon_unsupported("_putenv unreviewed assignment grammar/length");
    for (n=0;n<length;++n) if ((unsigned char)assignment[n]<32 || (unsigned char)assignment[n]>=127)
        recon_unsupported("_putenv nonASCII/control assignment");
    count=target_environment_count(recon_crt_environment);
    validate_os_environment(recon_crt_os_environment,recon_crt_os_environment_units);
    if (target_environment_has_name(assignment,name_length))
        recon_unsupported("_putenv replacement/removal of existing target entry");
    if (!recon_crt_environment_updates) {
        if (equals[1] || recon_crt_environment!=initial_crt_environment ||
            recon_mb_codepage || recon_lc_codepage || recon_mb_cur_max!=1 ||
            locale_profile_applied || recon_pctype!=character_flags+1)
            recon_unsupported("_putenv initial absent-empty CP0 transition");
        for (n=0;n<6;++n) if (recon_lc_handle[n])
            recon_unsupported("_putenv initial locale handles");
        {
            unsigned char expected_mb[257];
            msvcrt510_mb_flags(expected_mb);
            if (memcmp(recon_mbctype,expected_mb,sizeof(expected_mb)) ||
                memcmp(character_flags,initial_ctype_510,sizeof(character_flags)))
                recon_unsupported("_putenv initial locale table state");
        }
        vector=malloc((count+1)*sizeof(*vector));
        if (!vector) recon_unsupported("_putenv vector allocation failure");
        for (n=0;n<count;++n) {
            vector[n]=strdup(recon_crt_environment[n]);
            if (!vector[n]) recon_unsupported("_putenv string allocation failure");
        }
        vector[count]=NULL;
        memcpy(environment_update_name,assignment,name_length);environment_update_name[name_length]=0;
        recon_crt_environment=owned_crt_environment=vector;
        recon_crt_environment_vectors[1].vector=vector;recon_crt_environment_vectors[1].count=count;
        recon_crt_environment_vectors[1].live=1;
    } else {
        unsigned char expected_mb[257];unsigned short expected_ctype[257];
        if (!equals[1] || strlen(environment_update_name)!=name_length ||
            memcmp(assignment,environment_update_name,name_length) ||
            recon_crt_environment!=owned_crt_environment || !locale_profile_applied ||
            recon_mb_codepage!=1252 || recon_lc_codepage!=1252 || recon_mb_cur_max!=1 ||
            recon_pctype!=locale_ctype_510+1)
            recon_unsupported("_putenv observed absent insertion/profile state");
        for (n=0;n<6;++n) if (recon_lc_handle[n]!=(n==2?1033:0))
            recon_unsupported("_putenv insertion locale handles");
        msvcrt510_mb_flags(expected_mb);msvcrt510_locale_ctype(expected_mb,expected_ctype);
        if (memcmp(recon_mbctype,expected_mb,sizeof(expected_mb)) ||
            memcmp(recon_pctype-1,expected_ctype,sizeof(expected_ctype)))
            recon_unsupported("_putenv insertion locale table state");
        units=recon_crt_os_environment_units+length+1;
        if (units>65536) recon_unsupported("_putenv OS environment overflow");
        vector=malloc((count+2)*sizeof(*vector));inserted=strdup(assignment);
        os_block=malloc(units*sizeof(*os_block));
        if (!vector || !inserted || !os_block) recon_unsupported("_putenv insertion allocation failure");
        memcpy(vector,recon_crt_environment,count*sizeof(*vector));
        vector[count]=inserted;vector[count+1]=NULL;
        memcpy(os_block,recon_crt_os_environment,(recon_crt_os_environment_units-1)*sizeof(*os_block));
        for (n=0;n<length;++n) os_block[recon_crt_os_environment_units-1+n]=(unsigned char)assignment[n];
        os_block[units-2]=os_block[units-1]=0;
        /* Actual vector moves; retained old strings now belong to the new
         * current vector. The initial borrowed vector/__initenv stays alive. */
        recon_crt_environment_vectors[1].live=0;free(owned_crt_environment);
        recon_crt_environment=owned_crt_environment=vector;
        recon_crt_environment_vectors[2].vector=vector;recon_crt_environment_vectors[2].count=count+1;
        recon_crt_environment_vectors[2].live=1;
        recon_crt_os_environment=os_block;recon_crt_os_environment_units=units;
    }
    ++recon_crt_environment_updates;
    errno=saved;recon_doserrno=dos;recon_SetLastError(error);return 0;
#else
    (void)assignment;recon_unsupported("unreviewed target CRT/OS environment mutation");return -1;
#endif
}

WEAK char *recon_strerror(const char *prefix)
{
    static char text[128];
    if (errno!=ENOENT || (prefix && *prefix))
        recon_unsupported("unreviewed legacy _strerror errno/prefix");
    memcpy(text,"No such file or directory\n",sizeof("No such file or directory\n"));
    return text;
}

WEAK void recon_searchenv(const char *filename,const char *variable,char *output)
{
    const char *directory,*cursor;
    char candidate[260];
    size_t length,n;
    ascii_multibyte_scope();
    if (!filename || !*filename || !variable || !*variable || !output)
        recon_unsupported("_searchenv null/empty argument");
    for (cursor=filename;*cursor;++cursor)
        if ((unsigned char)*cursor>=128 || strchr("\\/:;\"*?",*cursor))
            recon_unsupported("_searchenv unreviewed filename");
    /* The provider tries the current directory first. That missing-leaf access
     * changes all three error slots before getenv and the successful candidate
     * access; preserving the entry error state here would be incorrect. */
    if (!recon_access(filename,0))
        recon_unsupported("_searchenv current-directory success");
    directory=recon_getenv(variable);
    if (!directory || !*directory)
        recon_unsupported("_searchenv unavailable environment directory");
    for (cursor=directory;*cursor;++cursor)
        if ((unsigned char)*cursor>=128 || *cursor==';' || *cursor=='\"')
            recon_unsupported("_searchenv list/quote/nonASCII directory");
    length=strlen(directory);
    if (length<3 || !((directory[0]>='A' && directory[0]<='Z') ||
                     (directory[0]>='a' && directory[0]<='z')) ||
        directory[1]!=':' || (directory[2]!='\\' && directory[2]!='/'))
        recon_unsupported("_searchenv nonabsolute directory");
    n=strlen(filename);
    if (length+n+2>sizeof(candidate))
        recon_unsupported("_searchenv candidate length");
    memcpy(candidate,directory,length);
    if (candidate[length-1]!='\\' && candidate[length-1]!='/')
        candidate[length++]='\\';
    memcpy(candidate+length,filename,n+1);
    if (!((uintptr_t)output+length+n+1<=(uintptr_t)filename ||
          (uintptr_t)filename+n+1<=(uintptr_t)output) ||
        !((uintptr_t)output+length+n+1<=(uintptr_t)directory ||
          (uintptr_t)directory+strlen(directory)+1<=(uintptr_t)output))
        recon_unsupported("_searchenv output/input overlap");
    if (recon_access(candidate,0))
        recon_unsupported("_searchenv unavailable candidate");
    memcpy(output,candidate,length+n+1);
}
