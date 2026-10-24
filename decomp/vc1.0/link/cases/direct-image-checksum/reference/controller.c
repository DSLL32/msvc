/* Explicit original-only external caller for the LINK image-helper lifecycle.
 * All target calls use the original cdecl ABI; this controller is not a vertex. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "files.h"
#include <recon.h>
typedef char slot_size[(sizeof(FileSlot)==40)?1:-1];
typedef char slot_offsets[(offsetof(FileSlot,length)==12 && offsetof(FileSlot,contents)==24 && offsetof(FileSlot,cursor)==32 && offsetof(FileSlot,next)==36)?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;
static FILE *log_file;
static DWORD teb,memory,stack_top,sentinel,phase;
static DWORD target,argument_count,arguments[4];
static unsigned char mask_byte;
static CONTEXT entered,initial;
static int in_call;
static char stage_directory[520],work_directory[520];
static int read_memory(DWORD address,void *buffer,DWORD count)
{
    DWORD got;
    return ReadProcessMemory(process,(void *)address,buffer,count,&got)&&got==count;
}
static int write_memory(DWORD address,const void *buffer,DWORD count)
{
    DWORD got;
    return WriteProcessMemory(process,(void *)address,(void *)buffer,count,&got)&&got==count;
}
static int code(DWORD address,unsigned char byte)
{
    DWORD old,again;
    return VirtualProtectEx(process,(void *)address,1,PAGE_EXECUTE_READWRITE,&old)&&
        write_memory(address,&byte,1)&&FlushInstructionCache(process,(void *)address,1)&&
        VirtualProtectEx(process,(void *)address,1,old,&again);
}
static void words(DWORD *values,DWORD count)
{
    DWORD i;fputc('[',log_file);
    for(i=0;i<count;++i)fprintf(log_file,"%s%lu",i?",":"",values[i]);
    fputc(']',log_file);
}
static void bytes(unsigned char *values,DWORD count)
{
    DWORD i;fputc('"',log_file);
    for(i=0;i<count;++i)fprintf(log_file,"%02x",values[i]);
    fputc('"',log_file);
}
static int snapshot(char *event,CONTEXT *c)
{
    DWORD errors[3],workspace[9],count,i;
    unsigned char outputs[16],headers[248],sections[640];
    int live=(phase==2 || (phase==1&&!strcmp(event,"return")) || (phase==3&&strcmp(event,"return")));
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||
       !read_memory(memory+512,workspace,sizeof(workspace))||!read_memory(memory+1024,outputs,16))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,target,c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);
    words(arguments,argument_count);fprintf(log_file,",\"errors\":");words(errors,3);
    fprintf(log_file,",\"output_address\":%lu,\"outputs\":",memory+1024);bytes(outputs,16);
    fprintf(log_file,",\"workspace_address\":%lu,\"workspace\":",memory+512);words(workspace,9);
    fprintf(log_file,",\"mapping_live\":%d",live);
    if(live){
        count=workspace[7];if(count>16||!read_memory(workspace[5],headers,sizeof(headers))||
            !read_memory(workspace[8],sections,count*40))return 0;
        fprintf(log_file,",\"headers\":");bytes(headers,sizeof(headers));fprintf(log_file,",\"sections\":");bytes(sections,count*40);
    }
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));
    fprintf(log_file,"}\n");fflush(log_file);return 1;
}
static int save_stage(void)
{
    char from[600],to[600];
    if(phase!=0&&phase!=4&&phase!=5&&phase!=6&&phase!=7&&phase!=8&&phase!=9)return 1;
    sprintf(from,"%s\\client.exe",work_directory);sprintf(to,"%s\\phase%lu.exe",stage_directory,phase);
    return CopyFileA(from,to,FALSE);
}
static int prepare(CONTEXT *c)
{
    DWORD frame[5],workspace[9],i;
    if(!read_memory(memory+512,workspace,sizeof(workspace)))return 0;
    argument_count=3;target=0x4ec70;arguments[0]=memory;arguments[1]=memory+1028;arguments[2]=memory+1036;
    switch(phase){
    case 0:case 5:case 7:break;
    case 1:target=0x4f880;argument_count=4;arguments[1]=memory+512;arguments[2]=0;arguments[3]=0;break;
    case 2:target=0x4f4f0;argument_count=1;arguments[0]=memory+512;break;
    case 3:target=0x5b1b2;argument_count=1;arguments[0]=workspace[4];break;
    case 4:target=0x5b19a;argument_count=1;arguments[0]=workspace[3];break;
    case 6:case 8:arguments[2]=0;break;
    case 9:arguments[0]=memory+32;break;
    default:return 0;
    }
    frame[0]=sentinel;for(i=0;i<argument_count;++i)frame[i+1]=arguments[i];
    if(!write_memory(stack_top,frame,(argument_count+1)*4)||!read_memory(target,&mask_byte,1))return 0;
    if(mask_byte!=0xcc&&!code(target,0xcc))return 0;c->Eip=target;c->Esp=stack_top;
    return SetThreadContext(thread,c);
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT ev;CONTEXT c;DWORD pid,tid,site,status,n,rva,size,got,failed=0,stopped=0,exit_code=999,at_main=0,main_return,popped;
    unsigned char owned[1100];
    unsigned char original_main,*mask;char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    if(argc!=8)return 2;
    strcpy(work_directory,argv[6]);strcpy(stage_directory,argv[7]);
    f=fopen(argv[2],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    log_file=fopen(argv[5],"wt");if(!log_file)return 3;
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[6],&startup,&launch))return 4;
    pid=launch.dwProcessId;tid=launch.dwThreadId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    for(;;){
        if(!WaitForDebugEvent(&ev,15000)){failed=5;break;}status=DBG_CONTINUE;
        if(ev.dwProcessId!=pid){failed=6;break;}
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=ev.u.CreateProcessInfo.hProcess;thread=ev.u.CreateProcessInfo.hThread;teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;
            if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x10000||!read_memory(0x26b40,&original_main,1)||!code(0x26b40,0xcc)){failed=7;break;}
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        }else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        }else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
            if(!GetThreadContext(thread,&c)){failed=9;break;}
            if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x26b40&&!at_main){
                DWORD old,again;at_main=1;initial=c;initial.Eip=site;
                if(!VirtualProtectEx(process,(void *)(0x10000+rva),size,PAGE_EXECUTE_READWRITE,&old)||!write_memory(0x10000+rva,mask+8,size)||!FlushInstructionCache(process,(void *)(0x10000+rva),size)||!VirtualProtectEx(process,(void *)(0x10000+rva),size,old,&again)||!read_memory(c.Esp,&main_return,4)){failed=10;break;}
                memory=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                memset(owned,0xa5,sizeof(owned));memset(owned+512,0,36);strcpy((char *)owned,"client.exe");strcpy((char *)owned+32,"missing.exe");
                if(!memory||stack_top==0xf000||!sentinel||!code(sentinel,0xcc)||!write_memory(memory,owned,sizeof(owned))||!prepare(&c)){failed=11;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==target&&!in_call){
                    if(!snapshot(mask_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(mask_byte==0xcc){stopped=2;TerminateProcess(process,17);}
                    else{entered=c;in_call=1;if(!code(target,mask_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&in_call){
                    popped=(phase==3||phase==4)?8:4;
                    if(c.Esp!=entered.Esp+popped||c.Ebx!=entered.Ebx||c.Esi!=entered.Esi||c.Edi!=entered.Edi||c.Ebp!=entered.Ebp||!snapshot("return",&c)||!save_stage()){failed=14;break;}
                    in_call=0;++phase;if(phase==10){stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=0;if(!SetThreadContext(thread,&c)){failed=17;break;}}
                    else if(!prepare(&c)){failed=18;break;}
                }else{fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!at_main){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"phase\":%lu,\"va\":%lu,\"code\":%lu}\n",phase,site,ev.u.Exception.ExceptionRecord.ExceptionCode);failed=19;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=20;break;}
    }
    if(failed&&process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,GetFileSize(output,NULL),GetFileSize(error,NULL));
    fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
