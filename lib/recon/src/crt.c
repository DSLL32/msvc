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
/* The Win32 CRT writes a 16-bit wchar_t. The C locale maps all unsigned
 * bytes directly; source==NULL or n==0 is a no-op, not a host EILSEQ. */
WEAK int recon_mbtowc(WCHAR *destination,const char *source,size_t count)
{
    unsigned char value;
    if (!source || !count) return 0;
    value=(unsigned char)*source;
    if (value && recon_lc_handle[2]) recon_unsupported("mbtowc nondefault LC_CTYPE");
    if (destination) *destination=value;
    return value?1:0;
}
ShimFILE recon_iob[20];
WEAK unsigned char recon_osfile[256];
WEAK unsigned recon_nhandle=256;
#ifndef RECON_DEFAULT_FMODE
#define RECON_DEFAULT_FMODE 0x4000
#endif
int recon_fmode=RECON_DEFAULT_FMODE,recon_commode,recon_cflush,recon_umaskval;
unsigned long recon_doserrno,recon_lc_handle[6];
unsigned recon_lc_codepage;
int recon_mb_cur_max=1;
static unsigned short character_flags[257];
unsigned short *recon_pctype=character_flags+1;
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
int (__cdecl *recon_crt_matherr)(struct _exception *);
static int recon_crt_arguments_bound,recon_crt_matherr_default;
struct _startupinfo { int newmode; };
WEAK long recon_osfhnd[256];
#define os_handles recon_osfhnd
char *recon_decimal_point=".";
static int pushed_byte[256];

/* Canonical VC5 MSVCRT _initterm: unsigned exclusive end, NULL slots skipped,
 * each callback pointer read when reached (earlier callbacks may change it). */
WEAK void recon_initterm(ReconInitializer *begin,ReconInitializer *end)
{
    while ((uintptr_t)begin<(uintptr_t)end) {
        if (*begin) (*begin)();
        ++begin;
    }
}

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

/* CVTRES's first help case does not execute this imported sharing-mode API.
 * Retain its correct compile-time ABI and fail until a real call is captured. */
WEAK ShimFILE *recon_fsopen(const char *path,const char *mode,int sharing)
{
    (void)path; (void)mode; (void)sharing;
    recon_unsupported("_fsopen without reviewed sharing contract");
    return NULL;
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
        root=getenv(variable);
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
    for (n=0;n<recon_nhandle;++n) if (!recon_osfile[n]) { recon_osfile[n]=1; return n; }
    errno=EMFILE; return -1;
}
int recon_set_osfhnd(int fd,long handle)
{
    if ((unsigned)fd>=recon_nhandle) { errno=EBADF; return -1; }
    os_handles[fd]=handle; return 0;
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

static int find_match(const char *pattern,const char *name)
{
    while (*pattern) {
        if (*pattern=='*') {
            const char *end=name+strlen(name),*dot;
            ++pattern;
            if (!*pattern) return 1;
            if (*pattern=='.' && (dot=strrchr(name,'.'))!=NULL) end=dot;
            for (;;) {
                if (find_match(pattern,end)) return 1;
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
        while ((entry=readdir(stream))!=NULL) if (find_match(mask,entry->d_name)) {
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
    int fd;
    (void)sharing;
    access=(mode&3)==2?GENERIC_READ|GENERIC_WRITE:(mode&1)?GENERIC_WRITE:GENERIC_READ;
    disposition=(mode&0x500)==0x500?CREATE_NEW:(mode&0x300)==0x300?CREATE_ALWAYS:
        mode&0x100?OPEN_ALWAYS:mode&0x200?TRUNCATE_EXISTING:OPEN_EXISTING;
    handle=recon_CreateFileA(name,access,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,disposition,FILE_ATTRIBUTE_NORMAL,NULL);
    if (handle==INVALID_HANDLE_VALUE) { recon_dosmaperr(recon_GetLastError()); return -1; }
    fd=recon_alloc_osfhnd();
    if (fd<0) { recon_CloseHandle(handle); return -1; }
    os_handles[fd]=(long)handle;
    recon_osfile[fd]=1|((mode&0x8000)?0:(mode&0x4000 || recon_fmode!=0x8000)?0x80:0);
    if (mode&8) recon_osfile[fd]|=0x20;
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
    if (!valid_descriptor(fd)) return -1;
    if (!recon_CloseHandle((HANDLE)os_handles[fd])) { recon_dosmaperr(recon_GetLastError()); return -1; }
    recon_osfile[fd]=0; os_handles[fd]=0; pushed_byte[fd]=-1; return 0;
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
int recon_access(const char *name,int mode)
{
    char path[PATH_MAX];
    return recon_host_path(name,path,sizeof(path))<0?-1:access(path,mode);
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
    if (recon_host_path(name,path,sizeof(path))<0) return -1;
    if (stat64(path,&st)<0) {
        if (errno==ENOENT || errno==ENOTDIR) { errno=ENOENT; recon_doserrno=2; }
        return -1;
    }
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
int recon_remove(const char *name)
{
    char path[PATH_MAX]; return recon_host_path(name,path,sizeof(path))<0?-1:remove(path);
}
int recon_rename(const char *from,const char *to)
{
    char a[PATH_MAX],b[PATH_MAX];
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
    if (s->_flag&8) free(s->_base);
    s->_base=s->_ptr=NULL; s->_cnt=0; s->_flag&=~8;
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
    if (s->_flag&2) { s->_flag|=0x20; return EOF; }
    s->_flag|=1;
    if (!s->_base) recon_getbuf(s);
    s->_ptr=s->_base; s->_cnt=recon_read(s->_file,s->_base,s->_bufsiz);
    if (s->_cnt<=0) { s->_flag|=s->_cnt?0x20:0x10; s->_cnt=0; return EOF; }
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
    if (!(s->_flag&2)) {
        if ((s->_flag&1) && !(s->_flag&0x80)) { s->_flag|=0x20; return EOF; }
        s->_flag=(s->_flag&~1)|2; s->_cnt=0; s->_ptr=s->_base;
    }
    if ((s->_flag&4) || (s->_base && s->_bufsiz<=0))
        return recon_write(s->_file,&byte,1)==1?byte:EOF;
    if (!s->_base) recon_getbuf(s);
    if (s->_ptr==s->_base+s->_bufsiz && recon_flush(s)) return EOF;
    s->_flag|=2;
    *s->_ptr++=byte;
    /* _cnt is observable to the reconstructed CRT. */
    s->_cnt=s->_bufsiz-(s->_ptr-s->_base);
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
    size_t n,total=size*count; int c;
    if (!size) return 0;
    for (n=0;n<total;++n) { c=recon_fgetc(s); if (c==EOF) break; ((char *)p)[n]=c; }
    return n/size;
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
int recon_vfprintf(ShimFILE *s,const char *format,va_list args)
{
    char *text,*copy=NULL; size_t length=strlen(format),percent=0;
    int count,result;
    /* VC1 prints the valid prefix of LINK's formats ending in a lone '%'.
     * glibc rejects the whole conversion. Preserve literal %% pairs. */
    while (percent<length && format[length-percent-1]=='%') ++percent;
    if (percent&1) {
        copy=strdup(format); if (!copy) return -1;
        copy[length-1]=0; format=copy;
    }
    count=vasprintf(&text,format,args); free(copy);
    if (count<0) return -1;
    result=recon_fwrite(text,1,count,s)==(unsigned)count?count:-1; free(text); return result;
}
int recon_fprintf(ShimFILE *s,const char *format,...)
{
    va_list args; int n; va_start(args,format); n=recon_vfprintf(s,format,args); va_end(args); return n;
}
int recon_printf(const char *format,...)
{
    va_list args; int n; va_start(args,format); n=recon_vfprintf(&recon_iob[1],format,args); va_end(args); return n;
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
    memset(s,0,sizeof(*s)); s->_file=fd;
    s->_flag=strchr(mode,'+')?0x80:*mode=='r'?1:2; return s;
}
ShimFILE *recon_fopen(const char *name,const char *mode)
{
    int n;
    for (n=3;n<20;++n) if (!recon_iob[n]._flag) return recon_openfile(name,mode,0x40,&recon_iob[n]);
    errno=EMFILE; return NULL;
}
WEAK int recon_fclose(ShimFILE *s)
{
    int result=recon_flush(s),closed=recon_close(s->_file);
    recon_freebuf(s); memset(s,0,sizeof(*s)); return result || closed?EOF:0;
}
WEAK int recon_setvbuf(ShimFILE *s,char *buffer,int mode,size_t size)
{
    recon_flush(s); recon_freebuf(s);
    if (mode==4) { s->_base=(char *)&s->_charbuf; s->_bufsiz=2; s->_flag|=4; }
    else if (!size || (mode!=0 && mode!=0x40)) return -1;
    else { s->_base=buffer?buffer:malloc(size); if (!s->_base) return -1; s->_bufsiz=size; s->_flag|=buffer?0x100:8; }
    s->_ptr=s->_base; s->_cnt=0; return 0;
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
    if (origin==SEEK_CUR) { offset+=recon_ftell(s); origin=SEEK_SET; }
    if (recon_flush(s)) return -1;
    s->_flag&=~0x10;
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
void recon_endstdio(void) { recon_fflush(NULL); }
void recon_rmtmp(void) { }
void recon_exit(int code) { recon_fflush(NULL); recon_record_terminal_status((uint32_t)code); exit(code); }

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
    const char *canonical=getenv("RECON_CANONICAL_CWD"),*input=name&&*name?name:".";
    char host[PATH_MAX],absolute[PATH_MAX],normalized[PATH_MAX],check[PATH_MAX],cwd[PATH_MAX];
    char *part,*save,*result=out,drive='Z'; size_t len=0;
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
    result[0]=drive; result[1]=':'; strcpy(result+2,normalized); return result;
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
char *recon_mktemp(char *pattern)
{
    const char *fixed=getenv("RECON_MKTEMP_SUFFIX");
    size_t length=strlen(pattern),i; unsigned pid=(unsigned)getpid();
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
char *recon_tempnam(const char *dir,const char *prefix)
{
    char *path; const char *base=dir?dir:getenv("TMP");
    if (!base) base=".";
    if (asprintf(&path,"%s\\%sXXXXXX",base,prefix?prefix:"")<0) return NULL;
    if (!recon_mktemp(path)) { free(path); return NULL; } return path;
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
int recon_wcscmp(const WCHAR *a,const WCHAR *b)
{
    while (*a && *a==*b) { ++a; ++b; }
    return *a<*b?-1:*a>*b;
}
WEAK int recon_toupper(int c) { return toupper(c); }
WEAK int recon_tolower(int c) { return tolower(c); }
WEAK void recon_tzset(void) { tzset(); recon_timezone=timezone; recon_daylight=daylight; }
WEAK int recon_isindst(struct tm *tm) { return tm->tm_isdst>0; }
time_t recon_time(time_t *out)
{
    const char *fixed=getenv("RECON_FIXED_TIME"); time_t value=fixed?(time_t)strtoll(fixed,NULL,10):time(NULL);
    if (out) *out=value;
    return value;
}
WEAK int recon_spawnvpe(int mode,const char *file,const char *const *argv,const char *const *env)
{
    const char *path=getenv("PATH"); char buffer[PATH_MAX],program[PATH_MAX]; int result;
    result=recon_spawnve(mode,file,argv,env);
    if (result!=-1 || errno!=ENOENT || !path) return result;
    while ((path=recon_getpath(path,buffer,sizeof(buffer)))!=NULL) {
        if (snprintf(program,sizeof(program),"%s\\%s",buffer,file)>=(int)sizeof(program)) continue;
        result=recon_spawnve(mode,program,argv,env); if (result!=-1 || errno!=ENOENT) return result;
    }
    return -1;
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
    for (i=0;i<256;++i) pushed_byte[i]=-1;
    for (i=0;i<3;++i) {
        os_handles[i]=(long)recon_GetStdHandle((DWORD)(-10-i)); recon_osfile[i]=0x81;
        if (recon_GetFileType((HANDLE)os_handles[i])==FILE_TYPE_PIPE) recon_osfile[i]|=8;
        else if (recon_GetFileType((HANDLE)os_handles[i])==FILE_TYPE_CHAR) recon_osfile[i]|=0x40;
        recon_iob[i]._file=i; recon_iob[i]._flag=i?2:1;
    }
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
        if (i==' ' || i=='\t') bits|=64;
        if (isxdigit(i)) bits|=128;
        recon_pctype[i]=bits;
    }
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
    if (stream->_flag) recon_fclose(stream);
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
    dest=recon_alloc_osfhnd(); if (dest<0) return -1;
    fd=dup((int)os_handles[source]-1);
    if (fd<0) { recon_osfile[dest]=0; return -1; }
    os_handles[dest]=fd+1; recon_osfile[dest]=recon_osfile[source];
    return dest;
}
int recon_dup2(int source,int dest)
{
    int host,old;
    if (!valid_descriptor(source)) return -1;
    if (dest<0 || (unsigned)dest>=recon_nhandle) { errno=EBADF; return -1; }
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

WEAK int recon_unlink(const char *name) { return recon_remove(name); }
