/* Explicit original-only direct IDE callback test controller. The loaded DLL
 * is a readable-C external test dependency, not a target graph vertex. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../cases/ide-callback-protocol/sources/ide-probe.h"
#include <recon.h>
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static AllocateRemote allocate_remote;
static HANDLE process,thread;
static FILE *log_file;
static DWORD base,teb,stack_top,sentinel,memory,module,probe_address;
static DWORD text_rva,text_size,phase;
static DWORD getter_rva,channel_rva,write_rva,assert_rva;
static unsigned char *text;
static unsigned char message_bytes[512];
static IdeTestProbe probe;
static CONTEXT initial,callback_entry;
static DWORD callback_return,callback_index;
static unsigned char callback_return_byte,callback_bytes[3];
static DWORD target_returns[10];
static const char *messages[4]={"entry","source.obj","library.lib","symbol_name"};
static const char flags_value[]="alpha.obj,beta.lib";
static DWORD values[7];
static int read_memory(DWORD address,void *buffer,DWORD length)
{
    DWORD got;
    return ReadProcessMemory(process,(void *)address,buffer,length,&got) && got==length;
}
static int write_memory(DWORD address,const void *buffer,DWORD length)
{
    DWORD got;
    return WriteProcessMemory(process,(void *)address,(void *)buffer,length,&got) && got==length;
}
static int write_code(DWORD address,const void *buffer,DWORD length)
{
    DWORD old,restored;
    return VirtualProtectEx(process,(void *)address,length,PAGE_EXECUTE_READWRITE,&old) &&
        write_memory(address,buffer,length) && FlushInstructionCache(process,(void *)address,length) &&
        VirtualProtectEx(process,(void *)address,length,old,&restored);
}
static void bytes(const unsigned char *data,DWORD count)
{
    DWORD i;
    fputc('"',log_file);for(i=0;i<count;++i)fprintf(log_file,"%02x",data[i]);fputc('"',log_file);
}
static void words(const DWORD *data,DWORD count)
{
    DWORD i;
    fputc('[',log_file);for(i=0;i<count;++i)fprintf(log_file,"%s%lu",i?",":"",data[i]);fputc(']',log_file);
}
static int snapshot(const char *event,CONTEXT *context,DWORD argument_count)
{
    DWORD args[4],stream[8],errors[3],globals[2],used,mbmax,ctype;
    unsigned char pending[4096],table[514],incoming[512],out[68];
    if(argument_count>3 || !read_memory(context->Esp,args,(argument_count+1)*4) ||
       !read_memory(0x85070,stream,32) || !read_memory(0x852d8,errors,8) ||
       !read_memory(teb+0x34,&errors[2],4) || !read_memory(0x6eca4,&globals[0],4) ||
       !read_memory(0x7eb54,&globals[1],4) || !read_memory(0x8554c,&mbmax,4) ||
       !read_memory(0x85340,&ctype,4) || !read_memory(ctype-2,table,514))return 0;
    used=stream[0]-stream[2];if(used>sizeof(pending))return 0;
    if(used && !read_memory(stream[2],pending,used))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,context->Eip,context->Esp,context->Eax,context->Ebx,context->Esi,context->Edi,context->Ebp);
    words(args,argument_count+1);fprintf(log_file,",\"stdout_file\":");words(stream,8);
    fprintf(log_file,",\"pending\":");bytes(pending,used);
    fprintf(log_file,",\"errors\":");words(errors,3);fprintf(log_file,",\"globals\":");words(globals,2);
    fprintf(log_file,",\"mb_cur_max\":%lu,\"ctype\":",mbmax);bytes(table,514);
    if(probe_address){
        if(!read_memory(probe_address,&probe,sizeof(probe)) || !read_memory(memory+4096,incoming,512) || !read_memory(memory+8192,out,68))return 0;
        fprintf(log_file,",\"probe_address\":%lu,\"probe\":",probe_address);bytes((unsigned char *)&probe,sizeof(probe));
        fprintf(log_file,",\"message_inputs\":");bytes(incoming,512);
        fprintf(log_file,",\"message_base\":%lu,\"message_values\":",memory+4096);words(values,7);
        fprintf(log_file,",\"out_slot_address\":%lu,\"out_slot\":",memory+8192+32);bytes(out,68);
    }
    fprintf(log_file,"}\n");fflush(log_file);return 1;
}
static int start_call(CONTEXT *context,DWORD address,DWORD count,DWORD first,DWORD second)
{
    DWORD frame[3];frame[0]=sentinel;frame[1]=first;frame[2]=second;
    if(count>2 || !write_memory(stack_top,frame,(count+1)*4))return 0;
    context->Eip=address;context->Esp=stack_top;
    return snapshot(phase<2?"external_entry":"target_entry",context,count) && SetThreadContext(thread,context);
}
static int next_call(CONTEXT *context)
{
    if(phase==0)return start_call(context,0x5b17c,1,memory,0); /* actual LoadLibraryA IAT thunk */
    if(phase==1)return start_call(context,module+getter_rva,0,0,0);
    if(phase==2)return start_call(context,0x35700,1,probe_address,0);
    if(phase<=9)return start_call(context,0x355a0,2,phase-3,values[phase-3]);
    if(phase==10)return start_call(context,0x35720,1,memory+8192+32,0);
    return 0;
}
static int intact(void)
{
    unsigned char incoming[512];unsigned i;
    if(!read_memory(memory+4096,incoming,512) || memcmp(incoming,message_bytes,512) ||
       !read_memory(probe_address,&probe,sizeof(probe)) || probe.failed || probe.assert_calls)return 0;
    for(i=0;i<32;++i)if(probe.before[i]!=0xa5 || probe.after[i]!=0xa5)return 0;
    return 1;
}
static int callback_start(CONTEXT *context,DWORD index)
{
    DWORD args[4],address=module+(index?write_rva:channel_rva),count=index?3:1;
    unsigned char trap=0xcc;
    if(callback_return || index>1 || !read_memory(context->Esp,args,(count+1)*4))return 0;
    if(index==0){if(phase!=2 || args[0]!=0x3570d || args[1]!=2)return 0;}
    else if(phase<3 || phase>9 || args[0]!=0x356f3 || args[1]!=7 ||
            args[2]!=probe_address+offsetof(IdeTestProbe,buffer) || args[3]>1023)return 0;
    callback_return=args[0];callback_index=index;callback_entry=*context;callback_entry.Eip=address;
    if(!read_memory(callback_return,&callback_return_byte,1) || callback_return_byte==0xcc ||
       !write_code(callback_return,&trap,1) || !write_code(address,&callback_bytes[index],1))return 0;
    context->Eip=address;
    return snapshot(index?"write_entry":"channel_entry",context,count) && SetThreadContext(thread,context);
}
static int callback_end(CONTEXT *context)
{
    unsigned char trap=0xcc;DWORD address=module+(callback_index?write_rva:channel_rva),args[4];
    if(context->Esp!=callback_entry.Esp+4 || context->Ebx!=callback_entry.Ebx ||
       context->Esi!=callback_entry.Esi || context->Edi!=callback_entry.Edi || context->Ebp!=callback_entry.Ebp ||
       !read_memory(callback_entry.Esp,args,(callback_index?4:2)*4) ||
       context->Eax!=(callback_index?args[3]:7))return 0;
    context->Eip=callback_return;
    if(!snapshot(callback_index?"write_return":"channel_return",context,0) ||
       !write_code(callback_return,&callback_return_byte,1) || !write_code(address,&trap,1))return 0;
    callback_return=0;
    return SetThreadContext(thread,context);
}
static int environment_pointer(DWORD pointer)
{
    DWORD array,entry,i;char value[256];
    if(!read_memory(0x852fc,&array,4))return 0;
    for(i=0;i<2048;++i){
        if(!read_memory(array+i*4,&entry,4) || !entry)return 0;
        memset(value,0,sizeof(value));
        if(!read_memory(entry,value,15+sizeof(flags_value)))continue;
        if(!memcmp(value,"_MSC_IDE_FLAGS=",15))
            return pointer==entry+15 && !strcmp(value+15,flags_value);
    }
    return 0;
}
static int save_protocol(const char *path,DWORD flags_pointer)
{
    FILE *file;DWORD head[8],n;char flags[sizeof(flags_value)];
    unsigned char outguard[68];
    if(!intact() || probe.get_calls!=1 || probe.last_kind!=2 || probe.write_calls!=7 ||
       probe.used>sizeof(probe.sink) || !environment_pointer(flags_pointer) ||
       !read_memory(flags_pointer,flags,sizeof(flags)) || strcmp(flags,flags_value) ||
       !read_memory(memory+8192,outguard,68))return 0;
    for(n=0;n<32;++n)if(outguard[n]!=0xa5 || outguard[n+36]!=0xa5)return 0;
    file=fopen(path,"wb");if(!file)return 0;
    head[0]=0x31454449;head[1]=1;head[2]=probe.get_calls;head[3]=probe.last_kind;
    head[4]=probe.write_calls;head[5]=probe.used;head[6]=probe.assert_calls;head[7]=probe.failed;
    if(fwrite(head,1,sizeof(head),file)!=sizeof(head) ||
       fwrite(probe.writes,sizeof(IdeTestWrite),7,file)!=7 ||
       fwrite(probe.sink,1,probe.used,file)!=probe.used ||
       fwrite(probe.before,1,32,file)!=32 || fwrite(probe.buffer,1,1024,file)!=1024 ||
       fwrite(probe.after,1,32,file)!=32 || fwrite(target_returns,4,9,file)!=9 ||
       fwrite(flags,1,sizeof(flags),file)!=sizeof(flags) || fclose(file))return 0;
    return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES security;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT event;CONTEXT context;HANDLE output,error;FILE *file;
    DWORD root_pid,got,code,site,status,bytes_count,old,failed=0,stopped=0,exit_code=999,at_main=0,loader_seen=0;
    unsigned char trap=0xcc,byte,outguard[68];
    DWORD addresses[3],i,expected_sp,flags_pointer,api;
    char command[4096];
    /* program,dll,exportRVAplan,textplan,stdout,stderr,log,protocol,cwd */
    if(argc!=10)return 2;
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    if(!allocate_remote)return 3;
    file=fopen(argv[3],"rt");if(!file || fscanf(file,"%lx %lx %lx %lx",&getter_rva,&channel_rva,&write_rva,&assert_rva)!=4 || fclose(file))return 3;
    file=fopen(argv[4],"rb");if(!file || fread(&text_rva,4,1,file)!=1 || fread(&text_size,4,1,file)!=1 || text_size>0x1000000)return 3;
    text=malloc(text_size);if(!text || fread(text,1,text_size,file)!=text_size || fgetc(file)!=EOF || fclose(file))return 3;
    log_file=fopen(argv[7],"wt");if(!log_file)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[5],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[6],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE)return 3;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    if(!SetEnvironmentVariableA("_MSC_IDE_FLAGS",flags_value))return 3;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[9],&startup,&launch))return 4;
    root_pid=launch.dwProcessId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    for(;;){
        if(!WaitForDebugEvent(&event,15000)){failed=5;break;}status=DBG_CONTINUE;
        if(event.dwProcessId!=root_pid){failed=6;break;}
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=event.u.CreateProcessInfo.hProcess;thread=event.u.CreateProcessInfo.hThread;
            base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            if(base!=0x10000 || !write_code(0x26b40,&trap,1)){failed=7;break;}
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){
            fprintf(log_file,"{\"event\":\"load_dll\",\"base\":%lu}\n",(DWORD)event.u.LoadDll.lpBaseOfDll);fflush(log_file);
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        }else if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            code=event.u.Exception.ExceptionRecord.ExceptionCode;site=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
            if(!GetThreadContext(thread,&context)){failed=9;break;}
            if(code==EXCEPTION_BREAKPOINT && site==0x26b40 && !at_main){
                at_main=1;initial=context;
                if(!write_code(base+text_rva,text,text_size)){failed=10;break;}
                memory=(DWORD)allocate_remote(process,NULL,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                if(!memory || stack_top==0xf000 || !sentinel || strlen(argv[2])>=4096){failed=11;break;}
                memset(message_bytes,0xa5,sizeof(message_bytes));
                values[0]=0;values[5]=0xfedcba98;values[6]=0x12345678;
                for(i=0;i<4;++i){memcpy(message_bytes+i*128+32,messages[i],strlen(messages[i])+1);values[i+1]=memory+4096+i*128+32;}
                memset(outguard,0xa5,sizeof(outguard));
                if(!write_memory(memory,argv[2],strlen(argv[2])+1) || !write_memory(memory+4096,message_bytes,sizeof(message_bytes)) ||
                   !write_memory(memory+8192,outguard,sizeof(outguard)) || !write_code(sentinel,&trap,1) || !next_call(&context)){failed=12;break;}
            }else if(code==EXCEPTION_BREAKPOINT && at_main && site==sentinel){
                expected_sp=stack_top+(phase?4:8);
                if(context.Esp!=expected_sp || context.Ebx!=initial.Ebx || context.Esi!=initial.Esi || context.Edi!=initial.Edi || context.Ebp!=initial.Ebp){failed=13;break;}
                context.Eip=site;
                if(!snapshot(phase<2?"external_return":"target_return",&context,0)){failed=14;break;}
                if(phase==0){if(!context.Eax){failed=15;break;}module=context.Eax;}
                else if(phase==1){
                    probe_address=context.Eax;if(!probe_address || !intact()){failed=16;break;}
                    addresses[0]=module+channel_rva;addresses[1]=module+write_rva;addresses[2]=module+assert_rva;
                    if((DWORD)probe.api.get_channel!=addresses[0] || (DWORD)probe.api.write!=addresses[1] ||
                       (DWORD)probe.api.assertion!=addresses[2] || (DWORD)probe.api.buffer!=probe_address+offsetof(IdeTestProbe,buffer) || probe.api.capacity!=1024){failed=17;break;}
                    for(i=0;i<3;++i)if(!read_memory(addresses[i],&callback_bytes[i],1) || !write_code(addresses[i],&trap,1)){failed=18;break;}
                    if(failed)break;
                }else if(phase==2){
                    if(!read_memory(0x6eca4,&api,4) || api!=probe_address || context.Eax!=7 || !intact() || probe.get_calls!=1 || probe.last_kind!=2){failed=19;break;}
                    target_returns[0]=context.Eax;
                }else if(phase<=9){
                    if(!intact() || probe.write_calls!=phase-2 || context.Eax!=probe.writes[phase-3].count){failed=20;break;}
                    target_returns[phase-2]=context.Eax;
                }else if(phase==10){
                    target_returns[8]=context.Eax;
                    if(context.Eax!=1 || !read_memory(memory+8192+32,&flags_pointer,4) || !save_protocol(argv[8],flags_pointer)){failed=21;break;}
                    fprintf(log_file,"{\"event\":\"environment_result\",\"pointer\":%lu,\"owned_CRT_environment\":true}\n",flags_pointer);fflush(log_file);
                    stopped=1;TerminateProcess(process,0);
                }
                if(phase<10){++phase;if(!next_call(&context)){failed=22;break;}}
            }else if(code==EXCEPTION_BREAKPOINT && callback_return && site==callback_return){
                if(!callback_end(&context)){failed=23;break;}
            }else if(code==EXCEPTION_BREAKPOINT && module && (site==module+channel_rva || site==module+write_rva)){
                if(!callback_start(&context,site==module+write_rva)){failed=24;break;}
            }else if(code==EXCEPTION_BREAKPOINT && !at_main && !loader_seen && (site<base+text_rva || site>=base+text_rva+text_size)){
                loader_seen=1;
            }else{
                context.Eip=site;snapshot("unexpected_trap",&context,phase>=3 && phase<=9?2:1);
                fprintf(log_file,"{\"event\":\"frontier\",\"code\":%lu,\"va\":%lu,\"phase\":%lu}\n",code,site,phase);fflush(log_file);
                stopped=2;TerminateProcess(process,17);
            }
        }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=event.u.ExitProcess.dwExitCode;ContinueDebugEvent(root_pid,event.dwThreadId,status);break;}
        if(!ContinueDebugEvent(root_pid,event.dwThreadId,status)){failed=25;break;}
    }
    if(failed && process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"phase\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,phase,GetFileSize(output,NULL),GetFileSize(error,NULL));
    fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
