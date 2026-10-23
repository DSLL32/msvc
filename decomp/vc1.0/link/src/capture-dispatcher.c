/* Explicit original-only dispatcher observations. This external direct caller
 * is test transport, never a reconstructed vertex or a claimed public caller. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "link.h"
#include <recon.h>
typedef char tool_size[(sizeof(Tool)==16)?1:-1];
typedef char tool_fields[(offsetof(Tool,length)==4 && offsetof(Tool,kind)==8 && offsetof(Tool,run)==12)?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;static FILE *log_file;static DWORD teb;
static DWORD namesizes[]={7,16,7,7,7,1};
static int read_memory(DWORD a,void *b,DWORD n){DWORD got;return ReadProcessMemory(process,(void *)a,b,n,&got)&&got==n;}
static int write_memory(DWORD a,const void *b,DWORD n){DWORD got;return WriteProcessMemory(process,(void *)a,(void *)b,n,&got)&&got==n;}
static int code(DWORD a,unsigned char c){DWORD old,again;return VirtualProtectEx(process,(void *)a,1,PAGE_EXECUTE_READWRITE,&old)&&write_memory(a,&c,1)&&FlushInstructionCache(process,(void *)a,1)&&VirtualProtectEx(process,(void *)a,1,old,&again);}
static void hex(unsigned char *p,DWORD n){DWORD i;fputc('"',log_file);for(i=0;i<n;++i)fprintf(log_file,"%02x",p[i]);fputc('"',log_file);}
static void words(DWORD *p,DWORD n){DWORD i;fputc('[',log_file);for(i=0;i<n;++i)fprintf(log_file,"%s%lu",i?",":"",p[i]);fputc(']',log_file);}
static int string(DWORD a,unsigned char *p,DWORD n){DWORD i;for(i=0;i<n;++i){if(!read_memory(a+i,p+i,1))return 0;if(!p[i])return i+1;}return 0;}
static int snapshot(const char *event,CONTEXT *c,int helper,int entering)
{
    DWORD stack[3],out[8],err[3],table[24],ctype,mb,pointer,used,i,j,argc=0,argv=0,n;
    unsigned char buffer[4096],text[512],ct[514];
    if(!read_memory(c->Esp,stack,entering&&helper?12:4)||!read_memory(0x85070,out,32)||!read_memory(0x852d8,err,8)||!read_memory(teb+0x34,err+2,4)||!read_memory(0x7b3d8,table,96)||!read_memory(0x7f8c0,&pointer,4)||!read_memory(0x85340,&ctype,4)||!read_memory(ctype-2,ct,514)||!read_memory(0x8554c,&mb,4))return 0;
    used=out[0]-out[2];if(used>4096||(used&&!read_memory(out[2],buffer,used)))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"name\":\"%s\",\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"caller\":%lu,\"formal_words\":[",event,helper?"helper_main":"driver_usage",c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp,stack[0]);
    if(entering&&helper){argc=stack[1];argv=stack[2];if(argc>32)return 0;fprintf(log_file,"%lu,%lu",argc,argv);}fprintf(log_file,"],\"argv\":[");
    for(i=0;i<argc;++i){if(!read_memory(argv+i*4,&j,4)||(n=string(j,text,512))==0)return 0;if(i)fputc(',',log_file);hex(text,n);}if(argc&&(!read_memory(argv+argc*4,&j,4)||j))return 0;
    fprintf(log_file,"],\"program\":");n=string(pointer,text,512);if(!n)return 0;hex(text,n);
    fprintf(log_file,",\"tools\":[");for(i=0;i<6;++i){if(i)fputc(',',log_file);if(!read_memory(table[i*4],text,namesizes[i]))return 0;fprintf(log_file,"{\"name_pointer\":%lu,\"length\":%u,\"padding\":%u,\"kind\":%lu,\"run\":%lu,\"bytes\":",table[i*4],table[i*4+1]&65535,table[i*4+1]>>16,table[i*4+2],table[i*4+3]);hex(text,namesizes[i]);fputc('}',log_file);}fprintf(log_file,"],\"stdout_file\":");words(out,8);fprintf(log_file,",\"pending\":");hex(buffer,used);fprintf(log_file,",\"errors\":");words(err,3);fprintf(log_file,",\"ctype\":");hex(ct,514);fprintf(log_file,",\"mb_cur_max\":%lu}\n",mb);fflush(log_file);return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES security;STARTUPINFOA startup;PROCESS_INFORMATION launch;DEBUG_EVENT ev;CONTEXT c,before,main_entry;
    DWORD pid,tid,site,status,got,n,rva,size,target,ret=0,sentinel=0,original_return=0,failed=0,stopped=0,exit_code=999,entered=0,at_main=0;
    unsigned char original_main,entry_byte,return_byte,*mask;FILE *f;HANDLE output,error;char command[2048];int helper;
    AllocateRemote allocate_remote;
    if(argc!=8)return 2;helper=!strcmp(argv[2],"helper");if(!helper&&strcmp(argv[2],"direct"))return 2;target=helper?0x26fe0:0x27050;
    f=fopen(argv[3],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    log_file=fopen(argv[6],"wt");if(!log_file)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);error=CreateFileA(argv[5],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"%s",argv[1],helper?" -helper":"");
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[7],&startup,&launch))return 4;
    pid=launch.dwProcessId;tid=launch.dwThreadId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    for(;;){
        if(!WaitForDebugEvent(&ev,15000)){failed=5;break;}status=DBG_CONTINUE;
        if(ev.dwProcessId!=pid){failed=6;break;}
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=ev.u.CreateProcessInfo.hProcess;thread=ev.u.CreateProcessInfo.hThread;teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;
            if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x10000||!read_memory(0x26b40,&original_main,1)||!code(0x26b40,0xcc)){failed=7;break;}if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        }else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        }else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;
            memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;if(!GetThreadContext(thread,&c)){failed=9;break;}
            if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x26b40&&!at_main){
                DWORD old,again;at_main=1;main_entry=c;main_entry.Eip=site;
                if(!VirtualProtectEx(process,(void *)(0x10000+rva),size,PAGE_EXECUTE_READWRITE,&old)||!write_memory(0x10000+rva,mask+8,size)||!FlushInstructionCache(process,(void *)(0x10000+rva),size)||!VirtualProtectEx(process,(void *)(0x10000+rva),size,old,&again)||!read_memory(target,&entry_byte,1)){failed=10;break;}
                if(entry_byte!=0xcc&&!code(target,0xcc)){failed=11;break;}
                c.Eip=site;
                if(!helper){
                    DWORD frame;sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);frame=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                    if(!sentinel||frame==0xf000||!code(sentinel,0xcc)||!write_memory(frame,&sentinel,4)||!read_memory(main_entry.Esp,&original_return,4)){failed=12;break;}c.Eip=target;c.Esp=frame;
                }
                if(!SetThreadContext(thread,&c)){failed=13;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==target&&!entered){
                    if(!snapshot(entry_byte==0xcc?"frontier":"entry",&c,helper,1)){failed=14;break;}
                    if(entry_byte==0xcc){stopped=2;TerminateProcess(process,17);}
                    else{before=c;entered=1;if(!read_memory(c.Esp,&ret,4)||!read_memory(ret,&return_byte,1)||!code(target,entry_byte)||!code(ret,0xcc)||!SetThreadContext(thread,&c)){failed=15;break;}}
                }else if(entered&&site==ret){
                    if(c.Esp!=before.Esp+4||c.Ebx!=before.Ebx||c.Esi!=before.Esi||c.Edi!=before.Edi||c.Ebp!=before.Ebp||c.Eax!=3||!snapshot("return",&c,helper,0)){failed=16;break;}
                    entered=0;stopped=1;
                    if(helper){if(!code(ret,return_byte)||!SetThreadContext(thread,&c)){failed=17;break;}}
                    else{c.Eip=original_return;c.Esp=main_entry.Esp+4;if(!SetThreadContext(thread,&c)){failed=18;break;}}
                }else{fprintf(log_file,"{\"event\":\"trap\",\"va\":%lu}\n",site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!at_main){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"va\":%lu,\"code\":%lu}\n",site,ev.u.Exception.ExceptionRecord.ExceptionCode);failed=19;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=20;break;}
    }
    if(failed&&process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,GetFileSize(output,NULL),GetFileSize(error,NULL));fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
