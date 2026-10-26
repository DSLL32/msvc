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
#include <link.h>
#include <dirent.h>

/* File HANDLEs encode fd+1; mapping handles own a dup of their source fd. */
static DWORD last_error;
static char *command_line;
static size_t command_line_bytes;
void recon_bind_command_line(char *text,size_t bytes)
{
    size_t n;
    if(command_line || !text || !bytes || bytes>32767 ||
       (uintptr_t)text>UINTPTR_MAX-bytes)
        recon_unsupported("command-line binding extent/state");
    for(n=0;n+1<bytes;++n)
        if(!text[n] || (unsigned char)text[n]>=128)
            recon_unsupported("command-line binding ASCII/terminator");
    if(text[bytes-1]) recon_unsupported("command-line binding final NUL");
    command_line=text;command_line_bytes=bytes;
}
LPSTR WINAPI recon_GetCommandLineA(void)
{
    if(!command_line) recon_unsupported("unbound process command line");
    return command_line;
}
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static void temp_current_directory(char path[MAX_PATH])
{
    const char *temporary;size_t n;
    if(!recon_crt_arguments_bound || !getenv("RECON_CANONICAL_CWD"))
        recon_unsupported("temporary path unbound target environment/cwd");
    temporary=recon_getenv("TMP");
    if(!temporary || strcmp(temporary,"."))
        recon_unsupported("temporary path outside current-directory TMP");
    if(!recon_fullpath(path,".",MAX_PATH))
        recon_unsupported("temporary path canonical cwd");
    n=strlen(path);
    if(n>MAX_PATH-15) recon_unsupported("temporary path length");
    if(n && path[n-1]!='\\') {path[n++]='\\';path[n]=0;}
}
static void temp_output_span(char *output,const void *input,size_t bytes)
{
    uintptr_t out=(uintptr_t)output,in=(uintptr_t)input;
    if(!output || out>UINTPTR_MAX-MAX_PATH || in>UINTPTR_MAX-bytes ||
       (out<in+bytes && in<out+MAX_PATH))
        recon_unsupported("temporary path output extent/overlap");
}
#endif
DWORD WINAPI recon_GetTempPathA(DWORD capacity,LPSTR output)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    char path[MAX_PATH];const char *temporary;size_t n;int saved_errno=errno;
    if(capacity!=MAX_PATH) recon_unsupported("GetTempPathA capacity");
    temp_current_directory(path);temporary=recon_getenv("TMP");
    temp_output_span(output,temporary,2);
    n=strlen(path);memcpy(output,path,n+1);errno=saved_errno;return n;
#else
    (void)capacity;(void)output;
    recon_unsupported("unreviewed GetTempPathA");return 0;
#endif
}
UINT WINAPI recon_GetTempFileNameA(LPCSTR directory,LPCSTR prefix,UINT unique,LPSTR output)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    static UINT next_number;
    char path[MAX_PATH],candidate[MAX_PATH];size_t n;UINT attempt,number;
    HANDLE file;int saved_errno=errno;DWORD saved_error=last_error;
    if(!directory || !prefix || unique || strnlen(prefix,4)!=3 || memcmp(prefix,"lah",3))
        recon_unsupported("GetTempFileNameA directory/prefix/unique");
    temp_current_directory(path);n=strlen(path);
    if(strnlen(directory,MAX_PATH)!=n || memcmp(directory,path,n+1))
        recon_unsupported("GetTempFileNameA outside canonical current directory");
    temp_output_span(output,directory,n+1);temp_output_span(output,prefix,4);
    if(!next_number) {next_number=recon_GetTickCount()&0xffffU;if(!next_number)next_number=1;}
    for(attempt=0;attempt<65535U;++attempt) {
        number=next_number;next_number=number==65535U?1:number+1;
        memcpy(candidate,path,n);
        snprintf(candidate+n,MAX_PATH-n,"lah%04x.tmp",number);
        file=recon_CreateFileA(candidate,GENERIC_READ|GENERIC_WRITE,0,NULL,
                              CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file==INVALID_HANDLE_VALUE) {
            if(last_error==ERROR_ALREADY_EXISTS) continue;
            recon_unsupported("GetTempFileNameA exclusive creation failure");
        }
        if(!recon_CloseHandle(file)) recon_unsupported("GetTempFileNameA close failure");
        memcpy(output,candidate,n+12);errno=saved_errno;last_error=saved_error;
        return number;
    }
    recon_unsupported("GetTempFileNameA exhausted namespace");return 0;
#else
    (void)directory;(void)prefix;(void)unique;(void)output;
    recon_unsupported("unreviewed GetTempFileNameA");return 0;
#endif
}
BOOL WINAPI recon_SetEnvironmentVariableA(LPCSTR name,LPCSTR value)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    extern BOOL recon_crt_set_link_repro_environment(LPCSTR,LPCSTR);
    return recon_crt_set_link_repro_environment(name,value);
#else
    (void)name;(void)value;
    recon_unsupported("unreviewed SetEnvironmentVariableA");return FALSE;
#endif
}
char *__cdecl recon_watcom_getcmd(char *buffer)
{
#ifdef RECON_CRT_PROFILE_WATCOM_100A
    char *text=recon_GetCommandLineA();size_t end,start=0,count;
    if(!buffer) recon_unsupported("Watcom getcmd NULL output");
    for(end=0;end<command_line_bytes && text[end];++end)
        if((unsigned char)text[end]>=128)
            recon_unsupported("Watcom getcmd nonASCII input");
    if(end==command_line_bytes || !end || text[0]==' ' || (text[0]>=9 && text[0]<=13))
        recon_unsupported("Watcom getcmd missing program token");
    while(start<end && text[start]!=' ' && !(text[start]>=9 && text[start]<=13)) ++start;
    while(start<end && (text[start]==' ' || (text[start]>=9 && text[start]<=13))) ++start;
    count=end-start+1;
    if((uintptr_t)buffer>UINTPTR_MAX-count ||
       ((uintptr_t)buffer<(uintptr_t)text+command_line_bytes &&
        (uintptr_t)text<(uintptr_t)buffer+count))
        recon_unsupported("Watcom getcmd output wrap/overlap");
    /* Old getcmd skips the first whitespace-delimited token and whitespace, then strcpy's
     * the exact remaining command text, including quotes and trailing spaces. */
    memcpy(buffer,text+start,count);return buffer;
#else
    (void)buffer;recon_unsupported("Watcom getcmd requires explicit profile");return NULL;
#endif
}
typedef struct NativeResource {
    struct NativeResource *next; DWORD kind,id,language,length; const char *text;
} NativeResource;
typedef struct NativeExport {
    char name[128]; WORD ordinal; FARPROC address; unsigned lookups;
} NativeExport;
typedef struct NativeModule {
    ReconModuleSpec identity;
    char name[MAX_PATH],path[MAX_PATH],sha256[65];
    BOOL thread_calls_disabled;
    NativeResource *resources;
    NativeExport exports[16]; unsigned export_count,provider_state;
    void (__cdecl *lifecycle)(HMODULE,unsigned,void *); void *provider_context;
} NativeModule;
static NativeModule native_modules[32],*primary_module;
static unsigned module_count;
static int imagehlp_loaded;
static HANDLE standard_handles[3]={(HANDLE)1,(HANDLE)2,(HANDLE)3};
typedef struct Mapping {
    struct Mapping *next; int fd,writable; size_t length;
} Mapping;
typedef struct View {
    struct View *next; void *address; size_t length;
    dev_t file_device; ino_t file_inode;
    int decommit_eligible,decommitted;size_t committed_prefix;
} View;
static Mapping *mappings;
static View *views,*allocations;
static BOOL (WINAPI *console_handler)(DWORD);
void recon_fsopen_check_mappings(void)
{
    if (mappings || views) recon_unsupported("_fsopen with active mappings/views");
}

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

/* Proven module data may live in a private native image segment rather than
 * a mapped output file. Metadata is supplied outside the target graph. */
typedef struct NativeModuleMemory {
    NativeModule *module; void *address; DWORD original_rva,size;
} NativeModuleMemory;
static NativeModuleMemory native_module_memory[32];
static unsigned module_memory_count;
typedef struct NativeImageOwner {
    uintptr_t base;const ElfW(Phdr) *headers;int bound;
} NativeImageOwner;
static NativeImageOwner application_image,case_callback_image;
typedef struct NativeImageAnchor { uintptr_t address;NativeImageOwner image; } NativeImageAnchor;
static int native_image_anchor(struct dl_phdr_info *info,size_t size,void *context)
{
    NativeImageAnchor *anchor=context;unsigned n;(void)size;
    for(n=0;n<info->dlpi_phnum;++n) {
        const ElfW(Phdr) *header=info->dlpi_phdr+n;
        uintptr_t begin=(uintptr_t)info->dlpi_addr+header->p_vaddr;
        if(header->p_type==PT_LOAD && (header->p_flags&PF_R) &&
           anchor->address>=begin && anchor->address-begin<header->p_memsz) {
            anchor->image.base=info->dlpi_addr;anchor->image.headers=info->dlpi_phdr;
            anchor->image.bound=1;return 1;
        }
    }
    return 0;
}
void recon_register_native_image(const void *address,unsigned role)
{
    NativeImageAnchor anchor;NativeImageOwner *owner;int saved=errno;DWORD error=last_error;
    if(!address || (role!=RECON_NATIVE_IMAGE_APPLICATION && role!=RECON_NATIVE_IMAGE_CASE_CALLBACKS) ||
       (role==RECON_NATIVE_IMAGE_CASE_CALLBACKS && !application_image.bound))
        recon_unsupported("native image anchor role/order");
    memset(&anchor,0,sizeof(anchor));anchor.address=(uintptr_t)address;
    dl_iterate_phdr(native_image_anchor,&anchor);
    if(!anchor.image.bound)recon_unsupported("native image anchor is not loaded ELF");
    owner=role==RECON_NATIVE_IMAGE_APPLICATION?&application_image:&case_callback_image;
    if(owner->bound && (owner->base!=anchor.image.base || owner->headers!=anchor.image.headers))
        recon_unsupported("native image owner replacement");
    owner->base=anchor.image.base;owner->headers=anchor.image.headers;owner->bound=1;
    errno=saved;last_error=error;
}
static int native_image_allowed(struct dl_phdr_info *info,int callbacks)
{
    if(!application_image.bound)return !info->dlpi_name || !*info->dlpi_name;
    if(application_image.base==(uintptr_t)info->dlpi_addr && application_image.headers==info->dlpi_phdr)return 1;
    return callbacks && case_callback_image.bound &&
        case_callback_image.base==(uintptr_t)info->dlpi_addr && case_callback_image.headers==info->dlpi_phdr;
}
typedef struct NativeImageSpan { uintptr_t begin,end; int found; } NativeImageSpan;
static int writable_image_span(struct dl_phdr_info *info,size_t size,void *context)
{
    NativeImageSpan *span=context; unsigned n; (void)size;
    if (!native_image_allowed(info,0)) return 0;
    for (n=0;n<info->dlpi_phnum;++n) {
        const ElfW(Phdr) *header=info->dlpi_phdr+n;
        uintptr_t begin=(uintptr_t)info->dlpi_addr+header->p_vaddr;
        if (header->p_type==PT_LOAD && (header->p_flags&PF_W) &&
            span->begin>=begin && span->end>=span->begin &&
            span->end-begin<=header->p_memsz) { span->found=1; return 1; }
    }
    return 0;
}
static int executable_image_span(struct dl_phdr_info *info,size_t size,void *context)
{
    NativeImageSpan *span=context; unsigned n; (void)size;
    if (!native_image_allowed(info,1)) return 0;
    for (n=0;n<info->dlpi_phnum;++n) {
        const ElfW(Phdr) *header=info->dlpi_phdr+n;
        uintptr_t begin=(uintptr_t)info->dlpi_addr+header->p_vaddr;
        if (header->p_type==PT_LOAD && (header->p_flags&PF_X) &&
            span->begin>=begin && span->begin-begin<header->p_memsz) {
            span->found=1; return 1;
        }
    }
    return 0;
}
static void require_native_callback(uintptr_t address)
{
    NativeImageSpan span;
    span.begin=address; span.end=address; span.found=0;
    dl_iterate_phdr(executable_image_span,&span);
    if (!address || !span.found)
        recon_unsupported("module callback is not native owner-image code");
}
void recon_register_module_exports(HMODULE handle,const ReconModuleExportsSpec *spec)
{
    NativeModule *module=recon_registered_module(handle); unsigned i,j;
    int saved=errno; DWORD error=last_error;
    if (!module || !spec || !spec->sha256 || strcmp(spec->sha256,module->sha256) ||
        !spec->exports || !spec->count || spec->count>16 || !spec->lifecycle ||
        module->provider_state || (module->identity.flags&RECON_MODULE_PRIMARY))
        recon_unsupported("module export registration identity/state");
    require_native_callback((uintptr_t)spec->lifecycle);
    for (i=0;i<spec->count;++i) {
        const ReconModuleExport *entry=spec->exports+i;
        if (!entry->name || !*entry->name || strlen(entry->name)>=128 || !entry->ordinal)
            recon_unsupported("module export name/ordinal");
        for (j=0;entry->name[j];++j)
            if ((unsigned char)entry->name[j]<32 || (unsigned char)entry->name[j]>=127)
                recon_unsupported("module export nonASCII name");
        require_native_callback((uintptr_t)entry->address);
        for (j=0;j<i;++j)
            if (!strcmp(spec->exports[j].name,entry->name) || spec->exports[j].ordinal==entry->ordinal)
                recon_unsupported("module duplicate export name/ordinal");
    }
    for (i=0;i<spec->count;++i) {
        strcpy(module->exports[i].name,spec->exports[i].name);
        module->exports[i].ordinal=spec->exports[i].ordinal;
        module->exports[i].address=spec->exports[i].address;
    }
    module->export_count=spec->count; module->lifecycle=spec->lifecycle;
    module->provider_context=spec->context;
    module->provider_state=RECON_MODULE_PROVIDER_REGISTERED;
    errno=saved; last_error=error;
}
unsigned recon_module_provider_state(HMODULE handle)
{
    NativeModule *module=recon_registered_module(handle);
    if (!module) recon_unsupported("unknown module provider state owner");
    return module->provider_state;
}
static void module_lifecycle(NativeModule *module,unsigned event)
{
    int saved=errno; unsigned long dos=recon_doserrno; DWORD error=last_error;
    module->lifecycle((HMODULE)module,event,module->provider_context);
    if (errno!=saved || recon_doserrno!=dos || last_error!=error)
        recon_unsupported("module lifecycle changed unreviewed error state");
}
void recon_register_module_memory(HMODULE handle,const ReconModuleMemorySpec *proof,void *address,size_t bytes)
{
    NativeModule *module=recon_registered_module(handle); NativeImageSpan span;
    NativeModuleMemory *entry; unsigned n; int saved_errno=errno; DWORD saved_error=last_error;
    if (!module || !proof || !proof->sha256 || strcmp(module->sha256,proof->sha256) ||
        !proof->original_rva || !proof->size || proof->size!=bytes ||
        !proof->original_image_base || proof->original_image_base>0xffffffffUL-proof->original_rva ||
        proof->original_image_base+proof->original_rva!=proof->original_va ||
        proof->original_va>0xffffffffUL-proof->size || !address ||
        (uintptr_t)address>UINTPTR_MAX-bytes || module_memory_count==32)
        recon_unsupported("module memory proof identity/extent");
    span.begin=(uintptr_t)address; span.end=span.begin+bytes; span.found=0;
    dl_iterate_phdr(writable_image_span,&span);
    if (!span.found) recon_unsupported("module memory is not writable owner image");
    for (n=0;n<module_memory_count;++n) {
        entry=native_module_memory+n;
        if (span.begin<(uintptr_t)entry->address+entry->size && span.end>(uintptr_t)entry->address)
            recon_unsupported("module memory overlapping owner");
        if (entry->module==module && proof->original_rva<entry->original_rva+entry->size &&
            proof->original_rva+proof->size>entry->original_rva)
            recon_unsupported("module memory overlapping original range");
    }
    entry=native_module_memory+module_memory_count++;
    entry->module=module; entry->address=address; entry->original_rva=proof->original_rva; entry->size=bytes;
    errno=saved_errno; last_error=saved_error;
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

/* A case-bound captured environment, not a guess from Linux hardware. */
typedef struct NativeSystemProfile {
    char reference_sha256[65];
    SYSTEM_INFO system; MEMORYSTATUS memory;
    unsigned bound,system_calls,memory_calls;
} NativeSystemProfile;
static NativeSystemProfile native_system_profile;

void recon_load_system_profile(const char *filename,const char *reference_sha256)
{
    unsigned char bytes[136]; struct stat st; int fd,saved_errno=errno;
    DWORD saved_error=last_error; size_t n; ssize_t count;
    if (native_system_profile.bound || !filename || !reference_sha256 || strlen(reference_sha256)!=64)
        recon_unsupported("system profile identity");
    for (n=0;n<64;++n) if (!((reference_sha256[n]>='0' && reference_sha256[n]<='9') ||
        (reference_sha256[n]>='a' && reference_sha256[n]<='f'))) recon_unsupported("system profile SHA256");
    fd=open(filename,O_RDONLY|O_CLOEXEC);
    if (fd<0 || fstat(fd,&st) || st.st_size!=sizeof(bytes)) recon_unsupported("system profile file");
    count=read(fd,bytes,sizeof(bytes)); close(fd);
    if (count!=sizeof(bytes) || memcmp(bytes,"RSE1",4) || memcmp(bytes+4,reference_sha256,64))
        recon_unsupported("system profile reference");
    memcpy(&native_system_profile.system,bytes+68,36);
    memcpy(&native_system_profile.memory,bytes+104,32);
    if (!native_system_profile.system.dwPageSize ||
        (native_system_profile.system.dwPageSize&(native_system_profile.system.dwPageSize-1)) ||
        native_system_profile.memory.dwLength!=32) recon_unsupported("system profile shape");
    memcpy(native_system_profile.reference_sha256,reference_sha256,65);
    native_system_profile.bound=1; errno=saved_errno; last_error=saved_error;
}

/* A separately reached call does not authorize an unvisited memory sample. */
void recon_load_system_info_profile(const char *filename,const char *reference_sha256)
{
    unsigned char bytes[104];struct stat st;int fd,saved_errno=errno;
    DWORD saved_error=last_error;size_t n;ssize_t count;
    if (native_system_profile.bound || !filename || !reference_sha256 || strlen(reference_sha256)!=64)
        recon_unsupported("system-info profile identity");
    for (n=0;n<64;++n) if (!((reference_sha256[n]>='0' && reference_sha256[n]<='9') ||
        (reference_sha256[n]>='a' && reference_sha256[n]<='f'))) recon_unsupported("system-info profile SHA256");
    fd=open(filename,O_RDONLY|O_CLOEXEC);
    if (fd<0 || fstat(fd,&st) || st.st_size!=sizeof(bytes)) recon_unsupported("system-info profile file");
    count=read(fd,bytes,sizeof(bytes));close(fd);
    if (count!=sizeof(bytes) || memcmp(bytes,"RSI1",4) || memcmp(bytes+4,reference_sha256,64))
        recon_unsupported("system-info profile reference");
    memcpy(&native_system_profile.system,bytes+68,36);
    if (!native_system_profile.system.dwPageSize ||
        (native_system_profile.system.dwPageSize&(native_system_profile.system.dwPageSize-1)))
        recon_unsupported("system-info profile shape");
    memcpy(native_system_profile.reference_sha256,reference_sha256,65);
    native_system_profile.bound=2;errno=saved_errno;last_error=saved_error;
}

void WINAPI recon_GetSystemInfo(LPSYSTEM_INFO output)
{
    if (!output || !native_system_profile.bound || native_system_profile.system_calls ||
        native_system_profile.memory_calls) recon_unsupported("GetSystemInfo captured inventory");
    memcpy(output,&native_system_profile.system,sizeof(*output));
    ++native_system_profile.system_calls;
}

void WINAPI recon_GlobalMemoryStatus(LPMEMORYSTATUS output)
{
    if (!output || native_system_profile.bound!=1 || native_system_profile.system_calls!=1 ||
        native_system_profile.memory_calls) recon_unsupported("GlobalMemoryStatus captured inventory");
    memcpy(output,&native_system_profile.memory,sizeof(*output));
    ++native_system_profile.memory_calls;
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

#ifdef RECON_CRT_PROFILE_MSVCRT_510
/* These fixed message layouts are exercised by typed native App units. The
 * registered template authenticates content; it is not a generic insert parser. */
static DWORD format_unit_diagnostic(DWORD id,const char *text,LPSTR output,
                                    DWORD capacity,va_list *arguments)
{
    const char *pattern,*middle="",*tail="",*filename=NULL,*name=NULL,*parts[7];
    const WCHAR *wide=NULL;char number[12],ascii[257],*destination;
    size_t n,length=0,file_length=0,name_length=0,sizes[7],count=0;
    uintptr_t out=(uintptr_t)output,source=(uintptr_t)text,file=0,third=0;
    int line;va_list copied;
    switch(id) {
    case 2104:pattern="%1 (%2!d!): error RC2104 : undefined keyword or key name: %3!ws!\r\n";
        middle="): error RC2104 : undefined keyword or key name: ";tail="\r\n";break;
    case 2175:pattern="%1 (%2!d!): error RC2175 : resource file %3!ws! is not in 3.00 format\r\n";
        middle="): error RC2175 : resource file ";tail=" is not in 3.00 format\r\n";break;
    case 2135:pattern="%1 (%2!d!): error RC2135 : file not found: %3\r\n";
        middle="): error RC2135 : file not found: ";tail="\r\n";break;
    case 1003:pattern="error count exceeds %1!d!; stopping compilation\r\n";break;
    case 20101:pattern="RC terminated by user\r\n";break;
    default:recon_unsupported("unit diagnostic ID");return 0;
    }
    if(strcmp(text,pattern) || recon_mb_codepage || recon_mb_codepage_gate ||
       recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("unit diagnostic exact template/default-C");
    for(n=0;n<6;++n)if(recon_lc_handle[n])recon_unsupported("unit diagnostic locale handles");
    if(id==20101){parts[0]=pattern;count=1;}
    else if(id==1003) {
        va_copy(copied,*arguments);line=va_arg(copied,int);va_end(copied);
        recon_itoa(line,number,10);parts[0]="error count exceeds ";parts[1]=number;
        parts[2]="; stopping compilation\r\n";count=3;
    }
    else {
        va_copy(copied,*arguments);filename=va_arg(copied,const char *);line=va_arg(copied,int);
        if(id==2135)name=va_arg(copied,const char *);else wide=va_arg(copied,const WCHAR *);
        va_end(copied);file=(uintptr_t)filename;third=wide?(uintptr_t)wide:(uintptr_t)name;
        if(file<4096 || file>UINTPTR_MAX-MAX_PATH || third<4096 ||
           third>UINTPTR_MAX-(wide?514:257) || (wide && (third&1)))
            recon_unsupported("unit diagnostic insert extents");
        for(file_length=0;file_length<MAX_PATH && filename[file_length];++file_length)
            if((unsigned char)filename[file_length]>=128)recon_unsupported("unit diagnostic ASCII filename");
        if(file_length==MAX_PATH)recon_unsupported("unit diagnostic filename length");
        for(name_length=0;name_length<257;++name_length) {
            unsigned character=wide?wide[name_length]:(unsigned char)name[name_length];
            if(character>=128)recon_unsupported("unit diagnostic ASCII third insert");
            ascii[name_length]=(char)character;if(!character)break;
        }
        if(name_length==257)recon_unsupported("unit diagnostic third insert length");
        recon_itoa(line,number,10);parts[0]=filename;parts[1]=" (";parts[2]=number;
        parts[3]=middle;parts[4]=ascii;parts[5]=tail;count=6;
    }
    for(n=0;n<count;++n){sizes[n]=strlen(parts[n]);length+=sizes[n];}
    if(out<4096 || capacity<=length || out>UINTPTR_MAX-length-1 ||
       source>UINTPTR_MAX-strlen(text)-1 ||
       (out<source+strlen(text)+1 && source<out+length+1) ||
       (filename && out<file+file_length+1 && file<out+length+1) ||
       (third && out<third+(name_length+1)*(wide?2:1) && third<out+length+1))
        recon_unsupported("unit diagnostic output capacity/span/overlap");
    destination=output;for(n=0;n<count;++n){memcpy(destination,parts[n],sizes[n]);destination+=sizes[n];}
    *destination=0;return (DWORD)length;
}
static DWORD format_duplicate_string_error(const char *text,LPSTR output,DWORD capacity,va_list *arguments)
{
    const char *filename,*parts[16];const WCHAR *names[2];
    size_t sizes[16],units[2],file_length,length=0,n,index;uintptr_t file,wide[2],out=(uintptr_t)output,source=(uintptr_t)text;
    char line_number[12],decimal[11],hex[9],strings[2][256],*destination;
    int line;unsigned ordinal;va_list copied;
    if(strcmp(text,"%1 (%2!d!): error RC2151 : cannot reuse string constants, %3!d!(0x%3!X!) - \"%4!ws!\" already defined. Cannot add %3!d!(0x%3!X!) - \"%5!ws!\".\r\n") ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("message2151 exact template/default-C domain");
    for(n=0;n<6;++n)if(recon_lc_handle[n])recon_unsupported("message2151 locale handles");
    va_copy(copied,*arguments);filename=va_arg(copied,const char *);line=va_arg(copied,int);
    ordinal=va_arg(copied,unsigned);names[0]=va_arg(copied,const WCHAR *);names[1]=va_arg(copied,const WCHAR *);va_end(copied);
    file=(uintptr_t)filename;
    if(ordinal>65535 || file<4096 || file>UINTPTR_MAX-MAX_PATH)
        recon_unsupported("message2151 WORD ordinal/filename extent");
    for(file_length=0;file_length<MAX_PATH && filename[file_length];++file_length)
        if((unsigned char)filename[file_length]>=128)recon_unsupported("message2151 ASCII ANSI filename");
    if(file_length==MAX_PATH)recon_unsupported("message2151 filename length");
    for(index=0;index<2;++index) {
        wide[index]=(uintptr_t)names[index];
        if(wide[index]<4096 || (wide[index]&1) || wide[index]>UINTPTR_MAX-256*sizeof(WCHAR))
            recon_unsupported("message2151 WCHAR string extent");
        for(units[index]=0;units[index]<256 && names[index][units[index]];++units[index])
            if(names[index][units[index]]>=128)recon_unsupported("message2151 ASCII WCHAR strings");
        if(units[index]==256)recon_unsupported("message2151 string length");
        for(n=0;n<=units[index];++n)strings[index][n]=(char)names[index][n];
    }
    /* The real caller zero-extends a WORD, so its signed decimal insert and
     * unsigned hexadecimal insert describe the same positive ordinal. */
    recon_itoa(line,line_number,10);recon_ultoa(ordinal,decimal,10);recon_ultoa(ordinal,hex,16);
    for(n=0;hex[n];++n)if(hex[n]>='a' && hex[n]<='f')hex[n]-='a'-'A';
    parts[0]=filename;parts[1]=" (";parts[2]=line_number;
    parts[3]="): error RC2151 : cannot reuse string constants, ";parts[4]=decimal;
    parts[5]="(0x";parts[6]=hex;parts[7]=") - \"";parts[8]=strings[0];
    parts[9]="\" already defined. Cannot add ";parts[10]=decimal;parts[11]="(0x";
    parts[12]=hex;parts[13]=") - \"";parts[14]=strings[1];parts[15]="\".\r\n";
    for(n=0;n<16;++n){sizes[n]=strlen(parts[n]);length+=sizes[n];}
    if(out<4096 || capacity<=length || out>UINTPTR_MAX-length-1 || source>UINTPTR_MAX-strlen(text)-1 ||
       (out<source+strlen(text)+1 && source<out+length+1) ||
       (out<file+file_length+1 && file<out+length+1))
        recon_unsupported("message2151 output capacity/extents/template or filename overlap");
    for(index=0;index<2;++index)if(out<wide[index]+(units[index]+1)*sizeof(WCHAR) && wide[index]<out+length+1)
        recon_unsupported("message2151 output/WCHAR overlap");
    destination=output;
    for(n=0;n<16;++n){memcpy(destination,parts[n],sizes[n]);destination+=sizes[n];}
    *destination=0;return (DWORD)length;
}
static DWORD format_directive_warning(const char *text,LPSTR output,DWORD capacity,va_list *arguments)
{
    const char *prefix="unexpected characters following '",*suffix="' directive; newline expected\r\n";
    const WCHAR *directive;size_t units,length,n;uintptr_t out=(uintptr_t)output,wide,source=(uintptr_t)text;
    char *destination;va_list copied;
    if(strcmp(text,"unexpected characters following '%1!ws!' directive; newline expected\r\n") ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("message4067 exact template/default-C domain");
    for(n=0;n<6;++n)if(recon_lc_handle[n])recon_unsupported("message4067 locale handles");
    va_copy(copied,*arguments);directive=va_arg(copied,const WCHAR *);va_end(copied);wide=(uintptr_t)directive;
    if(wide<4096 || (wide&1) || wide>UINTPTR_MAX-257*sizeof(WCHAR))
        recon_unsupported("message4067 WCHAR insert extent");
    for(units=0;units<257 && directive[units];++units)
        if(directive[units]>=128)recon_unsupported("message4067 ASCII WCHAR directive");
    if(units==257)recon_unsupported("message4067 directive length");
    length=strlen(prefix)+units+strlen(suffix);
    if(out<4096 || capacity<=length || out>UINTPTR_MAX-length-1 ||
       source>UINTPTR_MAX-strlen(text)-1 ||
       (out<source+strlen(text)+1 && source<out+length+1) ||
       (out<wide+(units+1)*sizeof(WCHAR) && wide<out+length+1))
        recon_unsupported("message4067 output capacity/extents/overlap");
    destination=output;memcpy(destination,prefix,strlen(prefix));destination+=strlen(prefix);
    for(n=0;n<units;++n)*destination++=(char)directive[n];
    memcpy(destination,suffix,strlen(suffix)+1);return (DWORD)length;
}
static DWORD format_mac_cursor_warning(const char *text,LPSTR output,DWORD capacity,va_list *arguments)
{
    const char *filename,*middle="): warning RC4506 : scaling cursor ",*tail=" down to 16 by 16\r\n";
    const WCHAR *name;char number[12],*destination;int line;
    size_t file_length,name_length,number_length,length,n;va_list copied;
    uintptr_t out=(uintptr_t)output,file,wide,source=(uintptr_t)text;
    if(strcmp(text,"%1 (%2!d!): warning RC4506 : scaling cursor %3!ws! down to 16 by 16\r\n") ||
       recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
        recon_unsupported("message4506 exact template/default-C domain");
    for(n=0;n<6;++n)if(recon_lc_handle[n])recon_unsupported("message4506 locale handles");
    va_copy(copied,*arguments);filename=va_arg(copied,const char *);
    line=va_arg(copied,int);name=va_arg(copied,const WCHAR *);va_end(copied);
    file=(uintptr_t)filename;wide=(uintptr_t)name;
    if(file<4096 || file>UINTPTR_MAX-MAX_PATH || wide<4096 || (wide&1) ||
       wide>UINTPTR_MAX-257*sizeof(WCHAR))
        recon_unsupported("message4506 insert extents");
    for(file_length=0;file_length<MAX_PATH && filename[file_length];++file_length)
        if((unsigned char)filename[file_length]>=128)recon_unsupported("message4506 ANSI filename");
    for(name_length=0;name_length<257 && name[name_length];++name_length)
        if(name[name_length]>=128)recon_unsupported("message4506 ASCII WCHAR name");
    if(file_length==MAX_PATH || name_length==257)recon_unsupported("message4506 insert lengths");
    recon_itoa(line,number,10);number_length=strlen(number);
    length=file_length+2+number_length+strlen(middle)+name_length+strlen(tail);
    if(out<4096 || capacity<=length || out>UINTPTR_MAX-length-1 ||
       source>UINTPTR_MAX-strlen(text)-1 ||
       (out<source+strlen(text)+1 && source<out+length+1) ||
       (out<file+file_length+1 && file<out+length+1) ||
       (out<wide+(name_length+1)*sizeof(WCHAR) && wide<out+length+1))
        recon_unsupported("message4506 output capacity/extents/overlap");
    destination=output;memcpy(destination,filename,file_length);destination+=file_length;
    memcpy(destination," (",2);destination+=2;
    memcpy(destination,number,number_length);destination+=number_length;
    memcpy(destination,middle,strlen(middle));destination+=strlen(middle);
    for(n=0;n<name_length;++n)*destination++=(char)name[n];
    memcpy(destination,tail,strlen(tail));destination+=strlen(tail);*destination=0;
    return (DWORD)length;
}
static DWORD format_mac_type_warning(const char *text,LPSTR output,DWORD capacity,va_list *arguments)
{
    const char *cursor,*inserts[6];size_t sizes[6],length=0,n,source_length=strlen(text);
    unsigned seen[6]={0,0,0,0,0,0},index,width;int value;char number[12],*destination;
    uintptr_t out=(uintptr_t)output,source=(uintptr_t)text;va_list copied;
    if(source_length>1024) recon_unsupported("message4509 template length");
    for(cursor=text;*cursor;++cursor) {
        if((unsigned char)*cursor>=128) recon_unsupported("message4509 template encoding");
        if(*cursor!='%')continue;
        if(!strncmp(cursor,"%2!d!",5)){index=2;width=5;}
        else if((cursor[1]=='1' || (cursor[1]>='3' && cursor[1]<='5')) &&
                cursor[2]!='!' && !isdigit((unsigned char)cursor[2])) {index=cursor[1]-'0';width=2;}
        else recon_unsupported("message4509 insert grammar");
        if(++seen[index]!=1) recon_unsupported("message4509 duplicate insert");
        cursor+=width-1;
    }
    for(index=1;index<=5;++index)if(seen[index]!=1)recon_unsupported("message4509 insert arity");
    va_copy(copied,*arguments);inserts[1]=va_arg(copied,const char *);value=va_arg(copied,int);
    for(index=3;index<=5;++index)inserts[index]=va_arg(copied,const char *);
    va_end(copied);recon_itoa(value,number,10);inserts[2]=number;
    for(index=1;index<=5;++index) {
        if(!inserts[index] || (uintptr_t)inserts[index]>UINTPTR_MAX-MAX_PATH)
            recon_unsupported("message4509 string extent");
        sizes[index]=strnlen(inserts[index],MAX_PATH);
        if(sizes[index]==MAX_PATH)recon_unsupported("message4509 string length");
        for(n=0;n<sizes[index];++n)if((unsigned char)inserts[index][n]>=128)
            recon_unsupported("message4509 nonASCII insert");
    }
    for(cursor=text;*cursor;++cursor) {
        if(*cursor!='%'){++length;continue;}
        index=cursor[1]-'0';length+=sizes[index];cursor+=index==2?4:1;
    }
    if(!output || capacity<=length || out>UINTPTR_MAX-length-1 ||
       source>UINTPTR_MAX-source_length-1 ||
       (out<source+source_length+1 && source<out+length+1))
        recon_unsupported("message4509 output capacity/extent/template overlap");
    for(index=1;index<=5;++index)if(out<(uintptr_t)inserts[index]+sizes[index]+1 &&
       (uintptr_t)inserts[index]<out+length+1)
        recon_unsupported("message4509 output/insert overlap");
    destination=output;
    for(cursor=text;*cursor;++cursor) {
        if(*cursor!='%')*destination++=*cursor;
        else {index=cursor[1]-'0';memcpy(destination,inserts[index],sizes[index]);
              destination+=sizes[index];cursor+=index==2?4:1;}
    }
    *destination=0;return (DWORD)length;
}
#endif

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
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if((entry->id==2104 || entry->id==2175 || entry->id==2135 || entry->id==20101 || entry->id==1003) && entry->language==0x409)
        return format_unit_diagnostic(entry->id,text,output,capacity,arguments);
    if(entry->id==2151 && entry->language==0x409) return format_duplicate_string_error(text,output,capacity,arguments);
    if(entry->id==4067 && entry->language==0x409) return format_directive_warning(text,output,capacity,arguments);
    if(entry->id==4506 && entry->language==0x409) return format_mac_cursor_warning(text,output,capacity,arguments);
    if(entry->id==4509 && entry->language==0x409) return format_mac_type_warning(text,output,capacity,arguments);
#endif
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
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(id==10056 && !language && source && output && capacity==4096 &&
       (uintptr_t)output>=4096 && (uintptr_t)output<=UINTPTR_MAX-4096) {
        NativeModule *module=recon_registered_module((HMODULE)source);
        if(module && !strcmp(module->sha256,"bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0")) {
            /* Full pristine message-table absence is separately established.
             * Native output is left untouched; original failure-buffer contents
             * are unmeasured and rc_message consumes only the zero return. */
            last_error=317;return 0; /* ERROR_MR_MID_NOT_FOUND, WINERROR.H */
        }
    }
#endif
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
#ifdef RECON_CRT_PROFILE_MSVCRT_510
static int headless_conout_state(void)
{
    struct stat input,output,error,null_device;
    int terminal;
    if(standard_handles[0]!=(HANDLE)1 || standard_handles[1]!=(HANDLE)2 ||
       standard_handles[2]!=(HANDLE)3) return 0;
    if(fstat(0,&input) || fstat(1,&output) || fstat(2,&error) ||
       stat("/dev/null",&null_device)) return 0;
    if(!S_ISCHR(input.st_mode) || !S_ISCHR(null_device.st_mode) ||
       input.st_rdev!=null_device.st_rdev || !S_ISREG(output.st_mode) ||
       !S_ISREG(error.st_mode)) return 0;
    terminal=open("/dev/tty",O_RDWR|O_NOCTTY|O_CLOEXEC);
    if(terminal>=0) { close(terminal);return 0; }
    return errno==ENXIO || errno==ENODEV || errno==ENOTTY;
}
#endif
HANDLE WINAPI recon_CreateFileA(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,DWORD disposition,DWORD attributes,HANDLE template)
{
    char path[PATH_MAX]; int flags,fd,saved_errno=errno;
    (void)share; (void)template;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(name && !strcmp(name,"CONOUT$")) {
        if(access!=(GENERIC_READ|GENERIC_WRITE) || share || security ||
           disposition!=OPEN_EXISTING || attributes || template)
            recon_unsupported("CONOUT$ outside reviewed open tuple");
        flags=headless_conout_state();errno=saved_errno;
        if(!flags) recon_unsupported("CONOUT$ outside reviewed headless state");
        last_error=ERROR_INVALID_HANDLE;
        return INVALID_HANDLE_VALUE;
    }
#endif
    if (recon_host_path(name,path,sizeof(path))<0) { last_error=error_code(errno); errno=saved_errno; return INVALID_HANDLE_VALUE; }
    recon_fsopen_check_path(path);
    flags=(access&GENERIC_WRITE)?(access&GENERIC_READ)?O_RDWR:O_WRONLY:O_RDONLY;
    switch (disposition) {
    case CREATE_NEW: flags|=O_CREAT|O_EXCL; break;
    case CREATE_ALWAYS: flags|=O_CREAT|O_TRUNC; break;
    case OPEN_ALWAYS: flags|=O_CREAT; break;
    case TRUNCATE_EXISTING: flags|=O_TRUNC; break;
    case OPEN_EXISTING: break;
    default: last_error=ERROR_INVALID_PARAMETER; errno=saved_errno; return INVALID_HANDLE_VALUE;
    }
    if (!security || !security->bInheritHandle) flags|=O_CLOEXEC;
    fd=open(path,flags,attributes&FILE_ATTRIBUTE_READONLY?0444:0666);
    if (fd<0) {
        int failure=errno;DWORD code=error_code(failure);char *slash;
        /* Linux ENOENT covers both a missing leaf and a missing containing
         * directory. Inspect only the parent on this failed-open path. */
        if (failure==ENOENT && (slash=strrchr(path,'/'))!=NULL) {
            struct stat parent;
            if (slash==path) slash[1]=0; else *slash=0;
            if (!stat(path,&parent)) {
                if (!S_ISDIR(parent.st_mode)) code=ERROR_PATH_NOT_FOUND;
            } else if (errno==ENOENT || errno==ENOTDIR) code=ERROR_PATH_NOT_FOUND;
        }
        last_error=code;errno=saved_errno;return INVALID_HANDLE_VALUE;
    }
    if (attributes&FILE_FLAG_DELETE_ON_CLOSE) unlink(path);
    last_error=0; errno=saved_errno; return (HANDLE)(uintptr_t)(fd+1);
}
/* The reached deletion occurs only after this file's views and handles close.
 * A different file may still have a live view during the cleanup loop. */
static void delete_file_closed(const struct stat *wanted)
{
    View *view;DIR *directory;struct dirent *entry;struct stat opened;char *end;int fd;
    for(view=views;view;view=view->next)
        if(view->file_device==wanted->st_dev && view->file_inode==wanted->st_ino)
            recon_unsupported("DeleteFileA live mapped view");
    directory=opendir("/proc/self/fd");
    if(!directory) recon_unsupported("DeleteFileA descriptor inventory");
    for(;;) {
        errno=0;entry=readdir(directory);
        if(!entry) {
            if(errno) recon_unsupported("DeleteFileA descriptor enumeration");
            break;
        }
        fd=(int)strtol(entry->d_name,&end,10);
        if(end==entry->d_name || *end || fd==dirfd(directory)) continue;
        if(fstat(fd,&opened)) {
            if(errno==EBADF) continue;
            recon_unsupported("DeleteFileA descriptor identity");
        }
        if(opened.st_dev==wanted->st_dev && opened.st_ino==wanted->st_ino)
            recon_unsupported("DeleteFileA open file/mapping alias");
    }
    if(closedir(directory)) recon_unsupported("DeleteFileA descriptor close");
}
BOOL WINAPI recon_DeleteFileA(LPCSTR name)
{
    char path[PATH_MAX],parent[PATH_MAX],*separator;struct stat information;
    int saved_errno=errno,failure;DWORD saved_error=last_error,code;
    if(!name || !*name || strpbrk(name,"*?") || recon_host_path(name,path,sizeof(path)))
        recon_unsupported("DeleteFileA path domain");
    if(!lstat(path,&information)) {
        if(!S_ISREG(information.st_mode)) recon_unsupported("DeleteFileA file kind");
        if(!(information.st_mode&0222)) {
            errno=saved_errno;last_error=ERROR_ACCESS_DENIED;return FALSE;
        }
        delete_file_closed(&information);
        if(!unlink(path)) {errno=saved_errno;last_error=saved_error;return TRUE;}
    }
    failure=errno;
    if(failure==ENOENT || failure==ENOTDIR) {
        strcpy(parent,path);separator=strrchr(parent,'/');
        if(!separator) strcpy(parent,".");
        else if(separator==parent) separator[1]=0;
        else *separator=0;
        if(!stat(parent,&information)) code=S_ISDIR(information.st_mode)?ERROR_FILE_NOT_FOUND:ERROR_PATH_NOT_FOUND;
        else if(errno==ENOENT || errno==ENOTDIR) code=ERROR_PATH_NOT_FOUND;
        else recon_unsupported("DeleteFileA parent failure");
    } else if(failure==EACCES || failure==EPERM) code=ERROR_ACCESS_DENIED;
    else recon_unsupported("DeleteFileA deletion failure");
    errno=saved_errno;last_error=code;return FALSE;
}
BOOL WINAPI recon_CloseHandle(HANDLE handle)
{
    Mapping **p,*m; int fd;
    for (p=&mappings;(m=*p)!=NULL;p=&m->next) if ((HANDLE)m==handle) {
        *p=m->next; close(m->fd); free(m); return TRUE;
    }
    fd=handle_fd(handle); if (fd<0) return FALSE;
    if (close(fd)<0) { last_error=error_code(errno); return FALSE; }
    return TRUE;
}
DWORD WINAPI recon_GetFileSize(HANDLE handle,LPDWORD high)
{
    int saved_errno=errno,fd; DWORD saved_error=last_error; struct stat st;
    if (high) recon_unsupported("GetFileSize high word");
    fd=handle_fd(handle);
    if (fd<0 || fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_size<0 ||
        (uint64_t)st.st_size>=0xffffffffULL) recon_unsupported("GetFileSize regular success");
    errno=saved_errno; last_error=saved_error; return (DWORD)st.st_size;
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
    /* The reached NULL-high, unambiguous success preserves the caller's
     * LastError (including a preceding ERROR_USER_MAPPED_FILE). Keep the
     * existing highword and ambiguous-lowword behavior separate. */
    if (high || (DWORD)position==0xffffffffUL) last_error=0;
    return position;
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
    int saved_errno=errno,fd=handle_fd(handle),regular; ssize_t n; struct stat info;
    if (written) *written=0;
    if (overlap) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    if (fd<0) return FALSE;
    regular=!fstat(fd,&info) && S_ISREG(info.st_mode);
    do n=write(fd,buffer,count); while (n<0 && errno==EINTR);
    if (n<0) { last_error=error_code(errno); return FALSE; }
    if (written) *written=n;
    /* Actual disk-file API probes preserve both independent error slots.
     * Other handle kinds keep their prior contract pending reached evidence. */
    if (regular) errno=saved_errno; else last_error=0;
    return TRUE;
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
    if (!page && getenv("RECON_ACP")) {
        int saved_errno=errno;
        if (recon_GetACP()!=1252) recon_unsupported("GetCPInfo unreviewed ACP profile");
        info->MaxCharSize=1;info->DefaultChar[0]='?';info->DefaultChar[1]=0;
        memset(info->LeadByte,0,sizeof(info->LeadByte));
        errno=saved_errno;return TRUE;
    }
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
    if (!strcasecmp(name,"ntdll") || !strcasecmp(name,"ntdll.dll")) return (HMODULE)3;
    for (n=0;n<module_count;++n)
        if (!strcasecmp(name,native_modules[n].name) || !strcasecmp(name,native_modules[n].path)) {
            if (native_modules[n].provider_state && native_modules[n].provider_state!=RECON_MODULE_PROVIDER_LOADED)
                recon_unsupported("GetModuleHandle outside loaded provider lease");
            return (HMODULE)(native_modules+n);
        }
    last_error=ERROR_MOD_NOT_FOUND; return NULL;
}
HMODULE WINAPI recon_LoadLibraryA(LPCSTR name)
{
    unsigned n,configured=0;
    if (name && !strcasecmp(name,"kernel32.dll")) return (HMODULE)1;
    if (name && !strcasecmp(name,"user32.dll")) return (HMODULE)2;
    if (name && (!strcasecmp(name,"imagehlp") || !strcasecmp(name,"imagehlp.dll"))) {
        imagehlp_loaded=1;return (HMODULE)4;
    }
    for (n=0;n<module_count;++n) {
        NativeModule *module=native_modules+n;
        if (!module->provider_state) continue;
        configured=1;
        /* The observed call supplies the captured full loaded DOS path. */
        if (name && !strcasecmp(name,module->path)) {
            if (module->provider_state!=RECON_MODULE_PROVIDER_REGISTERED)
                recon_unsupported("repeated/reloaded module provider lease");
            module->provider_state=RECON_MODULE_PROVIDER_LOADING;
            module_lifecycle(module,RECON_MODULE_EVENT_LOAD);
            module->provider_state=RECON_MODULE_PROVIDER_LOADED;
            return (HMODULE)module;
        }
    }
    if (configured) recon_unsupported("unregistered module provider path");
    last_error=ERROR_MOD_NOT_FOUND; return NULL;
}
BOOL WINAPI recon_FreeLibrary(HMODULE handle)
{
    NativeModule *module=recon_registered_module(handle); unsigned n;
    if (!module || module->provider_state!=RECON_MODULE_PROVIDER_LOADED)
        recon_unsupported("FreeLibrary outside observed provider lease");
    for (n=0;n<module->export_count;++n)
        if (module->exports[n].lookups!=1)
            recon_unsupported("module export lookup inventory before free");
    module->provider_state=RECON_MODULE_PROVIDER_UNLOADING;
    module_lifecycle(module,RECON_MODULE_EVENT_FREE);
    module->provider_state=RECON_MODULE_PROVIDER_RELEASED;
    return TRUE;
}
FARPROC WINAPI recon_GetProcAddress(HMODULE module,LPCSTR name)
{
    NativeModule *provider=recon_registered_module(module); unsigned n;
    if (provider && provider->provider_state) {
        if (provider->provider_state!=RECON_MODULE_PROVIDER_LOADED)
            recon_unsupported("export lookup outside loaded provider lease");
        for (n=0;n<provider->export_count;++n) {
            NativeExport *entry=provider->exports+n;
            if (((uintptr_t)name<65536 && (uintptr_t)name==entry->ordinal) ||
                ((uintptr_t)name>=65536 && !strcmp(name,entry->name))) {
                if (entry->lookups) recon_unsupported("repeated provider export lookup");
                ++entry->lookups; return entry->address;
            }
        }
        recon_unsupported("unregistered provider export name/ordinal");
    }
    if ((uintptr_t)name<65536) recon_unsupported("unreviewed ordinal/null export lookup");
    if (module==(HMODULE)4) {
        if(!imagehlp_loaded) recon_unsupported("IMAGEHLP export before LoadLibraryA");
        if(!strcmp(name,"CheckSumMappedFile")) return (FARPROC)recon_CheckSumMappedFile;
        recon_unsupported("unreviewed IMAGEHLP export");return NULL;
    }
    if (module==(HMODULE)3) {
        if (name && !strcmp(name,"LdrProcessRelocationBlock")) return (FARPROC)recon_LdrProcessRelocationBlock;
        recon_unsupported("unreviewed NTDLL export"); return NULL;
    }
    if (module==(HMODULE)2) {
        if (!strcmp(name,"LoadStringA")) return (FARPROC)recon_LoadStringA;
        recon_unsupported("unreviewed USER32 export"); return NULL;
    }
    if (module!=(HMODULE)1) { last_error=ERROR_INVALID_HANDLE; return NULL; }
    if (!strcmp(name,"CreateFileMappingA")) return (FARPROC)recon_CreateFileMappingA;
    if (!strcmp(name,"MapViewOfFile")) return (FARPROC)recon_MapViewOfFile;
    if (!strcmp(name,"UnmapViewOfFile")) return (FARPROC)recon_UnmapViewOfFile;
    /* This absence is actually observed by LINK/CVPACK, not inferred for
     * arbitrary unimplemented exports. */
    if (!strcmp(name,"IsTNT")) { last_error=127; return NULL; }
    recon_unsupported("unreviewed KERNEL32 export"); return NULL;
}
HANDLE WINAPI recon_CreateFileMappingA(HANDLE file,LPSECURITY_ATTRIBUTES security,DWORD protection,DWORD high,DWORD low,LPCSTR name)
{
    Mapping *m; struct stat st; int fd=handle_fd(file); size_t length;
    (void)security;
    if (fd<0) return NULL;
    if (recon_fsopen_restricted(file)) recon_unsupported("mapping restricted _fsopen file");
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
    Mapping *m; View *v; void *address; size_t length;struct stat identity;
    for (m=mappings;m && (HANDLE)m!=handle;m=m->next) ;
    if (!m) { last_error=ERROR_INVALID_HANDLE; return NULL; }
    if (high || (access!=FILE_MAP_ALL_ACCESS && (access&~(FILE_MAP_READ|FILE_MAP_WRITE)))) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    if (low>=m->length || (bytes && bytes>m->length-low) || ((access&FILE_MAP_WRITE) && !m->writable)) { last_error=ERROR_INVALID_PARAMETER; return NULL; }
    length=bytes?bytes:m->length-low;
    if(fstat(m->fd,&identity)) {last_error=error_code(errno);return NULL;}
    address=mmap(NULL,length,PROT_READ|((access&FILE_MAP_WRITE)?PROT_WRITE:0),MAP_SHARED,m->fd,low);
    if (address==MAP_FAILED) { last_error=error_code(errno); return NULL; }
    v=malloc(sizeof(*v)); if (!v) { munmap(address,length); last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    v->address=address; v->length=length;v->file_device=identity.st_dev;v->file_inode=identity.st_ino;
    v->decommit_eligible=0;v->decommitted=0;v->committed_prefix=0;
    v->next=views; views=v; last_error=0; return address;
}
static DWORD checksum_u32(const unsigned char *bytes)
{
    return (DWORD)bytes[0]|((DWORD)bytes[1]<<8)|((DWORD)bytes[2]<<16)|((DWORD)bytes[3]<<24);
}
static int checksum_external_output(LPDWORD output,const View *view)
{
    uintptr_t address=(uintptr_t)output,base=(uintptr_t)view->address;
    if(!output || (address&3) || address>UINTPTR_MAX-sizeof(DWORD)) return 0;
    return address<base?base-address>=sizeof(DWORD):address-base>=view->length;
}
PIMAGE_NT_HEADERS WINAPI recon_CheckSumMappedFile(LPVOID base,DWORD length,LPDWORD old_sum,LPDWORD new_sum)
{
    const unsigned char *bytes=base;View *view;DWORD nt,optional_size,field,original;
    DWORD position=0,sum=0,word;uintptr_t old_address=(uintptr_t)old_sum,new_address=(uintptr_t)new_sum;
    for(view=views;view && view->address!=base;view=view->next) ;
    if(!view || length<sizeof(IMAGE_DOS_HEADER) || length>view->length ||
       length>UINTPTR_MAX-(uintptr_t)base)
        recon_unsupported("CheckSumMappedFile live bounded view");
    if(!checksum_external_output(old_sum,view) || !checksum_external_output(new_sum,view) ||
       (old_address<new_address?new_address-old_address:old_address-new_address)<sizeof(DWORD))
        recon_unsupported("CheckSumMappedFile external disjoint outputs");
    nt=checksum_u32(bytes+60);
    if(bytes[0]!='M' || bytes[1]!='Z' || nt<sizeof(IMAGE_DOS_HEADER) || nt>INT_MAX || (nt&3) || nt>length ||
       length-nt<sizeof(IMAGE_NT_HEADERS) || checksum_u32(bytes+nt)!=IMAGE_NT_SIGNATURE)
        recon_unsupported("CheckSumMappedFile DOS/PE headers");
    optional_size=(DWORD)bytes[nt+20]|((DWORD)bytes[nt+21]<<8);
    if(optional_size<sizeof(IMAGE_OPTIONAL_HEADER) || optional_size>length-nt-24 ||
       bytes[nt+24]!=0x0b || bytes[nt+25]!=0x01)
        recon_unsupported("CheckSumMappedFile PE32 optional header");
    field=nt+offsetof(IMAGE_NT_HEADERS,OptionalHeader.CheckSum);
    original=checksum_u32(bytes+field);
    /* Standard PE checksum: one's-complement little-endian WORD sum, with
     * the existing checksum DWORD excluded, followed by the file length.
     * An odd final byte is the low byte of its zero-extended WORD. */
    for(;;) {
        word=bytes[position];
        if(length-position>1) word|=(DWORD)bytes[position+1]<<8;
        if(position!=field && position!=field+2) sum+=word;
        sum=(sum&0xffff)+(sum>>16);
        if(length-position<=2) break;
        position+=2;
    }
    sum=(sum&0xffff)+(sum>>16);
    *old_sum=original;*new_sum=(sum&0xffff)+length;
    return (PIMAGE_NT_HEADERS)(bytes+nt);
}
BOOL WINAPI recon_FlushViewOfFile(LPCVOID address,DWORD bytes)
{
    View *view; unsigned n; int saved_errno=errno; DWORD saved_error=last_error;
    if (bytes) recon_unsupported("FlushViewOfFile whole view only");
    for (view=views;view && view->address!=address;view=view->next) ;
    if (!view) {
        for (n=0;n<module_memory_count;++n) if (native_module_memory[n].address==address) {
            errno=saved_errno; last_error=saved_error; return TRUE;
        }
        recon_unsupported("FlushViewOfFile unregistered image/view");
    }
    if (msync(view->address,view->length,MS_SYNC))
        recon_unsupported("FlushViewOfFile registered success");
    errno=saved_errno; last_error=saved_error; return TRUE;
}
LPVOID WINAPI recon_MapViewOfFileEx(HANDLE handle,DWORD access,DWORD high,DWORD low,DWORD bytes,LPVOID base)
{
    DWORD saved_error=last_error;int saved_errno=errno;LPVOID result;Mapping *mapping;
    if (base || high || low) recon_unsupported("MapViewOfFileEx fixed/partial view");
    if (bytes) {
        for(mapping=mappings;mapping && (HANDLE)mapping!=handle;mapping=mapping->next) ;
        if (!mapping || bytes!=mapping->length)
            recon_unsupported("MapViewOfFileEx explicit length must cover full owned mapping");
    }
    result=recon_MapViewOfFile(handle,access,high,low,bytes);
    if (result) {last_error=saved_error;errno=saved_errno;}
    return result;
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
BOOL WINAPI recon_SystemTimeToFileTime(const SYSTEMTIME *input,FILETIME *output)
{
    static const unsigned short cumulative[12]={0,31,59,90,120,151,181,212,243,273,304,334};
    static const unsigned char month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned year,month,limit,leap; uint64_t days,ticks;
    if (!input || !output) recon_unsupported("SystemTimeToFileTime null argument");
    year=input->wYear; month=input->wMonth;
    if (year<1601 || year>9999 || !month || month>12 || input->wHour>23 ||
        input->wMinute>59 || input->wSecond>59 || input->wMilliseconds>999)
        recon_unsupported("SystemTimeToFileTime unreviewed date range");
    leap=!(year%4) && ((year%100) || !(year%400));
    limit=month_days[month-1]+(month==2 && leap);
    if (!input->wDay || input->wDay>limit) recon_unsupported("SystemTimeToFileTime invalid date");
    days=(uint64_t)(year-1601)*365+(year-1)/4-(year-1)/100+(year-1)/400-388;
    days+=cumulative[month-1]+(month>2 && leap)+input->wDay-1;
    ticks=(((days*24+input->wHour)*60+input->wMinute)*60+input->wSecond)*10000000;
    ticks+=(uint64_t)input->wMilliseconds*10000;
    output->dwLowDateTime=(DWORD)ticks; output->dwHighDateTime=(DWORD)(ticks>>32);
    return TRUE;
}
BOOL WINAPI recon_SetFileTime(HANDLE handle,const FILETIME *creation,const FILETIME *access,const FILETIME *write_time)
{
    uint64_t ticks,seconds; struct timespec times[2]; struct stat st; int fd,saved_errno=errno;
    DWORD saved_error=last_error;
    if (creation || access || !write_time) recon_unsupported("SetFileTime last-write-only scope");
    ticks=((uint64_t)write_time->dwHighDateTime<<32)|write_time->dwLowDateTime;
    if (ticks<116444736000000000ULL) recon_unsupported("SetFileTime pre-Unix range");
    ticks-=116444736000000000ULL; seconds=ticks/10000000;
    if (seconds>0x7fffffffU) recon_unsupported("SetFileTime unreviewed native time range");
    fd=handle_fd(handle);
    if (fd<0 || fstat(fd,&st) || !S_ISREG(st.st_mode)) recon_unsupported("SetFileTime regular success");
    times[0].tv_sec=0; times[0].tv_nsec=UTIME_OMIT;
    times[1].tv_sec=(time_t)seconds; times[1].tv_nsec=(long)(ticks%10000000)*100;
    if (futimens(fd,times)) recon_unsupported("SetFileTime uncharacterized failure");
    errno=saved_errno; last_error=saved_error; return TRUE;
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
    if (kind&MEM_TOP_DOWN) {
        size_t total,prefix,suffix;uintptr_t aligned;void *reservation;
        int saved_errno=errno;DWORD saved_error=last_error;
        /* Only opaque successful NULL reservations are proved. Host placement
         * is not a promise of Windows' global highest-address ordering. */
        if (address || kind!=(MEM_RESERVE|MEM_TOP_DOWN) || protection!=PAGE_READWRITE ||
            !bytes || (bytes&4095) || bytes>SIZE_MAX-65536)
            recon_unsupported("VirtualAlloc top-down reserve scope");
        total=(size_t)bytes+65536;
        reservation=mmap(NULL,total,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if (reservation==MAP_FAILED || !reservation)
            recon_unsupported("VirtualAlloc top-down unobserved allocation failure");
        aligned=((uintptr_t)reservation+65535)&~(uintptr_t)65535;
        prefix=aligned-(uintptr_t)reservation;suffix=total-prefix-bytes;
        if ((prefix && munmap(reservation,prefix)) ||
            (suffix && munmap((void *)(aligned+bytes),suffix)))
            recon_unsupported("VirtualAlloc top-down reservation trim");
        v=malloc(sizeof(*v));
        if (!v) recon_unsupported("VirtualAlloc top-down ownership record");
        v->address=(void *)aligned;v->length=bytes;v->decommit_eligible=0;v->decommitted=0;v->committed_prefix=0;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        v->decommit_eligible=2; /* separately reserved, contiguous RW commits */
#endif
        v->next=allocations;allocations=v;
        errno=saved_errno;last_error=saved_error;
        return v->address;
    }
    if (!bytes || (kind&~(MEM_COMMIT|MEM_RESERVE)) || !(kind&(MEM_COMMIT|MEM_RESERVE)) || (protection!=PAGE_READWRITE && protection!=PAGE_READONLY)) { last_error=ERROR_NOT_SUPPORTED; return NULL; }
    prot=(kind&MEM_COMMIT)?PROT_READ|(protection==PAGE_READWRITE?PROT_WRITE:0):PROT_NONE;
    if (address && kind==MEM_COMMIT) {
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        if((uintptr_t)address>UINTPTR_MAX-bytes)
            recon_unsupported("VirtualAlloc commit wrapped span");
#endif
        for (v=allocations;v;v=v->next) if ((char *)address>=(char *)v->address && (char *)address+bytes<=(char *)v->address+v->length) {
            size_t end=0;
            if(v->decommitted)recon_unsupported("VirtualAlloc recommit after decommit");
#ifdef RECON_CRT_PROFILE_MSVCRT_510
            if(v->decommit_eligible==2) {
                size_t offset=(uintptr_t)address-(uintptr_t)v->address;
                if(protection!=PAGE_READWRITE || ((uintptr_t)address&4095) ||
                   (bytes&4095) || offset>v->committed_prefix)
                    recon_unsupported("VirtualAlloc outside contiguous readwrite prefix");
                end=offset+bytes;
            }
#endif
            if (mprotect(address,bytes,prot)<0) { last_error=error_code(errno); return NULL; }
            if(end>v->committed_prefix)v->committed_prefix=end;
            return address;
        }
        last_error=ERROR_INVALID_PARAMETER; return NULL;
    }
    if (address) flags|=MAP_FIXED_NOREPLACE;
    result=mmap(address,bytes,prot,flags,-1,0);
    if (result==MAP_FAILED) { last_error=error_code(errno); return NULL; }
    v=malloc(sizeof(*v)); if (!v) { munmap(result,bytes); last_error=ERROR_NOT_ENOUGH_MEMORY; return NULL; }
    v->address=result; v->length=bytes;
    v->decommit_eligible=!address && kind==(MEM_COMMIT|MEM_RESERVE) && protection==PAGE_READWRITE;
    v->decommitted=0;v->committed_prefix=0;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(!address && kind==MEM_RESERVE && protection==PAGE_READWRITE && !(bytes&4095))
        v->decommit_eligible=2;
#endif
    v->next=allocations;allocations=v;return result;
}
BOOL WINAPI recon_VirtualFree(LPVOID address,DWORD bytes,DWORD kind)
{
    View **p,*v;
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(kind==MEM_DECOMMIT) {
        int saved_errno=errno;
        for(v=allocations;v && v->address!=address;v=v->next){}
        if(!v || !bytes || v->decommitted ||
           !((v->decommit_eligible==1 && bytes==v->length) ||
             (v->decommit_eligible==2 && v->committed_prefix &&
              (bytes==v->committed_prefix || bytes==v->length))))
            recon_unsupported("VirtualFree whole owned allocation/prefix decommit scope");
        /* Decommit discards storage while retaining the inaccessible reserved
         * address range and its ownership until the later MEM_RELEASE. */
        if(mprotect(address,v->length,PROT_NONE) || madvise(address,v->length,MADV_DONTNEED))
            recon_unsupported("VirtualFree decommit host protection/discard failure");
        v->decommitted=1;v->committed_prefix=0;errno=saved_errno;return TRUE;
    }
#endif
    if (bytes || kind!=MEM_RELEASE) { last_error=ERROR_NOT_SUPPORTED; return FALSE; }
    for (p=&allocations;(v=*p)!=NULL;p=&v->next) if (v->address==address) {
        if (munmap(address,v->length)<0) { last_error=error_code(errno); return FALSE; }
        *p=v->next; free(v); return TRUE;
    }
    last_error=ERROR_INVALID_PARAMETER; return FALSE;
}
typedef struct NativeLocalMemory {
    struct NativeLocalMemory *next;
    HLOCAL memory; DWORD requested_size,flags;
} NativeLocalMemory;
static NativeLocalMemory *native_local_memory;
HLOCAL WINAPI recon_LocalAlloc(UINT flags,DWORD bytes)
{
    NativeLocalMemory *entry; void *memory; int saved_errno=errno;
    DWORD saved_error=last_error;
    if ((flags!=LMEM_FIXED && flags!=LMEM_ZEROINIT) || !bytes)
        recon_unsupported("LocalAlloc fixed allocation scope");
    memory=flags&LMEM_ZEROINIT?calloc(1,bytes):malloc(bytes);
    if (!memory) recon_unsupported("LocalAlloc uncharacterized allocation failure");
    entry=mmap(NULL,sizeof(*entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (entry==MAP_FAILED) recon_unsupported("LocalAlloc ownership record");
    entry->memory=memory; entry->requested_size=bytes; entry->flags=flags;
    entry->next=native_local_memory; native_local_memory=entry;
    errno=saved_errno; last_error=saved_error; return memory;
}
HLOCAL WINAPI recon_LocalFree(HLOCAL memory)
{
    NativeLocalMemory **link,*entry; int saved_errno=errno; DWORD saved_error=last_error;
    for (link=&native_local_memory;(entry=*link)!=NULL;link=&entry->next) if (entry->memory==memory) {
        *link=entry->next; free(memory); munmap(entry,sizeof(*entry));
        errno=saved_errno; last_error=saved_error; return NULL;
    }
    recon_unsupported("LocalFree unowned memory"); return memory;
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
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(page==10000 && flags==1 && count==-1 && capacity>0) {
        size_t length,n,output_bytes;uintptr_t source=(uintptr_t)input,destination=(uintptr_t)output;
        if(capacity>4177 || source<4096 || destination<4096 || (destination&1) ||
           recon_mb_codepage || recon_mb_codepage_gate || recon_mb_cur_max!=1 || recon_lc_codepage)
            recon_unsupported("MacRoman reverse ASCII/default-state/output domain");
        for(n=0;n<6;++n)if(recon_lc_handle[n])
            recon_unsupported("MacRoman reverse locale handles");
        output_bytes=(size_t)capacity*sizeof(WCHAR);
        if(source>UINTPTR_MAX-(size_t)capacity || destination>UINTPTR_MAX-output_bytes)
            recon_unsupported("MacRoman reverse input/output wrap");
        for(length=0;length<(size_t)capacity && input[length];++length)
            if((unsigned char)input[length]>=128)
                recon_unsupported("MacRoman reverse nonASCII input");
        if(length==(size_t)capacity)
            recon_unsupported("MacRoman reverse missing NUL/insufficient capacity");
        ++length;
        if(destination<source+length && source<destination+output_bytes)
            recon_unsupported("MacRoman reverse input/output overlap");
        /* The terminated ASCII arm needs no codepage table or normalization.
         * Validate its entire domain before copying the included terminator. */
        for(n=0;n<length;++n)output[n]=(WCHAR)(unsigned char)input[n];
        return (int)length;
    }
#endif
    if (!encoding || count==0 || count< -1 || capacity<0 || (flags&~9U)) { last_error=ERROR_NOT_SUPPORTED; return 0; }
    bytes=count<0?(int)strlen(input)+1:count;
    return convert("UTF-16LE",encoding,input,bytes,capacity?output:NULL,(size_t)capacity*2)/2;
}
int WINAPI recon_WideCharToMultiByte(UINT page,DWORD flags,LPCWSTR input,int count,LPSTR output,int capacity,LPCSTR replacement,BOOL *used)
{
    const char *encoding=code_page(page); int n=count;
    if(page==10000) {
#ifdef RECON_CRT_PROFILE_MSVCRT_510
        size_t bytes;uintptr_t source=(uintptr_t)input,destination=(uintptr_t)output;int i;
        /* RCDLL also queries and repeats a zero count for a menu separator.
         * This is invalid-count failure, without pointed input/output access. */
        if(!flags && !count && !capacity && !replacement && !used && input && !(source&1)) {
            last_error=ERROR_INVALID_PARAMETER;return 0;
        }
        if(flags || count<=0 || replacement || used || !input || (source&1) ||
           (size_t)count>UINTPTR_MAX/sizeof(WCHAR))
            recon_unsupported("MacRoman conversion input/flags/count");
        bytes=(size_t)count*sizeof(WCHAR);
        if(source>UINTPTR_MAX-bytes ||
           (capacity==0?output!=NULL:capacity!=count || !output))
            recon_unsupported("MacRoman conversion span/query/capacity");
        if(output && (destination>UINTPTR_MAX-(size_t)count ||
           (destination<source+bytes && source<destination+(size_t)count)))
            recon_unsupported("MacRoman conversion output wrap/overlap");
        /* Counted ASCII is identical in MacRoman. Validate the complete input
         * before writing; no terminating byte is added to an explicit count. */
        for(i=0;i<count;++i) if(input[i]>=128)
            recon_unsupported("MacRoman conversion nonASCII input");
        if(output) for(i=0;i<count;++i) output[i]=(char)input[i];
        return count;
#else
        recon_unsupported("MacRoman conversion requires explicit510 profile");return 0;
#endif
    }
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

BOOL WINAPI recon_IsValidCodePage(UINT page)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    if(page==10000 || page==1252) return TRUE;
#endif
    (void)page;recon_unsupported("unreviewed IsValidCodePage");return FALSE;
}
UINT WINAPI recon_GetACP(void)
{
    const char *profile=getenv("RECON_ACP");
    if (!profile || strcmp(profile,"1252")) recon_unsupported("GetACP without captured1252 profile");
    return 1252;
}
LPSTR WINAPI recon_CharNextA(LPCSTR text)
{
    if (recon_GetACP()!=1252 || !text || (unsigned char)*text>=128)
        recon_unsupported("CharNextA outside captured ACP1252 ASCII scope");
    return (LPSTR)(text+(*text!=0));
}

int __cdecl recon_wsprintfA(LPSTR output,LPCSTR format,...)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    va_list arguments;int value;uintptr_t destination=(uintptr_t)output,source=(uintptr_t)format;
    /* The caller owns at least12 writable bytes (RCDLL supplies128). */
    if(!output || !format || destination>UINTPTR_MAX-12 || source>UINTPTR_MAX-3 ||
       format[0]!='%' || format[1]!='d' || format[2] ||
       (destination<source+3 && source<destination+12))
        recon_unsupported("wsprintfA outside signed decimal output domain");
    va_start(arguments,format);value=va_arg(arguments,int);va_end(arguments);
    recon_itoa(value,output,10);return (int)strlen(output);
#else
    (void)output;(void)format;recon_unsupported("unreviewed wsprintfA");return 0;
#endif
}

/* Private heap support preserves the observed RC owner/flags/success path. These
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

BOOL WINAPI recon_HeapDestroy(HANDLE handle)
{
#ifdef RECON_CRT_PROFILE_MSVCRT_510
    NativeHeap **slot,*heap;NativeHeapBlock *block,*next;int saved_errno=errno;
    for(slot=&native_heaps;(heap=*slot)!=NULL;slot=&heap->next)
        if((HANDLE)heap==handle)break;
    if(!heap)recon_unsupported("HeapDestroy unowned/dead private heap");
    /* Outstanding allocations are owned by this heap, including allocations
     * intentionally retained by RC cleanup. Other heaps remain independent. */
    for(block=heap->blocks;block;block=next) {
        next=block->next;
        if(munmap(block,block->mapping_size))recon_unsupported("HeapDestroy block release failure");
    }
    *slot=heap->next;
    if(munmap(heap,sizeof(*heap)))recon_unsupported("HeapDestroy owner release failure");
    errno=saved_errno;return TRUE;
#else
    (void)handle;recon_unsupported("HeapDestroy requires explicit510 profile");return FALSE;
#endif
}

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
    if (!heap || flags!=9 || !size) recon_unsupported("unreviewed private HeapAlloc owner/flags/size");
    page=(size_t)sysconf(_SC_PAGESIZE);
    if (!page || (page&(page-1))) recon_unsupported("private heap host page size");
    prefix=(sizeof(*block)+15)&~(size_t)15;
    if ((uint64_t)prefix+size+page-1>SIZE_MAX) recon_unsupported("private HeapAlloc size overflow");
    length=(prefix+size+page-1)&~(page-1);
    block=mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (block==MAP_FAILED) recon_unsupported("private HeapAlloc allocation failure");
    block->owner=heap; block->payload=(char *)block+prefix;
    block->requested_size=size; block->flags=flags; block->mapping_size=length;
    block->next=heap->blocks; heap->blocks=block;
    errno=saved_errno;
    return block->payload;
}
LPVOID WINAPI recon_HeapReAlloc(HANDLE handle,DWORD flags,LPVOID payload,DWORD size)
{
    (void)handle;(void)flags;(void)payload;(void)size;
    recon_unsupported("unreviewed HeapReAlloc");return NULL;
}
BOOL WINAPI recon_HeapFree(HANDLE handle,DWORD flags,LPVOID payload)
{
    NativeHeap *heap; NativeHeapBlock **slot,*block; size_t length;
    int saved_errno=errno;
    for (heap=native_heaps;heap && (HANDLE)heap!=handle;heap=heap->next) {}
    if (!heap || flags!=1 || !payload) recon_unsupported("unreviewed private HeapFree owner/flags");
    for (slot=&heap->blocks;(block=*slot)!=NULL;slot=&block->next) if (block->payload==payload) break;
    if (!block) recon_unsupported("private HeapFree unowned/dead/interior block");
    length=block->mapping_size; *slot=block->next;
    if (munmap(block,length)) recon_unsupported("private HeapFree host failure");
    errno=saved_errno; return TRUE;
}
LRESULT WINAPI recon_SendMessageA(HWND window,UINT message,WPARAM wparam,LPARAM lparam)
{
    (void)window; (void)message; (void)wparam; (void)lparam;
    recon_unsupported("unreviewed SendMessageA"); return 0;
}

DWORD WINAPI recon_GetTickCount(void)
{
    struct timespec now; int saved_errno=errno; uint64_t milliseconds;
#ifdef CLOCK_BOOTTIME
    if (clock_gettime(CLOCK_BOOTTIME,&now)) recon_unsupported("GetTickCount boot clock");
#else
    if (clock_gettime(CLOCK_MONOTONIC,&now)) recon_unsupported("GetTickCount monotonic clock");
#endif
    milliseconds=(uint64_t)now.tv_sec*1000+(unsigned long)now.tv_nsec/1000000;
    errno=saved_errno;
    return (DWORD)(uint32_t)milliseconds;
}

USHORT *WINAPI recon_LdrProcessRelocationBlock(LPVOID page,DWORD count,USHORT *records,LONG delta)
{
    DWORD n; uint32_t value;
    if (!count) return records;
    if (!page || !records) recon_unsupported("relocation block pointer");
    for (n=0;n<count;++n) if ((records[n]>>12)!=0 && (records[n]>>12)!=3)
        recon_unsupported("unreviewed native relocation type");
    for (n=0;n<count;++n) if ((records[n]>>12)==3) {
        unsigned char *target=(unsigned char *)page+(records[n]&0xfff);
        memcpy(&value,target,4); value+=(uint32_t)delta; memcpy(target,&value,4);
    }
    return records+count;
}
DWORD WINAPI recon_GetFullPathNameA(LPCSTR name,DWORD capacity,LPSTR output,LPSTR *basename)
{
    char full[PATH_MAX],*part; size_t length; int saved_errno=errno;
    if (!name || !*name || strchr(name,':') || strchr(name,'\\') || strchr(name,'/') ||
        !strcmp(name,".") || !strcmp(name,"..") || !getenv("RECON_CANONICAL_CWD"))
        recon_unsupported("unreviewed GetFullPathNameA path form");
    if (!recon_fullpath(full,name,sizeof(full))) recon_unsupported("GetFullPathNameA canonical transport");
    length=strlen(full);
    if (!output || capacity<=length) recon_unsupported("unreviewed GetFullPathNameA capacity");
    memcpy(output,full,length+1); part=strrchr(output,'\\');
    if (basename) *basename=part?part+1:output;
    errno=saved_errno;
    return (DWORD)length;
}

DWORD WINAPI recon_SearchPathA(LPCSTR path,LPCSTR file,LPCSTR extension,DWORD capacity,LPSTR buffer,LPSTR *part)
{
    (void)path; (void)file; (void)extension; (void)capacity; (void)buffer; (void)part;
    recon_unsupported("unreviewed SearchPathA fallback"); return 0;
}

BOOL WINAPI recon_IsDBCSLeadByteEx(UINT page,BYTE byte)
{
    if (page!=1252 || byte>=128) recon_unsupported("IsDBCSLeadByteEx outside1252 ASCII scope");
    return FALSE;
}

void WINAPI recon_DebugBreak(void)
{
    recon_unsupported("unreviewed DebugBreak exception path");
}
