/* Explicit original-only reference capture; never linked into C1 or fixtures. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../cases/expression-templates/external.h"
#include "../cases/legacy-pdb-object/external.h"
#include <recon.h>

/* Inspect pending bytes without flushing or changing the debugged process.
   The addresses are RVAs of FILE* objects in the old 32-bit CRT ABI. */
static void dump_stream_buffers(HANDLE process, HANDLE log, DWORD base, char *rvas)
{
    static unsigned char bytes[65536];
    static char line[131136];
    static char digits[] = "0123456789abcdef";
    DWORD address, stream, fields[3], got, written, count;
    char *next;
    unsigned index = 0, i, length;
    while (*rvas) {
        address = base + strtoul(rvas, &next, 16);
        if (next == rvas) return;
        length = sprintf(line, "buffer %u ", index++);
        if (!ReadProcessMemory(process, (void *)address, &stream, 4, &got) || got != 4)
            length += sprintf(line + length, "unreadable\n");
        else if (!stream)
            length += sprintf(line + length, "unopened\n");
        else if (!ReadProcessMemory(process, (void *)stream, fields, 12, &got) || got != 12)
            length += sprintf(line + length, "unreadable-file\n");
        else {
            count = fields[0] - fields[2]; /* FILE._ptr - FILE._base */
            if (count > sizeof(bytes))
                length += sprintf(line + length, "invalid\n");
            else if (count && (!ReadProcessMemory(process, (void *)fields[2], bytes, count, &got) || got != count))
                length += sprintf(line + length, "unreadable-bytes\n");
            else {
                length += sprintf(line + length, "%lu ", count);
                for (i = 0; i < count; ++i) {
                    line[length++] = digits[bytes[i] >> 4];
                    line[length++] = digits[bytes[i] & 15];
                }
                line[length++] = '\n';
            }
        }
        WriteFile(log, line, length, &written, 0);
        rvas = *next == ',' ? next + 1 : next;
    }
}

/* Optional explicit raw-memory observation for a reached CRT/state seam.
   The request is debugger input, never child input or fixture behavior. */
static void dump_requested_memory(HANDLE process, HANDLE log, DWORD base)
{
    char *request = getenv("C1_READ_MEMORY"), *next, line[600];
    unsigned char bytes[256];
    static char digits[] = "0123456789abcdef";
    DWORD rva, count, got, written;
    unsigned i, length;
    if (!request) return;
    while (*request) {
        rva = strtoul(request,&next,16);
        if (next == request || *next != ':') return;
        count = strtoul(next+1,&next,16);
        if (!count || count > sizeof(bytes)) return;
        if (!ReadProcessMemory(process,(void *)(base+rva),bytes,count,&got) || got != count) return;
        length = sprintf(line,"memory %08lx %lu ",rva,count);
        for (i=0;i<count;++i) {
            line[length++] = digits[bytes[i] >> 4];
            line[length++] = digits[bytes[i] & 15];
        }
        line[length++] = '\n';
        WriteFile(log,line,length,&written,0);
        request = *next == ',' ? next+1 : next;
    }
}

/* Optional ABI string objects: observe the pointer owner and exact NUL
   terminated bytes without changing target storage or path spelling. */
static void dump_requested_strings(HANDLE process, HANDLE log, DWORD base)
{
    char *request=getenv("C1_READ_STRINGS"), *next, line[1100];
    unsigned char byte;
    static char digits[]="0123456789abcdef";
    DWORD rva, maximum, pointer, got, written, index, length;
    if (!request) return;
    while (*request) {
        rva=strtoul(request,&next,16);
        if (next==request || *next!=':') return;
        maximum=strtoul(next+1,&next,16);
        if (!maximum || maximum>512 ||
            !ReadProcessMemory(process,(void *)(base+rva),&pointer,4,&got) || got!=4) return;
        length=sprintf(line,"string %08lx %08lx ",rva,pointer);
        if (pointer) {
            for(index=0;index<maximum;++index) {
                if (!ReadProcessMemory(process,(void *)(pointer+index),&byte,1,&got) || got!=1) return;
                line[length++]=digits[byte>>4]; line[length++]=digits[byte&15];
                if (!byte) break;
            }
            if (index==maximum) return;
        }
        line[length++]='\n'; WriteFile(log,line,length,&written,0);
        request=*next==','?next+1:next;
    }
}

/* Independent raw offsets from505ba..5061c and4eb15..4eb41. These
   observations are debugger-only and never target reconstruction data. */
static void dump_asm_operand(HANDLE process, HANDLE log, DWORD base, const char *boundary)
{
    DWORD pointer, count, got, written, symbol, name_pointer, index;
    unsigned char operand[16], address[12], declaration[32];
    char line[400], name[128];
    unsigned length;
    if (!getenv("C1_ASM_OPERAND_PROBE")) return;
    if (!ReadProcessMemory(process,(void *)(base+0x4b000),&pointer,4,&got) || got!=4 || !pointer) return;
    if (!ReadProcessMemory(process,(void *)(base+0x4b024),&count,4,&got) || got!=4 ||
        !ReadProcessMemory(process,(void *)pointer,operand,16,&got) || got!=16) return;
    pointer=*(DWORD *)operand;
    if (!pointer || !ReadProcessMemory(process,(void *)pointer,address,12,&got) || got!=12) return;
    symbol=*(DWORD *)address;
    memset(declaration,0,sizeof(declaration)); name[0]=0;
    if (symbol) {
        if (!ReadProcessMemory(process,(void *)symbol,declaration,32,&got) || got!=32) return;
        name_pointer=*(DWORD *)(declaration+8);
        for(index=0;index<sizeof(name)-1;++index) {
            if (!ReadProcessMemory(process,(void *)(name_pointer+index),name+index,1,&got) || got!=1) return;
            if (!name[index]) break;
        }
        name[sizeof(name)-1]=0;
    }
    length=sprintf(line,"asm-state %s %lu %u %u %lu %u %u %u %u %lu %u %lu\n",boundary,count,
        operand[4],operand[5],*(DWORD *)(address+4),address[8],address[9],address[10],address[11],
        symbol!=0,declaration[26],*(DWORD *)(declaration+16));
    WriteFile(log,line,length,&written,0);
    length=sprintf(line,"asm-name %s %s\n",boundary,name);
    WriteFile(log,line,length,&written,0);
}

static void dump_pointer_text(HANDLE process, HANDLE log, const char *label, DWORD pointer)
{
    unsigned char value;
    DWORD got, written, index, length;
    char line[1100];
    static char digits[]="0123456789abcdef";
    length=sprintf(line,"pointer-text %s %08lx ",label,pointer);
    for(index=0;pointer && index<512;++index) {
        if (!ReadProcessMemory(process,(void *)(pointer+index),&value,1,&got) || got!=1) return;
        line[length++]=digits[value>>4]; line[length++]=digits[value&15];
        if (!value) break;
    }
    if (index==512) return;
    line[length++]='\n'; WriteFile(log,line,length,&written,0);
}

/* Raw input descriptor proof for the reached incomplete-type diagnostic.
   Only the fields read by32b90 are interpreted; remaining bytes stay raw. */
static void dump_complete_type(HANDLE process, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD type, tag, got, written;
    unsigned char description[16], declaration[12];
    char line[200];
    if (!getenv("C1_COMPLETE_TYPE_PROBE")) return;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(thread,&context) ||
        !ReadProcessMemory(process,(void *)(context.Esp+4),&type,4,&got) || got!=4 ||
        !ReadProcessMemory(process,(void *)type,description,sizeof(description),&got) || got!=sizeof(description)) return;
    tag=*(DWORD *)(description+8);
    memset(declaration,0,sizeof(declaration));
    if (tag && (!ReadProcessMemory(process,(void *)tag,declaration,sizeof(declaration),&got) || got!=sizeof(declaration))) return;
    sprintf(line,"complete-type %08lx %08lx %u\n",type,tag,declaration[1]);
    WriteFile(log,line,strlen(line),&written,0);
    if (tag) dump_pointer_text(process,log,"complete-tag-name",*(DWORD *)(declaration+8));
}

static void dump_type_pair(HANDLE process, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD stack[4], got, written, descriptor[4], shape[3], pointer;
    unsigned which, depth;
    char line[200], *name;
    if (!getenv("C1_TYPE_PAIR_PROBE")) return;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(thread,&context) ||
        !ReadProcessMemory(process,(void *)context.Esp,stack,sizeof(stack),&got) || got!=sizeof(stack)) return;
    for(which=0;which<2;++which) {
        name=which?"source":"target";
        if (!ReadProcessMemory(process,(void *)stack[which+2],descriptor,sizeof(descriptor),&got) || got!=sizeof(descriptor)) return;
        sprintf(line,"type-pair %s %u %u\n",name,descriptor[0]&65535,descriptor[0]>>16);
        WriteFile(log,line,strlen(line),&written,0);
        pointer=descriptor[1];depth=0;
        while(pointer && depth<8) {
            if (!ReadProcessMemory(process,(void *)pointer,shape,sizeof(shape),&got) || got!=sizeof(shape)) return;
            sprintf(line,"type-shape %s %u %u %u %lu\n",name,depth,shape[0]&65535,shape[0]>>16,shape[2]);
            WriteFile(log,line,strlen(line),&written,0);
            pointer=shape[1];++depth;
        }
    }
}

/* Original355c0 reads these two global pointers and the scope-depth byte.
   The observer follows only the owned next/kind prefix consumed by355e1. */
static void dump_declaration_scopes(HANDLE process, HANDLE log, DWORD base)
{
    DWORD current, directory, selected, got, written, next;
    unsigned char depth, kind;
    unsigned index = 0;
    char line[200];
    if (!getenv("C1_SCOPE_PROBE")) return;
    if (!ReadProcessMemory(process,(void *)(base+0x5b734),&current,4,&got) || got!=4 ||
        !ReadProcessMemory(process,(void *)(base+0x5b738),&directory,4,&got) || got!=4 ||
        !ReadProcessMemory(process,(void *)(base+0x5ce6c),&depth,1,&got) || got!=1 ||
        !ReadProcessMemory(process,(void *)(directory+depth*4),&selected,4,&got) || got!=4) return;
    sprintf(line,"scope-context %08lx %08lx %u %08lx\n",current,directory,depth,selected);
    WriteFile(log,line,strlen(line),&written,0);
    while (current && index<16) {
        if (!ReadProcessMemory(process,(void *)current,&next,4,&got) || got!=4 ||
            !ReadProcessMemory(process,(void *)(current+14),&kind,1,&got) || got!=1) return;
        sprintf(line,"scope-link %u %08lx %08lx %u\n",index,current,next,kind);
        WriteFile(log,line,strlen(line),&written,0);
        if (kind!=1) break;
        current=next;
        ++index;
    }
}

/* The overflow body writes FR5d5b8 and active filename5b588; the
   FILE owner5d5a0 remains live for the ordinary final browser pass. */
static void dump_browser_limit(HANDLE process, HANDLE log, DWORD base, const char *boundary)
{
    DWORD addresses[5], values[5], got, written;
    unsigned index;
    char line[200];
    addresses[0]=0x6b7f4; addresses[1]=0x6b7f8;
    addresses[2]=0x5d5b8; addresses[3]=0x5b588; addresses[4]=0x5d5a0;
    for(index=0;index<5;++index)
        if (!ReadProcessMemory(process,(void *)(base+addresses[index]-0x10000),values+index,4,&got) || got!=4) return;
    sprintf(line,"browser-limit %s %lu %lu %08lx %08lx %08lx\n",boundary,
        values[0],values[1],values[2],values[3],values[4]);
    WriteFile(log,line,strlen(line),&written,0);
    dump_stream_buffers(process,log,base,"4d5a0");
}

/* Raw2a4e0..2a524 writes five distinct DWORD objects, including the
   active parameter-context word which precedes declaration flag bytes. */
static void dump_aborted_function(HANDLE process, HANDLE log, DWORD base, const char *boundary, unsigned ordinal)
{
    DWORD addresses[5], values[5], got, written;
    unsigned index;
    char line[200];
    addresses[0]=0x5ff14; addresses[1]=0x60064; addresses[2]=0x60228;
    addresses[3]=0x60798; addresses[4]=0x5b140;
    for(index=0;index<5;++index)
        if (!ReadProcessMemory(process,(void *)(base+addresses[index]-0x10000),values+index,4,&got) || got!=4) return;
    sprintf(line,"abort-state %u %s %08lx %08lx %08lx %08lx %08lx\n",ordinal,boundary,
        values[0],values[1],values[2],values[3],values[4]);
    WriteFile(log,line,strlen(line),&written,0);
}

/* Keep caller/register evidence for diagnosing a logical-trap divergence. */
static void dump_context(HANDLE process, HANDLE thread, HANDLE log)
{
    CONTEXT context;
    DWORD words[32], got, written;
    char line[256];
    unsigned i;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) return;
    sprintf(line, "registers eax %08lx ecx %08lx edx %08lx ebx %08lx esi %08lx edi %08lx ebp %08lx esp %08lx\n",
            context.Eax, context.Ecx, context.Edx, context.Ebx, context.Esi,
            context.Edi, context.Ebp, context.Esp);
    WriteFile(log, line, strlen(line), &written, 0);
    if (!ReadProcessMemory(process, (void *)context.Esp, words, sizeof(words), &got)) return;
    for (i = 0; i < got / 4; ++i) {
        sprintf(line, "stack %u %08lx\n", i, words[i]);
        WriteFile(log, line, strlen(line), &written, 0);
    }
    if (!ReadProcessMemory(process, (void *)(context.Ebp - sizeof(words)), words, sizeof(words), &got)) return;
    for (i = 0; i < got / 4; ++i) {
        sprintf(line, "frame -%lu %08lx\n", sizeof(words) - i * 4, words[i]);
        WriteFile(log, line, strlen(line), &written, 0);
    }
}

/* Optional debugger-only all-call proof for the enum sort callback. These
   bytes are observations, never reconstruction data or fixture execution. */
static int patch_debug_byte(HANDLE process, DWORD address, unsigned char byte)
{
    DWORD old, restored, written;
    if (!VirtualProtectEx(process,(void *)address,1,PAGE_EXECUTE_READWRITE,&old)) return 0;
    if (!WriteProcessMemory(process,(void *)address,&byte,1,&written) || written != 1 ||
        !FlushInstructionCache(process,(void *)address,1)) return 0;
    return VirtualProtectEx(process,(void *)address,1,old,&restored);
}
static int dump_enum_callback(HANDLE process, HANDLE log, CONTEXT *context, unsigned count)
{
    DWORD stack[3], left, right, a[7], b[7], got, written;
    char line[160];
    if (!ReadProcessMemory(process,(void *)context->Esp,stack,sizeof(stack),&got) || got != sizeof(stack) ||
        !ReadProcessMemory(process,(void *)stack[1],&left,4,&got) || got != 4 ||
        !ReadProcessMemory(process,(void *)stack[2],&right,4,&got) || got != 4 ||
        !ReadProcessMemory(process,(void *)left,a,sizeof(a),&got) || got != sizeof(a) ||
        !ReadProcessMemory(process,(void *)right,b,sizeof(b),&got) || got != sizeof(b)) return 0;
    sprintf(line,"enum-callback %u %08lx %08lx %08lx %08lx\n",count,a[4],a[6],b[4],b[6]);
    return WriteFile(log,line,strlen(line),&written,0);
}

/* Optional full-word flag observations at one nonrecursive application entry
   and its caller return. The three RVAs come from separately audited raw
   instructions. This debugger never writes the observed data words. */
static DWORD flag_entry, flag_return, flag_scanner, flag_parser, flag_symbol;
static unsigned char flag_entry_byte, flag_return_byte;
static unsigned flag_count;
static int flag_active, flag_step;
static PdbObserver pdb_observer;
static HANDLE pdb_process,pdb_log;
static DWORD pdb_list_address,pdb_free_site;
static unsigned char pdb_free_byte;
static int pdb_free_step;
static int pdb_original_read(void *unused,unsigned address,void *output,unsigned bytes)
{
    DWORD got;
    return ReadProcessMemory(pdb_process,(void *)address,output,bytes,&got)&&got==bytes;
}
static void pdb_original_emit(void *unused,const char *line,unsigned bytes)
{
    DWORD written;
    WriteFile(pdb_log,line,bytes,&written,0);
}
static int pdb_free_event(HANDLE process,HANDLE thread,HANDLE log,DWORD code,DWORD site)
{
    CONTEXT c;
    DWORD pointer,got,written;
    char line[100];
    if(!pdb_free_site)return 0;
    if(pdb_free_step && code==EXCEPTION_SINGLE_STEP) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(!GetThreadContext(thread,&c)||!patch_debug_byte(process,pdb_free_site,0xcc))return -1;
        c.EFlags&=~0x100UL;pdb_free_step=0;
        return SetThreadContext(thread,&c)?1:-1;
    }
    if(code!=EXCEPTION_BREAKPOINT||site!=pdb_free_site)return 0;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&c)||!ReadProcessMemory(process,(void *)(c.Esp+4),&pointer,4,&got)||got!=4||!patch_debug_byte(process,site,pdb_free_byte))return -1;
    sprintf(line,"pdb-free %08lx\n",pointer);WriteFile(log,line,strlen(line),&written,0);
    c.Eip=site;c.EFlags|=0x100;pdb_free_step=1;
    return SetThreadContext(thread,&c)?1:-1;
}
static int timestamp_probe;
static DWORD timestamp_handle, timestamp_pointer;
static int dump_function_symbol(HANDLE process, HANDLE log, const char *boundary)
{
    DWORD pointer, fields[7], got, written;
    unsigned count;
    char name[96], line[256];
    if (!flag_symbol) return 1;
    if (!ReadProcessMemory(process,(void *)flag_symbol,&pointer,4,&got) || got != 4) return 0;
    if (!pointer) sprintf(line,"function-symbol %u %s absent\n",flag_count,boundary);
    else {
        if (!ReadProcessMemory(process,(void *)pointer,fields,sizeof(fields),&got) || got != sizeof(fields)) return 0;
        for (count=0; count<sizeof(name); ++count) {
            if (!ReadProcessMemory(process,(void *)(fields[2]+count),name+count,1,&got) || got != 1) return 0;
            if (!name[count]) break;
            if ((unsigned char)name[count] <= ' ') return 0;
        }
        if (count == sizeof(name)) return 0;
        sprintf(line,"function-symbol %u %s %02x %02x %08lx %08lx %s\n",flag_count,boundary,
                ((unsigned char *)fields)[0],((unsigned char *)fields)[1],fields[4],fields[5],name);
    }
    return WriteFile(log,line,strlen(line),&written,0);
}
static int return8_probe, segment_probe, strdup_probe, complete_probe;
static DWORD complete_pointer;

static int dump_flag_words(HANDLE process, HANDLE log, const char *boundary)
{
    DWORD scanner, parser, got, written, base, values[4], locations[4];
    unsigned i;
    char line[128];
    if (!ReadProcessMemory(process,(void *)flag_scanner,&scanner,4,&got) || got != 4 ||
        !ReadProcessMemory(process,(void *)flag_parser,&parser,4,&got) || got != 4) return 0;
    sprintf(line,"flag-words %u %s %08lx %08lx\n",flag_count,boundary,scanner,parser);
    if(!WriteFile(log,line,strlen(line),&written,0)||!dump_function_symbol(process,log,boundary))return 0;
    if(getenv("C1_QC_OPTIONS")) {
        /* Four actual option/normalization words; no target state is changed. */
        base=flag_scanner-0x5bb44;
        locations[0]=base+0x5bc1c;locations[1]=base+0x5bbec;
        locations[2]=base+0x5c044;locations[3]=base+0x4b200;
        for(i=0;i<4;++i)
            if(!ReadProcessMemory(process,(void *)locations[i],values+i,4,&got)||got!=4)return 0;
        sprintf(line,"qc-options %s %08lx %08lx %08lx %08lx\n",boundary,values[0],values[1],values[2],values[3]);
        if(!WriteFile(log,line,strlen(line),&written,0))return 0;
    }
    return 1;
}
static int observe_flag_boundary(HANDLE process, HANDLE thread, HANDLE log,
                                 DWORD code, DWORD site, DWORD base)
{
    CONTEXT context;
    DWORD got, arguments[3], timestamp, written;
    unsigned char instruction;
    char line[128];
    if (!flag_entry) return 0;
    if (flag_step && code == EXCEPTION_SINGLE_STEP) {
        memset(&context,0,sizeof(context)); context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(thread,&context) || !patch_debug_byte(process,flag_entry,0xcc)) return -1;
        context.EFlags &= ~0x100UL;
        if (!SetThreadContext(thread,&context)) return -1;
        flag_step = 0;
        return 1;
    }
    if (code != EXCEPTION_BREAKPOINT || (site != flag_entry && (!flag_active || site != flag_return))) return 0;
    memset(&context,0,sizeof(context)); context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread,&context)) return -1;
    if (site == flag_entry) {
        if(getenv("C1_PDB_CLEANUP")) {
            pdb_process=process;pdb_log=log;
            memset(&pdb_observer,0,sizeof(pdb_observer));
            pdb_observer.read=pdb_original_read;pdb_observer.emit=pdb_original_emit;
            pdb_list_address=context.Ecx;
            pdb_memory(&pdb_observer,"object-before",context.Ecx-72,116);
            pdb_list(&pdb_observer,"before",context.Ecx,1);
            sprintf(line,"pdb-registers entry %08lx %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Ecx,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
            pdb_free_site=base+0x40c00;
            if(pdb_observer.failed||!ReadProcessMemory(process,(void *)pdb_free_site,&pdb_free_byte,1,&got)||got!=1||pdb_free_byte==0xcc||!patch_debug_byte(process,pdb_free_site,0xcc))return -1;
        }
        if (getenv("C1_ABORT_PROBE")) {
            dump_aborted_function(process,log,base,"entry",flag_count);
            sprintf(line,"abort-registers %u entry %08lx %08lx %08lx %08lx %08lx\n",flag_count,context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (getenv("C1_BROWSER_LIMIT_PROBE")) {
            dump_browser_limit(process,log,base,"entry");
            sprintf(line,"browser-registers entry %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (getenv("C1_SCOPE_PROBE")) {
            dump_declaration_scopes(process,log,base);
            sprintf(line,"scope-registers entry %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (segment_probe) dump_asm_operand(process,log,base,"entry");
        if (return8_probe || segment_probe || strdup_probe || complete_probe) {
            if (!ReadProcessMemory(process,(void *)context.Esp,arguments,sizeof(arguments),&got) || got != sizeof(arguments)) return -1;
            sprintf(line,"%s-entry %u %08lx %08lx\n",complete_probe?"type":strdup_probe?"strdup":segment_probe?"segment":"return8",flag_count,context.Esp,arguments[1]);
            if (!WriteFile(log,line,strlen(line),&written,0)) return -1;
            if (strdup_probe) dump_pointer_text(process,log,"entry",arguments[1]);
            if (complete_probe) {
                complete_pointer=arguments[1];
                if (!ReadProcessMemory(process,(void *)(complete_pointer+8),&arguments[2],4,&got) || got!=4 ||
                    !ReadProcessMemory(process,(void *)(arguments[2]+1),&instruction,1,&got) || got!=1) return -1;
                sprintf(line,"tag-byte entry %08lx %u\n",arguments[2],instruction);
                WriteFile(log,line,strlen(line),&written,0);
            }
        }
        if (timestamp_probe) {
            if (!ReadProcessMemory(process,(void *)context.Esp,arguments,sizeof(arguments),&got) || got != sizeof(arguments)) return -1;
            timestamp_handle = arguments[1]; timestamp_pointer = arguments[2];
        }
        if (flag_active || !ReadProcessMemory(process,(void *)context.Esp,&flag_return,4,&got) || got != 4 ||
            flag_return == flag_entry ||
            !ReadProcessMemory(process,(void *)flag_return,&flag_return_byte,1,&got) || got != 1 ||
            flag_return_byte == 0xcc || !dump_flag_words(process,log,"entry") ||
            !patch_debug_byte(process,flag_return,0xcc) ||
            !patch_debug_byte(process,flag_entry,flag_entry_byte)) return -1;
        flag_active = 1;
        flag_step = 1;
        context.EFlags |= 0x100;
    } else {
        if(getenv("C1_PDB_CLEANUP")) {
            pdb_memory(&pdb_observer,"object-after",pdb_list_address-72,116);
            pdb_list(&pdb_observer,"after",pdb_list_address,0);
            sprintf(line,"pdb-registers return %08lx %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Eax,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
            if(pdb_observer.failed||!patch_debug_byte(process,pdb_free_site,pdb_free_byte))return -1;
            pdb_free_site=0;
        }
        if (getenv("C1_ABORT_PROBE")) {
            dump_aborted_function(process,log,base,"finish",flag_count);
            sprintf(line,"abort-registers %u finish %08lx %08lx %08lx %08lx %08lx\n",flag_count,context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (getenv("C1_BROWSER_LIMIT_PROBE")) {
            dump_browser_limit(process,log,base,"finish");
            sprintf(line,"browser-registers finish %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (getenv("C1_SCOPE_PROBE")) {
            sprintf(line,"scope-registers finish %08lx %08lx %08lx %08lx %08lx %08lx\n",context.Esp,context.Ebx,context.Esi,context.Edi,context.Ebp,context.Eax);
            WriteFile(log,line,strlen(line),&written,0);
        }
        if (segment_probe) dump_asm_operand(process,log,base,"finish");
        if (return8_probe || segment_probe || strdup_probe || complete_probe) {
            sprintf(line,"%s-finish %u %08lx %08lx %08lx\n",complete_probe?"type":strdup_probe?"strdup":segment_probe?"segment":"return8",flag_count,context.Esp,context.Eax,context.Edx);
            if (!WriteFile(log,line,strlen(line),&written,0)) return -1;
            if (strdup_probe) dump_pointer_text(process,log,"finish",context.Eax);
            if (complete_probe) {
                if (!ReadProcessMemory(process,(void *)(complete_pointer+8),&arguments[2],4,&got) || got!=4 ||
                    !ReadProcessMemory(process,(void *)(arguments[2]+1),&instruction,1,&got) || got!=1) return -1;
                sprintf(line,"tag-byte finish %08lx %u\n",arguments[2],instruction);
                WriteFile(log,line,strlen(line),&written,0);
            }
        }
        if (timestamp_probe) {
            if (!ReadProcessMemory(process,(void *)timestamp_pointer,&timestamp,4,&got) || got != 4) return -1;
            sprintf(line,"source-timestamp %u %lu %lu %lu\n",flag_count,timestamp_handle,context.Eax,timestamp);
            if (!WriteFile(log,line,strlen(line),&written,0)) return -1;
        }
        if (!dump_flag_words(process,log,"finish") ||
            !patch_debug_byte(process,flag_return,flag_return_byte)) return -1;
        flag_active = 0;
        ++flag_count;
    }
    context.Eip = site;
    return SetThreadContext(thread,&context) ? 1 : -1;
}

/* Explicit debugger-only calls for support routines unreachable from source.
   The initialized child is stopped at a real accepted marker. Only a private
   stack and CPU call context are prepared; target globals remain untouched. */
typedef struct TemplateProcess { HANDLE process, log; } TemplateProcess;
static TemplateProcess template_process;
static TemplateObserver template_observer;
static int template_unit = -1;
static unsigned template_input_address, template_slot_address;
static int template_process_read(void *context, unsigned address, void *output, unsigned bytes)
{
    TemplateProcess *state=context;
    DWORD got;
    return ReadProcessMemory(state->process,(void *)address,output,bytes,&got) && got==bytes;
}
static void template_process_emit(void *context,const char *bytes,unsigned length)
{
    TemplateProcess *state=context;
    DWORD written;
    WriteFile(state->log,bytes,length,&written,0);
}
static unsigned parsed_global_symbol(HANDLE process,unsigned base,const char *wanted)
{
    DWORD scope,buckets,symbol,fields[4],got;
    unsigned short mask;
    unsigned bucket,index,step;
    char name[64],byte;
    if(!ReadProcessMemory(process,(void *)(base+0x5b730),&scope,4,&got) || got!=4 ||
       !ReadProcessMemory(process,(void *)(scope+8),&buckets,4,&got) || got!=4 ||
       !ReadProcessMemory(process,(void *)(scope+12),&mask,2,&got) || got!=2 || mask>1023)return 0;
    for(bucket=0;bucket<=mask;++bucket) {
        if(!ReadProcessMemory(process,(void *)(buckets+bucket*4),&symbol,4,&got) || got!=4)return 0;
        for(step=0;symbol && step<1024;++step) {
            if(!ReadProcessMemory(process,(void *)symbol,fields,sizeof(fields),&got) || got!=sizeof(fields))return 0;
            for(index=0;index<sizeof(name)-1;++index) {
                if(!ReadProcessMemory(process,(void *)(fields[2]+index),&byte,1,&got) || got!=1)return 0;
                name[index]=byte;if(!byte)break;
            }
            name[sizeof(name)-1]=0;
            if(!strcmp(name,wanted))return symbol;
            symbol=fields[1];
        }
    }
    return 0;
}
static unsigned template_global_type(HANDLE process,unsigned base)
{
    DWORD symbol=parsed_global_symbol(process,base,"template_text"),type,got;
    if(!symbol || !ReadProcessMemory(process,(void *)(symbol+12),&type,4,&got) || got!=4)return 0;
    return type;
}
static void template_observe(unsigned base,const char *boundary)
{
    char label[100];
    sprintf(label,"input-%s",boundary);
    template_memory(&template_observer,label,template_input_address,sizeof(TemplateInput));
    sprintf(label,"pool-%s",boundary);
    template_memory(&template_observer,label,base+0x5e1f8,sizeof(ArenaPool));
    sprintf(label,"slot-%s",boundary);
    if(template_slot_address)template_memory(&template_observer,label,template_slot_address,80);
    else template_hex(&template_observer,label,0,0,0);
    sprintf(label,"arena-%s",boundary);
    template_memory(&template_observer,label,base+0x5e1c8,8);
    {
        DWORD free_block,got;
        if(!ReadProcessMemory(template_process.process,(void *)(base+0x5e1cc),&free_block,4,&got) || got!=4)template_observer.failed=1;
        else if(free_block) {
            sprintf(label,"free-%s",boundary);
            template_memory(&template_observer,label,free_block,8);
        }
    }
    template_owners(&template_observer,boundary);
}
/* Minimal direct-support observations; all owners come from real declarations. */
static int support_unit=-1;
static DWORD support_symbol;
static int observe_support(HANDLE process,HANDLE log,DWORD base,const char *boundary)
{
    DWORD fields[7],type[2],shape=0,global,debug,errors,got,written;
    char line[512];
    if(!support_symbol ||
       !ReadProcessMemory(process,(void *)support_symbol,fields,sizeof(fields),&got)||got!=sizeof(fields)||
       !ReadProcessMemory(process,(void *)fields[3],type,sizeof(type),&got)||got!=sizeof(type)||
       !ReadProcessMemory(process,(void *)(base+0x4ff10),&global,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(base+0x5bb98),&debug,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(base+0x4b228),&errors,4,&got)||got!=4)return 0;
    if(type[1]&&(!ReadProcessMemory(process,(void *)type[1],&shape,4,&got)||got!=4))return 0;
    sprintf(line,"support-state %s %u %08lx %08lx %08lx %08lx %u %08lx %u %04x %04x %04x %04x %08lx %lu %lu\n",
        boundary,support_unit,support_symbol,fields[2],fields[3],type[1],
        fields[0]&255,fields[5],fields[6]&65535,type[0]&65535,type[0]>>16,
        shape&65535,shape>>16,global,debug,errors);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int observe_support_result(HANDLE process,HANDLE log,DWORD result)
{
    DWORD type, symbol, first[2], second[2], third[2], got, written;
    unsigned short head;
    char line[512];
    if(support_unit==4) {
        if(!result || !ReadProcessMemory(process,(void *)result,&head,2,&got)||got!=2||
           !ReadProcessMemory(process,(void *)(result+4),&type,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(result+12),&symbol,4,&got)||got!=4)return 0;
        sprintf(line,"support-result leaf %08lx %u %u %08lx %08lx\n",result,head&255,head>>8,type,symbol);
    } else if(support_unit==5||support_unit==6) {
        if(!result||!ReadProcessMemory(process,(void *)result,first,8,&got)||got!=8)return 0;
        second[0]=second[1]=third[0]=third[1]=0;
        if(first[1]&&(!ReadProcessMemory(process,(void *)first[1],second,8,&got)||got!=8))return 0;
        if(second[1]&&(!ReadProcessMemory(process,(void *)second[1],third,8,&got)||got!=8))return 0;
        sprintf(line,"support-result type %08lx %04x %04x %08lx %04x %04x %08lx %04x %04x %08lx\n",result,
            first[0]&65535,first[0]>>16,first[1],second[0]&65535,second[0]>>16,second[1],third[0]&65535,third[0]>>16,third[1]);
    } else return 1;
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
/* Observe only defined constructor/result fields for v3's real owners. */
static int observe_support_v3(HANDLE process,HANDLE log,DWORD base,const char *boundary)
{
    DWORD next,association=0,got,written,name,type,index,attributes,found;
    unsigned short head,references;
    unsigned char storage[3];
    char line[512],text[256];
    unsigned i;
    if(support_unit==12) {
        DWORD option,interned;
        if(!ReadProcessMemory(process,(void *)(base+0x5bbe8),&option,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(base+0x5b66c),&interned,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(base+0x5b7f4),&next,4,&got)||got!=4)return 0;
        sprintf(line,"support-intrinsic %s Oi %lu interned %08lx next %lu\n",boundary,option,interned,next);
        return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
    }
    if(support_unit==11) {
        DWORD buffer,first,last,fields[4];
        if(!ReadProcessMemory(process,(void *)(base+0x4b24c),&buffer,4,&got)||got!=4||!buffer||
           !ReadProcessMemory(process,(void *)(base+0x5e070),&first,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(base+0x5e074),&last,4,&got)||got!=4)return 0;
        sprintf(line,"support-fixup %s buffer %08lx allocation4096 head %08lx tail %08lx\n",boundary,buffer,first,last);
        WriteFile(log,line,strlen(line),&written,0);
        if(strcmp(boundary,"after"))return 1;
        if(!first||first!=last||!ReadProcessMemory(process,(void *)first,fields,16,&got)||got!=16)return 0;
        sprintf(line,"support-fixup-result symbol %08lx offset %lu bytes %lu next %08lx\n",fields[0],fields[1],fields[2],fields[3]);
        return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
    }
    if(support_unit!=10)return 1;
    if(!ReadProcessMemory(process,(void *)(base+0x5b7f4),&next,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(support_symbol+40),&association,4,&got)||got!=4)return 0;
    sprintf(line,"support-segment %s next %lu association %08lx\n",boundary,next,association);
    WriteFile(log,line,strlen(line),&written,0);
    if(strcmp(boundary,"after"))return 1;
    if(!association||!ReadProcessMemory(process,(void *)association,&head,2,&got)||got!=2||
       !ReadProcessMemory(process,(void *)(association+8),&name,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(association+12),&type,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(association+16),&index,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(association+20),&attributes,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(association+24),&references,2,&got)||got!=2||
       !ReadProcessMemory(process,(void *)(association+26),storage,3,&got)||got!=3||!name)return 0;
    for(i=0;i<sizeof(text);++i) {
        if(!ReadProcessMemory(process,(void *)(name+i),text+i,1,&got)||got!=1)return 0;
        if(!text[i])break;
    }
    if(i==sizeof(text))return 0;
    found=parsed_global_symbol(process,base,text);
    if(found!=association)return 0;
    sprintf(line,"support-segment-result kind %u flags %u name %s type %08lx index %lu attributes %08lx references %u storage %02x%02x%02x global-owner %08lx\n",
        head&255,head>>8,text,type,index,attributes,references,storage[0],storage[1],storage[2],found);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static DWORD direct_marker_rva, direct_target_rva, direct_arguments[3];
static unsigned direct_count;
static int direct_active;
static DWORD direct_stack, direct_marker;
static CONTEXT direct_saved;

#include "capture-container-unit.h"

/* Direct literal support uses the genuine stopped initializer arguments.
   Only debugger breakpoints and an owned call stack are changed. */
static DWORD literal_return, literal_storage, literal_token, literal_writer;
static unsigned char literal_return_byte, literal_writer_byte;
static int literal_return_patched, literal_marker_open, literal_writer_patched;
static int literal_reentered, literal_written, literal_stepping;
static CONTEXT literal_step_context;
static int literal_cleanup(HANDLE process,HANDLE log)
{
    DWORD written,got;
    unsigned char byte;
    char line[160];
    int ok=1;
    if(literal_return_patched) {
        if(!patch_debug_byte(process,literal_return,literal_return_byte))ok=0;
        else literal_return_patched=0;
    }
    if(literal_marker_open) {
        if(!patch_debug_byte(process,direct_marker,0xcc))ok=0;
        else literal_marker_open=0;
    }
    if(literal_writer_patched) {
        if(!patch_debug_byte(process,literal_writer,literal_writer_byte))ok=0;
        else literal_writer_patched=0;
    }
    if(literal_return && (!ReadProcessMemory(process,(void *)literal_return,&byte,1,&got)||got!=1||byte!=literal_return_byte))ok=0;
    if(direct_marker && (!ReadProcessMemory(process,(void *)direct_marker,&byte,1,&got)||got!=1||byte!=0xcc))ok=0;
    sprintf(line,"literal-cleanup %u return %08lx byte %02x marker %08lx byte cc\n",
        ok,literal_return,literal_return_byte,direct_marker);
    WriteFile(log,line,strlen(line),&written,0);
    return ok;
}
static int literal_observe(HANDLE process,HANDLE log,DWORD base,
                           const char *boundary,CONTEXT *current)
{
    DWORD token_type,extended,line_stream,auxiliary,index,got,written;
    unsigned short head,type_flags;
    unsigned char value[10];
    char line[512],*out;
    unsigned i;
    if(!ReadProcessMemory(process,(void *)literal_token,&head,2,&got)||got!=2||
       !ReadProcessMemory(process,(void *)(literal_token+4),&token_type,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)token_type,&type_flags,2,&got)||got!=2||
       !ReadProcessMemory(process,(void *)(literal_token+12),&extended,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)extended,value,10,&got)||got!=10||
       !ReadProcessMemory(process,(void *)(base+0x4b594),&line_stream,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(base+0x5e434),&auxiliary,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(support_symbol+16),&index,4,&got)||got!=4||
       !line_stream || ((head>>8)&7)!=3 || !(type_flags&0x100))return 0;
    sprintf(line,"literal-state %s symbol %08lx token %08lx head %04x type %08lx flags %04x extended %08lx line %08lx auxiliary %08lx index %lu value ",
        boundary,support_symbol,literal_token,head,token_type,type_flags,extended,line_stream,auxiliary,index);
    out=line+strlen(line);for(i=0;i<10;++i){sprintf(out,"%02x",value[i]);out+=2;}strcpy(out,"\n");
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int literal_event(HANDLE process,HANDLE thread,HANDLE log,
                         DWORD code,DWORD site,DWORD base,char *buffers)
{
    CONTEXT current;
    DWORD words[8],got,written,line_stream,auxiliary,pointer,displacement;
    unsigned char call[5],value[8];
    char line[512],name[32],*out;
    unsigned i,width=support_unit==8?4:8;
    typedef LPVOID (WINAPI *AllocateInProcess)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
    AllocateInProcess allocate;
    if(literal_stepping) {
        memset(&current,0,sizeof(current));current.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
        if(code!=EXCEPTION_SINGLE_STEP||site!=literal_return+2||
           !GetThreadContext(thread,&current)||current.Eip!=literal_return+2||
           current.Esp!=literal_step_context.Esp||current.Eax!=literal_step_context.Eax||
           current.Ecx!=literal_step_context.Ecx||current.Edx!=literal_step_context.Edx||
           current.Ebx!=literal_step_context.Ebx||current.Esi!=literal_step_context.Esi||
           current.Edi!=literal_step_context.Edi||current.Ebp!=literal_step_context.Ebp)return -1;
        if(!patch_debug_byte(process,literal_return,0xcc))return -1;
        literal_return_patched=1;literal_stepping=0;current.EFlags&=~0x100UL;
        sprintf(line,"literal-internal-step pc %08lx esp %08lx restored-trap cc\n",current.Eip,current.Esp);
        WriteFile(log,line,strlen(line),&written,0);
        return SetThreadContext(thread,&current)?1:-1;
    }
    if(code!=EXCEPTION_BREAKPOINT)return 0;
    if(site!=base+direct_marker_rva && (!direct_active || (site!=literal_return&&site!=literal_writer)))return 0;
    memset(&current,0,sizeof(current));current.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if(!GetThreadContext(thread,&current))return -1;
    if(!direct_active) {
        if(!ReadProcessMemory(process,(void *)current.Esp,words,16,&got)||got!=16||words[3])return -1;
        direct_saved=current;direct_marker=site;literal_return=words[0];support_symbol=words[1];literal_token=words[2];
        if(literal_return<base+0x1000||literal_return>=base+0x44af2||
           !ReadProcessMemory(process,(void *)(literal_return-5),call,5,&got)||got!=5||call[0]!=0xe8)return -1;
        memcpy(&displacement,call+1,4);
        if(literal_return+displacement!=site ||
           !ReadProcessMemory(process,(void *)literal_return,&literal_return_byte,1,&got)||got!=1||literal_return_byte==0xcc||
           !ReadProcessMemory(process,(void *)(support_symbol+8),&pointer,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)pointer,name,sizeof(name),&got)||got!=sizeof(name)||
           strcmp(name,support_unit==8?"support_single":"support_double"))return -1;
        literal_writer=base+(support_unit==8?0x3b960:0x3b980);
        if(!ReadProcessMemory(process,(void *)literal_writer,&literal_writer_byte,1,&got)||got!=1)return -1;
        allocate=(AllocateInProcess)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
        if(!allocate)return -1;
        literal_storage=(DWORD)allocate(process,0,65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!literal_storage)return -1;
        direct_stack=literal_storage+65536-512;
        if(!observe_support(process,log,base,"before")||!literal_observe(process,log,base,"before",&current))return -1;
        for(i=0;i<8;++i)words[i]=0xa5a5a5a5;
        words[0]=literal_return;words[1]=support_symbol;words[2]=literal_token;
        if(!WriteProcessMemory(process,(void *)direct_stack,words,sizeof(words),&written)||written!=sizeof(words)||
           !patch_debug_byte(process,literal_return,0xcc))return -1;
        literal_return_patched=1;
        if(literal_writer_byte!=0xcc) {
            if(!patch_debug_byte(process,literal_writer,0xcc))return -1;
            literal_writer_patched=1;
        }
        sprintf(line,"literal-start parent %08lx marker %08lx return %08lx byte %02x storage %08lx stack %08lx symbol %08lx token %08lx\n",
            base+direct_target_rva,site,literal_return,literal_return_byte,literal_storage,direct_stack,support_symbol,literal_token);
        WriteFile(log,line,strlen(line),&written,0);
        if(buffers)dump_stream_buffers(process,log,base,buffers);
        current.Eip=base+direct_target_rva;current.Esp=direct_stack;direct_active=1;
        if(!SetThreadContext(thread,&current))return -1;
        return 1;
    }
    if(site==direct_marker) {
        if(literal_reentered++||current.Esp<literal_storage||current.Esp+16>direct_stack||
           !ReadProcessMemory(process,(void *)current.Esp,words,16,&got)||got!=16||
           words[1]!=support_symbol||words[2]!=literal_token||
           !ReadProcessMemory(process,(void *)(base+0x4b594),&line_stream,4,&got)||got!=4||!line_stream||words[3]!=line_stream)return -1;
        sprintf(line,"literal-reentry marker %08lx esp %08lx symbol %08lx token %08lx stream %08lx\n",site,current.Esp,words[1],words[2],words[3]);
        WriteFile(log,line,strlen(line),&written,0);
        if(!patch_debug_byte(process,direct_marker,0x83))return -1;
        literal_marker_open=1;current.Eip=site;
        return SetThreadContext(thread,&current)?1:-1;
    }
    if(site==literal_writer) {
        if(literal_written++||literal_reentered!=1||current.Esp<literal_storage||current.Esp+8>direct_stack||
           !ReadProcessMemory(process,(void *)(current.Esp+4),&pointer,4,&got)||got!=4||
           pointer<literal_storage||pointer+width>direct_stack||
           !ReadProcessMemory(process,(void *)pointer,value,width,&got)||got!=width||
           !ReadProcessMemory(process,(void *)(base+0x4b594),&line_stream,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(base+0x5e434),&auxiliary,4,&got)||got!=4||!line_stream||auxiliary!=line_stream)return -1;
        sprintf(line,"literal-writer entry %08lx esp %08lx value %08lx width %u stream %08lx bytes ",site,current.Esp,pointer,width,auxiliary);
        out=line+strlen(line);for(i=0;i<width;++i){sprintf(out,"%02x",value[i]);out+=2;}strcpy(out,"\n");WriteFile(log,line,strlen(line),&written,0);
        if(literal_writer_byte==0xcc)return 0; /* Ordinary frontier/entry-plan handler owns this trap. */
        if(!patch_debug_byte(process,literal_writer,literal_writer_byte))return -1;
        literal_writer_patched=0;current.Eip=site;
        return SetThreadContext(thread,&current)?1:-1;
    }
    if(current.Esp!=direct_stack+12) {
        DWORD format;
        unsigned char instruction[2];
        if(literal_return!=base+0x2b2d5||literal_return_byte!=0x38||
           current.Esp!=direct_stack-48||current.Esi!=support_symbol||
           !ReadProcessMemory(process,(void *)(base+0x4b594),&line_stream,4,&got)||got!=4||current.Ebp!=line_stream||
           !ReadProcessMemory(process,(void *)(base+0x5ed30),&format,4,&got)||got!=4||
           current.Edi<format+1||current.Edi>format+4||
           !ReadProcessMemory(process,(void *)literal_return,instruction,2,&got)||got!=2||
           instruction[0]!=0xcc||instruction[1]!=0x1f)return -1;
        sprintf(line,"literal-internal-hit pc %08lx esp %08lx symbol %08lx stream %08lx format-offset %lu instruction 381f\n",
            site,current.Esp,current.Esi,current.Ebp,current.Edi-format);
        WriteFile(log,line,strlen(line),&written,0);
        if(!patch_debug_byte(process,literal_return,literal_return_byte))return -1;
        literal_return_patched=0;literal_stepping=1;literal_step_context=current;
        current.Eip=literal_return;current.EFlags|=0x100UL;
        return SetThreadContext(thread,&current)?1:-1;
    }
    if(literal_reentered!=1||literal_written!=1||current.Esp!=direct_stack+12||
       current.Ebx!=direct_saved.Ebx||current.Esi!=direct_saved.Esi||current.Edi!=direct_saved.Edi||current.Ebp!=direct_saved.Ebp||
       current.FloatSave.ControlWord!=direct_saved.FloatSave.ControlWord||
       !ReadProcessMemory(process,(void *)(direct_stack+12),words,16,&got)||got!=16)return -1;
    for(i=0;i<4;++i)if(words[i]!=0xa5a5a5a5)return -1;
    sprintf(line,"literal-return esp %08lx expected %08lx nonvolatile 1 fctrl %08lx before %08lx canary a5a5a5a5 a5a5a5a5 a5a5a5a5 a5a5a5a5\n",current.Esp,direct_stack+12,current.FloatSave.ControlWord,direct_saved.FloatSave.ControlWord);
    WriteFile(log,line,strlen(line),&written,0);
    if(!observe_support(process,log,base,"after")||!literal_observe(process,log,base,"after",&current))return -1;
    if(buffers)dump_stream_buffers(process,log,base,buffers);
    if(!literal_cleanup(process,log))return -1;
    TerminateProcess(process,99);return 1;
}

static int direct_event(HANDLE process, HANDLE thread, HANDLE log,
                        DWORD code, DWORD site, DWORD base, char *buffers)
{
    CONTEXT current;
    DWORD words[8], got, written;
    unsigned char *storage;
    typedef LPVOID (WINAPI *AllocateInProcess)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
    AllocateInProcess allocate;
    char line[512];
    unsigned index;
    if(support_unit==8||support_unit==9)return literal_event(process,thread,log,code,site,base,buffers);
    if (!direct_marker_rva || code != EXCEPTION_BREAKPOINT ||
        site != base + direct_marker_rva) return 0;
    memset(&current,0,sizeof(current));
    current.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread,&current)) return -1;
    if (!direct_active) {
        direct_saved = current;
        direct_marker = site;
        allocate = (AllocateInProcess)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
        if (!allocate) return -1;
        storage = allocate(process,0,65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if (!storage) return -1;
        direct_stack = (DWORD)storage + 65536 - 512;
        if(template_unit>=0) {
            TemplateInput input;
            ArenaPool pool;
            template_process.process=process;template_process.log=log;
            memset(&template_observer,0,sizeof(template_observer));
            template_observer.context=&template_process;
            template_observer.read=template_process_read;template_observer.emit=template_process_emit;
            if(!ReadProcessMemory(process,(void *)(base+0x5b6dc),template_observer.owners,4,&got) || got!=4 ||
               !ReadProcessMemory(process,(void *)(base+0x5b6d8),template_observer.owners+1,4,&got) || got!=4 ||
               !ReadProcessMemory(process,(void *)(base+0x5e1f8),&pool,sizeof(pool),&got) || got!=sizeof(pool))return -1;
            template_observer.owners[2]=template_global_type(process,base);
            sprintf(line,"expr-context %08x %08x %08x pool %08x remaining %d\n",template_observer.owners[0],template_observer.owners[1],template_observer.owners[2],(unsigned)pool.next,pool.remaining);
            WriteFile(log,line,strlen(line),&written,0);
            if(!template_observer.owners[0] || !template_observer.owners[1] || !template_observer.owners[2] || (pool.remaining<80 && (pool.remaining || pool.next || pool.first || pool.last)))return -1;
            template_input_address=(unsigned)storage+4096;
            template_slot_address=(unsigned)pool.next;
            if(!template_slot_address) {
                DWORD free_block;
                if(!ReadProcessMemory(process,(void *)(base+0x5e1cc),&free_block,4,&got) || got!=4 || !free_block)return -1;
                template_slot_address=free_block+offsetof(ArenaBlock,data);
            }
            template_build_input(template_unit,template_input_address,template_observer.owners,&input);
            if(!WriteProcessMemory(process,(void *)template_input_address,&input,sizeof(input),&written) || written!=sizeof(input))return -1;
            if(template_unit)direct_arguments[0]=template_input_address;
            sprintf(line,"expr-unit %u mode %u input %08x\n",template_unit,template_unit?template_mode(template_unit):0,template_input_address);
            WriteFile(log,line,strlen(line),&written,0);
            template_observe(base,"before");if(template_observer.failed)return -1;
        }
        if(support_unit>=0) {
            DWORD name;
            DWORD type,depth;
            const char *owner=(support_unit==10||support_unit==11)?"support_segment_value":support_unit==12?"memcpy":support_unit==5?"support_callback":
                (support_unit==1||support_unit==3||support_unit==4||support_unit==6)?"object_base":"base_pointer";
            support_symbol=parsed_global_symbol(process,base,owner);
            if(support_unit==10||support_unit==11) {
                DWORD shape,head,segment;
                if(!support_symbol||!ReadProcessMemory(process,(void *)(support_symbol+12),&type,4,&got)||got!=4||
                   !ReadProcessMemory(process,(void *)(type+4),&shape,4,&got)||got!=4||!shape||
                   !ReadProcessMemory(process,(void *)shape,&head,4,&got)||got!=4||
                   (head&65535)!=0x400||((head>>16)&7)!=5||
                   !ReadProcessMemory(process,(void *)(shape+8),&segment,4,&got)||got!=4||!segment)return -1;
                support_symbol=segment;
            }
            if(!support_symbol || !ReadProcessMemory(process,(void *)(support_symbol+8),&name,4,&got)||got!=4 ||
               !ReadProcessMemory(process,(void *)(support_symbol+12),&type,4,&got)||got!=4 ||
               !observe_support(process,log,base,"before")||!observe_support_v3(process,log,base,"before"))return -1;
            if(support_unit<2)direct_arguments[1]=name;
            else if(support_unit==3)direct_arguments[0]=support_symbol;
            else if(support_unit==4)direct_arguments[1]=support_symbol;
            else if(support_unit==5||support_unit==6)direct_arguments[0]=type;
            else if(support_unit==7){direct_arguments[0]=name;direct_arguments[1]=name+5;}
            if(support_unit==10||support_unit==11)direct_arguments[0]=support_symbol;
            if(support_unit==11) {
                DWORD buffer;
                if(!ReadProcessMemory(process,(void *)(base+0x4b24c),&buffer,4,&got)||got!=4||!buffer||direct_arguments[2]!=4)return -1;
                direct_arguments[1]=buffer+8;
                sprintf(line,"support-debug-buffer %08lx allocation4096 position %08lx bytes4\n",buffer,direct_arguments[1]);WriteFile(log,line,strlen(line),&written,0);
            }
            if(support_unit>=3) {
                if(!ReadProcessMemory(process,(void *)(base+0x4ff18),&depth,4,&got)||got!=4)return -1;
                sprintf(line,"support-type-depth before %lu\n",depth);WriteFile(log,line,strlen(line),&written,0);
            }
        }
        if(container_unit>=0&&!container_prepare(process,log,base))return -1;
        for (index=0;index<8;++index) words[index]=0xa5a5a5a5;
        words[0]=site;
        for (index=0;index<direct_count;++index) words[index+1]=direct_arguments[index];
        if (!WriteProcessMemory(process,(void *)direct_stack,words,sizeof(words),&written) || written!=sizeof(words)) return -1;
        sprintf(line,"direct-start target %08lx marker %08lx stack %08lx count %u args %08lx %08lx\n",
            base+direct_target_rva,site,direct_stack,direct_count,direct_arguments[0],direct_arguments[1]);
        WriteFile(log,line,strlen(line),&written,0);
        if(direct_count==3){sprintf(line,"direct-third %08lx\n",direct_arguments[2]);WriteFile(log,line,strlen(line),&written,0);}
        dump_context(process,thread,log);
        dump_requested_memory(process,log,base);
        dump_asm_operand(process,log,base,"direct-entry");
        if (buffers) dump_stream_buffers(process,log,base,buffers);
        current.Eip=base+direct_target_rva;
        current.Esp=direct_stack;
        if (!SetThreadContext(thread,&current)) return -1;
        direct_active=1;
        return 1;
    }
    sprintf(line,"direct-return eax %08lx edx %08lx esp %08lx expected %08lx nonvolatile %u fctrl %08lx before %08lx\n",
        current.Eax,current.Edx,current.Esp,direct_stack+4+4*direct_count,
        current.Ebx==direct_saved.Ebx && current.Esi==direct_saved.Esi &&
        current.Edi==direct_saved.Edi && current.Ebp==direct_saved.Ebp,
        current.FloatSave.ControlWord,direct_saved.FloatSave.ControlWord);
    WriteFile(log,line,strlen(line),&written,0);
    if (!ReadProcessMemory(process,(void *)(direct_stack+4+4*direct_count),words,16,&got) || got!=16) return -1;
    sprintf(line,"direct-canary %08lx %08lx %08lx %08lx\n",words[0],words[1],words[2],words[3]);
    WriteFile(log,line,strlen(line),&written,0);
    dump_context(process,thread,log);
    dump_requested_memory(process,log,base);
    if(template_unit>=0) {
        if(!template_slot_address)template_slot_address=current.Eax;
        template_observe(base,"after");
        template_output(&template_observer,current.Eax);
        if(template_observer.failed)return -1;
    }
    if(support_unit>=0 && (!observe_support(process,log,base,"after")||!observe_support_v3(process,log,base,"after")))return -1;
    if(support_unit>=3) {
        DWORD depth;
        if(!observe_support_result(process,log,current.Eax)||
           !ReadProcessMemory(process,(void *)(base+0x4ff18),&depth,4,&got)||got!=4)return -1;
        sprintf(line,"support-type-depth after %lu\n",depth);WriteFile(log,line,strlen(line),&written,0);
    }
    if(container_unit>=0&&!container_return(process,log,base,current.Eax))return -1;
    dump_asm_operand(process,log,base,"direct-return");
    if (buffers) dump_stream_buffers(process,log,base,buffers);
    TerminateProcess(process,99);
    return 1;
}

typedef struct EntryPlan {
    unsigned long rva;
    unsigned byte;
    char name[128];
} EntryPlan;
static EntryPlan entries[2048];

#include "capture-pdb-unit.h"

int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT ev;
    HANDLE out, err, log;
    DWORD base = 0, written, status, code;
    char command[2048], line[256], child_environment[8192];
    void *environment = 0;
    int loader_break = 0, failed = 0;
    unsigned entry_count = 0, next_entry = 0;
    FILE *plan;
    CONTEXT context;
    DWORD old_protection, restored_protection, site;
    unsigned char instruction, callback_byte;
    char *callback_option;
    DWORD callback_rva = 0, callback_site = 0, got;
    int callback_step = 0, flag_result;
    char *flag_option;
    DWORD flag_entry_rva = 0, flag_scanner_rva = 0, flag_parser_rva = 0, flag_symbol_rva = 0;
    unsigned callback_count = 0;

    flag_option=getenv("C1_PDB_UNIT");
    if(flag_option) {
        pdb_unit=atoi(flag_option);pdb_signature=0x31415926;
        if(pdb_unit<0||pdb_unit>5||argc==11)return 2;
    }
    flag_option = getenv("C1_DIRECT_CALL");
    if (flag_option) {
        if (sscanf(flag_option,"%lx,%lx,%u,%lx,%lx,%lx",&direct_marker_rva,
                &direct_target_rva,&direct_count,&direct_arguments[0],&direct_arguments[1],&direct_arguments[2]) < 5 ||
                direct_count>3 || (!direct_count && !getenv("C1_TEMPLATE_UNIT")) || (argc==11 && !getenv("C1_SUPPORT_UNIT") && !getenv("C1_CONTAINER_UNIT"))) return 2;
    }
    flag_option=getenv("C1_SUPPORT_UNIT");
    if(flag_option) {
        support_unit=atoi(flag_option);
        {
            static const DWORD targets[13]={0x15ed0,0x15f90,0x16030,0x2b2f0,0x2f3b0,0x302b0,0x236f0,0x25a30,0x2b080,0x2b080,0x25c30,0x38d10,0x1dd70};
            static const unsigned counts[13]={2,2,1,1,2,1,2,2,2,2,1,3,1};
            if(support_unit<0||support_unit>12||direct_marker_rva!=((support_unit==8||support_unit==9)?0x2b3a0:0x262c0)||
               direct_target_rva!=targets[support_unit]||direct_count!=counts[support_unit]||
               direct_arguments[0]!=(support_unit<3?0x800:support_unit==4?2:support_unit==12?172:0)||
               (support_unit==6&&direct_arguments[1]!=7))return 2;
        }
    }
    flag_option=getenv("C1_CONTAINER_UNIT");
    if(flag_option) {
        container_unit=atoi(flag_option);
        {
            static DWORD targets[]={0x226b0,0x226d0,0x227b0,0x22800,0x22810,0x22480,0x25910,0x303e0};
            static unsigned counts[]={2,2,2,1,2,1,2,2};
            if(container_unit<0||container_unit>7||support_unit>=0||getenv("C1_TEMPLATE_UNIT")||direct_marker_rva!=0x262c0||
               direct_target_rva!=targets[container_unit]||direct_count!=counts[container_unit]||direct_arguments[0]!=(container_unit==7?2:0)||
               direct_arguments[1]!=(container_unit<2?1:0))return 2;
        }
    }
    flag_option=getenv("C1_TEMPLATE_UNIT");
    if(flag_option) {
        template_unit=atoi(flag_option);
        if(template_unit<0 || template_unit>5 || !direct_marker_rva ||
           direct_marker_rva!=0x262c0 || direct_target_rva!=(template_unit?0x2c430:0x2c420) ||
           direct_count!=(template_unit?2:0))return 2;
    }
    callback_option = getenv("C1_ENUM_CALLBACK_RVA");
    if (callback_option) callback_rva = strtoul(callback_option,0,16);
    flag_option = getenv("C1_FLAG_PROBE_RVAS");
    if (flag_option && sscanf(flag_option,"%lx,%lx,%lx",&flag_entry_rva,&flag_scanner_rva,&flag_parser_rva) != 3) return 2;
    if ((callback_rva || flag_entry_rva) && argc == 11) return 2;
    if (callback_rva && flag_entry_rva) return 2;
    flag_option = getenv("C1_FUNCTION_SYMBOL_RVA");
    if (flag_option) flag_symbol_rva = strtoul(flag_option,0,16);
    if (flag_symbol_rva && !flag_entry_rva) return 2;
    complete_probe = getenv("C1_COMPLETE_TYPE_RETURN") != 0;
    if (complete_probe && !flag_entry_rva) return 2;
    strdup_probe = getenv("C1_STRDUP_PROBE") != 0;
    if (strdup_probe && !flag_entry_rva) return 2;
    segment_probe = getenv("C1_ASM_SEGMENT_PROBE") != 0;
    if (segment_probe && !flag_entry_rva) return 2;
    return8_probe = getenv("C1_RETURN8_PROBE") != 0;
    if (return8_probe && !flag_entry_rva) return 2;
    timestamp_probe = getenv("C1_TIMESTAMP_PROBE") != 0;
    if (timestamp_probe && !flag_entry_rva) return 2;

    if (argc == 11) {
        plan = fopen(argv[10],"r");
        if (!plan) return 2;
        while (entry_count < 2048 && fscanf(plan,"%lx %x %127s",
                &entries[entry_count].rva,&entries[entry_count].byte,
                entries[entry_count].name) == 3) ++entry_count;
        if (!feof(plan)) { fclose(plan); return 2; }
        fclose(plan);
    }

    if (argc < 6 || argc > 11) return 2;
    if (argc >= 8) {
        if (strlen(argv[7]) + 2 > sizeof(child_environment)) return 2;
        strcpy(child_environment, argv[7]);
        child_environment[strlen(argv[7]) + 1] = 0;
        environment = child_environment;
    }
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    err = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &sa,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    log = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out;
    si.hStdError = argc >= 10 && !strcmp(argv[9], "stderr-to-stdout") ? out : err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    if (!CreateProcessA(argv[1], command, 0, 0, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, environment, argc >= 7 ? argv[6] : 0, &si, &pi)) {
        sprintf(line, "create-error %lu\n", GetLastError());
        WriteFile(log, line, strlen(line), &written, 0);
        return 3;
    }
    for (;;) {
        if (!WaitForDebugEvent(&ev, 30000)) { if(support_unit==8||support_unit==9)literal_cleanup(pi.hProcess,log); TerminateProcess(pi.hProcess,99); return 4; }
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            pdb_teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if(pdb_unit>=0&&ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            failed=11;TerminateProcess(pi.hProcess,99);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if(pdb_unit>=0&&ev.dwThreadId!=pi.dwThreadId) {
                failed=11;TerminateProcess(pi.hProcess,99);
                ContinueDebugEvent(ev.dwProcessId,ev.dwThreadId,DBG_CONTINUE);
                continue;
            }
            if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
                if(pdb_unit>=0&&!pdb_unit_initialize(pi.hProcess,base)) {
                    failed=11;TerminateProcess(pi.hProcess,99);
                }
                if (flag_entry_rva) {
                    flag_entry = base + flag_entry_rva;
                    flag_scanner = base + flag_scanner_rva;
                    flag_parser = base + flag_parser_rva;
                    if (flag_symbol_rva) flag_symbol = base + flag_symbol_rva;
                    if (!ReadProcessMemory(pi.hProcess,(void *)flag_entry,&flag_entry_byte,1,&got) || got != 1 ||
                        flag_entry_byte == 0xcc || !patch_debug_byte(pi.hProcess,flag_entry,0xcc)) {
                        failed = 8; TerminateProcess(pi.hProcess,99);
                    }
                }
                if (callback_rva) {
                    callback_site = base + callback_rva;
                    if (!ReadProcessMemory(pi.hProcess,(void *)callback_site,&callback_byte,1,&got) || got != 1 ||
                        callback_byte == 0xcc || !patch_debug_byte(pi.hProcess,callback_site,0xcc)) {
                        failed = 7; TerminateProcess(pi.hProcess,99);
                    }
                }
            } else if ((flag_result = pdb_unit_event(pi.hProcess,pi.hThread,log,code,
                        (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress,base)) != 0) {
                if(flag_result<0) {failed=11;TerminateProcess(pi.hProcess,99);}
            } else if ((flag_result = pdb_free_event(pi.hProcess,pi.hThread,log,code,
                        (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress)) != 0) {
                if(flag_result<0) {failed=10;TerminateProcess(pi.hProcess,99);}
            } else if ((flag_result = direct_event(pi.hProcess,pi.hThread,log,code,
                        (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress,base,argc>=9?argv[8]:0)) != 0) {
                if (flag_result < 0) { if(support_unit==8||support_unit==9)literal_cleanup(pi.hProcess,log); failed=9; TerminateProcess(pi.hProcess,99); }
            } else if ((flag_result = observe_flag_boundary(pi.hProcess,pi.hThread,log,code,
                        (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress,base)) != 0) {
                if (flag_result < 0) { failed = 8; TerminateProcess(pi.hProcess,99); }
            } else if (callback_step && code == EXCEPTION_SINGLE_STEP) {
                memset(&context,0,sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(pi.hThread,&context) ||
                    !patch_debug_byte(pi.hProcess,callback_site,0xcc)) {
                    failed = 7; TerminateProcess(pi.hProcess,99);
                } else {
                    context.EFlags &= ~0x100UL;
                    if (!SetThreadContext(pi.hThread,&context)) { failed=7; TerminateProcess(pi.hProcess,99); }
                }
                callback_step = 0;
            } else if (callback_site && code == EXCEPTION_BREAKPOINT &&
                       (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress == callback_site) {
                memset(&context,0,sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(pi.hThread,&context) ||
                    !dump_enum_callback(pi.hProcess,log,&context,callback_count++) ||
                    !patch_debug_byte(pi.hProcess,callback_site,callback_byte)) {
                    failed=7; TerminateProcess(pi.hProcess,99);
                } else {
                    context.Eip = callback_site; context.EFlags |= 0x100;
                    if (!SetThreadContext(pi.hThread,&context)) { failed=7; TerminateProcess(pi.hProcess,99); }
                    callback_step = 1;
                }
            } else if (argc == 11 && code == EXCEPTION_BREAKPOINT &&
                       next_entry < entry_count &&
                       (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base == entries[next_entry].rva) {
                site = base + entries[next_entry].rva;
                instruction = (unsigned char)entries[next_entry].byte;
                memset(&context,0,sizeof(context));
                context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(pi.hThread,&context) ||
                    !VirtualProtectEx(pi.hProcess,(void *)site,1,PAGE_EXECUTE_READWRITE,&old_protection) ||
                    !WriteProcessMemory(pi.hProcess,(void *)site,&instruction,1,&written) || written != 1 ||
                    !FlushInstructionCache(pi.hProcess,(void *)site,1) ||
                    !VirtualProtectEx(pi.hProcess,(void *)site,1,old_protection,&restored_protection)) {
                    failed = 6;
                    if(support_unit==8||support_unit==9)literal_cleanup(pi.hProcess,log);
                    TerminateProcess(pi.hProcess,99);
                } else {
                    context.Eip = site;
                    if (!SetThreadContext(pi.hThread,&context)) {
                        failed = 6;
                        if(support_unit==8||support_unit==9)literal_cleanup(pi.hProcess,log);
                        TerminateProcess(pi.hProcess,99);
                    } else {
                        sprintf(line,"entry %s rva %08lx byte %02x\n",
                                entries[next_entry].name,entries[next_entry].rva,
                                entries[next_entry].byte);
                        WriteFile(log,line,strlen(line),&written,0);
                        ++next_entry;
                    }
                }
            } else {
                if (argc == 11 && next_entry < entry_count) {
                    sprintf(line,"entry-divergence expected %s rva %08lx\n",
                            entries[next_entry].name,entries[next_entry].rva);
                    WriteFile(log,line,strlen(line),&written,0);
                    failed = 5;
                }
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress - base,
                        base, ev.u.Exception.dwFirstChance);
                WriteFile(log, line, strlen(line), &written, 0);
                dump_context(pi.hProcess, pi.hThread, log);
                dump_requested_memory(pi.hProcess,log,base);
                dump_requested_strings(pi.hProcess,log,base);
                dump_complete_type(pi.hProcess,pi.hThread,log);
                dump_type_pair(pi.hProcess,pi.hThread,log);
                dump_declaration_scopes(pi.hProcess,log,base);
                if(getenv("C1_PDB_CLEANUP")) {
                    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                    if(GetThreadContext(pi.hThread,&context)) {
                        pdb_process=pi.hProcess;pdb_log=log;
                        memset(&pdb_observer,0,sizeof(pdb_observer));pdb_observer.read=pdb_original_read;pdb_observer.emit=pdb_original_emit;
                        pdb_memory(&pdb_observer,"object-entry",context.Ecx-72,116);
                        pdb_list(&pdb_observer,"entry",context.Ecx,1);
                    }
                }
                dump_asm_operand(pi.hProcess,log,base,"stop");
                if (argc >= 9) dump_stream_buffers(pi.hProcess, log, base, argv[8]);
                if((support_unit==8||support_unit==9)&&!literal_cleanup(pi.hProcess,log))failed=9;
                TerminateProcess(pi.hProcess, 99);
            }
        } else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            if (argc == 11 && next_entry != entry_count) failed = 5;
            if (flag_entry && (!flag_count || flag_active)) failed = 8;
            sprintf(line, "exit %lu\n", ev.u.ExitProcess.dwExitCode);
            WriteFile(log, line, strlen(line), &written, 0);
            ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
            break;
        }
        ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(log);
    return failed;
}
