/* Explicit original-only debugger. This is analysis tooling, not reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static FILE *events;
static HANDLE process, thread;
static DWORD base, text_rva, text_size, target_teb, target_tid;
static unsigned char *mask;
typedef struct Entry { DWORD rva; unsigned byte, seen; char name[128]; } Entry;
static Entry entries[2048];
static unsigned entry_count;
static DWORD full_link_receiver, full_link_output, full_link_entry_stack,pass_one_receiver;
static BYTE full_link_initial;
static unsigned virtual_count, section_flags_count, section_enum_count, section_next_count,section_end_count;
static unsigned section_lookup_count,section_name_count,section_resolve_count,group_enum_count;
static DWORD group_enum_arguments[2],group_enum_stack;
static DWORD group_arguments[4][2],group_stacks[4],group_counts[4],sections_initialize_stack;
static DWORD section_lookup_arguments[5],section_lookup_stack,section_name_pointer,section_name_stack,section_resolve_pointer,section_resolve_stack;
static DWORD section_next_pointer,section_next_stack,section_end_pointer,section_end_stack;
static DWORD section_enum_arguments[2],section_enum_stack;
static DWORD section_flags_arguments[2][3],section_flags_stacks[2];
static DWORD section_arguments[2][4], section_stacks[2];
static DWORD section_initializer_this;
static DWORD module_output_slot, module_output_initial, name_arguments[2], name_stacks[2];
static DWORD crt_base, main_frame_base, configuration_context, parser_allocation_bytes, parser_allocation, duplicate_result, virtual_arguments[4];
static DWORD library_search_arguments[2], library_search_stack;
static DWORD released_argument, release_stack, release_count;
typedef struct StateRequest { char scope[16], name[128]; DWORD rva, size; } StateRequest;
static StateRequest requests[64];
static unsigned request_count;
static const char *output_directory;

static void quoted(const char *value)
{
    unsigned char c;
    fputc('"', events);
    while ((c = (unsigned char)*value++) != 0) {
        if (c == '"' || c == '\\') fputc('\\', events);
        if (c < 32 || c >= 127) fprintf(events, "\\u%04x", c);
        else fputc(c, events);
    }
    fputc('"', events);
}

static int remote_string(DWORD address, char *value, unsigned capacity)
{
    DWORD got;
    unsigned index;
    for (index = 0; index + 1 < capacity; ++index) {
        if (!ReadProcessMemory(process, (void *)(address + index), value + index, 1, &got) || got != 1) return 0;
        if (!value[index]) return 1;
    }
    return 0;
}

static int patch(DWORD rva, const void *bytes, DWORD size)
{
    DWORD old, restored, written;
    void *address = (void *)(base + rva);
    return VirtualProtectEx(process, address, size, PAGE_EXECUTE_READWRITE, &old) &&
        WriteProcessMemory(process, address, bytes, size, &written) && written == size &&
        FlushInstructionCache(process, address, size) &&
        VirtualProtectEx(process, address, size, old, &restored);
}

static int save_module(HANDLE file, DWORD address, const char *directory, unsigned index)
{
    HANDLE output;
    DWORD old, got, written, total = 0;
    unsigned char buffer[16384];
    char path[1024];
    if (!file || file == INVALID_HANDLE_VALUE) return 0;
    sprintf(path, "%s\\module-%03u.bin", directory, index);
    output = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE) return 0;
    old = SetFilePointer(file, 0, NULL, FILE_CURRENT);
    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &got, NULL)) return 0;
        if (!got) break;
        if (!WriteFile(output, buffer, got, &written, NULL) || written != got) return 0;
        total += got;
    }
    SetFilePointer(file, old, NULL, FILE_BEGIN);
    CloseHandle(output);
    fprintf(events, "{\"event\":\"module\",\"index\":%u,\"base\":%lu,\"bytes\":%lu}\n",
            index, address, total);
    if (total == 277776) {
        if (crt_base) return 0;
        crt_base = address;
    }
    fflush(events);
    return 1;
}

/* Explicit original-only typed heap capture, bounded to this help case. */
static int save_memory(DWORD address,unsigned size,const char *name)
{
    unsigned char bytes[65536];
    DWORD got,written;
    char path[1024];
    HANDLE file;
    if(size>sizeof(bytes) || !ReadProcessMemory(process,(void *)address,bytes,size,&got)||got!=size)return 0;
    sprintf(path,"%s\\%s",output_directory,name);
    file=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE||!WriteFile(file,bytes,size,&written,NULL)||written!=size)return 0;
    CloseHandle(file);
    return 1;
}
/* The free wrapper has one call and one return site. Re-arm the opposite
 * site only, so all three actual option-copy lifetimes remain observable. */
static int rearm_observer(const char *name)
{
    unsigned index;
    unsigned char trap=0xcc;
    for(index=0;index<entry_count;++index)if(!strcmp(entries[index].name,name)) {
        if(!patch(entries[index].rva,&trap,1))return 0;
        entries[index].seen=0;
        return 1;
    }
    return 0;
}
static int save_context_heap(DWORD context)
{
    DWORD pointer,got,table[10],hash[14],arena[4];
    unsigned index,kind;
    char name[128];
    for(index=0;index<2;++index) {
        if(!ReadProcessMemory(process,(void *)(context+0x1d0+index*4),&pointer,4,&got)||got!=4||!pointer||
           !ReadProcessMemory(process,(void *)pointer,table,sizeof(table),&got)||got!=sizeof(table))return 0;
        sprintf(name,"table-%u.bin",index);if(!save_memory(pointer,40,name))return 0;
        if(!ReadProcessMemory(process,(void *)table[0],hash,sizeof(hash),&got)||got!=sizeof(hash))return 0;
        sprintf(name,"hash-%u.bin",index);if(!save_memory(table[0],56,name))return 0;
        for(kind=0;kind<2;++kind) {
            pointer=hash[11+kind];
            if(!pointer||!ReadProcessMemory(process,(void *)pointer,arena,sizeof(arena),&got)||got!=sizeof(arena))return 0;
            sprintf(name,"arena-%u-%u.bin",index,kind);if(!save_memory(pointer,16,name))return 0;
            sprintf(name,"arena-data-%u-%u.bin",index,kind);if(!save_memory(arena[3],arena[1]+arena[2],name))return 0;
        }
    }
    if(!ReadProcessMemory(process,(void *)(context+0x260),&pointer,4,&got)||got!=4||!pointer)return 0;
    return save_memory(pointer,44,"context-auxiliary.bin");
}

/* Same canonical-provider read-only TLS seam reviewed for CL/RCDLL. */
static int snapshot_errors(const char *phase)
{
    unsigned char teb[4096],ptd[116],body[9];
    DWORD index,pointer,self,tid,operand,got,error,doserror,last;
    char path[128];
    if(!target_teb||!crt_base||
       !ReadProcessMemory(process,(void *)(crt_base+0x6436),body,9,&got)||got!=9||
       memcmp(body,"\xe8\x88\xb1\xff\xff\x83\xc0\x08\xc3",9)||
       !ReadProcessMemory(process,(void *)(crt_base+0x15ce),&operand,4,&got)||got!=4||operand!=crt_base+0x3d004||
       !ReadProcessMemory(process,(void *)operand,&index,4,&got)||got!=4||index>=64||
       !ReadProcessMemory(process,(void *)target_teb,teb,sizeof(teb),&got)||got!=sizeof(teb))return 0;
    memcpy(&self,teb+0x18,4);memcpy(&tid,teb+0x24,4);memcpy(&pointer,teb+0xe10+4*index,4);
    if(self!=target_teb||tid!=target_tid||!pointer||
       !ReadProcessMemory(process,(void *)pointer,ptd,sizeof(ptd),&got)||got!=sizeof(ptd))return 0;
    sprintf(path,"%s-teb.bin",phase);if(!save_memory(target_teb,sizeof(teb),path))return 0;
    sprintf(path,"%s-ptd.bin",phase);if(!save_memory(pointer,sizeof(ptd),path))return 0;
    sprintf(path,"%s-tls-index.bin",phase);if(!save_memory(operand,4,path))return 0;
    memcpy(&error,ptd+8,4);memcpy(&doserror,ptd+12,4);memcpy(&last,teb+0x34,4);
    fprintf(events,"{\"event\":\"crt_error_state\",\"phase\":\"%s\",\"teb\":%lu,\"tid\":%lu,\"tls_index\":%lu,\"ptd\":%lu,\"errno\":%lu,\"doserrno\":%lu,\"last_error\":%lu}\n",phase,target_teb,target_tid,index,pointer,error,doserror,last);
    return 1;
}
static int remote_environment(const char *name,char *value,unsigned capacity)
{
    DWORD array,pointer,got,index;
    char entry[4096];
    if(!ReadProcessMemory(process,(void *)(crt_base+0x3f8fc),&array,4,&got)||got!=4||!array)return 0;
    for(index=0;index<1024;++index) {
        if(!ReadProcessMemory(process,(void *)(array+index*4),&pointer,4,&got)||got!=4)return 0;
        if(!pointer)return 0;
        if(!remote_string(pointer,entry,sizeof(entry)))return 0;
        if(!_strnicmp(entry,name,strlen(name))&&entry[strlen(name)]=='=') {
            if(strlen(entry+strlen(name)+1)>=capacity)return 0;
            strcpy(value,entry+strlen(name)+1);return 1;
        }
    }
    return 0;
}

/* Input argument nodes are reached program state, not implementation data. */
static int snapshot_arguments(void)
{
    static const DWORD offsets[3]={0x7f108,0x7f040,0x7ef28};
    static const char *names[3]={"input_files","option_arguments","link_files"};
    DWORD header[3],node[6],pointer,got;
    unsigned kind,index,word;
    char text[4096],original[4096];
    for(kind=0;kind<3;++kind) {
        if(!ReadProcessMemory(process,(void *)(base+offsets[kind]),header,12,&got)||got!=12||header[2]>64)return 0;
        fprintf(events,"{\"event\":\"argument_list\",\"name\":\"%s\",\"header\":[%lu,%lu,%lu],\"nodes\":[",names[kind],header[0],header[1],header[2]);
        pointer=header[0];
        for(index=0;index<header[2];++index) {
            if(!pointer||!ReadProcessMemory(process,(void *)pointer,node,24,&got)||got!=24||
               !remote_string(node[0],text,sizeof(text))||!remote_string(node[1],original,sizeof(original)))return 0;
            fprintf(events,"%s{\"address\":%lu,\"words\":[",index?",":"",pointer);
            for(word=0;word<6;++word)fprintf(events,"%s%lu",word?",":"",node[word]);
            fprintf(events,"],\"text\":");quoted(text);fprintf(events,",\"original\":");quoted(original);fprintf(events,"}");
            if(index+1==header[2]&&pointer!=header[1])return 0;
            pointer=node[5];
        }
        if(pointer)return 0;
        fprintf(events,"]}\n");
    }
    return 1;
}

static int snapshot(DWORD address)
{
    CONTEXT context;
    DWORD got, stack[32];
    unsigned index, argument_index;
    unsigned char bytes[65536];
    DWORD address_base, written, argument_count, arguments, pointer;
    HANDLE output;
    char path[1024], value[4096];
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread, &context)) return 0;
    fprintf(events, "{\"event\":\"trap\",\"address\":%lu,\"rva\":%lu,\"eax\":%lu,\"ecx\":%lu,\"edx\":%lu,\"esi\":%lu,\"edi\":%lu,\"esp\":%lu,\"stack\":[",
            address, address - base, context.Eax, context.Ecx, context.Edx, context.Esi, context.Edi, context.Esp);
    if (!ReadProcessMemory(process, (void *)context.Esp, stack, sizeof(stack), &got)) return 0;
    for (index = 0; index < got / 4; ++index)
        fprintf(events, "%s%lu", index ? "," : "", stack[index]);
    fprintf(events, "],\"x87_control_word\":%lu,\"crt_base\":%lu,\"crt_argv\":[",
            context.FloatSave.ControlWord & 0xffff, crt_base);
    if (!crt_base || !ReadProcessMemory(process, (void *)(crt_base + 0x3fa84), &argument_count, 4, &got) || got != 4 ||
        argument_count > 1024 || !ReadProcessMemory(process, (void *)(crt_base + 0x3fa8c), &arguments, 4, &got) || got != 4) return 0;
    for (index = 0; index < argument_count; ++index) {
        if (!ReadProcessMemory(process, (void *)(arguments + 4 * index), &pointer, 4, &got) || got != 4 ||
            !remote_string(pointer, value, sizeof(value))) return 0;
        if (index) fputc(',', events);
        quoted(value);
    }
    fprintf(events, "],\"crt_argv_pointer\":%lu,\"ebp\":%lu",arguments,context.Ebp);
    pointer=0;
    if(argument_count>1 && (!ReadProcessMemory(process,(void *)(arguments+4),&pointer,4,&got)||got!=4))return 0;
    fprintf(events,",\"crt_first_argument_pointer\":%lu",pointer);
    fprintf(events,",\"crt_argv_addresses\":[");
    for(index=0;index<argument_count;++index) {
        if(!ReadProcessMemory(process,(void *)(arguments+4*index),&pointer,4,&got)||got!=4)return 0;
        fprintf(events,"%s%lu",index?",":"",pointer);
    }
    fprintf(events,"]");
    if(address==base+0x15230) {
        if(!remote_string(stack[2],value,sizeof(value)))return 0;
        fprintf(events,",\"debug_options_pointer\":%lu,\"debug_environment\":",stack[1]);quoted(value);
    }
    if(main_frame_base || address==base+0xb560 || address==base+0x152f0 || address==base+0x4fe70 || address==base+0xb660 || address==base+0x4f560 || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        if(!ReadProcessMemory(process,(void *)(base+0x7f074),&pointer,4,&got)||got!=4||!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"program_name_pointer\":%lu,\"main_frame_pointer\":%lu,\"program_name\":",pointer,main_frame_base?main_frame_base:address==base+0x44b10?stack[2]:address==base+0x5caa0?stack[6]:address==base+0x5cb20?stack[13]:context.Ebp);quoted(value);
    }
    if(main_frame_base || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        if(!ReadProcessMemory(process,(void *)(base+0x7f060),&pointer,4,&got)||got!=4||!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"selected_tool_name_pointer\":%lu,\"selected_tool_name\":",pointer);quoted(value);
    }
    /* Parameter roles are keyed to the exact known function ABI. */
    argument_index=0;
    switch(address-base) {
    case 0x61840: /* check_output_input_collision(output, inputs) */
    case 0xe700: /* create_contribution_group(name, section) */
    case 0x4fd80: /* duplicate_link_string(text) */
    case 0x5c530: /* search_library_paths(name, found) */
    case 0x5c650: /* canonicalize_argument(text, extension) */
    case 0x5c780: /* parse_argument(text, source) */
    case 0x5c750: /* is_libpath(text) */
    case 0x4fed0: /* duplicate_argument(text) */
    case 0x4fe30: /* release_memory(memory), reached owned string */
    case 0x44f20: /* has_switch(name) */
        argument_index=1; break;
    case 0xe670: /* find_contribution_group(section, name) */
    case 0xd0c0: /* parse_switch(file, text) */
    case 0x5cca0: /* append_argument(list, text) */
    case 0x5cc00: /* append_argument_record(list, text, original_name) */
        argument_index=2; break;
    }
    if(argument_index) {
        pointer=stack[argument_index];
        if(!remote_string(pointer,value,sizeof(value)))return 0;
        fprintf(events,",\"argument_pointer\":%lu,\"argument_text\":",pointer);quoted(value);
    }
    if(address==base+0x5c530) {
        static const unsigned offsets[4]={16,0x118,0x318,0x218};
        static const char *names[4]={"drive","directory","filename","suffix"};
        fprintf(events,",\"result_buffer_pointer\":%lu,\"canonicalize_components\":{",stack[2]);
        for(index=0;index<4;++index) {
            if(!remote_string(context.Esp+offsets[index],value,sizeof(value)))return 0;
            fprintf(events,"%s\"%s\":",index?",":"",names[index]);quoted(value);
        }
        fprintf(events,"}");
    }
    if((address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840) || address==base+0xe620 || address==base+0xfd90 || address==base+0xfc00 || address==base+0xfbd0 || address==base+0x5e820 || address==base+0xf130 || address==base+0xef90 || address==base+0x3e170 || address==base+0x62eb0 || address==base+0x4f5b0 || address==base+0xe360 || address==base+0x48630 || address==base+0x42f50 || address==base+0xe3b0 || address==base+0x4fd80) {
        DWORD output;
        if(!ReadProcessMemory(process,(void *)(base+0x7efa4),&output,4,&got)||got!=4||
           !remote_string(output,value,sizeof(value)))return 0;
        fprintf(events,",\"validation_context\":%lu,\"output_name_pointer\":%lu,\"output_name\":",((address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)||address==base+0xe620||address==base+0xfd90||address==base+0xfc00||address==base+0xfbd0||address==base+0x5e820||address==base+0xf130||address==base+0xef90||address==base+0x3e170||address==base+0x62eb0||address==base+0x4f5b0||address==base+0xe360||address==base+0xe3b0||address==base+0x4fd80)?configuration_context:address==base+0x42f50?context.Ecx:stack[1],output);quoted(value);
        if(address==base+0x42f50) {
            unsigned char initial;
            if(!ReadProcessMemory(process,(void *)stack[1],&initial,1,&got)||got!=1||!save_memory(stack[1],1,"pack-requested-before.bin"))return 0;
            fprintf(events,",\"full_link_formals\":{\"this\":%lu,\"pack_requested\":%lu,\"pack_requested_initial\":%u,\"initial_value_contract\":\"uninitialized caller local; no cross-platform value equality\"}",context.Ecx,stack[1],initial);
        }
    }
    if(address==base+0xe3b0) {
        DWORD initial;
        if(!ReadProcessMemory(process,(void *)stack[1],&initial,4,&got)||got!=4||
           !save_memory(stack[1],4,"output-module-before.bin"))return 0;
        fprintf(events,",\"output_module_formals\":{\"module\":%lu,\"destination_global\":%lu,\"initial\":%lu},\"full_link_formals\":{\"this\":%lu,\"pack_requested\":%lu,\"pack_requested_initial\":%u,\"initial_value_contract\":\"uninitialized caller local; no cross-platform value equality\"}",stack[1],base+0x7f028,initial,full_link_receiver,full_link_output,full_link_initial);
    }
    if(address==base+0x4fd80 || address==base+0xe360 || address==base+0x4f5b0 || address==base+0x62eb0 || address==base+0x3e170 || address==base+0xef90 || address==base+0xf130 || address==base+0x5e820 || address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620 || (address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)) {
        DWORD module=context.Esi,library,arena,header[4],kind,mp=0,lp=0;
        char filename[128];
        if((address==base+0xe360 || address==base+0x4f5b0 || address==base+0x62eb0 || address==base+0x3e170 || address==base+0xef90 || address==base+0xf130 || address==base+0x5e820 || address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620 || (address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)) && (!ReadProcessMemory(process,(void *)(base+0x7f028),&module,4,&got)||got!=4))return 0;
        if(!ReadProcessMemory(process,(void *)(module+0x64),&library,4,&got)||got!=4||
           !save_memory(module,156,"output-module.bin")||!save_memory(library,68,"output-library.bin"))return 0;
        fprintf(events,",\"module_constructor\":{\"result_slot\":%lu,\"initial\":%lu,\"module\":%lu,\"library\":%lu},\"full_link_formals\":{\"this\":%lu,\"pack_requested\":%lu,\"pack_requested_initial\":%u,\"initial_value_contract\":\"uninitialized caller local; no cross-platform value equality\"}",module_output_slot,module_output_initial,module,library,full_link_receiver,full_link_output,full_link_initial);
        if(address==base+0xe360 || address==base+0x4f5b0 || address==base+0x62eb0 || address==base+0x3e170 || address==base+0xef90 || address==base+0xf130 || address==base+0x5e820 || address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620 || (address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)) {
            if(!ReadProcessMemory(process,(void *)(module+4),&mp,4,&got)||got!=4||
               !ReadProcessMemory(process,(void *)library,&lp,4,&got)||got!=4||
               !save_memory(mp,23,"output-module-name.bin")||!save_memory(lp,24,"output-library-name.bin"))return 0;
            fprintf(events,",\"output_names\":[%lu,%lu],\"frontier_inputs\":{",mp,lp);
            if(address!=base+0x4b860)fprintf(events,"\"hidden_register_inputs\":[],");
            fprintf(events,"\"formals\":[");
            if(address==base+0x4f5b0)fprintf(events,"%lu,%lu,%lu,%lu",stack[1],stack[2],stack[3],stack[4]);
            if(address==base+0xef90 || address==base+0xf130)fprintf(events,"%lu,%lu,%lu,%lu,%lu",stack[1],stack[2],stack[3],stack[4],stack[5]);
            if(address==base+0x5e820)fprintf(events,"%lu,%lu,%lu",stack[1],stack[2],stack[3]);
            if(address==base+0xfbd0 || (address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0x61840))fprintf(events,"%lu,%lu",stack[1],stack[2]);
            if(address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620 || address==base+0xfc70)fprintf(events,"%lu",stack[1]);
            if(address==base+0x4b860)fprintf(events,"],\"hidden_register_inputs\":[{\"register\":\"ecx\",\"value\":%lu}]}",context.Ecx);
            else fprintf(events,"]}");
            if(address==base+0x4f5b0 && (!save_memory(base+0x94700,4,"section-map-global.bin")||
               !save_memory(base+0x94898,4,"section-headers-global.bin")||!save_memory(base+0x94894,4,"section-capacity.bin")||
               !save_memory(base+0x9485c,1,"section-pools-ready.bin")))return 0;
        }
        for(kind=0;kind<2;++kind) {
            DWORD slot=base+(kind?0x6758c:0x67590);
            sprintf(filename,"output-%s-arena-pointer.bin",kind?"library":"module");if(!save_memory(slot,4,filename))return 0;
            if(!ReadProcessMemory(process,(void *)slot,&arena,4,&got)||got!=4||!arena||
               !ReadProcessMemory(process,(void *)arena,header,16,&got)||got!=16)return 0;
            sprintf(filename,"output-%s-arena.bin",kind?"library":"module");if(!save_memory(arena,16,filename))return 0;
            sprintf(filename,"output-%s-page.bin",kind?"library":"module");if(!save_memory(header[3],header[1]+header[2],filename))return 0;
        }
    }
    if(address==base+0x62eb0 || address==base+0x3e170 || address==base+0xef90 || address==base+0xf130 || address==base+0x5e820 || address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620 || (address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)) {
        DWORD map,headers;
        if(!ReadProcessMemory(process,(void *)(base+0x94700),&map,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(base+0x94898),&headers,4,&got)||got!=4||
           !save_memory(base+0x94700,4,"section-map-global.bin")||!save_memory(base+0x94898,4,"section-headers-global.bin")||
           !save_memory(base+0x94894,4,"section-capacity.bin")||!save_memory(base+0x9485c,1,"section-pools-ready.bin")||
           !save_memory(map,4000,"section-map.bin")||!save_memory(headers,20000,"section-headers.bin")||
           !save_memory(base+0x94920,4,"comdat-root.bin"))return 0;
        if(address==base+0x3e170)fprintf(events,",\"section_initializer_formals\":{\"this\":%lu,\"stack_formals\":[],\"other_hidden_inputs\":[]}",context.Ecx);
        fprintf(events,",\"module_section_pools\":{\"map\":%lu,\"headers\":%lu}",map,headers);
    }
    if(address==base+0xef90 || address==base+0xf130 || address==base+0x5e820 || address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620) {
        static const DWORD slots[]={0x7ef60,0x7ef44,0x7ef58,0x7f058,0x7f0ac,0x7f098,0x7f054,0x7f050,0x7ef00,0x7f184,0x7f134,0x7f140,0x7f068,0x7ef80,0x7f15c,0x7ef70,0x7f0c4,0x7efb8,0x7efbc};
        DWORD value,arguments[5];
        char value_buffer[128];
        if(address==base+0x5e820) {
            if(!ReadProcessMemory(process,(void *)(context.Esp+0x23c),arguments,20,&got)||got!=20||
               !ReadProcessMemory(process,(void *)stack[1],&value,4,&got)||got!=4||
               !save_memory(stack[1],4,"section-normalized-before.bin")||!save_memory(context.Esp+0x23c,20,"section-lookup-inputs.bin"))return 0;
            fprintf(events,",\"section_flag_formals\":{\"flags\":%lu,\"before\":%lu,\"name\":%lu,\"header\":%lu,\"owner_stack\":%lu}",stack[1],value,stack[2],stack[3],context.Esp+0x23c);
        } else if(address==base+0xfbd0 || address==base+0xfc00 || address==base+0xfd90 || address==base+0xe620) {
            DWORD normalized,initialized=address!=base+0xfbd0,owner_delta=initialized?0x234:0x238;
            DWORD enumerator=address==base+0xe620?context.Esp+0x1c:stack[1];
            DWORD normalized_delta=initialized?0x18:0x1c;
            if(!ReadProcessMemory(process,(void *)(context.Esp+owner_delta),arguments,20,&got)||got!=20||
               !ReadProcessMemory(process,(void *)(context.Esp+normalized_delta),&normalized,4,&got)||got!=4||
               !save_memory(context.Esp+owner_delta,20,"section-lookup-inputs.bin")||
               !save_memory(enumerator,16,initialized?"section-enumerator.bin":"section-enumerator-before.bin"))return 0;
            fprintf(events,",\"section_enumerator_formals\":{\"enumerator\":%lu,\"sections\":%lu,\"owner_stack\":%lu,\"normalized_pointer\":%lu,\"normalized\":%lu,\"initial_value_contract\":\"%s\"}",enumerator,arguments[3],context.Esp+owner_delta,context.Esp+normalized_delta,normalized,initialized?"fully initialized enumerator record":"uninitialized output record; no incoming bytes consumed");
            if(address==base+0xe620)fprintf(events,",\"resolution_formals\":{\"section\":%lu,\"caller_current_slot\":%lu}",stack[1],enumerator+12);
        } else memcpy(arguments,stack+1,20);
        if(!remote_string(arguments[1],value_buffer,sizeof(value_buffer)))return 0;
        fprintf(events,",\"section_creation_formals\":{\"module\":%lu,\"name\":%lu,\"flags\":%lu,\"sections\":%lu,\"header\":%lu,\"text\":",arguments[0],arguments[1],arguments[2],arguments[3],arguments[4]);quoted(value_buffer);fprintf(events,"},\"section_initializer_this\":%lu,\"section_handles\":[",section_initializer_this);
        for(index=0;index<sizeof(slots)/sizeof(slots[0]);++index) {
            if(!ReadProcessMemory(process,(void *)(base+slots[index]),&value,4,&got)||got!=4)return 0;
            fprintf(events,"%s%lu",index?",":"",value);
        }
        fprintf(events,"]");
    }
    if((address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840)) {
        static const DWORD slots[]={0x7ef60,0x7ef44,0x7ef58,0x7f058,0x7f0ac,0x7f098,0x7f054,0x7f050,0x7ef00,0x7f184,0x7f134,0x7f140,0x7f068,0x7ef80,0x7f15c,0x7ef70,0x7f0c4,0x7efb8,0x7efbc};
        DWORD list[2],node,record[18],arena,header[4],value,count=0;
        DWORD name=stack[address==base+0xe670?2:1],section=stack[address==base+0xe670?1:2];
        WORD section_count;
        char filename[128],text[128];
        if(address==base+0xfc40 || address==base+0xfc70) {
            DWORD owner=context.Esp+(address==base+0xfc40?0x30:0x2c);
            if(!ReadProcessMemory(process,(void *)(owner+4),&name,4,&got)||got!=4||
               !ReadProcessMemory(process,(void *)owner,&section,4,&got)||got!=4||
               !save_memory(owner,8,"group-finder-inputs.bin")||
               !save_memory(stack[1],16,address==base+0xfc40?"group-enumerator-before.bin":"group-enumerator.bin"))return 0;
            fprintf(events,",\"group_enumerator_formals\":{\"enumerator\":%lu,\"section\":%lu,\"owner_stack\":%lu,\"initial_value_contract\":\"%s\"}",stack[1],section,owner,address==base+0xfc40?"uninitialized output record; no incoming bytes consumed":"fully initialized group enumerator record");
        }
        if(!ReadProcessMemory(process,(void *)(configuration_context+0x1b4),list,8,&got)||got!=8)return 0;
        if(address==base+0x4b860 || address==base+0x61840) {
            if(!ReadProcessMemory(process,(void *)(base+0x670c0),&value,4,&got)||got!=4||!remote_string(value,text,sizeof(text)))return 0;
            fprintf(events,",\"default_phase\":{\"address\":%lu,\"text\":",value);quoted(text);
            fprintf(events,"},\"pass_one_formals\":{\"this\":%lu,\"stack_formals\":[],\"other_hidden_inputs\":[]}",address==base+0x4b860?context.Ecx:pass_one_receiver);
        } else {
            if(!remote_string(name,text,sizeof(text)))return 0;
            fprintf(events,",\"section_group_formals\":{\"name\":%lu,\"section\":%lu,\"text\":",name,section);quoted(text);fprintf(events,"}");
        }
        fprintf(events,",\"section_initializer_this\":%lu,\"section_list\":[%lu,%lu],\"section_records\":[",section_initializer_this,list[0],list[1]);
        for(node=list[0];node;node=record[17]) {
            if(count>=32||!ReadProcessMemory(process,(void *)node,record,72,&got)||got!=72||
               !remote_string(record[0],text,sizeof(text)))return 0;
            sprintf(filename,"output-section-%u.bin",count);if(!save_memory(node,72,filename))return 0;
            sprintf(filename,"output-section-name-%u.bin",count);if(!save_memory(record[0],strlen(text)+1,filename))return 0;
            fprintf(events,"%s{\"address\":%lu,\"name\":%lu,\"next\":%lu,\"text\":",count?",":"",node,record[0],record[17]);quoted(text);fprintf(events,"}");++count;
        }
        if(!ReadProcessMemory(process,(void *)(base+0x67598),&arena,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)arena,header,16,&got)||got!=16||
           !ReadProcessMemory(process,(void *)(base+0x67584),&section_count,2,&got)||got!=2||
           !save_memory(arena,16,"output-section-arena.bin")||!save_memory(header[3],header[1]+header[2],"output-section-page.bin"))return 0;
        fprintf(events,"],\"output_section_arena\":%lu,\"output_section_count\":%u,\"section_handles\":[",arena,section_count);
        for(count=0;count<sizeof(slots)/sizeof(slots[0]);++count) {
            if(!ReadProcessMemory(process,(void *)(base+slots[count]),&value,4,&got)||got!=4)return 0;
            fprintf(events,"%s%lu",count?",":"",value);
        }
        fprintf(events,"]");
        if(address==base+0x4b860 || address==base+0x61840) {
            if(!ReadProcessMemory(process,(void *)(base+0x6759c),&arena,4,&got)||got!=4||
               !ReadProcessMemory(process,(void *)arena,header,16,&got)||got!=16||header[1]>4096||header[1]%44||
               !save_memory(arena,16,"contribution-group-arena.bin")||!save_memory(header[3],header[1]+header[2],"contribution-group-page.bin"))return 0;
            fprintf(events,",\"contribution_group_arena\":%lu,\"group_records\":[",arena);
            for(count=0;count<header[1]/44;++count) {
                node=header[3]+count*44;
                if(!ReadProcessMemory(process,(void *)node,record,44,&got)||got!=44||!remote_string(record[0],text,sizeof(text)))return 0;
                sprintf(filename,"contribution-group-%u.bin",count);if(!save_memory(node,44,filename))return 0;
                sprintf(filename,"contribution-group-name-%u.bin",count);if(!save_memory(record[0],strlen(text)+1,filename))return 0;
                fprintf(events,"%s{\"address\":%lu,\"name\":%lu,\"text\":",count?",":"",node,record[0]);quoted(text);fprintf(events,"}");
            }
            fprintf(events,"]");
        }
    }
    if(address==base+0x61840) {
        BYTE verbose,opened,ignored;
        DWORD current;
        if(!remote_string(stack[1],value,sizeof(value))||
           !ReadProcessMemory(process,(void *)(base+0x7eff4),&verbose,1,&got)||got!=1||
           !ReadProcessMemory(process,(void *)(base+0x7efb0),&opened,1,&got)||got!=1||
           !ReadProcessMemory(process,(void *)(base+0x7f008),&ignored,1,&got)||got!=1||
           !ReadProcessMemory(process,(void *)(base+0x672cc),&current,4,&got)||got!=4)return 0;
        fprintf(events,",\"collision_formals\":{\"output\":%lu,\"inputs\":%lu,\"text\":",stack[1],stack[2]);quoted(value);
        fprintf(events,"},\"pass_one_flags\":{\"verbose_link\":%u,\"output_file_open\":%u,\"current_image_file\":%ld,\"arch_data_ignored\":%u}",verbose,opened,(long)current,ignored);
    }
    if(parser_allocation)fprintf(events,",\"parsed_switch_pointer\":%lu",parser_allocation);
    if(duplicate_result)fprintf(events,",\"duplicate_pointer\":%lu",duplicate_result);
    if(configuration_context)fprintf(events,",\"configuration_context\":%lu",configuration_context);
    fprintf(events,"}\n");
    for (index = 0; index < request_count; ++index) {
        address_base = strcmp(requests[index].scope, "crt") ? base : crt_base;
        pointer=address_base+requests[index].rva;
        if(!strcmp(requests[index].scope,"targetptr")) {
            if(!ReadProcessMemory(process,(void *)pointer,&pointer,4,&got)||got!=4||!pointer)return 0;
        }
        if (!address_base || !ReadProcessMemory(process, (void *)pointer,
            bytes, requests[index].size, &got) || got != requests[index].size) return 0;
        sprintf(path, "%s\\%s", output_directory, requests[index].name);
        output = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (output == INVALID_HANDLE_VALUE || !WriteFile(output, bytes, requests[index].size, &written, NULL) ||
            written != requests[index].size) return 0;
        CloseHandle(output);
    }
    if(main_frame_base || address==base+0x4f560 || address==base+0x48cb0 || address==base+0x44b10 || address==base+0x5caa0 || address==base+0x5cb20) {
        DWORD slots[16],array;
        if(!ReadProcessMemory(process,(void *)(base+0x67538),&array,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)array,slots,sizeof(slots),&got)||got!=sizeof(slots))return 0;
        for(index=0;index<16;++index) {
            if(!ReadProcessMemory(process,(void *)slots[index],bytes,40,&got)||got!=40)return 0;
            sprintf(path,"%s\\file-record-%02u.bin",output_directory,index);
            output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
            if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,40,&written,NULL)||written!=40)return 0;
            CloseHandle(output);
        }
    }
    if(address==base+0x337d0 || address==base+0x62220 || address==base+0x30c70 || address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
        pointer=address==base+0x337d0?context.Ecx:context.Ebp;
        if(!ReadProcessMemory(process,(void *)pointer,bytes,664,&got)||got!=664)return 0;
        sprintf(path,"%s\\link-context.bin",output_directory);
        output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,664,&written,NULL)||written!=664)return 0;
        CloseHandle(output);
        if(address==base+0x30c70 || address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
            if(!ReadProcessMemory(process,(void *)(pointer+0x1d0),&pointer,4,&got)||got!=4||!pointer||
               !ReadProcessMemory(process,(void *)pointer,bytes,40,&got)||got!=40)return 0;
            sprintf(path,"%s\\string-table.bin",output_directory);
            output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
            if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,40,&written,NULL)||written!=40)return 0;
            CloseHandle(output);
            if(address==base+0x30c30 || address==base+0x4f5e0 || address==base+0x4f640 || address==base+0x4fe50) {
                if(!ReadProcessMemory(process,(void *)pointer,&pointer,4,&got)||got!=4||!pointer||
                   !ReadProcessMemory(process,(void *)pointer,bytes,56,&got)||got!=56)return 0;
                sprintf(path,"%s\\hash-state.bin",output_directory);
                output=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
                if(output==INVALID_HANDLE_VALUE||!WriteFile(output,bytes,56,&written,NULL)||written!=56)return 0;
                CloseHandle(output);
            }
        }
    }
    if(address==base+0x45130 || (((address==base+0xe700 || address==base+0xe670 || address==base+0xfc40 || address==base+0xfc70 || address==base+0x4b860 || address==base+0x61840) || address==base+0xe620 || address==base+0xfd90 || address==base+0xfc00 || address==base+0xfbd0 || address==base+0x5e820 || address==base+0xf130 || address==base+0xef90 || address==base+0x3e170 || address==base+0x62eb0 || address==base+0x4f5b0 || address==base+0xe360 || address==base+0x4fd80 || address==base+0xe3b0 || address==base+0x42f50 || address==base+0x48630 || address==base+0xd0c0 || address==base+0x4fed0 || address==base+0x4fe30 || address==base+0x44f80 || address==base+0x615b0 || address==base+0x44eb0 || address==base+0x31e30 || address==base+0x31dc0 || address==base+0x24eb0) && configuration_context)) {
        pointer=address==base+0x45130?stack[1]:configuration_context;
        if(!save_memory(pointer,664,"link-context.bin") || !save_context_heap(pointer))return 0;
    }
    if(address==base+0x4fed0 || address==base+0x4fe30 || address==base+0x44f80 || address==base+0x615b0 || address==base+0x44eb0 || address==base+0x31e30 || address==base+0x31dc0 || address==base+0x24eb0) {
        if(!parser_allocation || (address!=base+0x44eb0 && address!=base+0x31e30 && address!=base+0x31dc0 && address!=base+0x24eb0 && context.Ebp!=parser_allocation) || !save_memory(parser_allocation,parser_allocation_bytes,"parsed-switch.bin"))return 0;
    }
    if(address==base+0x4fe30 && (!duplicate_result || stack[1]!=duplicate_result ||
       !remote_string(stack[1],value,sizeof(value)) || !save_memory(stack[1],strlen(value)+1,"release-argument.bin")))return 0;
    if((address==base+0x44f80 || address==base+0x615b0 || address==base+0x44eb0 || address==base+0x31e30 || address==base+0x31dc0 || address==base+0x24eb0) && (!duplicate_result || !save_memory(duplicate_result,2,"duplicated-option.bin")))return 0;
    if(address==base+0x44eb0 || address==base+0x31e30 || address==base+0x31dc0 || address==base+0x24eb0) {
        DWORD file_state[8];
        if(!ReadProcessMemory(process,(void *)(crt_base+0x3f590),file_state,sizeof(file_state),&got)||got!=sizeof(file_state) ||
           file_state[0]<file_state[2] || file_state[0]-file_state[2]>file_state[6] ||
           !save_memory(file_state[2],file_state[0]-file_state[2],"stdout-pending.bin"))return 0;
    }
    if(!snapshot_arguments())return 0;
    fflush(events);
    return 1;
}

int main(int argc, char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION launched;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE output, error;
    FILE *file;
    DWORD status, site, code, result = 999, done = 0;
    unsigned index, modules = 0;
    int failed = 0, loader_seen = 0;
    char command[4096], path[1024];
    if (argc != 6 && argc != 7) return 2;
    /* target, output-directory, mask-or-dash, entries, working-directory */
    sprintf(path, "%s\\events.jsonl", argv[2]);
    events = fopen(path, "wt");
    if (!events) return 3;
    output_directory = argv[2];
    sprintf(path, "%s\\state-plan.txt", argv[2]);
    file = fopen(path, "rt");
    if (!file) return 3;
    while (request_count < 64 && fscanf(file, "%15s %lx %lu %127s", requests[request_count].scope,
           &requests[request_count].rva, &requests[request_count].size, requests[request_count].name) == 4) {
        if ((strcmp(requests[request_count].scope,"target") && strcmp(requests[request_count].scope,"crt") && strcmp(requests[request_count].scope,"targetptr")) ||
            requests[request_count].size > 65536) return 3;
        ++request_count;
    }
    if (!feof(file)) return 3;
    fclose(file);
    if (strcmp(argv[3], "-")) {
        file = fopen(argv[3], "rb");
        if (!file || fread(&text_rva, 4, 1, file) != 1 ||
            fread(&text_size, 4, 1, file) != 1 || text_size > 0x1000000) return 3;
        mask = malloc(text_size);
        if (!mask || fread(mask, 1, text_size, file) != text_size || fgetc(file) != EOF) return 3;
        fclose(file);
    }
    file = fopen(argv[4], "rt");
    if (!file) return 3;
    while (entry_count < 2048 && fscanf(file, "%lx %x %u %127s", &entries[entry_count].rva,
           &entries[entry_count].byte, &entries[entry_count].seen, entries[entry_count].name) == 4)
        ++entry_count;
    if (!feof(file)) return 3;
    fclose(file);
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security); security.bInheritHandle = TRUE;
    sprintf(path, "%s\\stdout.bin", argv[2]);
    output = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    sprintf(path, "%s\\stderr.bin", argv[2]);
    error = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE || error == INVALID_HANDLE_VALUE) return 3;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = output; startup.hStdError = error;
    if (argc == 7) {
        if (strlen(argv[6]) >= sizeof(command)) return 2;
        strcpy(command, argv[6]);
    } else sprintf(command, "\"%s\" /?", argv[1]);
    if (!CreateProcessA(argv[1], command, NULL, NULL, TRUE, DEBUG_ONLY_THIS_PROCESS,
                        NULL, argv[5], &startup, &launched)) return 4;
    CloseHandle(launched.hProcess); CloseHandle(launched.hThread);
    while (!done) {
        if (!WaitForDebugEvent(&event, 30000)) { failed = 5; break; }
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            process = event.u.CreateProcessInfo.hProcess;
            thread = event.u.CreateProcessInfo.hThread;
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            target_teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;target_tid=event.dwThreadId;
            if (base != 0x400000 || !save_module(event.u.CreateProcessInfo.hFile, base, argv[2], modules++)) { failed = 6; break; }
            CloseHandle(event.u.CreateProcessInfo.hFile);
            if (mask && !patch(text_rva, mask, text_size)) { failed = 6; break; }
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (!save_module(event.u.LoadDll.hFile, (DWORD)event.u.LoadDll.lpBaseOfDll, argv[2], modules++)) { failed = 6; break; }
            CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            /* No extra target thread is expected for this simple first case. */
            CloseHandle(event.u.CreateThread.hThread); failed = 7; break;
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            site = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == EXCEPTION_BREAKPOINT && !loader_seen &&
                (site < base || site >= base + 0x9c000)) loader_seen = 1;
            else if (code == EXCEPTION_BREAKPOINT) {
                for (index = 0; index < entry_count && entries[index].rva != site - base; ++index);
                if (index == entry_count || entries[index].seen) {
                    if (!snapshot(site)) { failed = 6; break; }
                    done = 1;
                    fprintf(events, "{\"event\":\"stopped\",\"code\":%lu}\n", code);
                } else {
                    unsigned char instruction = (unsigned char)entries[index].byte;
                    if (!patch(entries[index].rva, &instruction, 1)) { failed = 6; break; }
                    memset(&context, 0, sizeof(context)); context.ContextFlags = CONTEXT_FULL;
                    if (!GetThreadContext(thread, &context)) { failed = 6; break; }
                    context.Eip = site;
                    if (!SetThreadContext(thread, &context)) { failed = 6; break; }
                    entries[index].seen = 1;
                    if (!strcmp(entries[index].name,"observe:main_frame")) {
                        main_frame_base=context.Ebp;
                        fprintf(events,"{\"event\":\"main_frame\",\"site\":%lu,\"ebp\":%lu}\n",site,main_frame_base);
                    } else if (!strncmp(entries[index].name, "observe:initerm_", 16)) {
                        DWORD arguments[2], got, count, item, value;
                        if (!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got) || got!=8 ||
                            arguments[0]>arguments[1] || (arguments[1]-arguments[0])%4 ||
                            (count=(arguments[1]-arguments[0])/4)>64) {failed=6;break;}
                        fprintf(events,"{\"event\":\"initerm_call\",\"site\":%lu,\"begin\":%lu,\"end\":%lu,\"values\":[",site,arguments[0],arguments[1]);
                        for(item=0;item<count;++item) {
                            if(!ReadProcessMemory(process,(void *)(arguments[0]+item*4),&value,4,&got)||got!=4){failed=6;break;}
                            fprintf(events,"%s%lu",item?",":"",value);
                        }
                        fprintf(events,"]}\n");
                        if(failed)break;
                    } else if(!strncmp(entries[index].name,"observe:delete_",15)) {
                        DWORD argument,got,descriptor[3],descriptor_address;
                        const char *suffix=strrchr(entries[index].name,'_')+1;
                        descriptor_address=base+(!strcmp(suffix,"4")?0x94398:!strcmp(suffix,"8")?0x943a8:0x809d0);
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)descriptor_address,descriptor,12,&got)||got!=12){failed=6;break;}
                        fprintf(events,"{\"event\":\"operator_delete_observation\",\"name\":\"%s\",\"site\":%lu,\"stack_pointer\":%lu,\"argument\":%lu,\"descriptor\":[%lu,%lu,%lu]}\n",entries[index].name,site,context.Esp,argument,descriptor[0],descriptor[1],descriptor[2]);
                    } else if(!strncmp(entries[index].name,"observe:new_",12)) {
                        DWORD argument,got,new_handler,new_mode;
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4){failed=6;break;}
                        if(!ReadProcessMemory(process,(void *)(crt_base+0x42034),&new_handler,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f198),&new_mode,4,&got)||got!=4){failed=6;break;}
                        fprintf(events,"{\"event\":\"operator_new_observation\",\"name\":\"%s\",\"site\":%lu,\"stack_top\":%lu,\"eax\":%lu,\"new_handler\":%lu,\"malloc_new_mode\":%lu}\n",entries[index].name,site,argument,context.Eax,new_handler,new_mode);
                    } else if(!strncmp(entries[index].name,"observe:atexit_",15)) {
                        DWORD argument,got;
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4){failed=6;break;}
                        fprintf(events,"{\"event\":\"atexit_observation\",\"name\":\"%s\",\"site\":%lu,\"stack_top\":%lu,\"eax\":%lu}\n",entries[index].name,site,argument,context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:console_",16)) {
                        DWORD arguments[2],got;
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got)||got!=8){failed=6;break;}
                        fprintf(events,"{\"event\":\"console_observation\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu],\"eax\":%lu}\n",entries[index].name,site,arguments[0],arguments[1],context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:module_",15) || !strncmp(entries[index].name,"observe:proc_",13)) {
                        DWORD arguments[2],got;
                        char requested[512];
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,8,&got)||got!=8){failed=6;break;}
                        fprintf(events,"{\"event\":\"module_probe\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu],\"eax\":%lu",entries[index].name,site,arguments[0],arguments[1],context.Eax);
                        if(!strcmp(entries[index].name,"observe:module_call") || !strcmp(entries[index].name,"observe:proc_call")) {
                            if(!remote_string(arguments[!strcmp(entries[index].name,"observe:proc_call")],requested,sizeof(requested))){failed=6;break;}
                            fprintf(events,",\"requested\":");quoted(requested);
                        }
                        fprintf(events,"}\n");
                    } else if(!strncmp(entries[index].name,"observe:free_",13)) {
                        DWORD got,argument;
                        char text[4096],phase[64],memory[64];
                        int before=!strcmp(entries[index].name,"observe:free_call");
                        if(!ReadProcessMemory(process,(void *)context.Esp,&argument,4,&got)||got!=4||release_count>=16){failed=6;break;}
                        if(before) {
                            released_argument=argument;release_stack=context.Esp;
                            if(!remote_string(argument,text,sizeof(text))){failed=6;break;}
                            sprintf(memory,"free-%u-before.bin",release_count);
                            if(!save_memory(argument,strlen(text)+1,memory)||!rearm_observer("observe:free_return")){failed=6;break;}
                        } else if(context.Esp!=release_stack||!rearm_observer("observe:free_call")){failed=6;break;}
                        sprintf(phase,"free-%u-%s",release_count,before?"before":"after");
                        if(!snapshot_errors(phase)){failed=6;break;}
                        fprintf(events,"{\"event\":\"free_observation\",\"name\":\"%s\",\"site\":%lu,\"index\":%u,\"argument\":%lu,\"argument_slot\":%lu,\"stack_pointer\":%lu",entries[index].name,site,release_count,released_argument,argument,context.Esp);
                        if(before){fprintf(events,",\"text\":");quoted(text);}
                        fprintf(events,"}\n");
                        if(!before)++release_count;
                    } else if(!strncmp(entries[index].name,"observe:library_search_",23)) {
                        DWORD got;
                        char filename[4096];
                        int before=!strcmp(entries[index].name,"observe:library_search_call");
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,library_search_arguments,8,&got)||got!=8){failed=6;break;}
                            library_search_stack=context.Esp;
                        } else if(context.Esp!=library_search_stack+8){failed=6;break;}
                        if(!remote_string(library_search_arguments[0],filename,sizeof(filename))||
                           !save_memory(library_search_arguments[1],260,before?"library-search-before.bin":"library-search-after.bin")){failed=6;break;}
                        fprintf(events,"{\"event\":\"library_search_observation\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu],\"stack_pointer\":%lu,\"filename\":",entries[index].name,site,library_search_arguments[0],library_search_arguments[1],context.Esp);quoted(filename);
                        if(!before)fprintf(events,",\"result\":%lu",context.Eax&255);
                        fprintf(events,"}\n");
                    } else if(!strncmp(entries[index].name,"observe:searchenv_",18)) {
                        DWORD arguments[3],got,codepage,locale,maximum;
                        char filename[4096],variable[256],environment[4096],output[4096];
                        int before=!strcmp(entries[index].name,"observe:searchenv_call");
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,12,&got)||got!=12||
                           !remote_string(arguments[0],filename,sizeof(filename))||!remote_string(arguments[1],variable,sizeof(variable))||
                           !remote_environment(variable,environment,sizeof(environment))||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1c0),&codepage,4,&got)||got!=4||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1b0),&locale,4,&got)||got!=4||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3fba0),&maximum,4,&got)||got!=4||
                           !save_memory(arguments[2],260,before?"searchenv-before.bin":"searchenv-after.bin")||
                           !snapshot_errors(before?"searchenv-before":"searchenv-after")){failed=6;break;}
                        fprintf(events,"{\"event\":\"searchenv_observation\",\"name\":\"%s\",\"site\":%lu,\"arguments\":[%lu,%lu,%lu],\"codepage\":%lu,\"lc_ctype_handle\":%lu,\"mb_cur_max\":%lu,\"filename\":",entries[index].name,site,arguments[0],arguments[1],arguments[2],codepage,locale,maximum);quoted(filename);
                        fprintf(events,",\"variable\":");quoted(variable);fprintf(events,",\"value\":");quoted(environment);
                        if(!before) {
                            if(!remote_string(arguments[2],output,sizeof(output))){failed=6;break;}
                            fprintf(events,",\"result\":");quoted(output);
                        }
                        fprintf(events,"}\n");
                    } else if(!strcmp(entries[index].name,"observe:canonicalize_return")) {
                        char text[4096];
                        if(!remote_string(context.Eax,text,sizeof(text))||!save_memory(context.Eax,strlen(text)+1,"canonicalized-argument.bin")){failed=6;break;}
                        fprintf(events,"{\"event\":\"canonicalize_return\",\"site\":%lu,\"input\":%lu,\"result\":%lu,\"text\":",site,context.Esi,context.Eax);quoted(text);fprintf(events,"}\n");
                    } else if(!strcmp(entries[index].name,"observe:timing_end")) {
                        fprintf(events,"{\"event\":\"timing_end\",\"site\":%lu,\"tick\":%lu}\n",site,context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:link_name_",18)) {
                        DWORD kind=strstr(entries[index].name,"_library_")!=NULL,got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char text[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&name_arguments[kind],4,&got)||got!=4){failed=6;break;}
                            name_stacks[kind]=context.Esp;
                        }
                        if(!remote_string(name_arguments[kind],text,sizeof(text))){failed=6;break;}
                        fprintf(events,"{\"event\":\"output_name_observation\",\"kind\":%lu,\"before\":%d,\"site\":%lu,\"argument\":%lu,\"entry_stack\":%lu,\"stack\":%lu,\"eax\":%lu,\"text\":",kind,before,site,name_arguments[kind],name_stacks[kind],context.Esp,context.Eax);quoted(text);fprintf(events,"}\n");
                    } else if(!strcmp(entries[index].name,"observe:duplicate_return")) {
                        duplicate_result=context.Eax;
                        fprintf(events,"{\"event\":\"duplicate_return\",\"site\":%lu,\"result\":%lu}\n",site,duplicate_result);
                    } else if(!strncmp(entries[index].name,"observe:parser_alloc_",21)) {
                        DWORD got;
                        if(!strcmp(entries[index].name,"observe:parser_alloc_call")) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&parser_allocation_bytes,4,&got)||got!=4){failed=6;break;}
                        } else parser_allocation=context.Eax;
                        fprintf(events,"{\"event\":\"parser_allocation\",\"name\":\"%s\",\"site\":%lu,\"bytes\":%lu,\"result\":%lu}\n",entries[index].name,site,parser_allocation_bytes,parser_allocation);
                    } else if(!strncmp(entries[index].name,"observe:sections_initialize_",28)) {
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        if(before)sections_initialize_stack=context.Esp;
                        if(!rearm_observer(before?"observe:sections_initialize_return":"observe:sections_initialize_call")){failed=6;break;}
                        fprintf(events,"{\"event\":\"sections_initialize_observation\",\"before\":%d,\"site\":%lu,\"this\":%lu,\"entry_stack\":%lu,\"stack\":%lu}\n",before,site,section_initializer_this?section_initializer_this:context.Ecx,sections_initialize_stack,context.Esp);
                    } else if(!strncmp(entries[index].name,"observe:group_find_",19)||!strncmp(entries[index].name,"observe:group_name_",19)||!strncmp(entries[index].name,"observe:group_next_",19)||!strncmp(entries[index].name,"observe:group_end_",18)) {
                        DWORD got,kind=!strncmp(entries[index].name,"observe:group_find_",19)?0:!strncmp(entries[index].name,"observe:group_name_",19)?1:!strncmp(entries[index].name,"observe:group_next_",19)?2:3;
                        DWORD count=group_counts[kind],nargs=kind==0?2:1;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        static const char *calls[]={"observe:group_find_call","observe:group_name_call","observe:group_next_call","observe:group_end_call"};
                        static const char *returns[]={"observe:group_find_return","observe:group_name_return","observe:group_next_return","observe:group_end_return"};
                        const char *rearm=before?returns[kind]:calls[kind];
                        char filename[128],text[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,group_arguments[kind],nargs*4,&got)||got!=nargs*4){failed=6;break;}
                            group_stacks[kind]=context.Esp;
                        }
                        if(kind==2&&strstr(entries[index].name,"_continue_"))rearm=before?"observe:group_next_continue_return":"observe:group_next_continue_call";
                        if(!rearm_observer(rearm)){failed=6;break;}
                        if(kind>=2) {
                            sprintf(filename,"group-%s-%s-%lu.bin",kind==2?"next":"end",before?"before":"after",count);
                            if(!save_memory(group_arguments[kind][0],16,filename)){failed=6;break;}
                        }
                        if(kind<=1&&!remote_string(group_arguments[kind][kind==0?1:0],text,sizeof(text))){failed=6;break;}
                        if(kind==1&&!before) {
                            sprintf(filename,"group-copied-name-%lu.bin",count);
                            if(!save_memory(context.Eax,strlen(text)+1,filename)){failed=6;break;}
                        }
                        fprintf(events,"{\"event\":\"group_observation\",\"kind\":%lu,\"index\":%lu,\"before\":%d,\"site\":%lu,\"arguments\":[%lu",kind,count,before,site,group_arguments[kind][0]);
                        if(nargs==2)fprintf(events,",%lu",group_arguments[kind][1]);
                        fprintf(events,"],\"entry_stack\":%lu,\"stack\":%lu,\"result\":%lu,\"byte_result\":%u",group_stacks[kind],context.Esp,context.Eax,(unsigned)(context.Eax&255));
                        if(kind<=1){fprintf(events,",\"text\":");quoted(text);}fprintf(events,"}\n");
                        if(!before)++group_counts[kind];
                    } else if(!strncmp(entries[index].name,"observe:section_lookup_",23)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,section_lookup_arguments,20,&got)||got!=20){failed=6;break;}
                            section_lookup_stack=context.Esp;
                        }
                        if(!rearm_observer(before?"observe:section_lookup_return":"observe:section_lookup_call")){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_lookup_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"arguments\":[%lu,%lu,%lu,%lu,%lu],\"entry_stack\":%lu,\"stack\":%lu,\"result\":%lu}\n",section_lookup_count,before,site,section_lookup_arguments[0],section_lookup_arguments[1],section_lookup_arguments[2],section_lookup_arguments[3],section_lookup_arguments[4],section_lookup_stack,context.Esp,context.Eax);
                        if(!before)++section_lookup_count;
                    } else if(!strncmp(entries[index].name,"observe:section_name_",21)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char text[128],filename[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&section_name_pointer,4,&got)||got!=4){failed=6;break;}
                            section_name_stack=context.Esp;
                        }
                        if(!remote_string(section_name_pointer,text,sizeof(text))||
                           !rearm_observer(before?"observe:section_name_return":"observe:section_name_call")){failed=6;break;}
                        if(!before) {sprintf(filename,"section-copied-name-%u.bin",section_name_count);if(!save_memory(context.Eax,strlen(text)+1,filename)){failed=6;break;}}
                        fprintf(events,"{\"event\":\"section_name_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"argument\":%lu,\"entry_stack\":%lu,\"stack\":%lu,\"result\":%lu,\"text\":",section_name_count,before,site,section_name_pointer,section_name_stack,context.Esp,context.Eax);quoted(text);fprintf(events,"}\n");
                        if(!before)++section_name_count;
                    } else if(!strncmp(entries[index].name,"observe:section_resolve_",24)||!strncmp(entries[index].name,"observe:group_resolve_",22)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&section_resolve_pointer,4,&got)||got!=4){failed=6;break;}
                            section_resolve_stack=context.Esp;
                        }
                        if(!rearm_observer(!strncmp(entries[index].name,"observe:group_resolve_",22)?(before?"observe:group_resolve_return":"observe:group_resolve_call"):(before?"observe:section_resolve_return":"observe:section_resolve_call"))){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_resolve_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"argument\":%lu,\"entry_stack\":%lu,\"stack\":%lu,\"result\":%lu}\n",section_resolve_count,before,site,section_resolve_pointer,section_resolve_stack,context.Esp,context.Eax);
                        if(!before)++section_resolve_count;
                    } else if(!strncmp(entries[index].name,"observe:section_end_",20)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char filename[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&section_end_pointer,4,&got)||got!=4){failed=6;break;}
                            section_end_stack=context.Esp;
                        }
                        sprintf(filename,"section-end-%s-%u.bin",before?"before":"after",section_end_count);
                        if(!save_memory(section_end_pointer,16,filename)||
                           !rearm_observer(before?"observe:section_end_return":"observe:section_end_call")){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_end_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"argument\":%lu,\"entry_stack\":%lu,\"stack\":%lu}\n",section_end_count,before,site,section_end_pointer,section_end_stack,context.Esp);
                        if(!before)++section_end_count;
                    } else if(!strncmp(entries[index].name,"observe:section_next_",21)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char filename[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&section_next_pointer,4,&got)||got!=4){failed=6;break;}
                            section_next_stack=context.Esp;
                        }
                        sprintf(filename,"section-next-%s-%u.bin",before?"before":"after",section_next_count);
                        if(!save_memory(section_next_pointer,16,filename)||
                           !rearm_observer(strstr(entries[index].name,"_continue_")?(before?"observe:section_next_continue_return":"observe:section_next_continue_call"):(before?"observe:section_next_return":"observe:section_next_call"))){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_next_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"argument\":%lu,\"entry_stack\":%lu,\"stack\":%lu,\"eax\":%lu,\"result\":%u}\n",section_next_count,before,site,section_next_pointer,section_next_stack,context.Esp,context.Eax,(unsigned)(context.Eax&255));
                        if(!before)++section_next_count;
                    } else if(!strncmp(entries[index].name,"observe:group_enum_",19)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char filename[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,group_enum_arguments,8,&got)||got!=8){failed=6;break;}
                            group_enum_stack=context.Esp;
                        }
                        sprintf(filename,"group-enumerator-%s-%u.bin",before?"before":"after",group_enum_count);
                        if(!save_memory(group_enum_arguments[0],16,filename)||
                           !rearm_observer(before?"observe:group_enum_return":"observe:group_enum_call")){failed=6;break;}
                        fprintf(events,"{\"event\":\"group_enum_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"arguments\":[%lu,%lu],\"entry_stack\":%lu,\"stack\":%lu}\n",group_enum_count,before,site,group_enum_arguments[0],group_enum_arguments[1],group_enum_stack,context.Esp);
                        if(!before)++group_enum_count;
                    } else if(!strncmp(entries[index].name,"observe:section_enum_",21)) {
                        DWORD got;
                        int before=strstr(entries[index].name,"_call")!=NULL;
                        char filename[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,section_enum_arguments,8,&got)||got!=8){failed=6;break;}
                            section_enum_stack=context.Esp;
                        }
                        sprintf(filename,"section-enumerator-%s-%u.bin",before?"before":"after",section_enum_count);
                        if(!save_memory(section_enum_arguments[0],16,filename)||
                           !rearm_observer(before?"observe:section_enum_return":"observe:section_enum_call")){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_enum_observation\",\"index\":%u,\"before\":%d,\"site\":%lu,\"arguments\":[%lu,%lu],\"entry_stack\":%lu,\"stack\":%lu}\n",section_enum_count,before,site,section_enum_arguments[0],section_enum_arguments[1],section_enum_stack,context.Esp);
                        if(!before)++section_enum_count;
                    } else if(!strncmp(entries[index].name,"observe:section_flags_",22)) {
                        DWORD kind=strstr(entries[index].name,"_create")!=NULL,got,value,subsystem;
                        BYTE initialize;
                        int before=strstr(entries[index].name,"_call_")!=NULL;
                        char text[128];
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,section_flags_arguments[kind],12,&got)||got!=12){failed=6;break;}
                            section_flags_stacks[kind]=context.Esp;
                        }
                        if(!ReadProcessMemory(process,(void *)section_flags_arguments[kind][0],&value,4,&got)||got!=4||
                           !ReadProcessMemory(process,(void *)(section_flags_arguments[kind][2]+0x44),&subsystem,2,&got)||got!=2||
                           !ReadProcessMemory(process,(void *)(base+0x7f100),&initialize,1,&got)||got!=1||
                           !remote_string(section_flags_arguments[kind][1],text,sizeof(text))||
                           !rearm_observer(kind?(before?"observe:section_flags_return_create":"observe:section_flags_call_create"):(before?"observe:section_flags_return_lookup":"observe:section_flags_call_lookup"))){failed=6;break;}
                        fprintf(events,"{\"event\":\"section_flags_observation\",\"index\":%u,\"kind\":%lu,\"before\":%d,\"site\":%lu,\"arguments\":[%lu,%lu,%lu],\"value\":%lu,\"entry_stack\":%lu,\"stack\":%lu,\"initialize_bss\":%u,\"subsystem\":%lu,\"text\":",section_flags_count,kind,before,site,section_flags_arguments[kind][0],section_flags_arguments[kind][1],section_flags_arguments[kind][2],value,section_flags_stacks[kind],context.Esp,initialize,subsystem&65535);quoted(text);fprintf(events,"}\n");
                        if(!before)++section_flags_count;
                    } else if(!strncmp(entries[index].name,"observe:section_alloc_",22)) {
                        const char *observer=entries[index].name;
                        DWORD kind=observer[strlen(observer)-1]-'0',got;
                        int before=strstr(observer,"_call_")!=NULL;
                        if(kind>1){failed=6;break;}
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,section_arguments[kind],16,&got)||got!=16){failed=6;break;}
                            section_stacks[kind]=context.Esp;
                        }
                        fprintf(events,"{\"event\":\"section_allocation_observation\",\"kind\":%lu,\"before\":%d,\"site\":%lu,\"arguments\":[%lu,%lu,%lu,%lu],\"entry_stack\":%lu,\"stack\":%lu,\"eax\":%lu}\n",kind,before,site,section_arguments[kind][0],section_arguments[kind][1],section_arguments[kind][2],section_arguments[kind][3],section_stacks[kind],context.Esp,context.Eax);
                    } else if(!strncmp(entries[index].name,"observe:virtual_",16)||!strncmp(entries[index].name,"observe:pool_virtual_",21)) {
                        DWORD got;
                        if(strstr(entries[index].name,"_call")!=NULL) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,virtual_arguments,sizeof(virtual_arguments),&got)||got!=sizeof(virtual_arguments)){failed=6;break;}
                            if(!rearm_observer(strstr(entries[index].name,"pool_")?"observe:pool_virtual_return":"observe:virtual_return")){failed=6;break;}
                        } else {
                            char filename[128];
                            sprintf(filename,"virtual-%u-zero.bin",virtual_count);
                            if(!context.Eax||!save_memory(context.Eax,virtual_arguments[1],filename)||
                               (!virtual_count&&!save_memory(context.Eax,virtual_arguments[1],"virtual-first-zero.bin"))||
                               !rearm_observer(strstr(entries[index].name,"pool_")?"observe:pool_virtual_call":"observe:virtual_call")){failed=6;break;}
                        }
                        fprintf(events,"{\"event\":\"virtual_observation\",\"name\":\"%s\",\"site\":%lu,\"index\":%u,\"arguments\":[%lu,%lu,%lu,%lu],\"eax\":%lu}\n",entries[index].name,site,virtual_count,virtual_arguments[0],virtual_arguments[1],virtual_arguments[2],virtual_arguments[3],context.Eax);
                        if(strstr(entries[index].name,"_return")!=NULL)++virtual_count;
                    } else if((!strncmp(entries[index].name,"observe:mbs_",12) || !strncmp(entries[index].name,"observe:mbspbrk_",16))) {
                        DWORD arguments[3],got,codepage,locale_handle,mb_cur_max,locale_codepage;
                        unsigned argument_count=!strncmp(entries[index].name,"observe:mbspbrk_",16)?2:3;
                        unsigned char table[257];
                        char left[1024],right[1024];
                        if(!ReadProcessMemory(process,(void *)context.Esp,arguments,argument_count*4,&got)||got!=argument_count*4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1c0),&codepage,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1c8),table,sizeof(table),&got)||got!=sizeof(table) ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f1b0),&locale_handle,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3fba0),&mb_cur_max,4,&got)||got!=4 ||
                           !ReadProcessMemory(process,(void *)(crt_base+0x3f8f8),&locale_codepage,4,&got)||got!=4) {failed=6;break;}
                        if(!remote_string(arguments[0],left,sizeof(left))||!remote_string(arguments[1],right,sizeof(right))){failed=6;break;}
                        if(argument_count==2)
                            fprintf(events,"{\"event\":\"mbs_observation\",\"name\":\"%s\",\"site\":%lu,\"codepage\":%lu,\"eax\":%lu,\"arguments\":[%lu,%lu],\"left\":",entries[index].name,site,codepage,context.Eax,arguments[0],arguments[1]);
                        else
                            fprintf(events,"{\"event\":\"mbs_observation\",\"name\":\"%s\",\"site\":%lu,\"codepage\":%lu,\"eax\":%lu,\"arguments\":[%lu,%lu,%lu],\"left\":",entries[index].name,site,codepage,context.Eax,arguments[0],arguments[1],arguments[2]);
                        quoted(left);
                        fprintf(events,",\"lc_ctype_handle\":%lu,\"mb_cur_max\":%lu,\"locale_codepage\":%lu",locale_handle,mb_cur_max,locale_codepage);
                        fprintf(events,",\"right\":");quoted(right);fprintf(events,",\"mbctype_hex\":\"");
                        for(got=0;got<sizeof(table);++got)fprintf(events,"%02x",table[got]);
                        fprintf(events,"\"}\n");
                    } else if(!strncmp(entries[index].name,"observe:system_",15) || !strncmp(entries[index].name,"observe:memory_",15)) {
                        DWORD pointer,size,got,written;
                        unsigned char observed[36];
                        HANDLE saved;
                        char path[1024];
                        int system=entries[index].name[8]=='s';
                        int before=!strcmp(entries[index].name+15,"before");
                        size=system?36:32;
                        if(before) {
                            if(!ReadProcessMemory(process,(void *)context.Esp,&pointer,4,&got)||got!=4){failed=6;break;}
                        } else pointer=context.Esp+(system?0x20:0);
                        if(!ReadProcessMemory(process,(void *)pointer,observed,size,&got)||got!=size){failed=6;break;}
                        sprintf(path,"%s\\%s-%s.bin",output_directory,system?"system":"memory",before?"before":"after");
                        saved=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
                        if(saved==INVALID_HANDLE_VALUE||!WriteFile(saved,observed,size,&written,NULL)||written!=size){failed=6;break;}
                        CloseHandle(saved);
                        fprintf(events,"{\"event\":\"system_observation\",\"name\":\"%s\",\"site\":%lu,\"buffer\":%lu,\"bytes\":%lu,\"eax\":%lu}\n",entries[index].name,site,pointer,size,context.Eax);
                    } else {
                        if(!strcmp(entries[index].name,"_configure_link_options@12")) {
                            DWORD arguments[3],got;
                            if(!ReadProcessMemory(process,(void *)(context.Esp+4),arguments,12,&got)||got!=12||
                               !arguments[0]||arguments[1]||arguments[2]){failed=6;break;}
                            configuration_context=arguments[0];
                            fprintf(events,"{\"event\":\"configuration_arguments\",\"context\":%lu,\"module\":%lu,\"file\":%lu}\n",arguments[0],arguments[1],arguments[2]);
                        }
                        if(!strcmp(entries[index].name,"LinkContext::PassOne")) {
                            pass_one_receiver=context.Ecx;
                            fprintf(events,"{\"event\":\"pass_one_entry\",\"this\":%lu,\"stack_pointer\":%lu,\"stack_formals\":[],\"other_hidden_inputs\":[]}\n",pass_one_receiver,context.Esp);
                        }
                        if(!strcmp(entries[index].name,"LinkContext::InitializeSections")) {
                            section_initializer_this=context.Ecx;
                            fprintf(events,"{\"event\":\"section_initializer_entry\",\"this\":%lu}\n",section_initializer_this);
                        }
                        if(!strcmp(entries[index].name,"_initialize_output_modules@4")) {
                            DWORD got;
                            if(!ReadProcessMemory(process,(void *)(context.Esp+4),&module_output_slot,4,&got)||got!=4||
                               !ReadProcessMemory(process,(void *)module_output_slot,&module_output_initial,4,&got)||got!=4){failed=6;break;}
                            fprintf(events,"{\"event\":\"output_modules_entry\",\"result_slot\":%lu,\"initial\":%lu}\n",module_output_slot,module_output_initial);
                        }
                        if(!strcmp(entries[index].name,"?FullLink@LinkContext@@QAEHPAE@Z")) {
                            DWORD got;
                            full_link_receiver=context.Ecx;full_link_entry_stack=context.Esp;
                            if(!ReadProcessMemory(process,(void *)(context.Esp+4),&full_link_output,4,&got)||got!=4||
                               !ReadProcessMemory(process,(void *)full_link_output,&full_link_initial,1,&got)||got!=1||
                               !save_memory(full_link_output,1,"pack-requested-before.bin")){failed=6;break;}
                            fprintf(events,"{\"event\":\"full_link_entry\",\"this\":%lu,\"pack_requested\":%lu,\"stack_pointer\":%lu,\"initial\":%u}\n",full_link_receiver,full_link_output,full_link_entry_stack,full_link_initial);
                        }
                        fprintf(events, "{\"event\":\"entry\",\"name\":\"%s\",\"rva\":%lu}\n",
                                   entries[index].name, entries[index].rva);
                    }
                }
            } else {
                snapshot(site); failed = 8; break;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            result = event.u.ExitProcess.dwExitCode; done = 1;
            fprintf(events, "{\"event\":\"exit\",\"status\":%lu}\n", result);
        }
        fflush(events);
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status)) { failed = 6; break; }
    }
    if (result == 999 && process) TerminateProcess(process, 99);
    fprintf(events, "{\"event\":\"summary\",\"failed\":%d,\"status\":%lu}\n", failed, result);
    fclose(events); CloseHandle(output); CloseHandle(error);
    if (process) CloseHandle(process);
    if (thread) CloseHandle(thread);
    return failed;
}
