/* Explicit original-only external caller for the LINK BIND parser lifecycle.
 * All target calls use the original cdecl ABI; this controller is not a vertex. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "editor-image.h"
#include <recon.h>
typedef char bound_size[(sizeof(BoundImage)==36)?1:-1];
typedef char bound_offsets[(offsetof(BoundImage,file)==12 && offsetof(BoundImage,base)==16 && offsetof(BoundImage,headers)==20 && offsetof(BoundImage,current_section)==24 && offsetof(BoundImage,section_count)==28 && offsetof(BoundImage,sections)==32)?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;
static FILE *log_file;
static DWORD teb,memory,stack_top,sentinel,phase;
static DWORD target,argument_count,arguments[4];
static unsigned char mask_byte,payload[1024];
static CONTEXT entered,initial;
static int in_call;
static DWORD report_rows[7][11];
static FILE *report;
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
static int string_at(DWORD address,char *buffer,DWORD capacity)
{
    DWORD i;for(i=0;i<capacity;++i){if(!read_memory(address+i,buffer+i,1))return 0;if(!buffer[i])return 1;}return 0;
}
static int snapshot(char *event,CONTEXT *c)
{
    DWORD errors[3],globals[5],workspace[9],actual[4],vectors[24],i,n;
    unsigned char names[1040],guards[56],headers[248],sections[640];
    int live=(phase==2&&!strcmp(event,"return"))||(phase==3&&strcmp(event,"return"));
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||
       !read_memory(0x6d23c,globals,4)||!read_memory(0x6d25c,globals+1,4)||
       !read_memory(0x6d4a0,globals+2,8)||!read_memory(0x7f8d4,globals+4,4)||
       !read_memory(memory,names,sizeof(names))||!read_memory(memory+2048,vectors,sizeof(vectors))||
       !read_memory(memory+4096,workspace,sizeof(workspace))||!read_memory(memory+4092,guards,sizeof(guards))||
       !read_memory((!strcmp(event,"return")?entered.Esp:c->Esp)+4,actual,argument_count*4))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,target,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(arguments,argument_count);
    fputs(",\"stack_argument_words\":",log_file);words(actual,argument_count);
    fputs(",\"errors\":",log_file);words(errors,3);fputs(",\"globals\":",log_file);words(globals,5);
    fprintf(log_file,",\"names_address\":%lu,\"names\":",memory);bytes(names,sizeof(names));
    fprintf(log_file,",\"vectors_address\":%lu,\"vectors\":",memory+2048);words(vectors,24);
    fprintf(log_file,",\"workspace_address\":%lu,\"workspace\":",memory+4096);words(workspace,9);
    fputs(",\"workspace_guards\":",log_file);bytes(guards,sizeof(guards));fprintf(log_file,",\"mapping_live\":%d",live);
    if(live){n=workspace[7];if(n>16||!read_memory(workspace[5],headers,sizeof(headers))||!read_memory(workspace[8],sections,n*40))return 0;
        fputs(",\"headers\":",log_file);bytes(headers,sizeof(headers));fputs(",\"sections\":",log_file);bytes(sections,n*40);}
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));
    fputs("}\n",log_file);fflush(log_file);
    if(!strcmp(event,"return")){
        DWORD *r=report_rows[phase];r[0]=phase;r[1]=c->Eax;r[2]=globals[1];r[3]=globals[2];r[4]=globals[4];
        r[5]=globals[0]?globals[0]-memory:0xffffffffUL;r[6]=globals[3]?globals[3]-memory:0xffffffffUL;
        memcpy(r+7,errors,12);r[10]=phase>=2?workspace[7]:0;
    }
    return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[5],workspace[9],i;
    if(!read_memory(memory+4096,workspace,sizeof(workspace)))return 0;
    target=0x4faf0;argument_count=2;
    if(phase==0){arguments[0]=1;arguments[1]=memory+2048;}
    else if(phase==1){arguments[0]=5;arguments[1]=memory+2072;}
    else if(phase==5){arguments[0]=3;arguments[1]=memory+2096;}
    else if(phase==6){arguments[0]=3;arguments[1]=memory+2120;}
    else if(phase==2){
        target=0x4f880;argument_count=4;
        if(!read_memory(0x6d23c,arguments,4)||!read_memory(0x6d4a0,arguments+3,4))return 0;
        arguments[1]=memory+4096;arguments[2]=0;
    }else if(phase==3){target=0x5b1b2;argument_count=1;arguments[0]=workspace[4];}
    else if(phase==4){target=0x5b19a;argument_count=1;arguments[0]=workspace[3];}
    else return 0;
    frame[0]=sentinel;for(i=0;i<argument_count;++i)frame[i+1]=arguments[i];
    if(!write_memory(stack_top,frame,(argument_count+1)*4)||!read_memory(target,&mask_byte,1))return 0;
    if(mask_byte!=0xcc&&!code(target,0xcc))return 0;c->Eip=target;c->Esp=stack_top;
    return SetThreadContext(thread,c);
}
static int write_report(void)
{
    DWORD header[3]={0x31444e42,1,7};unsigned char names[1024];
    if(!read_memory(memory,names,sizeof(names)))return 0;
    return fwrite(header,4,3,report)==3&&fwrite(names,1,sizeof(names),report)==sizeof(names)&&fwrite(report_rows,1,sizeof(report_rows),report)==sizeof(report_rows);
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT ev;CONTEXT c;DWORD pid,tid,site,status,n,rva,size,got,failed=0,stopped=0,exit_code=999,at_main=0,main_return,argv_data[24],i;
    unsigned char guard[16];
    unsigned char original_main,*mask;char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    if(argc!=9)return 2;
    f=fopen(argv[2],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    f=fopen(argv[8],"rb");if(!f||fread(payload,1,1024,f)!=1024||fgetc(f)!=EOF||fclose(f))return 3;
    log_file=fopen(argv[5],"wt");report=fopen(argv[6],"wb");if(!log_file||!report)return 3;
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[7],&startup,&launch))return 4;
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
                memset(guard,0xa5,16);memset(argv_data,0,sizeof(argv_data));
                argv_data[0]=memory;
                for(i=0;i<5;++i)argv_data[6+i]=memory+256+i*32;
                for(i=0;i<3;++i){argv_data[12+i]=memory+512+i*32;argv_data[18+i]=memory+768+i*32;}
                if(!memory||stack_top==0xf000||!sentinel||!code(sentinel,0xcc)||!write_memory(memory,payload,1024)||!write_memory(memory+1024,guard,16)||!write_memory(memory+2048,argv_data,sizeof(argv_data))||!write_memory(memory+4092,guard,4)||!write_memory(memory+4132,guard,16)||!prepare(&c)){failed=11;break;}

            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==target&&!in_call){
                    if(!snapshot(mask_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(mask_byte==0xcc){stopped=2;TerminateProcess(process,17);}
                    else{entered=c;in_call=1;if(!code(target,mask_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&in_call){
                    if(c.Esp!=entered.Esp+((phase==3||phase==4)?8:4)||c.Ebx!=entered.Ebx||c.Esi!=entered.Esi||c.Edi!=entered.Edi||c.Ebp!=entered.Ebp){failed=14;break;}
                    if(!snapshot("return",&c)){failed=22;break;}
                    in_call=0;++phase;if(phase==7){
                        if(!write_report()){failed=23;break;}stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=0;
                        if(c.Ebx!=initial.Ebx||c.Esi!=initial.Esi||c.Edi!=initial.Edi||c.Ebp!=initial.Ebp||!SetThreadContext(thread,&c)){failed=17;break;}
                    }else if(!prepare(&c)){failed=18;break;}
                }else{fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!at_main){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"phase\":%lu,\"va\":%lu,\"code\":%lu}\n",phase,site,ev.u.Exception.ExceptionRecord.ExceptionCode);failed=19;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=20;break;}
    }
    if(failed&&process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,GetFileSize(output,NULL),GetFileSize(error,NULL));
    fclose(report);fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
