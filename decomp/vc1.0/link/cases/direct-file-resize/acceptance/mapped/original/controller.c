/* Explicit original-only external caller for the LINK file subsystem.
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
static DWORD teb,memory,stack_top,sentinel,file_handle,phase,mapped;
static DWORD target,argument_count,arguments[8];
static DWORD file_table,slot;
static unsigned char payload[12289],mask_byte,return_byte;
static CONTEXT entered,initial;
static int in_call;
static DWORD child_site,child_return;
static unsigned char child_bytes[2],child_return_byte;
static CONTEXT child_entry;
static DWORD api_sites[3]={0x5b188,0x5b18e,0x53740},api_return,api_index,api_frame[5];
static unsigned char api_bytes[3],api_return_byte;
static CONTEXT api_entry;
static int api_armed;
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
    DWORD state[10],errors[3],page_table[8],page_head[6],page_tail[4],i,j;
    unsigned char buffer[64];
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,target,c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);
    words(arguments,argument_count);fprintf(log_file,",\"errors\":");words(errors,3);
    if(phase>=2 && read_memory(0x5c000,&file_table,4) &&
       read_memory(file_table+file_handle*4,&slot,4) && read_memory(slot,state,sizeof(state))){
        fprintf(log_file,",\"file_handle\":%lu,\"slot_address\":%lu,\"slot\":",file_handle,slot);words(state,10);
        if(state[4] && !(state[1]&8) && state[5] && state[7]>=32768){
            if(!read_memory(state[5],page_table,32))return 0;
            fprintf(log_file,",\"page_table\":");words(page_table,8);fprintf(log_file,",\"pages\":[");
            for(i=0,j=0;i<8;++i)if(page_table[i]>1){
                if(!read_memory(page_table[i],page_head,24)||!read_memory(page_table[i]+4120,page_tail,16))return 0;
                if(j++)fputc(',',log_file);fprintf(log_file,"{\"index\":%lu,\"address\":%lu,\"head\":",i,page_table[i]);words(page_head,6);fprintf(log_file,",\"tail\":");words(page_tail,4);fputc('}',log_file);
            }
            fputc(']',log_file);
        }
    }
    if(!read_memory(memory+16384,buffer,64))return 0;
    fprintf(log_file,",\"read_buffer\":");bytes(buffer,64);fprintf(log_file,"}\n");fflush(log_file);
    return 1;
}
static int child_snapshot(char *event,CONTEXT *c)
{
    DWORD old_target=target,old_count=argument_count,old_arguments[8],frame[3];int result;
    memcpy(old_arguments,arguments,sizeof(arguments));target=child_site;argument_count=2;
    if(!read_memory(child_entry.Esp,frame,12))return 0;arguments[0]=frame[1];arguments[1]=frame[2];
    result=snapshot(event,c);target=old_target;argument_count=old_count;memcpy(arguments,old_arguments,sizeof(arguments));return result;
}
static int api_snapshot(char *event,CONTEXT *c)
{
    DWORD errors[3];
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"api\":%lu,\"esp\":%lu,\"eax\":%lu,\"caller\":%lu,\"arguments\":",event,phase,api_sites[api_index],c->Esp,c->Eax,api_frame[0]);
    words(api_frame+1,api_index==0?4:(api_index==1?1:2));fprintf(log_file,",\"errors\":");words(errors,3);
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));fprintf(log_file,"}\n");fflush(log_file);return 1;
}
static int arm_apis(int arm)
{
    unsigned i,first=mapped?0:2,last=mapped?2:3;
    if(mapped&&phase!=8)return 1;
    for(i=first;i<last;++i){
        if(arm){if(!read_memory(api_sites[i],api_bytes+i,1)||api_bytes[i]==0xcc||!code(api_sites[i],0xcc))return 0;}
        else if(!code(api_sites[i],api_bytes[i]))return 0;
    }
    api_armed=arm;return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[9],i;
    argument_count=2;arguments[0]=file_handle;arguments[1]=0;
    switch(phase){
    case 0:target=0x3c020;argument_count=7;arguments[0]=30;arguments[1]=16;arguments[2]=14;arguments[3]=8;arguments[4]=6;arguments[5]=mapped;arguments[6]=0x26b10;break;
    case 1:case 12:target=0x3c470;argument_count=3;arguments[0]=memory;arguments[1]=phase==1?0x8102:0x8000;arguments[2]=0x180;break;
    case 2:target=0x3ce90;argument_count=3;arguments[1]=memory+1024;arguments[2]=sizeof(payload);break;
    case 3:target=0x3caf0;argument_count=3;arguments[1]=9000;arguments[2]=0;break;
    case 4:case 5:case 8:target=0x3c340;arguments[1]=phase==4?sizeof(payload):(phase==5?0x500001:4097);break;
    case 6:case 9:target=0x3caf0;argument_count=3;arguments[1]=0;arguments[2]=0;break;
    case 7:case 10:case 13:target=0x3cc20;argument_count=3;arguments[1]=memory+16384;arguments[2]=64;break;
    case 11:case 14:target=0x3c6d0;arguments[1]=1;break;
    default:return 0;
    }
    frame[0]=sentinel;for(i=0;i<argument_count;++i)frame[i+1]=arguments[i];
    if(!write_memory(stack_top,frame,(argument_count+1)*4)||!read_memory(target,&mask_byte,1))return 0;
    if(mask_byte!=0xcc && !code(target,0xcc))return 0;
    c->Eip=target;c->Esp=stack_top;
    return SetThreadContext(thread,c);
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT ev;CONTEXT c;DWORD pid,tid,site,status,n,rva,size,got,failed=0,stopped=0,exit_code=999,at_main=0,main_return;
    unsigned char original_main,*mask;char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    if(argc!=9)return 2;mapped=!strcmp(argv[2],"mapped");if(!mapped&&strcmp(argv[2],"buffered"))return 2;
    f=fopen(argv[3],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    f=fopen(argv[8],"rb");if(!f||fread(payload,1,sizeof(payload),f)!=sizeof(payload)||fgetc(f)!=EOF||fclose(f))return 3;
    log_file=fopen(argv[6],"wt");if(!log_file)return 3;
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    output=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[5],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
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
                if(!memory||stack_top==0xf000||!sentinel||!code(sentinel,0xcc)||!write_memory(memory,"work.bin",9)||!write_memory(memory+1024,payload,sizeof(payload))||!read_memory(0x3c380,child_bytes,1)||!read_memory(0x3c3f0,child_bytes+1,1)||!code(0x3c380,0xcc)||!code(0x3c3f0,0xcc)||!prepare(&c)){failed=11;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(api_armed&&!api_return&&(site==api_sites[0]||site==api_sites[1]||site==api_sites[2])){
                    DWORD count;api_index=site==api_sites[0]?0:(site==api_sites[1]?1:2);count=api_index==0?4:(api_index==1?1:2);api_entry=c;
                    if(!read_memory(c.Esp,api_frame,(count+1)*4)||!api_snapshot("api_entry",&c)){failed=24;break;}
                    api_return=api_frame[0];if(!read_memory(api_return,&api_return_byte,1)||!code(api_return,0xcc)||!code(site,api_bytes[api_index])||!SetThreadContext(thread,&c)){failed=25;break;}
                }else if(api_return&&site==api_return){
                    DWORD popped=api_index==0?20:(api_index==1?8:4);
                    if(c.Esp!=api_entry.Esp+popped||c.Ebx!=api_entry.Ebx||c.Esi!=api_entry.Esi||c.Edi!=api_entry.Edi||c.Ebp!=api_entry.Ebp||!api_snapshot("api_return",&c)||!code(api_return,api_return_byte)||!code(api_sites[api_index],0xcc)||!SetThreadContext(thread,&c)){failed=26;break;}api_return=0;
                }else if((site==0x3c380||site==0x3c3f0)&&!child_return){
                    unsigned index=site==0x3c380?0:1;child_site=site;child_entry=c;
                    if(!child_snapshot(child_bytes[index]==0xcc?"frontier":"entry",&c)){failed=21;break;}
                    if(child_bytes[index]==0xcc){stopped=2;TerminateProcess(process,17);}
                    else if(!arm_apis(1)||!read_memory(c.Esp,&child_return,4)||!read_memory(child_return,&child_return_byte,1)||!code(child_return,0xcc)||!code(site,child_bytes[index])||!SetThreadContext(thread,&c)){failed=22;break;}
                }else if(child_return&&site==child_return){
                    if(c.Esp!=child_entry.Esp+4||c.Ebx!=child_entry.Ebx||c.Esi!=child_entry.Esi||c.Edi!=child_entry.Edi||c.Ebp!=child_entry.Ebp||!child_snapshot("return",&c)||!arm_apis(0)||!code(child_return,child_return_byte)||!code(child_site,0xcc)||!SetThreadContext(thread,&c)){failed=23;break;}
                    child_return=0;
                }else if(site==target&&!in_call){
                    if(!snapshot(mask_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(mask_byte==0xcc){stopped=2;TerminateProcess(process,17);}
                    else{entered=c;in_call=1;if(!code(target,mask_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&in_call){
                    if(c.Esp!=entered.Esp+4||c.Ebx!=entered.Ebx||c.Esi!=entered.Esi||c.Edi!=entered.Edi||c.Ebp!=entered.Ebp||!snapshot("return",&c)){failed=14;break;}
                    in_call=0;if(phase==1||phase==12)file_handle=c.Eax;
                    if((phase==1||phase==12)&&file_handle>15){failed=15;break;}
                    if(phase==7||phase==10||phase==13){unsigned char got_bytes[64];if(c.Eax!=64||!read_memory(memory+16384,got_bytes,64)||memcmp(got_bytes,payload,64)){failed=16;break;}}
                    ++phase;if(phase==15){stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=0;if(!SetThreadContext(thread,&c)){failed=17;break;}}
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
