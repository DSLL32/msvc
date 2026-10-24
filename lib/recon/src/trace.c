#define _GNU_SOURCE
/* This file is compiled at the native final link, without instrumentation.
 * Its callbacks consume function pointers and emit first-encounter ELF names.
 * There are no compiler-generated logical IDs and no Win32/original execution. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include "../include/recon-trace.h"

#define UNINSTRUMENTED __attribute__((no_instrument_function))
typedef struct TraceName {
    char *name,*line;
    size_t length;
    unsigned resolved,seen;
} TraceName;
typedef struct TraceSlot {
    uintptr_t address;
    const char *line;
    size_t length;
    unsigned seen;
    unsigned char *shared_seen;
} TraceSlot;
static TraceName *names;
static size_t name_count;
static TraceSlot *slots;
static size_t slot_mask;
static int trace_active,trace_fd,trace_configured;
static unsigned char *shared_seen;
static void *shared_mapping;
static size_t shared_mapping_size;
/* Only the filter records, strings and address table persist per config. */
static struct { void *address;size_t size; } trace_mappings[3];
static unsigned trace_mapping_count;

static void UNINSTRUMENTED recon_trace_trace_error(const char *format,...)
{
    va_list args;
    va_start(args,format);
    dprintf(STDERR_FILENO,"recon trace: ");
    vdprintf(STDERR_FILENO,format,args);
    dprintf(STDERR_FILENO,"\n");
    va_end(args);
    _exit(126);
}
/* Tracing must not populate the application's libc heap: even temporary
 * symbol/name allocations can become observable in historical CRT code. */
static void *UNINSTRUMENTED recon_trace_allocate(size_t size)
{
    void *mapping;
    if (trace_mapping_count==3) recon_trace_trace_error("too many trace mappings");
    mapping=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if (mapping==MAP_FAILED) recon_trace_trace_error("cannot allocate trace mapping");
    trace_mappings[trace_mapping_count].address=mapping;
    trace_mappings[trace_mapping_count++].size=size;
    return mapping;
}
static char *UNINSTRUMENTED recon_trace_map_file(int fd,struct stat *st,int writable)
{
    char *mapping;
    if (fstat(fd,st)<0 || !S_ISREG(st->st_mode) || st->st_size<0 ||
        (uint64_t)st->st_size>SIZE_MAX)
        recon_trace_trace_error("invalid trace input file");
    if (!st->st_size) return NULL;
    mapping=mmap(NULL,(size_t)st->st_size,PROT_READ|(writable?PROT_WRITE:0),MAP_PRIVATE,fd,0);
    if (mapping==MAP_FAILED) recon_trace_trace_error("cannot map trace input file");
    return mapping;
}
static void UNINSTRUMENTED recon_trace_sift_names(size_t root,size_t count)
{
    TraceName value=names[root];
    while (root<count/2) {
        size_t child=root*2+1;
        if (child+1<count && strcmp(names[child].name,names[child+1].name)<0) ++child;
        if (strcmp(value.name,names[child].name)>=0) break;
        names[root]=names[child]; root=child;
    }
    names[root]=value;
}
static void UNINSTRUMENTED recon_trace_sort_names(void)
{
    size_t n; TraceName temporary;
    for (n=name_count/2;n;--n) recon_trace_sift_names(n-1,name_count);
    for (n=name_count;n>1;--n) {
        temporary=names[0]; names[0]=names[n-1]; names[n-1]=temporary;
        recon_trace_sift_names(0,n-1);
    }
}
static TraceName *UNINSTRUMENTED recon_trace_find_name(const char *name)
{
    size_t lo=0,hi=name_count;
    while (lo<hi) {
        size_t mid=lo+(hi-lo)/2;
        int cmp=strcmp(name,names[mid].name);
        if (!cmp) return names+mid;
        if (cmp<0) hi=mid;
        else lo=mid+1;
    }
    return NULL;
}
static void UNINSTRUMENTED recon_trace_read_name_file(const char *path,int prefix)
{
    int fd=open(path,O_RDONLY|O_CLOEXEC);
    struct stat st; char *mapping,*strings=NULL;
    size_t offset=0,count=0,size,n;
    if (fd<0) recon_trace_trace_error("cannot read %s: %s",path,strerror(errno));
    mapping=recon_trace_map_file(fd,&st,1); close(fd); size=(size_t)st.st_size;
    /* Count records before allocating; no per-record host allocation. */
    for (n=0;n<size;++n) if (mapping[n]=='\n') ++count;
    if (size && mapping[size-1]!='\n') ++count;
    if (!prefix && count) {
        if (count>100000) recon_trace_trace_error("application filter is too large");
        if (size>(SIZE_MAX/2)-count) recon_trace_trace_error("application filter is too large");
        names=recon_trace_allocate(count*sizeof(*names));
        strings=recon_trace_allocate((size+count)*2);
    }
    while (offset<size) {
        size_t start=offset;
        char *line;
        TraceName *item;
        while (offset<size && mapping[offset]!='\n') ++offset;
        n=offset-start;
        if (offset<size) ++offset;
        line=mapping+start;
        if (n && line[n-1]=='\r') --n;
        if (!n || memchr(line,0,n) || memchr(line,'\t',n) || memchr(line,' ',n))
            recon_trace_trace_error("invalid name in %s",path);
        if (prefix) {
            /* The final record need not have a newline or spare file byte. */
            size_t lo=0,hi=name_count;
            item=NULL;
            while (lo<hi) {
                size_t mid=lo+(hi-lo)/2,length=strlen(names[mid].name),common=n<length?n:length;
                int cmp=memcmp(line,names[mid].name,common);
                if (!cmp) cmp=n<length?-1:n>length?1:0;
                if (!cmp) { item=names+mid; break; }
                if (cmp<0) hi=mid; else lo=mid+1;
            }
            if (!item) recon_trace_trace_error("prefix function is outside application filter: %.*s",(int)n,line);
            if (item->seen) recon_trace_trace_error("duplicate prefix function: %s",item->name);
            item->seen=1;
        } else {
            item=names+name_count++;
            item->name=strings; memcpy(strings,line,n); strings[n]=0; strings+=n+1;
            item->line=strings; strings+=n+1;
            memcpy(item->line,line,n); item->line[n]='\n'; item->length=n+1;
        }
    }
    if (mapping) munmap(mapping,size);
}
static size_t UNINSTRUMENTED recon_trace_address_hash(uintptr_t address)
{
    return ((address>>4)^(address>>13))&slot_mask;
}
static void UNINSTRUMENTED recon_trace_insert_symbol(uintptr_t address,TraceName *name)
{
    size_t slot=recon_trace_address_hash(address);
    if (!address) recon_trace_trace_error("null application function address: %s",name->name);
    while (slots[slot].address) {
        if (slots[slot].address==address)
            recon_trace_trace_error("ambiguous application function address: %s",name->name);
        slot=(slot+1)&slot_mask;
    }
    slots[slot].address=address;
    slots[slot].line=name->line;
    slots[slot].length=name->length;
    slots[slot].seen=name->seen;
    if (shared_seen) slots[slot].shared_seen=shared_seen+(name-names);
}
/* The shared name bitmap survives exec, unlike native pointer addresses. The
 * adapter supplies one zero-filled state file per synchronous process tree. */
static void UNINSTRUMENTED recon_trace_attach_shared_state(void)
{
    const char *value=getenv("RECON_TRACE_STATE_FD");
    char *end; long descriptor; struct stat st;
    uint32_t hash=2166136261u,*header;
    size_t n,length=32+name_count; unsigned char *mapping;
    if (!value) return;
    errno=0; descriptor=strtol(value,&end,10);
    if (!*value || *end || errno || descriptor<0 || descriptor>INT_MAX ||
        fstat((int)descriptor,&st)<0 || !S_ISREG(st.st_mode) ||
        st.st_size!=(off_t)length)
        recon_trace_trace_error("invalid RECON_TRACE_STATE_FD or state size");
    mapping=mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_SHARED,(int)descriptor,0);
    if (mapping==MAP_FAILED) recon_trace_trace_error("cannot map shared trace state");
    for (n=0;n<name_count;++n) {
        const unsigned char *p=(const unsigned char *)names[n].name;
        do { hash=(hash^*p)*16777619u; } while (*p++);
    }
    header=(uint32_t *)mapping;
    if (!header[0]) {
        for (n=0;n<length;++n) if (mapping[n]) recon_trace_trace_error("shared trace state is not initialized to zero");
        header[1]=1; header[2]=(uint32_t)name_count; header[3]=hash;
        header[0]=0x31545352u; /* RST1, initialized before any child can spawn. */
    } else if (header[0]!=0x31545352u || header[1]!=1 ||
               header[2]!=name_count || header[3]!=hash)
        recon_trace_trace_error("shared trace state application filter mismatch");
    shared_mapping=mapping;shared_mapping_size=length;
    shared_seen=mapping+32;
    for (n=0;n<name_count;++n) if (names[n].seen) shared_seen[n]=1;
}
static int UNINSTRUMENTED recon_trace_main_image_base(struct dl_phdr_info *info,size_t size,void *result)
{
    (void)size;
    if (!info->dlpi_name || !*info->dlpi_name) {
        *(uintptr_t *)result=info->dlpi_addr;
        return 1;
    }
    return 0;
}
static int UNINSTRUMENTED recon_trace_in_file(uint64_t offset,uint64_t count,off_t size)
{
    return offset<=(uint64_t)size && count<=(uint64_t)size-offset;
}
static void UNINSTRUMENTED recon_trace_load_symbols(void)
{
    Elf32_Ehdr elf;
    Elf32_Shdr *sections;
    Elf32_Sym *symbols;
    char *strings,*mapping;
    struct stat st;
    uintptr_t base=0;
    int fd=open("/proc/self/exe",O_RDONLY|O_CLOEXEC);
    unsigned i; size_t n;
    if (fd<0) recon_trace_trace_error("cannot open native executable");
    mapping=recon_trace_map_file(fd,&st,0); close(fd);
    if (st.st_size<(off_t)sizeof(elf)) recon_trace_trace_error("short native ELF header");
    memcpy(&elf,mapping,sizeof(elf));
    if (memcmp(elf.e_ident,ELFMAG,SELFMAG) || elf.e_ident[EI_CLASS]!=ELFCLASS32 ||
        elf.e_ident[EI_DATA]!=ELFDATA2LSB || elf.e_machine!=EM_386 ||
        (elf.e_type!=ET_EXEC && elf.e_type!=ET_DYN) ||
        !elf.e_shnum || elf.e_shentsize!=sizeof(*sections) ||
        !recon_trace_in_file(elf.e_shoff,(uint64_t)elf.e_shnum*sizeof(*sections),st.st_size))
        recon_trace_trace_error("unsupported or invalid native ELF symbol layout");
    if (!dl_iterate_phdr(recon_trace_main_image_base,&base)) recon_trace_trace_error("cannot determine native executable load address");
    if (elf.e_type==ET_EXEC && base) recon_trace_trace_error("unexpected ET_EXEC load address");
    sections=(Elf32_Shdr *)(mapping+elf.e_shoff);
    for (i=0;i<elf.e_shnum;++i) if (sections[i].sh_type==SHT_SYMTAB) break;
    if (i==elf.e_shnum) recon_trace_trace_error("native executable has no symbol table; retain its symbols");
    {
        Elf32_Shdr sym=sections[i],str;
        if (sym.sh_link>=elf.e_shnum || sym.sh_entsize!=sizeof(*symbols) ||
            sym.sh_size%sizeof(*symbols)) recon_trace_trace_error("invalid native symbol section");
        str=sections[sym.sh_link];
        if (str.sh_type!=SHT_STRTAB || !recon_trace_in_file(sym.sh_offset,sym.sh_size,st.st_size) ||
            !recon_trace_in_file(str.sh_offset,str.sh_size,st.st_size)) recon_trace_trace_error("invalid native symbol/string ranges");
        symbols=(Elf32_Sym *)(mapping+sym.sh_offset); strings=mapping+str.sh_offset;
        for (n=0;n<sym.sh_size/sizeof(*symbols);++n) {
            TraceName *name; Elf32_Sym *symbol=symbols+n;
            if (ELF32_ST_TYPE(symbol->st_info)!=STT_FUNC || symbol->st_shndx==SHN_UNDEF) continue;
            if (symbol->st_name>=str.sh_size || !memchr(strings+symbol->st_name,0,str.sh_size-symbol->st_name))
                recon_trace_trace_error("invalid native symbol name");
            name=recon_trace_find_name(strings+symbol->st_name);
            if (!name) continue;
            if (name->resolved) recon_trace_trace_error("ambiguous native function name: %s",name->name);
            name->resolved=1; recon_trace_insert_symbol(base+symbol->st_value,name);
        }
    }
    munmap(mapping,(size_t)st.st_size);
    for (n=0;n<name_count;++n)
        if (!names[n].resolved) recon_trace_trace_error("application function has no ELF symbol: %s",names[n].name);
}
static void UNINSTRUMENTED __attribute__((constructor(101))) recon_trace_initialize_trace(void)
{
    const char *descriptor=getenv("RECON_TRACE_FD"),*filter,*prefix;
    char *end; long value; size_t count=1,n; int flags,saved_errno=errno;
    if (!descriptor) return; /* Disabled: no name files, symbols or allocation. */
    errno=0; value=strtol(descriptor,&end,10);
    if (!*descriptor || *end || errno || value<0 || value>INT_MAX)
        recon_trace_trace_error("invalid RECON_TRACE_FD");
    trace_fd=(int)value; flags=fcntl(trace_fd,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)==O_RDONLY) recon_trace_trace_error("RECON_TRACE_FD is not writable");
    filter=getenv("RECON_TRACE_FILTER"); prefix=getenv("RECON_TRACE_SEEN");
    if (!filter || !prefix) recon_trace_trace_error("enabled tracing requires RECON_TRACE_FILTER and RECON_TRACE_SEEN");
    recon_trace_read_name_file(filter,0);
    if (!name_count) recon_trace_trace_error("application filter is empty");
    recon_trace_sort_names();
    for (n=1;n<name_count;++n) if (!strcmp(names[n-1].name,names[n].name))
        recon_trace_trace_error("duplicate application function: %s",names[n].name);
    recon_trace_read_name_file(prefix,1);
    recon_trace_attach_shared_state();
    while (count<name_count*2) count*=2;
    slots=recon_trace_allocate(count*sizeof(*slots)); slot_mask=count-1;
    recon_trace_load_symbols(); trace_configured=1;trace_active=1;errno=saved_errno;
}
static void UNINSTRUMENTED recon_trace_release_configuration(void)
{
    trace_active=0;trace_configured=0;
    if (shared_mapping) munmap(shared_mapping,shared_mapping_size);
    shared_mapping=NULL;shared_mapping_size=0;shared_seen=NULL;
    while (trace_mapping_count) {
        --trace_mapping_count;
        munmap(trace_mappings[trace_mapping_count].address,
               trace_mappings[trace_mapping_count].size);
    }
    names=NULL;name_count=0;slots=NULL;slot_mask=0;
}
void UNINSTRUMENTED recon_trace_configure(const ReconTraceEntry *entries,size_t count,int fd)
{
    int saved_errno=errno,flags;size_t n,bytes=0,capacity=1;char *strings;
    if (count>100000 || (count && !entries))
        recon_trace_trace_error("invalid explicit application registry");
    flags=fcntl(fd,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)==O_RDONLY)
        recon_trace_trace_error("explicit trace fd is not writable");
    for (n=0;n<count;++n) {
        size_t length;
        if (!entries[n].address || !entries[n].name || !*entries[n].name)
            recon_trace_trace_error("invalid explicit application entry");
        length=strlen(entries[n].name);
        if (strchr(entries[n].name,'\n') || strchr(entries[n].name,'\r') ||
            length>=SIZE_MAX/2 || bytes>SIZE_MAX/2-length-1)
            recon_trace_trace_error("invalid explicit application label");
        bytes+=length+1;
    }
    recon_trace_release_configuration();trace_fd=fd;name_count=count;
    if (count) {
        names=recon_trace_allocate(count*sizeof(*names));
        strings=recon_trace_allocate(bytes*2);
        for (n=0;n<count;++n) {
            size_t length=strlen(entries[n].name);
            names[n].name=strings;memcpy(strings,entries[n].name,length+1);strings+=length+1;
            names[n].line=strings;memcpy(strings,entries[n].name,length);strings[length]='\n';strings+=length+1;
            names[n].length=length+1;
        }
        recon_trace_sort_names();
        for (n=1;n<count;++n) if (!strcmp(names[n-1].name,names[n].name))
            recon_trace_trace_error("duplicate application function: %s",names[n].name);
        shared_mapping=mmap(NULL,count,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
        if (shared_mapping==MAP_FAILED) recon_trace_trace_error("cannot allocate shared trace state");
        shared_mapping_size=count;shared_seen=shared_mapping;
        while (capacity<count*2) capacity*=2;
        slots=recon_trace_allocate(capacity*sizeof(*slots));slot_mask=capacity-1;
        for (n=0;n<count;++n)
            recon_trace_insert_symbol((uintptr_t)entries[n].address,recon_trace_find_name(entries[n].name));
    }
    trace_configured=1;errno=saved_errno;
}
void UNINSTRUMENTED recon_trace_enable(int enabled)
{
    if (enabled && !trace_configured) recon_trace_trace_error("tracing is not configured");
    trace_active=enabled && name_count;
}
void UNINSTRUMENTED recon_trace_reset(void)
{
    size_t n;trace_active=0;
    for (n=0;n<name_count;++n) names[n].seen=0;
    if (shared_seen) memset(shared_seen,0,name_count);
    if (slots) for (n=0;n<=slot_mask;++n) slots[n].seen=0;
}
/* libc exports no-op instrumentation hooks. Bind a dlopened application DSO's
 * calls locally to its own state, without changing unrelated symbol binding. */
void UNINSTRUMENTED __attribute__((visibility("protected"))) __cyg_profile_func_enter(void *function,void *caller)
{
    uintptr_t address;
    size_t slot,done;
    TraceSlot *entry; int saved_errno;
    (void)caller;
    if (!trace_active) return;
    address=(uintptr_t)function; slot=recon_trace_address_hash(address);
    while (slots[slot].address && slots[slot].address!=address) slot=(slot+1)&slot_mask;
    entry=slots+slot;
    if (!entry->address || entry->seen) return;
    entry->seen=1;
    if (entry->shared_seen && __sync_lock_test_and_set(entry->shared_seen,1)) return;
    saved_errno=errno;
    for (done=0;done<entry->length;) {
        ssize_t n=write(trace_fd,entry->line+done,entry->length-done);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) recon_trace_trace_error("cannot write trace: %s",strerror(errno));
        done+=(size_t)n;
    }
    errno=saved_errno;
}
void UNINSTRUMENTED __attribute__((visibility("protected"))) __cyg_profile_func_exit(void *function,void *caller)
{
    (void)function; (void)caller;
}
#endif
