/* Explicit original-only external minimal-code test controller.
 * Never linked into LINK or invoked by a native fixture. The callable is read
 * from the original resource table after real CRT startup. Its caller and
 * return INT3 are owned by this driver, not an original application function.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static AllocateRemote allocate_remote;
static HANDLE process,thread;
static FILE *log_file;
static DWORD base,teb;
static unsigned char *payload,*guarded,*text;
static DWORD size,text_rva,text_size,synthetic_stack,sentinel,remote_input;
static CONTEXT initial;
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
static void words(const DWORD *data,unsigned count)
{
    unsigned i;
    fprintf(log_file,"[");
    for(i=0;i<count;++i)fprintf(log_file,"%s%lu",i?",":"",data[i]);
    fprintf(log_file,"]");
}
static void bytes(const unsigned char *data,unsigned count)
{
    unsigned i;
    fprintf(log_file,"\"");for(i=0;i<count;++i)fprintf(log_file,"%02x",data[i]);fprintf(log_file,"\"");
}
static int snapshot(const char *name,CONTEXT *context)
{
    DWORD state[8],args[3],err[3],mbmax,ctype,used;
    unsigned char buffer[4096],table[514];
    if(!read_memory(context->Esp,args,sizeof(args)) ||
       !read_memory(0x85070,state,sizeof(state)) ||
       !read_memory(0x852d8,err,8) || !read_memory(teb+0x34,&err[2],4) ||
       !read_memory(0x8554c,&mbmax,4) || !read_memory(0x85340,&ctype,4) ||
       !read_memory(ctype-2,table,sizeof(table)))return 0;
    used=state[0]-state[2];if(used>sizeof(buffer))return 0;
    if(used && !read_memory(state[2],buffer,used))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",name,context->Eip,context->Esp,context->Eax,context->Ebx,context->Esi,context->Edi,context->Ebp);
    words(args,3);fprintf(log_file,",\"stdout_file\":");words(state,8);
    fprintf(log_file,",\"buffered\":");bytes(buffer,used);
    fprintf(log_file,",\"errors\":");words(err,3);
    fprintf(log_file,",\"mb_cur_max\":%lu,\"ctype\":",mbmax);bytes(table,sizeof(table));
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu}\n",context->FloatSave.ControlWord,context->FloatSave.StatusWord);
    fflush(log_file);return 1;
}
static unsigned char *load(const char *path,DWORD *length)
{
    FILE *file;long n;unsigned char *data;
    file=fopen(path,"rb");if(!file)return NULL;
    if(fseek(file,0,SEEK_END) || (n=ftell(file))<0 || fseek(file,0,SEEK_SET))return NULL;
    data=malloc(n?n:1);if(!data || fread(data,1,n,file)!=(unsigned long)n || fclose(file))return NULL;
    *length=n;return data;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES security;
    STARTUPINFOA startup;
    PROCESS_INFORMATION launch;
    DEBUG_EVENT event;
    CONTEXT context;
    HANDLE output,error;
    DWORD root_pid,root_tid,status,code,site,type,expected,table[4],frame[3],mask_size;
    DWORD old,exit_code=999,at_main=0,phase=0,failed=0,stopped=0;
    unsigned char trap=0xcc,main_byte,main_original;
    char command[4096];
    /* program,payload,type,expected VA,text plan,stdout,stderr,log,cwd */
    if(argc!=10)return 2;
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    if(!allocate_remote)return 3;
    payload=load(argv[2],&size);text=load(argv[5],&mask_size);
    if(!payload || !size || size>4096 || !text || mask_size<8)return 3;
    text_rva=*(DWORD *)text;text_size=*(DWORD *)(text+4);
    if(text_size+8!=mask_size)return 3;
    type=strtoul(argv[3],NULL,0);expected=strtoul(argv[4],NULL,0);
    if(type!=4 && type!=5 && type!=6)return 3;
    log_file=fopen(argv[8],"wt");if(!log_file)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[6],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[7],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE)return 3;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[9],&startup,&launch))return 4;
    root_pid=launch.dwProcessId;root_tid=launch.dwThreadId;
    CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    for(;;){
        if(!WaitForDebugEvent(&event,15000)){failed=5;break;}
        status=DBG_CONTINUE;
        if(event.dwProcessId!=root_pid){failed=6;break;}
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=event.u.CreateProcessInfo.hProcess;thread=event.u.CreateProcessInfo.hThread;
            base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            if(base!=0x10000 || !read_memory(0x26b40,&main_original,1) || !write_code(0x26b40,&trap,1)){failed=7;break;}
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        }else if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            code=event.u.Exception.ExceptionRecord.ExceptionCode;site=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
            if(!GetThreadContext(thread,&context)){failed=9;break;}
            if(code==EXCEPTION_BREAKPOINT && site==0x26b40 && !at_main){
                at_main=1;initial=context;
                /* Replace code only after real original CRT startup completed. */
                main_byte=text[8+0x26b40-base-text_rva];
                if(!write_code(base+text_rva,text+8,text_size) || !read_memory(0x7ff98+type*16,table,16) || table[2]!=expected){failed=10;break;}
                remote_input=(DWORD)allocate_remote(process,NULL,size+64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                synthetic_stack=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                if(!remote_input || synthetic_stack==0xf000 || !sentinel){failed=11;break;}
                guarded=malloc(size+64);memset(guarded,0xa5,size+64);memcpy(guarded+32,payload,size);
                frame[0]=sentinel;frame[1]=remote_input+32;frame[2]=size;
                if(!write_memory(remote_input,guarded,size+64) || !write_memory(synthetic_stack,frame,12) || !write_code(sentinel,&trap,1)){failed=12;break;}
                context.Eip=table[2];context.Esp=synthetic_stack;
                fprintf(log_file,"{\"event\":\"external_driver\",\"type\":%lu,\"table\":",type);words(table,4);
                fprintf(log_file,",\"sentinel\":%lu,\"input\":%lu,\"size\":%lu,\"original_main_byte\":%u,\"planned_main_byte\":%u}\n",sentinel,remote_input+32,size,main_original,main_byte);
                if(!snapshot("entry",&context) || !SetThreadContext(thread,&context)){failed=13;break;}
            }else if(code==EXCEPTION_BREAKPOINT && site==sentinel && at_main){
                unsigned char *after=malloc(size+64);
                if(!read_memory(remote_input,after,size+64) || memcmp(after,guarded,size+64) ||
                    context.Ebx!=initial.Ebx || context.Esi!=initial.Esi || context.Edi!=initial.Edi || context.Ebp!=initial.Ebp ||
                    context.Esp!=synthetic_stack+4){failed=14;break;}
                free(after);context.Eip=site;
                if(!snapshot(phase?"flush_return":"return",&context)){failed=15;break;}
                if(!phase){
                    phase=1;frame[0]=sentinel;frame[1]=0x85070;
                    if(!write_memory(synthetic_stack,frame,8)){failed=16;break;}
                    context.Esp=synthetic_stack;context.Eip=0x50e00;
                    fprintf(log_file,"{\"event\":\"external_fflush\",\"stream\":%lu}\n",frame[1]);fflush(log_file);
                    if(!SetThreadContext(thread,&context)){failed=16;break;}
                }else{stopped=1;TerminateProcess(process,0);}
            }else if(code==EXCEPTION_BREAKPOINT && !at_main && (site<base+text_rva || site>=base+text_rva+text_size)){
                /* Initial loader breakpoint only. */
            }else{
                context.Eip=site;snapshot("unexpected_trap",&context);
                fprintf(log_file,"{\"event\":\"frontier\",\"code\":%lu,\"va\":%lu}\n",code,site);fflush(log_file);
                stopped=2;TerminateProcess(process,17);
            }
        }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=event.u.ExitProcess.dwExitCode;ContinueDebugEvent(root_pid,event.dwThreadId,status);break;}
        if(!ContinueDebugEvent(root_pid,event.dwThreadId,status)){failed=18;break;}
    }
    if(failed && process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,GetFileSize(output,NULL),GetFileSize(error,NULL));
    fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
