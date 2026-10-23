#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <recon.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <signal.h>
#include <iconv.h>
#include <locale.h>
#include <wctype.h>
#include <sys/ioctl.h>

/* File HANDLEs encode fd+1; mapping handles own a dup of their source fd. */
static DWORD last_error;
typedef struct NativeResource {
    struct NativeResource *next; DWORD kind,id,language,length; const char *text;
} NativeResource;
typedef struct NativeModule {
    ReconModuleSpec identity;
    char name[MAX_PATH],path[MAX_PATH],sha256[65];
    BOOL thread_calls_disabled;
    NativeResource *resources;
} NativeModule;
static NativeModule native_modules[32],*primary_module;
static unsigned module_count;
static HANDLE standard_handles[3]={(HANDLE)1,(HANDLE)2,(HANDLE)3};
typedef struct Mapping {
    struct Mapping *next; int fd,writable; size_t length;
} Mapping;
typedef struct View { struct View *next; void *address; size_t length; } View;
static Mapping *mappings;
static View *views,*allocations;
static BOOL (WINAPI *console_handler)(DWORD);

static NativeModule *recon_registered_module(HMODULE handle)
{
    unsigned n;
    for (n=0;n<module_count;++n) if (handle==(HMODULE)(native_modules+n)) return native_modules+n;
    return NULL;
}

HMODULE recon_register_module(const ReconModuleSpec *spec)
{
    NativeModule *module;
    const char *basename,*slash;
    unsigned n;
    if (!spec || !spec->name || !spec->path || !spec->sha256 || !*spec->name ||
        strlen(spec->name)>=MAX_PATH || strlen(spec->path)>=MAX_PATH || strlen(spec->sha256)!=64 ||
        (spec->flags&~(RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS)) || !(spec->flags&RECON_MODULE_NO_TLS) ||
        !isalpha((unsigned char)spec->path[0]) || spec->path[1]!=':' ||
        (spec->path[2]!='\\' && spec->path[2]!='/')) {
        last_error=ERROR_INVALID_PARAMETER; return NULL;
    }
    for (n=0;n<64;++n) if (!isxdigit((unsigned char)spec->sha256[n])) {
        last_error=ERROR_INVALID_PARAMETER; return NULL;
    }
    basename=strrchr(spec->path,'\\'); slash=strrchr(spec->path,'/');
    if (slash && (!basename || slash>basename)) basename=slash;
    if (!basename || strcasecmp(basename+1,spec->name)) { last_error=ERROR_INVALID_PARAMETER; return NULL; }
    for (n=0;n<module_count;++n) if (!strcasecmp(native_modules[n].name,spec->name)) {
        module=native_modules+n;
        if (strcasecmp(module->path,spec->path) || strcasecmp(module->sha256,spec->sha256) ||
            module->identity.flags!=spec->flags) { last_error=183; return NULL; }
        return (HMODULE)module;
    }
    if ((spec->flags&RECON_MODULE_PRIMARY) && primary_module) { last_error=183; return NULL; }
    if (module_count==sizeof(native_modules)/sizeof(native_modules[0])) {
        last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL;
    }
    module=native_modules+module_count++;
    strcpy(module->name,spec->name); strcpy(module->path,spec->path); strcpy(module->sha256,spec->sha256);
    module->identity.name=module->name; module->identity.path=module->path;
    module->identity.sha256=module->sha256; module->identity.flags=spec->flags;
    if (spec->flags&RECON_MODULE_PRIMARY) primary_module=module;
    return (HMODULE)module;
}

const ReconModuleSpec *recon_module_identity(HMODULE handle)
{
    NativeModule *module=recon_registered_module(handle);
    if (!module) { last_error=ERROR_MOD_NOT_FOUND; return NULL; }
    return &module->identity;
}

BOOL recon_module_thread_calls_disabled(HMODULE handle)
{
    NativeModule *module=recon_registered_module(handle);
    if (!module) { last_error=ERROR_MOD_NOT_FOUND; return FALSE; }
    return module->thread_calls_disabled;
}

BOOL WINAPI recon_DisableThreadLibraryCalls(HMODULE handle)
{
    NativeModule *module=recon_registered_module(handle);
    if (!module) { last_error=ERROR_MOD_NOT_FOUND; return FALSE; }
    module->thread_calls_disabled=TRUE;
    return TRUE;
}

/* Entries are supplied by a verified host adapter, never read from a PE here. */
void recon_load_module_resources(HMODULE handle,const char *filename)
{
    NativeModule *module=recon_registered_module(handle);
    unsigned char *bytes; size_t size,offset; struct stat st;
    DWORD count,n; int fd,saved_errno=errno; DWORD saved_error=last_error;
    if (!module || !filename) recon_unsupported("resource provider module");
    fd=open(filename,O_RDONLY|O_CLOEXEC);
    if (fd<0 || fstat(fd,&st)<0 || st.st_size<72 || st.st_size>1048576)
        recon_unsupported("resource provider file");
    size=(size_t)st.st_size;
    bytes=mmap(NULL,size,PROT_READ,MAP_PRIVATE,fd,0); close(fd);
    if (bytes==MAP_FAILED) recon_unsupported("resource provider mapping");
    if (memcmp(bytes,"RMR1",4) || memcmp(bytes+8,module->sha256,64))
        recon_unsupported("resource provider identity");
    memcpy(&count,bytes+4,4); offset=72;
    if (count>4096) recon_unsupported("resource provider count");
    for (n=0;n<count;++n) {
        DWORD kind,id,language,length; NativeResource *entry; size_t k;
        if (size-offset<16) recon_unsupported("resource provider record");
        memcpy(&kind,bytes+offset,4); memcpy(&id,bytes+offset+4,4);
        memcpy(&language,bytes+offset+8,4); memcpy(&length,bytes+offset+12,4); offset+=16;
        if ((kind!=1 && kind!=2) || language!=0x409 || length>65535 || length>size-offset)
            recon_unsupported("resource provider encoding");
        for (k=0;k<length;++k) if (!bytes[offset+k] || bytes[offset+k]>127)
            recon_unsupported("resource provider non-ASCII text");
        for (entry=module->resources;entry;entry=entry->next)
            if (entry->kind==kind && entry->id==id && entry->language==language)
                recon_unsupported("duplicate resource provider entry");
        entry=mmap(NULL,sizeof(*entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if (entry==MAP_FAILED) recon_unsupported("resource entry allocation");
        entry->kind=kind; entry->id=id; entry->language=language; entry->length=length;
        entry->text=(const char *)bytes+offset; entry->next=module->resources; module->resources=entry;
        offset+=length;
    }
    if (offset!=size) recon_unsupported("resource provider trailing bytes");
    errno=saved_errno; last_error=saved_error;
}

static NativeResource *resource_entry(HMODULE handle,DWORD kind,DWORD id,DWORD language)
{
    NativeModule *module=handle?recon_registered_module(handle):primary_module;
    NativeResource *entry;
    if (!module) recon_unsupported("unregistered resource module");
    for (entry=module->resources;entry;entry=entry->next)
        if (entry->kind==kind && entry->id==id && entry->language==language) return entry;
    recon_unsupported("unencountered resource entry");
    return NULL;
}

int WINAPI recon_LoadStringA(HINSTANCE handle,UINT id,LPSTR output,int capacity)
{
    NativeResource *entry=resource_entry((HMODULE)handle,1,id,0x409);
    if (!output || capacity<=0 || (DWORD)capacity<=entry->length)
        recon_unsupported("unreviewed LoadStringA capacity");
    memcpy(output,entry->text,entry->length); output[entry->length]=0;
    return entry->length;
}

static DWORD format_registered_inserts(LPCVOID source,DWORD id,DWORD language,
                                       LPSTR output,DWORD capacity,va_list *arguments)
{
    const char *text=(const char *)source,*cursor,*insert; NativeResource *entry=NULL,*candidate;
    size_t n,length,insert_length,number_length; unsigned module,saw_string=0,saw_number=0;
    int value,saved_errno=errno; char number[12],*destination; va_list copied;
    if (!source || !arguments || id || language) recon_unsupported("unreviewed FormatMessageA inserts mode");
    length=strlen(text);
    for (module=0;module<module_count;++module)
        for (candidate=native_modules[module].resources;candidate;candidate=candidate->next)
            if (candidate->kind==2 && candidate->length==length && !memcmp(text,candidate->text,length)) entry=candidate;
    if (!entry) recon_unsupported("unregistered FormatMessageA template");
    for (cursor=text;*cursor;++cursor) if (*cursor=='%') {
        if (cursor[1]=='1' && cursor[2]!='!' && !isdigit((unsigned char)cursor[2])) { ++saw_string; ++cursor; }
        else if (!strncmp(cursor,"%2!d!",5)) { ++saw_number; cursor+=4; }
        else recon_unsupported("unreviewed FormatMessageA insert grammar");
    }
    if (saw_string!=1 || saw_number!=1) recon_unsupported("unreviewed FormatMessageA insert arity");
    va_copy(copied,*arguments); insert=va_arg(copied,const char *); value=va_arg(copied,int); va_end(copied);
    if (!insert) recon_unsupported("null FormatMessageA string insert");
    insert_length=strlen(insert);
    if (insert_length>65535) recon_unsupported("FormatMessageA string insert length");
    for (n=0;n<insert_length;++n) if ((unsigned char)insert[n]>127)
        recon_unsupported("unreviewed FormatMessageA insert encoding");
    snprintf(number,sizeof(number),"%d",value); number_length=strlen(number);
    length=length-7+insert_length+number_length;
    if (!output || capacity<=length) recon_unsupported("unreviewed FormatMessageA insert capacity");
    destination=output;
    for (cursor=text;*cursor;++cursor) {
        if (*cursor!='%') *destination++=*cursor;
        else if (cursor[1]=='1') { memcpy(destination,insert,insert_length); destination+=insert_length; ++cursor; }
        else { memcpy(destination,number,number_length); destination+=number_length; cursor+=4; }
    }
    *destination=0; errno=saved_errno;
    return (DWORD)length;
}

DWORD WINAPI recon_FormatMessageA(DWORD flags,LPCVOID source,DWORD id,DWORD language,
                                  LPSTR output,DWORD capacity,va_list *arguments)
{
    NativeResource *entry;
    if (flags==0x400) return format_registered_inserts(source,id,language,output,capacity,arguments);
    if (flags!=0xa00 || (language && language!=0x409) || arguments)
        recon_unsupported("unreviewed FormatMessageA mode");
    entry=resource_entry((HMODULE)source,2,id,0x409);
    if (!output || capacity<=entry->length) recon_unsupported("unreviewed FormatMessageA capacity");
    memcpy(output,entry->text,entry->length); output[entry->length]=0;
    return entry->length;
}

static DWORD error_code(int error)
{
    switch (error) {
    case ENOENT: return ERROR_FILE_NOT_FOUND;
    case ENOTDIR: return ERROR_PATH_NOT_FOUND;
    case EACCES: case EPERM: return ERROR_ACCESS_DENIED;
    case EBADF: return ERROR_INVALID_HANDLE;
    case ENOMEM: return ERROR_NOT_ENOUGH_MEMORY;
    case EEXIST: return ERROR_ALREADY_EXISTS;
    case EINVAL: return ERROR_INVALID_PARAMETER;
    case ENOSPC: return 112;
    case ENOTSUP: case ENOSYS: return ERROR_NOT_SUPPORTED;
    default: return ERROR_INVALID_DATA;
    }
}
static int handle_fd(HANDLE handle)
{
    uintptr_t value=(uintptr_t)handle;
    if (!value || value>=65536) { last_error=ERROR_INVALID_HANDLE; return -1; }
    return value-1;
}
DWORD WINAPI recon_GetLastError(void) { return last_error; }
void WINAPI recon_SetLastError(DWORD error) { last_error=error; }
HANDLE WINAPI recon_CreateFileA(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,DWORD disposition,DWORD attributes,HANDLE template)
{
    char path[PATH_MAX]; int flags,fd;
    (void)share; (void)template;
    if (recon_host_path(name,path,sizeof(path))<0) { last_error=error_code(errno); return INVALID_HANDLE_VALUE; }
    flags=(access&GENERIC_WRITE)?(access&GENERIC_READ)?O_RDWR:O_WRONLY:O_RDONLY;
    switch (disposition) {
    case CREATE_NEW: flags|=O_CREAT|O_EXCL; break;
    case CREATE_ALWAYS: flags|=O_CREAT|O_TRUNC; break;
    case OPEN_ALWAYS: flags|=O_CREAT; break;
    case TRUNCATE_EXISTING: flags|=O_TRUNC; break;
    case OPEN_EXISTING: break;
    default: last_error=ERROR_INVALID_PARAMETER; return INVALID_HANDLE_VALUE;
    }
    if (!security || !security->bInheritHandle) flags|=O_CLOEXEC;
    fd=open(path,flags,attributes&FILE_ATTRIBUTE_READONLY?0444:0666);
    if (fd<0) { last_error=error_code(errno); return INVALID_HANDLE_VALUE; }
    if (attributes&FILE_FLAG_DELETE_ON_CLOSE) unlink(path);
    last_error=0; return (HANDLE)(uintptr_t)(fd+1);
}
BOOL WINAPI recon_CloseHandle(HANDLE handle)
{
    Mapping **p,*m; int fd;
    for (p=&mappings;(m=*p)!=NULL;p=&m->next) if ((HANDLE)m==handle) {
        *p=m->next; close(m->fd); free(m); last_error=0; return TRUE;
    }
    fd=handle_fd(handle); if (fd<0) return FALSE;
    if (close(fd)<0) { last_error=error_code(errno); return FALSE; }
    last_error=0; return TRUE;
}
DWORD WINAPI recon_GetFileType(HANDLE handle)
{
    struct stat st; int fd=handle_fd(handle);
    if (fd<0) return FILE_TYPE_UNKNOWN;
    if (fstat(fd,&st)<0) { last_error=error_code(errno); return FILE_TYPE_UNKNOWN; }
    last_error=0;
    if (S_ISCHR(st.st_mode)) return FILE_TYPE_CHAR;
    if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode)) return FILE_TYPE_PIPE;
    return FILE_TYPE_DISK;
}
DWORD WINAPI recon_SetFilePointer(HANDLE handle,LONG low,LONG *high,DWORD origin)
{
    off_t position; int fd=handle_fd(handle);
    if (fd<0) return 0xffffffffUL;
    if (high && *high) { last_error=ERROR_NOT_SUPPORTED; return 0xffffffffUL; }
    position=lseek(fd,low,origin);
    if (position<0) {
        int error=errno;
        off_t base=-1; struct stat st;
        if (error==EINVAL && low<0) {
            if (origin==FILE_BEGIN) base=0;
            else if (origin==FILE_CURRENT) base=lseek(fd,0,SEEK_CUR);
            else if (origin==FILE_END && fstat(fd,&st)==0) base=st.st_size;
        }
        last_error=(base>=0 && base+(off_t)low<0)?ERROR_NEGATIVE_SEEK:error_code(error);
        errno=error; return 0xffffffffUL;
    }
    if (high) *high=0;
    last_error=0; return position;
}
BOOL WINAPI recon_SetEndOfFile(HANDLE handle)
{
    int fd=handle_fd(handle); off_t position; struct stat source,other;
    Mapping *mapping;
    if (fd<0) return FALSE;
    position=lseek(fd,0,SEEK_CUR);
    /* NT refuses truncation while a mapping object for the file is open,
     * even after its last view is unmapped. LINK observes this by retaining
     * its original mapped allocation size on disk. */
    if (fstat(fd,&source)==0) for (mapping=mappings;mapping;mapping=mapping->next)
        if (fstat(mapping->fd,&other)==0 && source.st_dev==other.st_dev && source.st_ino==other.st_ino) {
            last_error=1224; return FALSE; /* ERROR_USER_MAPPED_FILE */
        }
    if (position<0 || ftruncate(fd,position)<0) { last_error=error_code(errno); return FALSE; }
    last_error=0; return TRUE;
}
BOOL WINAPI recon_ReadFile(HANDLE handle,LPVOID buffer,DWORD count,LPDWORD got,LPVOID overlap)
{
    int fd=handle_fd(handle); ssize_t n;
    if (got) *got=0;
    if (overlap) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    if (fd<0) return FALSE;
    do n=read(fd,buffer,count); while (n<0 && errno==EINTR);
    if (n<0) { last_error=error_code(errno); return FALSE; }
    if (got) *got=n;
    last_error=0; return TRUE;
}
BOOL WINAPI recon_WriteFile(HANDLE handle,LPCVOID buffer,DWORD count,LPDWORD written,LPVOID overlap)
{
    int fd=handle_fd(handle); ssize_t n;
    if (written) *written=0;
    if (overlap) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    if (fd<0) return FALSE;
    do n=write(fd,buffer,count); while (n<0 && errno==EINTR);
    if (n<0) { last_error=error_code(errno); return FALSE; }
    if (written) *written=n;
    last_error=0; return TRUE;
}
BOOL WINAPI recon_FlushFileBuffers(HANDLE handle)
{
    int fd=handle_fd(handle);
    if (fd<0) return FALSE;
    if (fsync(fd)<0) { last_error=error_code(errno); return FALSE; }
    last_error=0; return TRUE;
}
HANDLE WINAPI recon_GetStdHandle(DWORD kind)
{
    if (kind<=STD_INPUT_HANDLE && kind>=STD_ERROR_HANDLE) return standard_handles[STD_INPUT_HANDLE-kind];
    last_error=ERROR_INVALID_PARAMETER; return INVALID_HANDLE_VALUE;
}
BOOL WINAPI recon_SetStdHandle(DWORD kind,HANDLE handle)
{
    if (kind>STD_INPUT_HANDLE || kind<STD_ERROR_HANDLE) { last_error=ERROR_INVALID_PARAMETER; return FALSE; }
    standard_handles[STD_INPUT_HANDLE-kind]=handle; return TRUE;
}
UINT WINAPI recon_SetHandleCount(UINT count)
{
    /* NT has no DOS handle-table limit. This compatibility call echoes the
     * requested count; actual descriptor allocation still fails normally. */
    return count;
}
UINT WINAPI recon_GetConsoleCP(void)
{
    const char *setting=getenv("RECON_CONSOLE_CP"); char *end; unsigned long page;
    if (!setting) return 437;
    errno=0; page=strtoul(setting,&end,10);
    if (!*setting || *end || errno || !page || page>UINT_MAX) {
        last_error=ERROR_INVALID_PARAMETER; return 0;
    }
    return (UINT)page;
}
BOOL WINAPI recon_GetCPInfo(UINT page,CPINFO *info)
{
    CPINFO result;
    if (!info) { last_error=ERROR_INVALID_PARAMETER; return FALSE; }
    memset(&result,0,sizeof(result)); result.DefaultChar[0]='?';
    switch (page) {
    case 0: case 1: case 437: case 1252: result.MaxCharSize=1; break;
    case 932:
        result.MaxCharSize=2;
        result.LeadByte[0]=0x81; result.LeadByte[1]=0x9f;
        result.LeadByte[2]=0xe0; result.LeadByte[3]=0xfc;
        break;
    case 65001: result.MaxCharSize=4; break;
    default: last_error=ERROR_INVALID_PARAMETER; return FALSE;
    }
    *info=result; last_error=0; return TRUE;
}
BOOL WINAPI recon_GetConsoleScreenBufferInfo(HANDLE handle,CONSOLE_SCREEN_BUFFER_INFO *info)
{
    struct winsize size; int fd=handle_fd(handle);
    if (fd<0) return FALSE;
    if (!info) { last_error=ERROR_INVALID_PARAMETER; return FALSE; }
    if (ioctl(fd,TIOCGWINSZ,&size)<0) {
        last_error=errno==ENOTTY?ERROR_INVALID_HANDLE:error_code(errno); return FALSE;
    }
    if (!size.ws_col || !size.ws_row || size.ws_col>SHRT_MAX || size.ws_row>SHRT_MAX) {
        last_error=ERROR_NOT_SUPPORTED; return FALSE;
    }
    memset(info,0,sizeof(*info));
    info->dwSize.X=size.ws_col; info->dwSize.Y=size.ws_row;
    info->wAttributes=7;
    info->srWindow.Right=size.ws_col-1; info->srWindow.Bottom=size.ws_row-1;
    info->dwMaximumWindowSize=info->dwSize;
    last_error=0; return TRUE;
}
HMODULE WINAPI recon_GetModuleHandleA(LPCSTR name)
{
    unsigned n;
    if (!name) return primary_module?(HMODULE)primary_module:(HMODULE)1;
    if (!strcasecmp(name,"kernel32.dll")) return (HMODULE)1;
    if (!strcasecmp(name,"user32.dll")) return (HMODULE)2;
    for (n=0;n<module_count;++n)
        if (!strcasecmp(name,native_modules[n].name) || !strcasecmp(name,native_modules[n].path))
            return (HMODULE)(native_modules+n);
    last_error=ERROR_MOD_NOT_FOUND; return NULL;
}
HMODULE WINAPI recon_LoadLibraryA(LPCSTR name)
{
    if (name && !strcasecmp(name,"kernel32.dll")) return (HMODULE)1;
    if (name && !strcasecmp(name,"user32.dll")) return (HMODULE)2;
    last_error=ERROR_MOD_NOT_FOUND; return NULL;
}
FARPROC WINAPI recon_GetProcAddress(HMODULE module,LPCSTR name)
{
    if (module==(HMODULE)2) {
        if (name && !strcmp(name,"LoadStringA")) return (FARPROC)recon_LoadStringA;
        last_error=127; return NULL;
    }
    if (module!=(HMODULE)1) { last_error=ERROR_INVALID_HANDLE; return NULL; }
    if (!strcmp(name,"CreateFileMappingA")) return (FARPROC)recon_CreateFileMappingA;
    if (!strcmp(name,"MapViewOfFile")) return (FARPROC)recon_MapViewOfFile;
    if (!strcmp(name,"UnmapViewOfFile")) return (FARPROC)recon_UnmapViewOfFile;
    /* In particular, IsTNT is absent: LINK must take its ordinary NT path. */
    last_error=127; return NULL;
}
HANDLE WINAPI recon_CreateFileMappingA(HANDLE file,LPSECURITY_ATTRIBUTES security,DWORD protection,DWORD high,DWORD low,LPCSTR name)
{
    Mapping *m; struct stat st; int fd=handle_fd(file); size_t length;
    (void)security;
    if (fd<0) return NULL;
    if (high || name || (protection!=PAGE_READONLY && protection!=PAGE_READWRITE)) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    if (fstat(fd,&st)<0) { last_error=error_code(errno); return NULL; }
    length=low?(size_t)low:(size_t)st.st_size;
    if (!length) { last_error=ERROR_INVALID_PARAMETER; return NULL; }
    if (length>(size_t)st.st_size) {
        if (protection!=PAGE_READWRITE) { last_error=ERROR_ACCESS_DENIED; return NULL; }
        if (ftruncate(fd,length)<0) { last_error=error_code(errno); return NULL; }
    }
    m=calloc(1,sizeof(*m)); if (!m) { last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    m->fd=dup(fd); if (m->fd<0) { last_error=error_code(errno); free(m); return NULL; }
    fcntl(m->fd,F_SETFD,FD_CLOEXEC);
    m->length=length; m->writable=protection==PAGE_READWRITE; m->next=mappings; mappings=m;
    last_error=0; return m;
}
LPVOID WINAPI recon_MapViewOfFile(HANDLE handle,DWORD access,DWORD high,DWORD low,DWORD bytes)
{
    Mapping *m; View *v; void *address; size_t length;
    for (m=mappings;m && (HANDLE)m!=handle;m=m->next) ;
    if (!m) { last_error=ERROR_INVALID_HANDLE; return NULL; }
    if (high || (access!=FILE_MAP_ALL_ACCESS && (access&~(FILE_MAP_READ|FILE_MAP_WRITE)))) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    if (low>=m->length || (bytes && bytes>m->length-low) || ((access&FILE_MAP_WRITE) && !m->writable)) { last_error=ERROR_INVALID_PARAMETER; return NULL; }
    length=bytes?bytes:m->length-low;
    address=mmap(NULL,length,PROT_READ|((access&FILE_MAP_WRITE)?PROT_WRITE:0),MAP_SHARED,m->fd,low);
    if (address==MAP_FAILED) { last_error=error_code(errno); return NULL; }
    v=malloc(sizeof(*v)); if (!v) { munmap(address,length); last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    v->address=address; v->length=length; v->next=views; views=v; last_error=0; return address;
}
BOOL WINAPI recon_UnmapViewOfFile(LPCVOID address)
{
    View **p,*v;
    for (p=&views;(v=*p)!=NULL;p=&v->next) if (v->address==address) {
        if (munmap(v->address,v->length)<0) { last_error=error_code(errno); return FALSE; }
        *p=v->next; free(v); last_error=0; return TRUE;
    }
    last_error=ERROR_INVALID_PARAMETER; return FALSE;
}
static void interrupted(int signal)
{
    if (console_handler && console_handler(signal==SIGINT?CTRL_C_EVENT:CTRL_BREAK_EVENT)) return;
    _exit(128+signal);
}
BOOL WINAPI recon_SetConsoleCtrlHandler(BOOL (WINAPI *handler)(DWORD),BOOL add)
{
    if (add && console_handler && handler!=console_handler) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    console_handler=add?handler:NULL;
    signal(SIGINT,interrupted); signal(SIGQUIT,interrupted); return TRUE;
}
DWORD WINAPI recon_GetModuleFileNameA(HMODULE module,LPSTR out,DWORD count)
{
    size_t length;
    NativeModule *registered=module?recon_registered_module(module):primary_module;
    const char *path=registered?registered->path:recon_pgmptr;
    if (module && module!=(HMODULE)1 && !registered) { last_error=ERROR_MOD_NOT_FOUND; return 0; }
    if (!count) { last_error=ERROR_INVALID_PARAMETER; return 0; }
    length=strlen(path); if (length>=count) { memcpy(out,path,count); return count; }
    memcpy(out,path,length+1); return length;
}
DWORD WINAPI recon_GetVersion(void) { return 0x00000a03; }
void WINAPI recon_GetSystemTime(SYSTEMTIME *out)
{
    struct tm tm; time_t now=recon_time(NULL);
    gmtime_r(&now,&tm); memset(out,0,sizeof(*out));
    out->wYear=tm.tm_year+1900; out->wMonth=tm.tm_mon+1; out->wDay=tm.tm_mday; out->wDayOfWeek=tm.tm_wday;
    out->wHour=tm.tm_hour; out->wMinute=tm.tm_min; out->wSecond=tm.tm_sec;
}
void WINAPI recon_GetLocalTime(SYSTEMTIME *out)
{
    struct tm tm; time_t now=recon_time(NULL);
    localtime_r(&now,&tm); memset(out,0,sizeof(*out));
    out->wYear=tm.tm_year+1900; out->wMonth=tm.tm_mon+1; out->wDay=tm.tm_mday; out->wDayOfWeek=tm.tm_wday;
    out->wHour=tm.tm_hour; out->wMinute=tm.tm_min; out->wSecond=tm.tm_sec;
}
void WINAPI recon_ExitProcess(UINT code) { recon_record_terminal_status(code); _exit(code); }
LPVOID WINAPI recon_VirtualAlloc(LPVOID address,DWORD bytes,DWORD kind,DWORD protection)
{
    View *v; void *result; int flags=MAP_PRIVATE|MAP_ANONYMOUS,prot;
    if (!bytes || (kind&~(MEM_COMMIT|MEM_RESERVE)) || !(kind&(MEM_COMMIT|MEM_RESERVE)) || (protection!=PAGE_READWRITE && protection!=PAGE_READONLY)) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    prot=(kind&MEM_COMMIT)?PROT_READ|(protection==PAGE_READWRITE?PROT_WRITE:0):PROT_NONE;
    if (address && kind==MEM_COMMIT) {
        for (v=allocations;v;v=v->next) if ((char *)address>=(char *)v->address && (char *)address+bytes<=(char *)v->address+v->length) {
            if (mprotect(address,bytes,prot)<0) { last_error=error_code(errno); return NULL; } return address;
        }
        last_error=ERROR_INVALID_PARAMETER; return NULL;
    }
    if (address) flags|=MAP_FIXED_NOREPLACE;
    result=mmap(address,bytes,prot,flags,-1,0);
    if (result==MAP_FAILED) { last_error=error_code(errno); return NULL; }
    v=malloc(sizeof(*v)); if (!v) { munmap(result,bytes); last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    v->address=result; v->length=bytes; v->next=allocations; allocations=v; return result;
}
BOOL WINAPI recon_VirtualFree(LPVOID address,DWORD bytes,DWORD kind)
{
    View **p,*v;
    if (bytes || kind!=MEM_RELEASE) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    for (p=&allocations;(v=*p)!=NULL;p=&v->next) if (v->address==address) {
        if (munmap(address,v->length)<0) { last_error=error_code(errno); return FALSE; }
        *p=v->next; free(v); return TRUE;
    }
    last_error=ERROR_INVALID_PARAMETER; return FALSE;
}
HGLOBAL WINAPI recon_GlobalAlloc(UINT flags,DWORD bytes)
{
    if (flags&~GMEM_ZEROINIT) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    return flags&GMEM_ZEROINIT?calloc(1,bytes):malloc(bytes);
}
HGLOBAL WINAPI recon_GlobalFree(HGLOBAL memory) { free(memory); return NULL; }
static int convert(const char *to,const char *from,const void *input,size_t bytes,void *output,size_t capacity)
{
    iconv_t cd; char scratch[256],*cursor=(char *)input,*dest; size_t left=bytes,available,done=0;
    cd=iconv_open(to,from); if (cd==(iconv_t)-1) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    do {
        dest=output?(char *)output+done:scratch; available=output?capacity-done:sizeof(scratch);
        { size_t before=available,r=iconv(cd,&cursor,&left,&dest,&available); done+=before-available;
          if (r==(size_t)-1 && (errno!=E2BIG || output)) {
              last_error=errno==E2BIG?ERROR_INSUFFICIENT_BUFFER:ERROR_NO_UNICODE_TRANSLATION;
              iconv_close(cd); return 0;
          } }
    } while (left);
    iconv_close(cd); last_error=0; return done;
}
static const char *code_page(UINT page)
{
    if (page==0 || page==1252) return "CP1252";
    if (page==65001) return "UTF-8";
    if (page==1 || page==437) return "CP437";
    if (page==932) return "CP932";
    return NULL;
}
int WINAPI recon_MultiByteToWideChar(UINT page,DWORD flags,LPCSTR input,int count,LPWSTR output,int capacity)
{
    const char *encoding=code_page(page); int bytes;
    if (!encoding || count==0 || count< -1 || capacity<0 || (flags&~9U)) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    bytes=count<0?(int)strlen(input)+1:count;
    return convert("UTF-16LE",encoding,input,bytes,capacity?output:NULL,(size_t)capacity*2)/2;
}
int WINAPI recon_WideCharToMultiByte(UINT page,DWORD flags,LPCWSTR input,int count,LPSTR output,int capacity,LPCSTR replacement,BOOL *used)
{
    const char *encoding=code_page(page); int n=count;
    (void)replacement;
    if (used) *used=FALSE;
    if (!encoding || !count || count< -1 || capacity<0 || (flags&~0x610U)) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    if (n<0) for (n=1;input[n-1];++n) ;
    return convert(encoding,"UTF-16LE",input,(size_t)n*2,capacity?output:NULL,capacity);
}
int WINAPI recon_LCMapStringW(DWORD locale,DWORD flags,LPCWSTR input,int count,LPWSTR output,int capacity)
{
    locale_t host; int n=count,i; (void)locale;
    if (!count || count< -1 || (flags!=0x100 && flags!=0x200)) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    if (n<0) for (n=1;input[n-1];++n) ;
    if (!capacity) return n;
    if (capacity<n) { last_error=122; return 0; }
    host=newlocale(LC_CTYPE_MASK,"C.UTF-8",NULL);
    if (!host) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    for (i=0;i<n;++i) output[i]=flags==0x200?towupper_l(input[i],host):towlower_l(input[i],host);
    freelocale(host); return n;
}

/* RC's NT counted strings use byte lengths and 16-bit UTF-16 characters,
 * independent of the host's wchar_t. The system ANSI page is CP1252, as for
 * MultiByteToWideChar(CP_ACP). Undefined CP1252 bytes retain their control
 * codepoints; iconv rejects those bytes, unlike the NT ANSI conversion. */
struct AnsiString { USHORT length,capacity; char *text; };
struct UnicodeString { USHORT length,capacity; WCHAR *text; };
static const WCHAR ansi_controls[32]={
    0x20ac,0x0081,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,
    0x02c6,0x2030,0x0160,0x2039,0x0152,0x008d,0x017d,0x008f,
    0x0090,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
    0x02dc,0x2122,0x0161,0x203a,0x0153,0x009d,0x017e,0x0178
};
LONG WINAPI recon_RtlAnsiStringToUnicodeString(struct UnicodeString *out,struct AnsiString *in,BYTE allocate)
{
    unsigned i,bytes; WCHAR *text;
    if (!out || !in || (in->length && !in->text) || in->length>in->capacity)
        return (LONG)0xc000000dUL; /* STATUS_INVALID_PARAMETER */
    bytes=(unsigned)in->length*2;
    if (bytes>65532) return (LONG)0xc000000dUL;
    if (allocate) {
        text=malloc(bytes+2);
        if (!text) return (LONG)0xc0000017UL; /* STATUS_NO_MEMORY */
        out->text=text; out->capacity=bytes+2;
    }
    if (!out->text || out->capacity<bytes+2)
        return (LONG)0x80000005UL; /* STATUS_BUFFER_OVERFLOW */
    for (i=0;i<in->length;++i) {
        BYTE value=(BYTE)in->text[i];
        out->text[i]=value>=0x80 && value<0xa0?ansi_controls[value-0x80]:value;
    }
    out->text[in->length]=0; out->length=bytes; return 0;
}
LONG WINAPI recon_RtlUnicodeStringToAnsiString(struct AnsiString *out,struct UnicodeString *in,BYTE allocate)
{
    unsigned i,j,length; BYTE value; WCHAR ch; char *text;
    if (!out || !in || (in->length && !in->text) || in->length>in->capacity || (in->length&1))
        return (LONG)0xc000000dUL;
    length=in->length/2;
    if (allocate) {
        text=malloc(length+1);
        if (!text) return (LONG)0xc0000017UL;
        out->text=text; out->capacity=length+1;
    }
    if (!out->text || out->capacity<length+1) return (LONG)0x80000005UL;
    for (i=0;i<length;++i) {
        ch=in->text[i]; value='?';
        if (ch<0x80 || (ch>=0xa0 && ch<=0xff)) value=(BYTE)ch;
        else for (j=0;j<32;++j) if (ansi_controls[j]==ch) { value=j+0x80; break; }
        out->text[i]=value;
    }
    out->text[length]=0; out->length=length; return 0;
}

/* Reached only in unvisited RC conversion branches; retain SDK call ABIs. */
BOOL WINAPI recon_IsValidCodePage(UINT page)
{ (void)page; recon_unsupported("unreviewed IsValidCodePage"); return FALSE; }
UINT WINAPI recon_GetACP(void)
{ recon_unsupported("unreviewed GetACP"); return 0; }
LPSTR WINAPI recon_CharNextA(LPCSTR text)
{ (void)text; recon_unsupported("unreviewed CharNextA"); return NULL; }

int __cdecl recon_wsprintfA(LPSTR output,LPCSTR format,...)
{ (void)output; (void)format; recon_unsupported("unreviewed wsprintfA"); return 0; }
BOOL WINAPI recon_HeapDestroy(HANDLE heap)
{ (void)heap; recon_unsupported("unreviewed HeapDestroy"); return FALSE; }

/* Private heap support is bounded to the observed RC help allocation. These
 * host-only lists expose ownership to GDB without borrowing target addresses. */
typedef struct NativeHeap NativeHeap;
typedef struct NativeHeapBlock {
    struct NativeHeapBlock *next;
    NativeHeap *owner;
    void *payload;
    DWORD requested_size,flags;
    size_t mapping_size;
} NativeHeapBlock;
struct NativeHeap {
    NativeHeap *next;
    DWORD options,initial_size,maximum_size;
    NativeHeapBlock *blocks;
};
static NativeHeap *native_heaps;

HANDLE WINAPI recon_HeapCreate(DWORD options,DWORD initial,DWORD maximum)
{
    NativeHeap *heap; int saved_errno=errno;
    if (options!=1 || initial || maximum) recon_unsupported("unreviewed private HeapCreate mode");
    heap=mmap(NULL,sizeof(*heap),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (heap==MAP_FAILED) recon_unsupported("private HeapCreate allocation failure");
    heap->options=options; heap->initial_size=initial; heap->maximum_size=maximum;
    heap->next=native_heaps; native_heaps=heap;
    errno=saved_errno;
    return (HANDLE)heap;
}
LPVOID WINAPI recon_HeapAlloc(HANDLE handle,DWORD flags,DWORD size)
{
    NativeHeap *heap; NativeHeapBlock *block;
    size_t prefix,length,page; int saved_errno=errno;
    for (heap=native_heaps;heap && (HANDLE)heap!=handle;heap=heap->next) {}
    if (!heap || flags!=9 || size!=528) recon_unsupported("unreviewed private HeapAlloc owner/flags/size");
    page=(size_t)sysconf(_SC_PAGESIZE);
    if (!page || (page&(page-1))) recon_unsupported("private heap host page size");
    prefix=(sizeof(*block)+15)&~(size_t)15;
    length=(prefix+size+page-1)&~(page-1);
    block=mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (block==MAP_FAILED) recon_unsupported("private HeapAlloc allocation failure");
    block->owner=heap; block->payload=(char *)block+prefix;
    block->requested_size=size; block->flags=flags; block->mapping_size=length;
    block->next=heap->blocks; heap->blocks=block;
    errno=saved_errno;
    return block->payload;
}
BOOL WINAPI recon_HeapFree(HANDLE heap,DWORD flags,LPVOID block)
{
    (void)heap; (void)flags; (void)block;
    recon_unsupported("unreviewed private HeapFree"); return FALSE;
}
LRESULT WINAPI recon_SendMessageA(HWND window,UINT message,WPARAM wparam,LPARAM lparam)
{
    (void)window; (void)message; (void)wparam; (void)lparam;
    recon_unsupported("unreviewed SendMessageA"); return 0;
}
