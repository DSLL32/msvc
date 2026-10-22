#define _GNU_SOURCE
/* This file is compiled at the native final link, without instrumentation.
 * Its callbacks consume function pointers and emit first-encounter ELF names.
 * There are no compiler-generated logical IDs and no Win32/original execution. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#include <elf.h>
#include <link.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

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
} TraceSlot;
static TraceName *names;
static size_t name_count;
static TraceSlot *slots;
static size_t slot_mask;
static int trace_active,trace_fd;

static void UNINSTRUMENTED trace_error(const char *format,...)
{
    va_list args;
    va_start(args,format);
    dprintf(STDERR_FILENO,"recon trace: ");
    vdprintf(STDERR_FILENO,format,args);
    dprintf(STDERR_FILENO,"\n");
    va_end(args);
    _exit(126);
}
static int UNINSTRUMENTED compare_names(const void *a,const void *b)
{
    return strcmp(((const TraceName *)a)->name,((const TraceName *)b)->name);
}
static TraceName *UNINSTRUMENTED find_name(const char *name)
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
static void UNINSTRUMENTED read_name_file(const char *path,int prefix)
{
    FILE *file=fopen(path,"rb");
    char *line=NULL; size_t capacity=0; ssize_t length;
    if (!file) trace_error("cannot read %s: %s",path,strerror(errno));
    while ((length=getline(&line,&capacity,file))>=0) {
        size_t n=(size_t)length;
        TraceName *item;
        if (n && line[n-1]=='\n') --n;
        if (n && line[n-1]=='\r') --n;
        if (!n || memchr(line,0,n) || memchr(line,'\t',n) || memchr(line,' ',n))
            trace_error("invalid name in %s",path);
        line[n]=0;
        if (prefix) {
            item=find_name(line);
            if (!item) trace_error("prefix function is outside application filter: %s",line);
            if (item->seen) trace_error("duplicate prefix function: %s",line);
            item->seen=1;
        } else {
            TraceName *grown;
            if (name_count>=100000) trace_error("application filter is too large");
            grown=realloc(names,(name_count+1)*sizeof(*names));
            if (!grown) trace_error("out of memory loading function names");
            names=grown; item=names+name_count++;
            memset(item,0,sizeof(*item));
            item->name=strdup(line);
            item->line=malloc(n+1);
            if (!item->name || !item->line) trace_error("out of memory loading function names");
            memcpy(item->line,line,n); item->line[n]='\n'; item->length=n+1;
        }
    }
    if (ferror(file)) trace_error("failed reading %s",path);
    free(line); fclose(file);
}
static size_t UNINSTRUMENTED address_hash(uintptr_t address)
{
    return ((address>>4)^(address>>13))&slot_mask;
}
static void UNINSTRUMENTED insert_symbol(uintptr_t address,TraceName *name)
{
    size_t slot=address_hash(address);
    if (!address) trace_error("null application function address: %s",name->name);
    while (slots[slot].address) {
        if (slots[slot].address==address)
            trace_error("ambiguous application function address: %s",name->name);
        slot=(slot+1)&slot_mask;
    }
    slots[slot].address=address;
    slots[slot].line=name->line;
    slots[slot].length=name->length;
    slots[slot].seen=name->seen;
}
static int UNINSTRUMENTED main_image_base(struct dl_phdr_info *info,size_t size,void *result)
{
    (void)size;
    if (!info->dlpi_name || !*info->dlpi_name) {
        *(uintptr_t *)result=info->dlpi_addr;
        return 1;
    }
    return 0;
}
static void UNINSTRUMENTED read_at(int fd,void *buffer,size_t count,off_t offset)
{
    size_t done=0;
    while (done<count) {
        ssize_t n=pread(fd,(char *)buffer+done,count-done,offset+done);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) trace_error("cannot read native ELF symbol data");
        done+=(size_t)n;
    }
}
static int UNINSTRUMENTED in_file(uint64_t offset,uint64_t count,off_t size)
{
    return offset<=(uint64_t)size && count<=(uint64_t)size-offset;
}
static void UNINSTRUMENTED load_symbols(void)
{
    Elf32_Ehdr elf;
    Elf32_Shdr *sections;
    Elf32_Sym *symbols;
    char *strings;
    struct stat st;
    uintptr_t base=0;
    int fd=open("/proc/self/exe",O_RDONLY|O_CLOEXEC);
    unsigned i; size_t n;
    if (fd<0 || fstat(fd,&st)<0) trace_error("cannot open native executable");
    read_at(fd,&elf,sizeof(elf),0);
    if (memcmp(elf.e_ident,ELFMAG,SELFMAG) || elf.e_ident[EI_CLASS]!=ELFCLASS32 ||
        elf.e_ident[EI_DATA]!=ELFDATA2LSB || elf.e_machine!=EM_386 ||
        (elf.e_type!=ET_EXEC && elf.e_type!=ET_DYN) ||
        !elf.e_shnum || elf.e_shentsize!=sizeof(*sections) ||
        !in_file(elf.e_shoff,(uint64_t)elf.e_shnum*sizeof(*sections),st.st_size))
        trace_error("unsupported or invalid native ELF symbol layout");
    if (!dl_iterate_phdr(main_image_base,&base)) trace_error("cannot determine native executable load address");
    if (elf.e_type==ET_EXEC && base) trace_error("unexpected ET_EXEC load address");
    sections=malloc((size_t)elf.e_shnum*sizeof(*sections));
    if (!sections) trace_error("out of memory loading ELF sections");
    read_at(fd,sections,(size_t)elf.e_shnum*sizeof(*sections),elf.e_shoff);
    for (i=0;i<elf.e_shnum;++i) if (sections[i].sh_type==SHT_SYMTAB) break;
    if (i==elf.e_shnum) trace_error("native executable has no symbol table; retain its symbols");
    {
        Elf32_Shdr sym=sections[i],str;
        if (sym.sh_link>=elf.e_shnum || sym.sh_entsize!=sizeof(*symbols) ||
            sym.sh_size%sizeof(*symbols)) trace_error("invalid native symbol section");
        str=sections[sym.sh_link];
        if (str.sh_type!=SHT_STRTAB || !in_file(sym.sh_offset,sym.sh_size,st.st_size) ||
            !in_file(str.sh_offset,str.sh_size,st.st_size)) trace_error("invalid native symbol/string ranges");
        symbols=malloc(sym.sh_size?sym.sh_size:1); strings=malloc(str.sh_size?str.sh_size:1);
        if (!symbols || !strings) trace_error("out of memory loading ELF symbols");
        read_at(fd,symbols,sym.sh_size,sym.sh_offset); read_at(fd,strings,str.sh_size,str.sh_offset);
        for (n=0;n<sym.sh_size/sizeof(*symbols);++n) {
            TraceName *name; Elf32_Sym *symbol=symbols+n;
            if (ELF32_ST_TYPE(symbol->st_info)!=STT_FUNC || symbol->st_shndx==SHN_UNDEF) continue;
            if (symbol->st_name>=str.sh_size || !memchr(strings+symbol->st_name,0,str.sh_size-symbol->st_name))
                trace_error("invalid native symbol name");
            name=find_name(strings+symbol->st_name);
            if (!name) continue;
            if (name->resolved) trace_error("ambiguous native function name: %s",name->name);
            name->resolved=1; insert_symbol(base+symbol->st_value,name);
        }
        free(symbols); free(strings);
    }
    free(sections); close(fd);
    for (n=0;n<name_count;++n)
        if (!names[n].resolved) trace_error("application function has no ELF symbol: %s",names[n].name);
}
static void UNINSTRUMENTED __attribute__((constructor(101))) initialize_trace(void)
{
    const char *descriptor=getenv("RECON_TRACE_FD"),*filter,*prefix;
    char *end; long value; size_t count=1,n; int flags,saved_errno=errno;
    if (!descriptor) return; /* Disabled: no name files, symbols or allocation. */
    errno=0; value=strtol(descriptor,&end,10);
    if (!*descriptor || *end || errno || value<0 || value>INT_MAX)
        trace_error("invalid RECON_TRACE_FD");
    trace_fd=(int)value; flags=fcntl(trace_fd,F_GETFL);
    if (flags<0 || (flags&O_ACCMODE)==O_RDONLY) trace_error("RECON_TRACE_FD is not writable");
    filter=getenv("RECON_TRACE_FILTER"); prefix=getenv("RECON_TRACE_SEEN");
    if (!filter || !prefix) trace_error("enabled tracing requires RECON_TRACE_FILTER and RECON_TRACE_SEEN");
    read_name_file(filter,0);
    if (!name_count) trace_error("application filter is empty");
    qsort(names,name_count,sizeof(*names),compare_names);
    for (n=1;n<name_count;++n) if (!strcmp(names[n-1].name,names[n].name))
        trace_error("duplicate application function: %s",names[n].name);
    read_name_file(prefix,1);
    while (count<name_count*2) count*=2;
    slots=calloc(count,sizeof(*slots)); slot_mask=count-1;
    if (!slots) trace_error("out of memory indexing native function pointers");
    load_symbols(); trace_active=1; errno=saved_errno;
}
void UNINSTRUMENTED __cyg_profile_func_enter(void *function,void *caller)
{
    uintptr_t address;
    size_t slot,done;
    TraceSlot *entry; int saved_errno;
    (void)caller;
    if (!trace_active) return;
    address=(uintptr_t)function; slot=address_hash(address);
    while (slots[slot].address && slots[slot].address!=address) slot=(slot+1)&slot_mask;
    entry=slots+slot;
    if (!entry->address || entry->seen) return;
    entry->seen=1;
    saved_errno=errno;
    for (done=0;done<entry->length;) {
        ssize_t n=write(trace_fd,entry->line+done,entry->length-done);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) trace_error("cannot write trace: %s",strerror(errno));
        done+=(size_t)n;
    }
    errno=saved_errno;
}
void UNINSTRUMENTED __cyg_profile_func_exit(void *function,void *caller)
{
    (void)function; (void)caller;
}
#endif
