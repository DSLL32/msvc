/* Explicit original-only external caller for the LINK priority archive arguments.
 * All target calls use the original cdecl ABI; this controller is not a vertex. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "arguments.h"
#include <recon.h>
typedef char argument_size[(sizeof(Argument)==16)?1:-1];
typedef char argument_layout[(offsetof(Argument,text)==0 && offsetof(Argument,original)==4 && offsetof(Argument,parsed)==8 && offsetof(Argument,next)==12 && sizeof(ArgumentList)==12)?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;
static FILE *log_file;
static DWORD teb,memory,stack_top,sentinel,phase;
static DWORD target,argument_count,arguments[4];
static unsigned char mask_byte,payload[256];
static CONTEXT entered,initial;
static int in_call;
static DWORD nodes[4],node_count,main_result;
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
static DWORD node_index(DWORD address)
{
    DWORD i;if(!address)return 0xffffffffUL;for(i=0;i<node_count;++i)if(nodes[i]==address)return i;return 0xfffffffeUL;
}
static int snapshot(char *event,CONTEXT *c)
{
    DWORD errors[3],list[3],cursors[2],row[4],i,actual[3],argv[5];
    unsigned char names[272];char text[512];
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||
       !read_memory(0x7b7e0,list,12)||!read_memory(0x5c42c,cursors,8)||!read_memory(memory,names,272)||
       !read_memory((!strcmp(event,"return")?entered.Esp:c->Esp)+4,actual,argument_count*4)||((phase!=4||strcmp(event,"return"))&&memcmp(actual,arguments,argument_count*4)))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,target,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(arguments,argument_count);fputs(",\"stack_argument_words\":",log_file);words(actual,argument_count);
    fprintf(log_file,",\"errors\":");words(errors,3);fprintf(log_file,",\"names_address\":%lu,\"names\":",memory);bytes(names,272);
    fputs(",\"list\":",log_file);words(list,3);fputs(",\"cursors\":",log_file);words(cursors,2);fputs(",\"nodes\":[",log_file);
    for(i=0;i<node_count;++i){
        if(!read_memory(nodes[i],row,16))return 0;if(i)fputc(',',log_file);
        fprintf(log_file,"{\"address\":%lu,\"raw\":",nodes[i]);words(row,4);fputc('}',log_file);
    }
    fputc(']',log_file);
    if(phase==4){
        if(!read_memory(memory+512,argv,20))return 0;
        fputs(",\"argv\":",log_file);words(argv,5);fputs(",\"argv_names\":[",log_file);
        for(i=0;i<5;++i){if(i)fputc(',',log_file);if(!argv[i])fputs("null",log_file);else{if(!string_at(argv[i],text,sizeof(text)))return 0;bytes(text,strlen(text)+1);}}fputc(']',log_file);
    }
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));
    fputs("}\n",log_file);fflush(log_file);return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[4],i;
    if(phase<4){target=0x20a10;argument_count=3;arguments[0]=arguments[1]=memory+phase*32;arguments[2]=phase&1;}
    else if(phase==4){target=0x26b40;argument_count=2;arguments[0]=4;arguments[1]=memory+512;}
    else return 0;
    frame[0]=sentinel;for(i=0;i<argument_count;++i)frame[i+1]=arguments[i];
    if(!write_memory(stack_top,frame,(argument_count+1)*4)||!read_memory(target,&mask_byte,1))return 0;
    if(mask_byte!=0xcc&&!code(target,0xcc))return 0;c->Eip=target;c->Esp=stack_top;
    return SetThreadContext(thread,c);
}
static int write_report(void)
{
    DWORD header[8],list[3],cursors[2],row[4],result[4],i;unsigned char names[256];
    if(!read_memory(0x7b7e0,list,12)||!read_memory(0x5c42c,cursors,8)||!read_memory(memory,names,256))return 0;
    header[0]=0x31495250;header[1]=1;header[2]=main_result;header[3]=list[2]&0xffff;
    header[4]=node_index(list[0]);header[5]=node_index(list[1]);header[6]=node_index(cursors[0]);header[7]=node_index(cursors[1]);
    if(fwrite(header,4,8,report)!=8||fwrite(names,1,256,report)!=256)return 0;
    for(i=0;i<4;++i){
        if(!read_memory(nodes[i],row,16))return 0;result[0]=i;result[1]=row[0]-memory;result[2]=row[1]-memory;result[3]=node_index(row[3]);
        if(fwrite(result,4,4,report)!=4)return 0;
    }
    return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT ev;CONTEXT c;DWORD pid,tid,site,status,n,rva,size,got,failed=0,stopped=0,exit_code=999,at_main=0,main_return,argv_data[5];
    unsigned char guard[16];
    unsigned char original_main,*mask;char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    if(argc!=9)return 2;
    f=fopen(argv[2],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    f=fopen(argv[8],"rb");if(!f||fread(payload,1,256,f)!=256||fgetc(f)!=EOF||fclose(f))return 3;
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
                memset(guard,0xa5,16);argv_data[0]=memory+768;argv_data[1]=memory+1024;argv_data[2]=memory+1088;argv_data[3]=memory+1152;argv_data[4]=0;
                if(!memory||stack_top==0xf000||!sentinel||!code(sentinel,0xcc)||!write_memory(memory,payload,256)||!write_memory(memory+256,guard,16)||!write_memory(memory+512,argv_data,20)||!write_memory(memory+768,argv[1],strlen(argv[1])+1)||!write_memory(memory+1024,"-dump",6)||!write_memory(memory+1088,"/nologo",8)||!write_memory(memory+1152,"/headers",9)||!prepare(&c)){failed=11;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==target&&!in_call){
                    if(!snapshot(mask_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(mask_byte==0xcc){stopped=2;TerminateProcess(process,17);}
                    else{entered=c;in_call=1;if(!code(target,mask_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&in_call){
                    if(c.Esp!=entered.Esp+4||c.Ebx!=entered.Ebx||c.Esi!=entered.Esi||c.Edi!=entered.Edi||c.Ebp!=entered.Ebp){failed=14;break;}
                    if(phase<4){if(!c.Eax){failed=21;break;}nodes[node_count++]=c.Eax;}else main_result=c.Eax;
                    if(!snapshot("return",&c)){failed=22;break;}
                    in_call=0;++phase;if(phase==5){
                        if(!write_report()){failed=23;break;}stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=main_result;
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
