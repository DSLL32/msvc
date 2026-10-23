/* Explicit original-only reference capture. Never linked into the reconstruction
   or launched by a fixture. All breakpoint/mask bytes are analysis inputs. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef LONG (WINAPI *QueryProcess)(HANDLE,ULONG,void *,ULONG,ULONG *);
typedef struct BasicInfo { LONG status; void *peb; ULONG affinity; LONG priority; ULONG pid,parent; } BasicInfo;
typedef struct String32 { unsigned short length,maximum; void *buffer; } String32;
typedef struct Process { DWORD pid,parent,base; HANDLE handle; int active,target,packer,loader,copy; DWORD copy_output,copy_error; } Process;
typedef struct Thread { DWORD pid,tid,teb; HANDLE handle; int active; } Thread;
typedef struct Entry { DWORD rva; unsigned byte,seen; char name[128]; } Entry;
static Process processes[64];
static Thread threads[256];
static Entry entries[2048];
static unsigned entry_count;
static DWORD current_exception_code;
static unsigned char *text;
static DWORD text_rva,text_size;
static FILE *log_file;
typedef struct CopyPlan { char image[4096],command[4096],cwd[4096],source[4096],destination[4096]; } CopyPlan;
static CopyPlan copy_plans[8];
static unsigned copy_count,next_copy;
static int cancel_public,cancel_dispatched,cancel_registered;
static DWORD cancel_handler,cancel_main_tid;

static void quoted(const char *s)
{
    unsigned char c;
    fputc('"',log_file);
    while (*s) {
        c=(unsigned char)*s++;
        if (c=='"' || c=='\\') fputc('\\',log_file);
        if (c<32) fprintf(log_file,"\\u%04x",c);
        else fputc(c,log_file);
    }
    fputc('"',log_file);
}

static int remote_string(HANDLE process,void *parameters,int offset,char *out)
{
    String32 value;
    WCHAR wide[2048];
    DWORD got;
    int n;
    if (!ReadProcessMemory(process,(char *)parameters+offset,&value,sizeof(value),&got) ||
        got!=sizeof(value) || value.length>=sizeof(wide)) return 0;
    if (!ReadProcessMemory(process,value.buffer,wide,value.length,&got) || got!=value.length)
        return 0;
    n=WideCharToMultiByte(CP_ACP,0,wide,value.length/2,out,4095,NULL,NULL);
    out[n]=0;
    return n!=0;
}

/* Optional narrow external-copy capture. Only explicitly pinned cmd children
 * are allowed; this helper never participates in native fixture execution. */
static int plan_line(FILE *file,char *line)
{
    unsigned n;
    if (!fgets(line,4096,file)) return 0;
    n=strlen(line);
    if (!n || line[n-1]!='\n') return 0;
    line[--n]=0;
    if (n && line[n-1]=='\r') line[--n]=0;
    return n!=0;
}
static int copy_environment(HANDLE process,void *parameters)
{
    WCHAR wide[32768];
    void *environment;
    DWORD got;
    unsigned n,start=0,i;
    char line[4096];
    const char *keys[]={"COMSPEC=","LIB=","LINK_REPRO=","TMP=","TEMP=","COPYCMD=",
        "CL=","_CL_=","LINK=","_LINK_=","MSC_CMD_FLAGS=","LINK32_DB="};
    int written=0,length;
    if (!ReadProcessMemory(process,(char *)parameters+0x48,&environment,4,&got) || got!=4) return 0;
    fprintf(log_file,"[");
    for (n=0;n<32768;++n) {
        if (!ReadProcessMemory(process,(char *)environment+n*2,&wide[n],2,&got) || got!=2) return 0;
        if (!wide[n]) {
            if (n==start) {fprintf(log_file,"]");return 1;}
            length=WideCharToMultiByte(CP_ACP,0,wide+start,n-start,line,4095,NULL,NULL);
            if (!length) return 0;
            line[length]=0;
            for (i=0;i<sizeof(keys)/sizeof(keys[0]);++i)
                if (!_strnicmp(line,keys[i],strlen(keys[i]))) {
                    if (written++) fprintf(log_file,",");quoted(line);break;
                }
            start=n+1;
        }
    }
    return 0;
}

static int write_code(Process *process,DWORD rva,const void *data,DWORD size)
{
    DWORD old,restored,written;
    void *where=(void *)(process->base+rva);
    return VirtualProtectEx(process->handle,where,size,PAGE_EXECUTE_READWRITE,&old) &&
        WriteProcessMemory(process->handle,where,(void *)data,size,&written) && written==size &&
        FlushInstructionCache(process->handle,where,size) &&
        VirtualProtectEx(process->handle,where,size,old,&restored);
}

/* Optional original-only data watch: auxiliary tail reused below the import
 * builder frame. Enabled only by an explicit analysis_aux_watch entry sample. */
static DWORD auxiliary_watch;
static void auxiliary_watch_start(Process *process,Thread *thread,CONTEXT *context)
{
    DWORD got,value;
    auxiliary_watch=context->Esp-0x1a4;
    context->ContextFlags=CONTEXT_FULL|CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(thread->handle,context);
    context->Dr0=auxiliary_watch;
    context->Dr6=0;context->Dr7=0x50001;
    SetThreadContext(thread->handle,context);
    ReadProcessMemory(process->handle,(void *)auxiliary_watch,&value,4,&got);
    fprintf(log_file,"{\"event\":\"auxiliary-watch-start\",\"address\":%lu,\"value\":%lu}\n",auxiliary_watch,value);
    fflush(log_file);
}
static void auxiliary_watch_event(Process *process,Thread *thread)
{
    CONTEXT context;
    DWORD got,value,stack[8],n,owner[10];
    char name[256];
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(thread->handle,&context);
    ReadProcessMemory(process->handle,(void *)auxiliary_watch,&value,4,&got);
    ReadProcessMemory(process->handle,(void *)context.Esp,stack,sizeof(stack),&got);
    fprintf(log_file,"{\"event\":\"auxiliary-watch-write\",\"eip\":%lu,\"address\":%lu,\"value\":%lu,\"esp\":%lu,\"stack\":[",context.Eip,auxiliary_watch,value,context.Esp);
    for(n=0;n<8;++n)fprintf(log_file,"%s%lu",n?",":"",stack[n]);
    fprintf(log_file,"],\"edi\":%lu,\"esi\":%lu",context.Edi,context.Esi);
    if (context.Eip==0x55bb2 && ReadProcessMemory(process->handle,(void *)context.Edi,owner,sizeof(owner),&got) && got==sizeof(owner)) {
        memset(name,0,sizeof(name));ReadProcessMemory(process->handle,(void *)owner[0],name,255,&got);
        fprintf(log_file,",\"owner_name\":");quoted(name);
        fprintf(log_file,",\"owner_flags\":%lu,\"owner_logical_handle\":%lu,\"owner_descriptor\":%lu",owner[1],owner[2],owner[4]);
    }
    fprintf(log_file,"}\n");fflush(log_file);
    context.Dr6=0;SetThreadContext(thread->handle,&context);
}

/* First observed public time boundary at each import-archive named caller. */
static DWORD clock_pointer[3],clock_stack[3],clock_errno[3];
static void clock_sample(Process *process,CONTEXT *context,DWORD site)
{
    DWORD index,pointer,got,result,after_errno;
    const char *caller;
    if (site==0x28f61 || site==0x28f66) index=0;
    else if (site==0x2fbe8 || site==0x2fbed) index=1;
    else if (site==0x1b099 || site==0x1b09e) index=2;
    else return;
    caller=index==2?"link_passes":(index?"build_archive_header":"build_import_library");
    if (site==0x28f61 || site==0x2fbe8 || site==0x1b099) {
        if (!ReadProcessMemory(process->handle,(void *)context->Esp,&pointer,4,&got) || got!=4) return;
        clock_pointer[index]=pointer;
        clock_stack[index]=context->Esp;
        if (!ReadProcessMemory(process->handle,(void *)0x852d8,&clock_errno[index],4,&got) || got!=4) return;
    } else {
        pointer=clock_pointer[index];
        if (!pointer || !ReadProcessMemory(process->handle,(void *)pointer,&result,4,&got) || got!=4) return;
        if (!ReadProcessMemory(process->handle,(void *)0x852d8,&after_errno,4,&got) || got!=4) return;
        fprintf(log_file,"{\"event\":\"clock_boundary\",\"caller\":\"%s\",\"result\":%lu,\"output\":%lu,\"argument_nonnull\":%d,\"errno_before\":%lu,\"errno_after\":%lu,\"caller_stack_unchanged\":%d}\n",
            caller,context->Eax,result,pointer!=0,clock_errno[index],after_errno,context->Esp==clock_stack[index]);
        fflush(log_file);
    }
}

/* Explicit first-call analysis of the resolved OS relocation primitive. */
static DWORD relocation_page,relocation_entries,relocation_pointer,relocation_delta,relocation_stack;
static unsigned short relocation_words[256];
static void relocation_sample(Process *process,CONTEXT *context,DWORD site)
{
    DWORD args[4],got,value,procedure,n;
    if (site==0x50379) {
        if (!ReadProcessMemory(process->handle,(void *)context->Esp,args,16,&got) || got!=16 || args[1]>256) return;
        relocation_page=args[0];relocation_entries=args[1];relocation_pointer=args[2];
        relocation_delta=args[3];relocation_stack=context->Esp;
        if (!ReadProcessMemory(process->handle,(void *)relocation_pointer,relocation_words,relocation_entries*2,&got) || got!=relocation_entries*2) return;
        if (!ReadProcessMemory(process->handle,(void *)0x6d468,&procedure,4,&got) || got!=4) return;
        fprintf(log_file,"{\"event\":\"relocation-primitive-before\",\"procedure\":%lu,\"page\":%lu,\"array\":%lu,\"count\":%lu,\"delta\":%lu,\"esp\":%lu,\"records\":[",
            procedure,relocation_page,relocation_pointer,relocation_entries,relocation_delta,relocation_stack);
    } else if (site==0x5037f) {
        fprintf(log_file,"{\"event\":\"relocation-primitive-after\",\"result\":%lu,\"result_array_offset\":%lu,\"stack_cleanup\":%lu,\"records\":[",
            context->Eax,context->Eax-relocation_pointer,context->Esp-relocation_stack);
    } else return;
    for (n=0;n<relocation_entries;++n) {
        value=0;
        if ((relocation_words[n]>>12)==3 &&
            (!ReadProcessMemory(process->handle,(void *)(relocation_page+(relocation_words[n]&0xfff)),&value,4,&got) || got!=4)) return;
        fprintf(log_file,"%s{\"word\":%u,\"type\":%u,\"offset\":%u,\"value\":%lu}",n?",":"",
            relocation_words[n],relocation_words[n]>>12,relocation_words[n]&0xfff,value);
    }
    fprintf(log_file,"]}\n");fflush(log_file);
}

/* One public base16 conversion in the frozen REBASE argument inventory. */
static DWORD strtoul_input,strtoul_end_location,strtoul_input_length;
static void strtoul_sample(Process *process,CONTEXT *context,DWORD site)
{
    DWORD args[4],got,end,n;
    char number[64];
    if (site==0x541d0) {
        if (!ReadProcessMemory(process->handle,(void *)context->Esp,args,16,&got) || got!=16) return;
        strtoul_input=args[1];strtoul_end_location=args[2];
        for(n=0;n<63;++n) {
            if (!ReadProcessMemory(process->handle,(void *)(strtoul_input+n),number+n,1,&got) || got!=1) return;
            if (!number[n]) break;
        }
        if(n==63)return;
        strtoul_input_length=n;
        fprintf(log_file,"{\"event\":\"strtoul-before\",\"input\":");quoted(number);
        fprintf(log_file,",\"base\":%lu,\"end_pointer_present\":%d}\n",args[3],strtoul_end_location!=0);
    } else if (site==0x4ff96) {
        if (!ReadProcessMemory(process->handle,(void *)strtoul_end_location,&end,4,&got) || got!=4) return;
        fprintf(log_file,"{\"event\":\"strtoul-after\",\"result\":%lu,\"end_offset\":%lu,\"end_points_into_input\":%d}\n",
            context->Eax,end-strtoul_input,end>=strtoul_input && end<=strtoul_input+strtoul_input_length);
    }
    fflush(log_file);
}

/* Explicit editor API observations; no oracle/fixture calls this capture helper. */
static DWORD bind_api_args[4],bind_api_stack;
static void bind_api_sample(Process *process,Thread *thread,CONTEXT *context,DWORD site)
{
    DWORD error=0xffffffffUL,crt_errno=0xffffffffUL,got,i,count=0;
    unsigned short system_time[8];
    DWORD file_time[2];
    const char *name=NULL;
    int before=0;
    switch(site) {
    case 0x4f81f:name="FlushViewOfFile";before=1;count=2;break;
    case 0x4f824:name="FlushViewOfFile";break;
    case 0x4f828:name="GetSystemTime";before=1;count=1;break;
    case 0x4f82d:name="GetSystemTime";break;
    case 0x4f835:name="SystemTimeToFileTime";before=1;count=2;break;
    case 0x4f83a:name="SystemTimeToFileTime";break;
    case 0x4f84d:name="SetFileTime";before=1;count=4;break;
    case 0x4f852:name="SetFileTime";break;
    default:return;
    }
    if(before) {
        memset(bind_api_args,0,sizeof(bind_api_args));
        if(!ReadProcessMemory(process->handle,(void *)context->Esp,bind_api_args,count*4,&got) || got!=count*4)return;
        bind_api_stack=context->Esp;
    }
    if(!ReadProcessMemory(process->handle,(void *)(thread->teb+0x34),&error,4,&got) || got!=4)return;
    if(!ReadProcessMemory(process->handle,(void *)0x852d8,&crt_errno,4,&got) || got!=4)return;
    fprintf(log_file,"{\"event\":\"editor-api\",\"name\":");quoted(name);
    fprintf(log_file,",\"before\":%d,\"result\":%lu,\"errno\":%lu,\"last_error\":%lu,\"cleanup\":%lu,\"args\":[",before,context->Eax,crt_errno,error,context->Esp-bind_api_stack);
    for(i=0;i<4;++i)fprintf(log_file,"%s%lu",i?",":"",bind_api_args[i]);
    fprintf(log_file,"]");
    if((site==0x4f82d || site==0x4f835) && ReadProcessMemory(process->handle,(void *)bind_api_args[0],system_time,16,&got) && got==16) {
        fprintf(log_file,",\"system_time\":[");
        for(i=0;i<8;++i)fprintf(log_file,"%s%u",i?",":"",system_time[i]);
        fprintf(log_file,"]");
    }
    if((site==0x4f83a || site==0x4f84d || site==0x4f852) && ReadProcessMemory(process->handle,(void *)bind_api_args[site==0x4f83a?1:3],file_time,8,&got) && got==8)
        fprintf(log_file,",\"file_time\":[%lu,%lu]",file_time[0],file_time[1]);
    fprintf(log_file,"}\n");fflush(log_file);
}

/* Original-only tempnam errno attribution at its actual resource caller. */
static void resource_tempnam_sample(Process *process,Thread *thread,CONTEXT *context,DWORD site)
{
    DWORD got,args[2],error,doserror,last; char directory[512],prefix[512],result[512];
    ReadProcessMemory(process->handle,(void *)0x852dc,&doserror,4,&got);
    ReadProcessMemory(process->handle,(void *)(thread->teb+0x34),&last,4,&got);
    if (site==0x2197d) {
        ReadProcessMemory(process->handle,(void *)0x852d8,&error,4,&got);
        fprintf(log_file,"{\"event\":\"resource_unlink_return\",\"result\":%lu,\"errno\":%lu,\"doserrno\":%lu,\"last_error\":%lu}\n",context->Eax,error,doserror,last);fflush(log_file);return;
    }
    if (site==0x5b321 || site==0x5b51e) {
        DWORD counter=0,attributes;
        memset(result,0,sizeof(result));
        ReadProcessMemory(process->handle,(void *)(site==0x5b321?context->Eax:context->Ebx),result,511,&got);
        ReadProcessMemory(process->handle,(void *)0x852d8,&error,4,&got);
        ReadProcessMemory(process->handle,(void *)0x8640c,&counter,4,&got);
        attributes=GetFileAttributesA(result);
        fprintf(log_file,"{\"event\":\"resource_tempnam_path\",\"site\":%lu,\"errno\":%lu,\"eax\":%lu,\"counter\":%lu,\"attributes\":%lu,\"path\":",site,error,context->Eax,counter,attributes);
        quoted(result);fprintf(log_file,",\"doserrno\":%lu,\"last_error\":%lu}\n",doserror,last);fflush(log_file);return;
    }
    if (site!=0x21027 && site!=0x2102c && site!=0x20b68 && site!=0x20b6d) return;
    ReadProcessMemory(process->handle,(void *)0x852d8,&error,4,&got);
    fprintf(log_file,"{\"event\":\"resource_tempnam\",\"before\":%d,\"errno\":%lu",site==0x21027 || site==0x20b68,error);
    if (site==0x21027 || site==0x20b68) {
        memset(directory,0,sizeof(directory));memset(prefix,0,sizeof(prefix));
        ReadProcessMemory(process->handle,(void *)context->Esp,args,8,&got);
        ReadProcessMemory(process->handle,(void *)args[0],directory,511,&got);
        ReadProcessMemory(process->handle,(void *)args[1],prefix,511,&got);
        fprintf(log_file,",\"directory\":");quoted(directory);
        fprintf(log_file,",\"prefix\":");quoted(prefix);
    } else {
        memset(result,0,sizeof(result));
        ReadProcessMemory(process->handle,(void *)context->Eax,result,511,&got);
        fprintf(log_file,",\"result\":");quoted(result);
    }
    fprintf(log_file,",\"doserrno\":%lu,\"last_error\":%lu}\n",doserror,last);fflush(log_file);
}

/* Complete ABI inputs for the active named-resource diagnostic frontiers.
 * Output display contents are not inputs: the original writes, never reads,
 * exactly count bytes. Its1024-byte caller-owned extent is independently proved. */
static void resource_abi_sample(Process *process,CONTEXT *context,DWORD *stack)
{
    DWORD got,n,value;unsigned short units[256],length;
    char file[512],argument[512];
    if (context->Eip==0x45181 || context->Eip==0x45180) {
        if (!stack[3] || stack[3]>256 || !ReadProcessMemory(process->handle,(void *)stack[2],units,stack[3]*2,&got) || got!=stack[3]*2) return;
        value=stack[2]-16;
        if (!ReadProcessMemory(process->handle,(void *)(value+14),&length,2,&got) || got!=2) return;
        fprintf(log_file,"{\"event\":\"resource_abi\",\"logical\":\"resource_name_to_ansi\",\"caller\":%lu,\"formals\":{\"count\":%lu,\"input_is_type_name\":%d,\"input_is_name\":%d,\"count_matches_owner\":%d,\"output_is_insert_resource_display\":%d,\"output_capacity\":1024,\"utf16\":[",stack[0],stack[3],stack[2]==context->Edi+16,stack[2]==context->Esi+16,stack[3]==(DWORD)length+1,stack[1]==context->Esp+48 && stack[0]>=0x42f90 && stack[0]<0x432d0);
        for(n=0;n<stack[3];++n)fprintf(log_file,"%s%u",n?",":"",units[n]);
        fprintf(log_file,"]}}\n");
    } else if (context->Eip==0x451e1 || context->Eip==0x451e0) {
        fprintf(log_file,"{\"event\":\"resource_abi\",\"logical\":\"resource_unicode_to_ansi_byte\",\"caller\":%lu,\"formals\":{\"character\":%u,\"argument_word\":%lu}}\n",stack[0],(unsigned short)stack[1],stack[1]);
    } else if (context->Eip==0x52841 || context->Eip==0x52840) {
        memset(file,0,sizeof(file));ReadProcessMemory(process->handle,(void *)stack[1],file,511,&got);
        fprintf(log_file,"{\"event\":\"resource_abi\",\"logical\":\"recon_unlink\",\"caller\":%lu,\"formals\":{\"file\":",stack[0]);quoted(file);fprintf(log_file,"}}\n");
    } else if (context->Eip==0x218d1 || context->Eip==0x218d0) {
        memset(file,0,sizeof(file));memset(argument,0,sizeof(argument));
        ReadProcessMemory(process->handle,(void *)stack[1],file,511,&got);
        ReadProcessMemory(process->handle,(void *)stack[3],argument,511,&got);
        fprintf(log_file,"{\"event\":\"resource_abi\",\"logical\":\"fatal_message\",\"caller\":%lu,\"formals\":{\"file\":",stack[0]);quoted(file);
        fprintf(log_file,",\"message\":%lu,\"first_variadic_string\":",stack[2]&0xffff);quoted(argument);
        fprintf(log_file,"}}\n");
    }
    fflush(log_file);
}

/* Public EDIT rejection/callback inputs: no hidden/register formals. */
static DWORD editor_message_output,editor_message_capacity;
static void editor_error_abi_sample(Process *process,CONTEXT *context,DWORD *stack)
{
    const char *logical;
    DWORD got;char first[512],second[512];
    if (context->Eip==0x287c6 || context->Eip==0x287c5) {
        DWORD error;
        memset(first,0,sizeof(first));
        ReadProcessMemory(process->handle,(void *)editor_message_output,first,editor_message_capacity,&got);
        ReadProcessMemory(process->handle,(void *)0x852d8,&error,4,&got);
        fprintf(log_file,"{\"event\":\"editor_snprintf_result\",\"result\":%lu,\"errno\":%lu,\"capacity\":%lu,\"text\":",context->Eax,error,editor_message_capacity);quoted(first);fprintf(log_file,"}\n");fflush(log_file);return;
    }
    if (context->Eip==0x52f21 || context->Eip==0x52f20) {
        editor_message_output=stack[1];editor_message_capacity=stack[2];
        if (editor_message_capacity>340) return;
        memset(first,0,sizeof(first));memset(second,0,sizeof(second));
        ReadProcessMemory(process->handle,(void *)stack[3],first,511,&got);
        ReadProcessMemory(process->handle,(void *)stack[4],second,511,&got);
        fprintf(log_file,"{\"event\":\"editor_error_abi\",\"logical\":\"recon_snprintf\",\"caller\":%lu,\"formals\":{\"output_is_error_message\":%d,\"output_extent\":340,\"capacity\":%lu,\"format\":",stack[0],stack[1]==context->Esp+20 && stack[0]==0x287c5,stack[2]);quoted(first);
        fprintf(log_file,",\"value\":");quoted(second);fprintf(log_file,"}}\n");fflush(log_file);return;
    }
    if (context->Eip==0x287a1 || context->Eip==0x287a0) logical="editor_rebase_error";
    else if (context->Eip==0x28751 || context->Eip==0x28750) logical="editor_bind_error";
    else if (context->Eip==0x26b11 || context->Eip==0x26b10) logical="file_failure";
    else return;
    memset(first,0,sizeof(first));memset(second,0,sizeof(second));
    if (stack[1]) ReadProcessMemory(process->handle,(void *)stack[1],first,511,&got);
    if (stack[logical[0]=='f'?3:2]) ReadProcessMemory(process->handle,(void *)stack[logical[0]=='f'?3:2],second,511,&got);
    fprintf(log_file,"{\"event\":\"editor_error_abi\",\"logical\":");quoted(logical);
    fprintf(log_file,",\"caller\":%lu,\"formals\":{",stack[0]);
    if (logical[0]=='f') {
        fprintf(log_file,"\"file\":");if(stack[1])quoted(first);else fprintf(log_file,"null");
        fprintf(log_file,",\"message\":%lu,\"detail\":",stack[2]);if(stack[3])quoted(second);else fprintf(log_file,"null");
    } else {
        fprintf(log_file,"\"format\":");if(stack[1])quoted(first);else fprintf(log_file,"null");
        fprintf(log_file,",\"value\":");if(stack[2])quoted(second);else fprintf(log_file,"null");
    }
    fprintf(log_file,"}}\n");fflush(log_file);
}

/* Actual public OMF continuation/formal and consumed-global observations. */
static void omf_continuation_sample(Process *process,CONTEXT *context,DWORD *stack)
{
    DWORD site=context->Eip&~1,got,pass,root,bucket,node,words[4],i,count,size,total,file,stream[2];
    unsigned short wide;short remaining;unsigned char bytes[256];char text[256],program[256];
    const char *logical;
    if(site==0x4817c){
        DWORD out[8],error,dos;
        ReadProcessMemory(process->handle,(void *)0x85070,out,32,&got);
        ReadProcessMemory(process->handle,(void *)0x852d8,&error,4,&got);
        ReadProcessMemory(process->handle,(void *)0x852dc,&dos,4,&got);
        count=out[0]-out[2];if(count>256)return;
        ReadProcessMemory(process->handle,(void *)out[2],bytes,count,&got);
        fprintf(log_file,"{\"event\":\"omf_vfprintf_result\",\"result\":%lu,\"errno\":%lu,\"doserrno\":%lu,\"count\":%ld,\"flags\":%lu,\"file\":%lu,\"buffer_size\":%lu,\"buffered\":[",context->Eax,error,dos,(long)out[1],out[3],out[4],out[6]);
        for(i=0;i<count;++i)fprintf(log_file,"%s%u",i?",":"",bytes[i]);fprintf(log_file,"]}\n");fflush(log_file);return;
    }
    if(site==0x472d0)logical="omf_insert_relocation_symbol";
    else if(site==0x47350)logical="omf_lookup_relocation_symbol";
    else if(site==0x48aa0)logical="omf_extend_section";
    else if(site==0x45f20)logical="omf_record_symbol_lines";
    else if(site==0x48150)logical="omf_warning";
    else if(site==0x535f0)logical="recon_vfprintf";
    else return;
    ReadProcessMemory(process->handle,(void *)0x77df4,&pass,4,&got);
    fprintf(log_file,"{\"event\":\"omf_continuation_abi\",\"logical\":");quoted(logical);
    fprintf(log_file,",\"caller\":%lu,\"pass\":%lu,\"formals\":{",stack[0],pass);
    if(site==0x472d0||site==0x47350){
        bucket=(stack[1]+stack[2])%211;
        ReadProcessMemory(process->handle,(void *)0x65200,&root,4,&got);
        ReadProcessMemory(process->handle,(void *)(root+bucket*4),&node,4,&got);
        fprintf(log_file,"\"symbol\":%lu,\"offset\":%lu},\"bucket\":%lu,\"nodes\":[",stack[1],stack[2],bucket);
        count=0;
        while(node && count<32){
            ReadProcessMemory(process->handle,(void *)node,words,16,&got);
            fprintf(log_file,"%s{\"symbol\":%lu,\"offset\":%lu",count?",":"",words[0],words[1]);
            if(site==0x47350)fprintf(log_file,",\"resolved\":%lu",words[2]);
            fprintf(log_file,"}");node=words[3];++count;
        }
        fprintf(log_file,"],\"terminated\":%d",!node);
    }else if(site==0x48aa0){
        if(stack[4]>256)return;
        ReadProcessMemory(process->handle,(void *)stack[3],bytes,stack[4],&got);
        ReadProcessMemory(process->handle,(void *)(0x61dec+((short)stack[1]-1)*40+16),&size,4,&got);
        ReadProcessMemory(process->handle,(void *)0x77e1c,&total,4,&got);
        fprintf(log_file,"\"section\":%d,\"offset\":%lu,\"length\":%lu,\"data\":[",(short)stack[1],stack[2],stack[4]);
        for(i=0;i<stack[4];++i)fprintf(log_file,"%s%u",i?",":"",bytes[i]);
        fprintf(log_file,"]},\"prior_section_size\":%lu,\"prior_total_bytes\":%lu",size,total);
    }else if(site==0x45f20){
        ReadProcessMemory(process->handle,(void *)0x6f19c,&remaining,2,&got);
        ReadProcessMemory(process->handle,(void *)0x6f198,&wide,2,&got);
        ReadProcessMemory(process->handle,(void *)0x77e0c,&file,4,&got);
        ReadProcessMemory(process->handle,(void *)file,stream,8,&got);
        if(remaining<0||remaining>256||stream[1]<(DWORD)remaining)return;
        ReadProcessMemory(process->handle,(void *)stream[0],bytes,remaining,&got);
        fprintf(log_file,"},\"remaining\":%d,\"wide\":%u,\"unread_record\":[",remaining,wide);
        for(i=0;i<(DWORD)remaining;++i)fprintf(log_file,"%s%u",i?",":"",bytes[i]);fprintf(log_file,"]");
    }else if(site==0x535f0){
        memset(text,0,sizeof(text));ReadProcessMemory(process->handle,(void *)stack[2],text,255,&got);
        fprintf(log_file,"\"stream_is_stdout\":%d,\"format\":",stack[1]==0x85070);quoted(text);
        fprintf(log_file,",\"variadic_is_warning_area\":%d,\"consumed_variadic_slots\":0},\"stream\":{",stack[0]==0x4817c && stack[3]==context->Esp+24);
        { DWORD out[8];ReadProcessMemory(process->handle,(void *)stack[1],out,32,&got);
          count=out[0]-out[2];if(count>256)return;
          ReadProcessMemory(process->handle,(void *)out[2],bytes,count,&got);
          fprintf(log_file,"\"count\":%ld,\"flags\":%lu,\"file\":%lu,\"buffer_size\":%lu,\"buffered\":[",(long)out[1],out[3],out[4],out[6]);
          for(i=0;i<count;++i)fprintf(log_file,"%s%u",i?",":"",bytes[i]);fprintf(log_file,"]}");
        }
    }else{
        memset(text,0,sizeof(text));memset(program,0,sizeof(program));
        ReadProcessMemory(process->handle,(void *)stack[1],text,255,&got);
        ReadProcessMemory(process->handle,(void *)0x651f0,&root,4,&got);
        ReadProcessMemory(process->handle,(void *)root,program,255,&got);
        fprintf(log_file,"\"format\":");quoted(text);fprintf(log_file,"},\"program\":");quoted(program);
    }
    fprintf(log_file,"}\n");fflush(log_file);
}

static void context_log(Process *process,HANDLE thread)
{
    CONTEXT context;
    DWORD stack[24],got,crt_errno=0xffffffffUL,dos_errno=0xffffffffUL,last=0xffffffffUL;
    unsigned n;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if (!GetThreadContext(thread,&context)) return;
    fprintf(log_file,"{\"event\":\"context\",\"pid\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ecx\":%lu,\"edx\":%lu,\"stack\":[",
        process->pid,context.Eip,context.Esp,context.Eax,context.Ecx,context.Edx);
    if (ReadProcessMemory(process->handle,(void *)context.Esp,stack,sizeof(stack),&got)) {
        for (n=0;n<got/4;++n) fprintf(log_file,"%s%lu",n?",":"",stack[n]);
    }
    ReadProcessMemory(process->handle,(void *)0x852d8,&crt_errno,4,&got);
    ReadProcessMemory(process->handle,(void *)0x852dc,&dos_errno,4,&got);
    for (n=0;n<256;++n) if (threads[n].active && threads[n].handle==thread) {
        ReadProcessMemory(process->handle,(void *)(threads[n].teb+0x34),&last,4,&got);break;
    }
    fprintf(log_file,"],\"crt_errno\":%lu,\"dos_errno\":%lu,\"last_error\":%lu,\"exception_code\":%lu}\n",crt_errno,dos_errno,last,current_exception_code);fflush(log_file);
    resource_abi_sample(process,&context,stack);
    editor_error_abi_sample(process,&context,stack);
    omf_continuation_sample(process,&context,stack);
    if (context.Eip==0x52778 || context.Eip==0x52777) {
        DWORD saved[8],registration[4],head=0;
        if (!ReadProcessMemory(process->handle,(void *)stack[1],saved,sizeof(saved),&got) || got!=sizeof(saved)) return;
        fprintf(log_file,"{\"event\":\"longjmp_boundary\",\"buffer_is_disassembler\":%d,\"value\":%lu,\"saved_eip\":%lu,\"saved_scope\":%lu",stack[1]==0x6d21c,stack[2],saved[5],saved[7]);
        fprintf(log_file,",\"buffer\":%lu,\"caller\":%lu",stack[1],stack[0]);
        if (ReadProcessMemory(process->handle,(void *)saved[6],registration,sizeof(registration),&got) && got==sizeof(registration))
            fprintf(log_file,",\"registration_scope\":%lu",registration[3]);
        for (n=0;n<256;++n) if (threads[n].active && threads[n].handle==thread) {
            if (!ReadProcessMemory(process->handle,(void *)threads[n].teb,&head,4,&got) || got!=4) return;
            fprintf(log_file,",\"registration_is_current\":%d",head==saved[6]);
            break;
        }
        fprintf(log_file,"}\n");fflush(log_file);
    }
    if (context.Eip==0x2bec1) {
        DWORD machine[9];
        if (!ReadProcessMemory(process->handle,(void *)stack[1],machine,36,&got) || got!=36) return;
        fprintf(log_file,"{\"event\":\"import_machine_state\",\"argument\":%lu,\"bytes\":36,\"words\":[",stack[1]);
        for (n=0;n<9;++n) fprintf(log_file,"%s%lu",n?",":"",machine[n]);
        fprintf(log_file,"]}\n");fflush(log_file);
    }
    /* Operand callbacks have a cdecl(data,operand,state) interface. Report
       pointer identity/offset separately from the54 scalar state words. */
    if (context.Eip>0x4b650 && context.Eip<=0x4e000) {
        DWORD state[56];
        if (!ReadProcessMemory(process->handle,(void *)stack[3],state,sizeof(state),&got) || got!=sizeof(state)) return;
        fprintf(log_file,"{\"event\":\"decoder_state\",\"words\":[");
        for (n=0;n<56;++n) {
            DWORD value=state[n];
            if (n==18) value=state[18]?state[18]-state[19]:0xffffffffUL;
            if (n==19) value=state[19]!=0;
            fprintf(log_file,"%s%lu",n?",":"",value);
        }
        fprintf(log_file,"],\"operand\":%lu,\"cursor\":%lu}\n",stack[2],stack[1]-state[19]);
        fflush(log_file);
    }
    /* Explicit ROM state analysis: only the writer/serializer's live fields.
       Record+4 is unconsumed and is deliberately neither reported nor compared. */
    if (context.Eip==0x1f281) {
        DWORD first,next,record[4],count;
        if (!ReadProcessMemory(process->handle,(void *)0x5c460,&first,4,&got) || got!=4 ||
            !ReadProcessMemory(process->handle,(void *)0x5c3ec,&next,4,&got) || got!=4 ||
            next<first || ((next-first)&15) || (next-first)/16>256) return;
        count=(next-first)/16;
        fprintf(log_file,"{\"event\":\"rom_fixups\",\"stride\":16,\"count\":%lu,\"records\":[",count);
        for (n=0;n<count;++n) {
            if (!ReadProcessMemory(process->handle,(void *)(first+n*16),record,16,&got) || got!=16) return;
            fprintf(log_file,"%s{\"type\":%lu,\"rva\":%lu,\"value\":%lu}",
                n?",":"",record[0]&0xffff,record[2],record[3]);
        }
        fprintf(log_file,"]}\n");fflush(log_file);
    }
}

/* Opt-in public-console cancellation preflight. Only an owned child group is
 * signalled. The original registered callback is observed, never invoked here. */
static int cancellation_sample(Process *process,Thread *thread,DWORD site,DWORD root_pid)
{
    CONTEXT context;DWORD got,args[3],slots[16],fields[10],i,n,active=0,error,suspended;
    char name[512];BOOL result;
    if(!cancel_public)return 1;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if(!GetThreadContext(thread->handle,&context))return 0;
    if(site==0x26b99){
        if(!ReadProcessMemory(process->handle,(void *)context.Esp,args,8,&got)||got!=8)return 0;
        cancel_handler=args[0];
        fprintf(log_file,"{\"event\":\"cancel_registration_before\",\"pid\":%lu,\"tid\":%lu,\"handler\":%lu,\"add\":%lu}\n",process->pid,thread->tid,args[0],args[1]);fflush(log_file);
        return cancel_handler==0x270f0 && args[1]==1;
    }
    if(site==0x26b9e){
        cancel_registered=context.Eax!=0;
        fprintf(log_file,"{\"event\":\"cancel_registration_after\",\"result\":%lu}\n",context.Eax);fflush(log_file);
        return cancel_registered;
    }
    if(site==0x270f0 || site==0x3d670){
        DWORD registration,last,crt,dos;
        if(!ReadProcessMemory(process->handle,(void *)context.Esp,args,8,&got)||got!=8)return 0;
        ReadProcessMemory(process->handle,(void *)thread->teb,&registration,4,&got);
        ReadProcessMemory(process->handle,(void *)(thread->teb+0x34),&last,4,&got);
        ReadProcessMemory(process->handle,(void *)0x852d8,&crt,4,&got);
        ReadProcessMemory(process->handle,(void *)0x852dc,&dos,4,&got);
        fprintf(log_file,"{\"event\":\"cancel_callback_context\",\"site\":%lu,\"pid\":%lu,\"tid\":%lu,\"main_tid\":%lu,\"teb\":%lu,\"registration\":%lu,\"return_address\":%lu,\"first_stack_word\":%lu,\"esp\":%lu,\"errno\":%lu,\"doserrno\":%lu,\"last_error\":%lu,\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu}\n",site,process->pid,thread->tid,cancel_main_tid,thread->teb,registration,args[0],args[1],context.Esp,crt,dos,last,context.FloatSave.ControlWord,context.FloatSave.StatusWord,context.FloatSave.TagWord);fflush(log_file);
        if(!cancel_dispatched || thread->tid==cancel_main_tid || (site==0x270f0 && args[1]!=CTRL_BREAK_EVENT))return 0;
        return 1;
    }
    if(site!=0x1c700)return 1;
    if(process->pid!=root_pid || !cancel_registered || cancel_dispatched)return 0;
    if(!ReadProcessMemory(process->handle,(void *)0x6ed40,slots,64,&got)||got!=64)return 0;
    fprintf(log_file,"{\"event\":\"cancel_safe_boundary\",\"pid\":%lu,\"tid\":%lu,\"site\":%lu,\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"slots\":[",process->pid,thread->tid,site,context.FloatSave.ControlWord,context.FloatSave.StatusWord,context.FloatSave.TagWord);
    for(i=0;i<16;++i){
        if(i)fprintf(log_file,",");
        if(!slots[i]){fprintf(log_file,"null");continue;}
        if(!ReadProcessMemory(process->handle,(void *)slots[i],fields,40,&got)||got!=40)return 0;
        memset(name,0,sizeof(name));ReadProcessMemory(process->handle,(void *)fields[0],name,511,&got);
        fprintf(log_file,"{\"pointer\":%lu,\"name\":",slots[i]);quoted(name);fprintf(log_file,",\"words\":[");
        for(n=0;n<10;++n)fprintf(log_file,"%s%lu",n?",":"",fields[n]);fprintf(log_file,"]}");
        if(!strcmp(name,"cancel.dll") && (fields[1]&2) && fields[4] && fields[5] && fields[6])++active;
    }
    fprintf(log_file,"],\"active_owned_output\":%lu}\n",active);fflush(log_file);
    if(active!=1)return 0;
    suspended=SuspendThread(thread->handle);
    fprintf(log_file,"{\"event\":\"cancel_suspend_main\",\"tid\":%lu,\"previous_count\":%lu}\n",thread->tid,suspended);fflush(log_file);
    if(suspended!=0)return 0;
    cancel_main_tid=thread->tid;
    SetLastError(0);result=GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,root_pid);error=GetLastError();
    fprintf(log_file,"{\"event\":\"cancel_public_dispatch\",\"caller_pid\":%lu,\"group\":%lu,\"control\":1,\"result\":%d,\"last_error\":%lu}\n",GetCurrentProcessId(),root_pid,result,error);fflush(log_file);
    if(!result)return 0;
    cancel_dispatched=1;
    return 1;
}

int main(int argc,char **argv)
{
    STARTUPINFOA startup;
    PROCESS_INFORMATION launched;
    SECURITY_ATTRIBUTES security;
    DEBUG_EVENT event;
    CONTEXT context;
    BasicInfo basic;
    QueryProcess query;
    HANDLE output,error;
    FILE *file;
    Process *process;
    Thread *thread;
    DWORD got,root_pid,root_exit=999,status,site,code,live=0,packers=0;
    DWORD before_output=0,after_output=0,before_error=0,after_error=0;
    unsigned n,i;
    int failed=0;
    unsigned char instruction;
    void *parameters;
    char command[8192],image[4096],child_command[4096],child_cwd[4096],dll[4096];
    char line[4096];
    DWORD parent_errno,creation_flags=DEBUG_PROCESS;
    cancel_public=getenv("LINK_CAPTURE_BREAK")!=NULL;
    if (argc!=12 && argc!=13) return 2;
    if (argc==13) {
        file=fopen(argv[12],"rt");
        if(!file || !plan_line(file,line))return 4;
        copy_count=atoi(line);if(!copy_count || copy_count>8)return 4;
        for(i=0;i<copy_count;++i)
            if(!plan_line(file,copy_plans[i].image) || !plan_line(file,copy_plans[i].command) ||
               !plan_line(file,copy_plans[i].cwd) || !plan_line(file,copy_plans[i].source) ||
               !plan_line(file,copy_plans[i].destination))return 4;
        if(fgetc(file)!=EOF)return 4;fclose(file);
    }
    query=(QueryProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryInformationProcess");
    if (!query) return 3;
    file=fopen(argv[7],"rb");
    if (!file) return 4;
    if (fread(&text_rva,4,1,file)!=1 || fread(&text_size,4,1,file)!=1 || text_size>0x1000000)
        return 4;
    text=malloc(text_size);
    if (!text || fread(text,1,text_size,file)!=text_size || fgetc(file)!=EOF) return 4;
    fclose(file);
    file=fopen(argv[8],"rt");
    if (!file) return 4;
    while (entry_count<2048 && fscanf(file,"%lx %x %u %127s",&entries[entry_count].rva,
        &entries[entry_count].byte,&entries[entry_count].seen,entries[entry_count].name)==4) {
        if (entries[entry_count].rva<text_rva || entries[entry_count].rva>=text_rva+text_size ||
            entries[entry_count].byte>255 || entries[entry_count].seen>1) return 4;
        ++entry_count;
    }
    if (!feof(file)) return 4;
    fclose(file);
    log_file=fopen(argv[5],"wt");if(!log_file)return 4;
    memset(&startup,0,sizeof(startup));memset(&security,0,sizeof(security));
    security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        &security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE) return 4;
    startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    if (strlen(argv[1])+strlen(argv[2])+5>=sizeof(command))return 4;
    sprintf(command,"\"%s\" %s",argv[1],argv[2]);
    sprintf(dll,"%s\\hello-debug.dll",argv[6]);
    /* Opt-in new-case external input: Wine may replace host TMP/TEMP before
       this helper starts. Change this process's child environment only. */
    {
        const char *temporary=getenv("LINK_CAPTURE_TMP");
        if (temporary) {
            DWORD attributes=GetFileAttributesA(temporary);
            if (strcmp(temporary,argv[6]) || attributes==0xffffffffUL ||
                !(attributes&FILE_ATTRIBUTE_DIRECTORY) ||
                !SetEnvironmentVariableA("TMP",temporary) ||
                !SetEnvironmentVariableA("TEMP",temporary)) return 12;
            fprintf(log_file,"{\"event\":\"controlled_tmp\",\"directory\":");
            quoted(temporary);fprintf(log_file,"}\n");fflush(log_file);
        }
    }
    if(cancel_public){
        BOOL detached,allocated;DWORD console_error;
        detached=FreeConsole();SetLastError(0);allocated=AllocConsole();console_error=GetLastError();
        fprintf(log_file,"{\"event\":\"cancel_owned_console\",\"helper_pid\":%lu,\"detached\":%d,\"allocated\":%d,\"last_error\":%lu}\n",GetCurrentProcessId(),detached,allocated,console_error);fflush(log_file);
        if(!allocated){fclose(log_file);CloseHandle(output);CloseHandle(error);return 13;}
        startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
        creation_flags|=CREATE_NEW_PROCESS_GROUP;
    }
    if (!CreateProcessA(argv[1],command,NULL,NULL,TRUE,creation_flags,NULL,argv[6],&startup,&launched))
        return 5;
    root_pid=launched.dwProcessId;CloseHandle(launched.hProcess);CloseHandle(launched.hThread);
    do {
        if (!WaitForDebugEvent(&event,cancel_dispatched?5000:30000)) {failed=6;break;}
        status=DBG_CONTINUE;process=NULL;thread=NULL;
        for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==event.dwProcessId)process=&processes[n];
        for(n=0;n<256;++n)if(threads[n].active && threads[n].tid==event.dwThreadId)thread=&threads[n];
        if (event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            for(n=0;n<64 && processes[n].active;++n);
            if(n==64){failed=7;break;}
            process=&processes[n];memset(process,0,sizeof(*process));
            process->pid=event.dwProcessId;process->handle=event.u.CreateProcessInfo.hProcess;
            process->base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;process->active=1;++live;
            if(query(process->handle,0,&basic,sizeof(basic),NULL) ||
                !ReadProcessMemory(process->handle,(char *)basic.peb+0x10,&parameters,4,&got) ||
                !remote_string(process->handle,parameters,0x38,image) ||
                !remote_string(process->handle,parameters,0x40,child_command)){failed=7;break;}
            process->parent=basic.parent;process->target=!_stricmp(image,argv[1]);
            process->packer=strcmp(argv[11],"-") && !_stricmp(image,argv[11]);
            fprintf(log_file,"{\"event\":\"create\",\"pid\":%lu,\"parent\":%lu,\"target\":%d,\"packer\":%d,\"image\":",
                process->pid,process->parent,process->target,process->packer);quoted(image);
            fprintf(log_file,",\"command_line\":");quoted(child_command);fprintf(log_file,"}\n");fflush(log_file);
            if (process->target) {
                if(process->base!=0x10000 || !write_code(process,text_rva,text,text_size)){failed=8;break;}
            } else if (process->packer) {
                ++packers;before_output=GetFileSize(output,NULL);before_error=GetFileSize(error,NULL);
                if(!CopyFileA(dll,argv[9],FALSE)){failed=8;break;}
            } else if (next_copy<copy_count && process->parent==root_pid &&
                       !_stricmp(image,copy_plans[next_copy].image) &&
                       !strcmp(child_command,copy_plans[next_copy].command)) {
                if(!remote_string(process->handle,parameters,0x24,child_cwd)){failed=9;break;}
                sprintf(line,"%s\\",copy_plans[next_copy].cwd);
                if((_stricmp(child_cwd,copy_plans[next_copy].cwd) && _stricmp(child_cwd,line)) ||
                   GetFileAttributesA(copy_plans[next_copy].source)==0xffffffffUL ||
                   GetFileAttributesA(copy_plans[next_copy].destination)!=0xffffffffUL){failed=9;break;}
                process->copy=++next_copy;
                process->copy_output=GetFileSize(output,NULL);process->copy_error=GetFileSize(error,NULL);
                parent_errno=0xffffffffUL;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==root_pid)
                    ReadProcessMemory(processes[n].handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,"{\"event\":\"copy-begin\",\"pid\":%lu,\"index\":%d,\"stdout_start\":%lu,\"stderr_start\":%lu,\"parent_errno\":%lu,\"cwd\":",
                    process->pid,process->copy,process->copy_output,process->copy_error,parent_errno);
                quoted(child_cwd);fprintf(log_file,",\"environment\":");
                if(!copy_environment(process->handle,parameters)){failed=9;break;}
                fprintf(log_file,"}\n");fflush(log_file);
            } else {failed=9;break;}
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            threads[n].handle=event.u.CreateProcessInfo.hThread;threads[n].active=1;
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            if(cancel_public){fprintf(log_file,"{\"event\":\"cancel_create_thread\",\"pid\":%lu,\"tid\":%lu,\"start\":%lu,\"teb\":%lu,\"after_dispatch\":%d}\n",event.dwProcessId,event.dwThreadId,(DWORD)event.u.CreateThread.lpStartAddress,(DWORD)event.u.CreateThread.lpThreadLocalBase,cancel_dispatched);fflush(log_file);}
            for(n=0;n<256 && threads[n].active;++n);
            if(n==256){failed=7;break;}
            threads[n].pid=event.dwProcessId;threads[n].tid=event.dwThreadId;
            threads[n].teb=(DWORD)event.u.CreateThread.lpThreadLocalBase;
            threads[n].handle=event.u.CreateThread.hThread;threads[n].active=1;
        } else if (event.dwDebugEventCode==EXIT_THREAD_DEBUG_EVENT) {
            if(thread){CloseHandle(thread->handle);thread->active=0;}
        } else if (event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            code=event.u.Exception.ExceptionRecord.ExceptionCode;
            current_exception_code=code;
            site=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if(!process || !thread){failed=7;break;}
            if(code==EXCEPTION_BREAKPOINT && !process->loader &&
                (!process->target || site<process->base+text_rva || site>=process->base+text_rva+text_size))
                process->loader=1;
            else if(code==EXCEPTION_SINGLE_STEP && auxiliary_watch && process->target) {
                auxiliary_watch_event(process,thread);
            } else if(code==EXCEPTION_BREAKPOINT && process->target) {
                for(i=0;i<entry_count && entries[i].rva!=site-process->base;++i);
                if(i==entry_count || entries[i].seen){failed=10;context_log(process,thread->handle);break;}
                instruction=(unsigned char)entries[i].byte;
                text[entries[i].rva-text_rva]=instruction;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].target &&
                    !write_code(&processes[n],entries[i].rva,&instruction,1)){failed=8;break;}
                if(failed)break;
                memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
                if(!GetThreadContext(thread->handle,&context)){failed=8;break;}
                context.Eip=site;
                if(!SetThreadContext(thread->handle,&context)){failed=8;break;}
                entries[i].seen=1;
                fprintf(log_file,"{\"event\":\"entry\",\"pid\":%lu,\"rva\":%lu,\"name\":",process->pid,entries[i].rva);
                quoted(entries[i].name);
                parent_errno=0xffffffffUL;
                ReadProcessMemory(process->handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,",\"crt_errno\":%lu}\n",parent_errno);fflush(log_file);
                if (!strncmp(entries[i].name,"analysis_decoder_",17)) context_log(process,thread->handle);
                if (!strncmp(entries[i].name,"analysis_aux_watch",18) && site==0x289b0)
                    auxiliary_watch_start(process,thread,&context);
                if (auxiliary_watch && site==0x2cb51) {
                    DWORD args[3],got,n;unsigned char bytes[18];
                    ReadProcessMemory(process->handle,(void *)context.Esp,args,sizeof(args),&got);
                    ReadProcessMemory(process->handle,(void *)args[1],bytes,sizeof(bytes),&got);
                    fprintf(log_file,"{\"event\":\"auxiliary-tail-read\",\"pointer\":%lu,\"size\":%lu,\"bytes\":[",args[1],args[2]);
                    for(n=0;n<18;++n)fprintf(log_file,"%s%u",n?",":"",bytes[n]);
                    fprintf(log_file,"]}\n");fflush(log_file);
                }
                if (!strcmp(entries[i].name,"analysis_decoder_resource_fatal_rearm") && site==0x218d0) {
                    unsigned char trap=0xcc;
                    for(n=0;n<entry_count;++n) if (entries[n].rva==0x2d4fb || entries[n].rva==0x2d500 || entries[n].rva==0x2d509 || entries[n].rva==0x2d512) {
                        entries[n].seen=0;
                        if (!write_code(process,entries[n].rva,&trap,1)) {failed=8;break;}
                    }
                }
                if (!strcmp(entries[i].name,"analysis_no_late_lasterror") && site==0x2197d) {
                    unsigned char trap=0xcc;
                    if (!write_code(process,0x4b224,&trap,1)) {failed=8;break;}
                    fprintf(log_file,"{\"event\":\"no-late-lasterror-guard\",\"va\":373284}\n");fflush(log_file);
                }
                resource_tempnam_sample(process,thread,&context,site);
                clock_sample(process,&context,site);
                relocation_sample(process,&context,site);
                strtoul_sample(process,&context,site);
                bind_api_sample(process,thread,&context,site);
                if(!cancellation_sample(process,thread,site,root_pid)){failed=14;break;}
            } else if(cancel_public && cancel_dispatched && (code==0x40010005 || code==0x40010008)){
                fprintf(log_file,"{\"event\":\"cancel_debug_console_exception\",\"code\":%lu,\"tid\":%lu,\"first_chance\":%lu}\n",code,event.dwThreadId,event.u.Exception.dwFirstChance);fflush(log_file);
                status=DBG_EXCEPTION_NOT_HANDLED;
            } else {failed=10;context_log(process,thread->handle);break;}
        } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            if(!process){failed=7;break;}
            fprintf(log_file,"{\"event\":\"exit\",\"pid\":%lu,\"code\":%lu,\"packer\":%d}\n",
                process->pid,event.u.ExitProcess.dwExitCode,process->packer);fflush(log_file);
            if(process->packer) {
                after_output=GetFileSize(output,NULL);after_error=GetFileSize(error,NULL);
                if(!CopyFileA(dll,argv[10],FALSE)){failed=8;break;}
            }
            if(process->copy) {
                parent_errno=0xffffffffUL;
                for(n=0;n<64;++n)if(processes[n].active && processes[n].pid==root_pid)
                    ReadProcessMemory(processes[n].handle,(void *)0x852d8,&parent_errno,4,&got);
                fprintf(log_file,"{\"event\":\"copy-end\",\"pid\":%lu,\"index\":%d,\"code\":%lu,\"stdout_end\":%lu,\"stderr_end\":%lu,\"parent_errno\":%lu}\n",
                    process->pid,process->copy,event.u.ExitProcess.dwExitCode,
                    GetFileSize(output,NULL),GetFileSize(error,NULL),parent_errno);fflush(log_file);
            }
            if(process->pid==root_pid)root_exit=event.u.ExitProcess.dwExitCode;
            CloseHandle(process->handle);process->active=0;--live;
            for(n=0;n<256;++n)if(threads[n].active && threads[n].pid==event.dwProcessId){
                CloseHandle(threads[n].handle);threads[n].active=0;
            }
        }
        ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
    } while(live);
    if(failed)for(n=0;n<64;++n)if(processes[n].active)TerminateProcess(processes[n].handle,99);
    fprintf(log_file,"{\"event\":\"summary\",\"root_exit\":%lu,\"failed\":%d,\"packers\":%lu,\"packer_stdout_start\":%lu,\"packer_stdout_end\":%lu,\"packer_stderr_start\":%lu,\"packer_stderr_end\":%lu}\n",
        root_exit,failed,packers,before_output,after_output,before_error,after_error);fclose(log_file);
    CloseHandle(output);CloseHandle(error);free(text);
    if(cancel_public)FreeConsole();
    return failed;
}
