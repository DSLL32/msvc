/* Explicit original-only reference capture; never linked into C1 or fixtures. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
static int return8_probe, segment_probe, strdup_probe;

static int dump_flag_words(HANDLE process, HANDLE log, const char *boundary)
{
    DWORD scanner, parser, got, written;
    char line[128];
    if (!ReadProcessMemory(process,(void *)flag_scanner,&scanner,4,&got) || got != 4 ||
        !ReadProcessMemory(process,(void *)flag_parser,&parser,4,&got) || got != 4) return 0;
    sprintf(line,"flag-words %u %s %08lx %08lx\n",flag_count,boundary,scanner,parser);
    return WriteFile(log,line,strlen(line),&written,0) && dump_function_symbol(process,log,boundary);
}
static int observe_flag_boundary(HANDLE process, HANDLE thread, HANDLE log,
                                 DWORD code, DWORD site, DWORD base)
{
    CONTEXT context;
    DWORD got, arguments[3], timestamp, written;
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
        if (segment_probe) dump_asm_operand(process,log,base,"entry");
        if (return8_probe || segment_probe || strdup_probe) {
            if (!ReadProcessMemory(process,(void *)context.Esp,arguments,sizeof(arguments),&got) || got != sizeof(arguments)) return -1;
            sprintf(line,"%s-entry %u %08lx %08lx\n",strdup_probe?"strdup":segment_probe?"segment":"return8",flag_count,context.Esp,arguments[1]);
            if (!WriteFile(log,line,strlen(line),&written,0)) return -1;
            if (strdup_probe) dump_pointer_text(process,log,"entry",arguments[1]);
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
        if (segment_probe) dump_asm_operand(process,log,base,"finish");
        if (return8_probe || segment_probe || strdup_probe) {
            sprintf(line,"%s-finish %u %08lx %08lx %08lx\n",strdup_probe?"strdup":segment_probe?"segment":"return8",flag_count,context.Esp,context.Eax,context.Edx);
            if (!WriteFile(log,line,strlen(line),&written,0)) return -1;
            if (strdup_probe) dump_pointer_text(process,log,"finish",context.Eax);
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
static DWORD direct_marker_rva, direct_target_rva, direct_arguments[2];
static unsigned direct_count;
static int direct_active;
static DWORD direct_stack, direct_marker;
static CONTEXT direct_saved;

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
        for (index=0;index<8;++index) words[index]=0xa5a5a5a5;
        words[0]=site;
        for (index=0;index<direct_count;++index) words[index+1]=direct_arguments[index];
        if (!WriteProcessMemory(process,(void *)direct_stack,words,sizeof(words),&written) || written!=sizeof(words)) return -1;
        sprintf(line,"direct-start target %08lx marker %08lx stack %08lx count %u args %08lx %08lx\n",
            base+direct_target_rva,site,direct_stack,direct_count,direct_arguments[0],direct_arguments[1]);
        WriteFile(log,line,strlen(line),&written,0);
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

    flag_option = getenv("C1_DIRECT_CALL");
    if (flag_option) {
        if (sscanf(flag_option,"%lx,%lx,%u,%lx,%lx",&direct_marker_rva,
                &direct_target_rva,&direct_count,&direct_arguments[0],&direct_arguments[1]) != 5 ||
                direct_count<1 || direct_count>2 || argc==11) return 2;
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
        if (!WaitForDebugEvent(&ev, 30000)) { TerminateProcess(pi.hProcess,99); return 4; }
        status = DBG_CONTINUE;
        if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)ev.u.CreateProcessInfo.lpBaseOfImage;
            if (ev.u.CreateProcessInfo.hFile)
                CloseHandle(ev.u.CreateProcessInfo.hFile);
        } else if (ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
        } else if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
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
            } else if ((flag_result = direct_event(pi.hProcess,pi.hThread,log,code,
                        (DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress,base,argc>=9?argv[8]:0)) != 0) {
                if (flag_result < 0) { failed=9; TerminateProcess(pi.hProcess,99); }
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
                    TerminateProcess(pi.hProcess,99);
                } else {
                    context.Eip = site;
                    if (!SetThreadContext(pi.hThread,&context)) {
                        failed = 6;
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
                dump_asm_operand(pi.hProcess,log,base,"stop");
                if (argc >= 9) dump_stream_buffers(pi.hProcess, log, base, argv[8]);
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
