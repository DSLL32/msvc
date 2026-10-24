/* External test caller using the established owned cdecl stack/sentinel seam. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "link-pass.h"
#include <recon.h>
typedef LPVOID (WINAPI *AllocateRemote)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
typedef char ModuleDebugInfoSize[sizeof(ModuleDebugInfo)==76?1:-1];
typedef char LibraryListSize[sizeof(LibraryList)==16?1:-1];
typedef char ModuleOwnerSize[(sizeof(Module)==56&&offsetof(Module,path)==4&&offsetof(Module,library)==48)?1:-1];
typedef char LibraryOwnerSize[(sizeof(Library)==48&&offsetof(Library,name)==4)?1:-1];
static HANDLE process,thread;
static FILE *log_file,*report;
static DWORD teb,memory,stack_top,sentinel,main_return,phase,target,args[3],count;
static DWORD image,main_result,module_owner,module_count,clock_pointer;
static CONTEXT initial,before,main_done;
static unsigned char entry_byte,clock_byte,clock_return_byte,payload[256];
static int active,clock_active,last_error_armed;
static int read_memory(DWORD address,void *buffer,DWORD size)
{ DWORD got;return ReadProcessMemory(process,(void *)address,buffer,size,&got)&&got==size; }
static int write_memory(DWORD address,const void *buffer,DWORD size)
{ DWORD got;return WriteProcessMemory(process,(void *)address,(void *)buffer,size,&got)&&got==size; }
static int code(DWORD address,unsigned char byte)
{
    DWORD old,again;
    return VirtualProtectEx(process,(void *)address,1,PAGE_EXECUTE_READWRITE,&old)&&
        write_memory(address,&byte,1)&&FlushInstructionCache(process,(void *)address,1)&&
        VirtualProtectEx(process,(void *)address,1,old,&again);
}
static void bytes(const void *data,DWORD size)
{ DWORD i;const unsigned char *p=data;fputc('"',log_file);for(i=0;i<size;++i)fprintf(log_file,"%02x",p[i]);fputc('"',log_file); }
static void words(DWORD *data,DWORD size)
{ DWORD i;fputc('[',log_file);for(i=0;i<size;++i)fprintf(log_file,"%s%lu",i?",":"",data[i]);fputc(']',log_file); }
static int string_at(DWORD address,char *text,DWORD capacity)
{
    DWORD i;if(!address)return 0;
    for(i=0;i<capacity;++i){if(!read_memory(address+i,text+i,1))return 0;if(!text[i])return 1;}
    return 0;
}
static int modules(void)
{
    ModuleDebugInfo row;Module owner;Library library;DWORD i;char text[512];
    if(!read_memory(0x7b794,&module_owner,4)||!read_memory(0x7b798,&module_count,4)||module_count>64)return 0;
    fprintf(log_file,"{\"owner\":%lu,\"count\":%lu,\"rows\":[",module_owner,module_count);
    for(i=0;i<module_count;++i){
        if(!module_owner||!read_memory(module_owner+i*76,&row,76)||!row.module)return 0;
        if(!read_memory((DWORD)row.module,&owner,sizeof(owner))||!read_memory((DWORD)owner.library,&library,sizeof(library))||row.module_name!=owner.path||row.library_name!=library.name)return 0;
        if(i)fputc(',',log_file);fprintf(log_file,"{\"address\":%lu,\"raw\":",module_owner+i*76);bytes(&row,76);
        fprintf(log_file,",\"owner_edges\":{\"module\":%lu,\"path\":%lu,\"library\":%lu,\"library_name\":%lu,\"source_is_path\":%d}",(DWORD)row.module,(DWORD)owner.path,(DWORD)owner.library,(DWORD)library.name,row.source_name==owner.path);
        fprintf(log_file,",\"names\":[");
        if(!string_at((DWORD)row.library_name,text,sizeof(text)))return 0;bytes(text,strlen(text)+1);fputc(',',log_file);
        if(!string_at((DWORD)row.module_name,text,sizeof(text)))return 0;bytes(text,strlen(text)+1);fputc(',',log_file);
        if(!string_at((DWORD)row.source_name,text,sizeof(text)))return 0;bytes(text,strlen(text)+1);fputs("]}",log_file);
    }
    fputs("]}",log_file);return 1;
}
static int libraries(void)
{
    Image value;DefaultLibrary row;DWORD next,seen=0;char text[128];
    if(!image){fputs("null",log_file);return 1;}
    if(!read_memory(image,&value,sizeof(value)))return 0;
    fprintf(log_file,"{\"image\":%lu,\"list\":%lu,\"header\":",image,image+offsetof(Image,libraries));bytes(&value.libraries,sizeof(value.libraries));fputs(",\"rows\":[",log_file);
    next=(DWORD)value.libraries.default_libraries;
    while(next){
        if(++seen>16||!read_memory(next,&row,sizeof(row))||!string_at((DWORD)row.name,text,sizeof(text)))return 0;
        if(seen>1)fputc(',',log_file);fprintf(log_file,"{\"address\":%lu,\"raw\":",next);bytes(&row,sizeof(row));fputs(",\"name\":",log_file);bytes(text,strlen(text)+1);fputc('}',log_file);next=(DWORD)row.next;
    }
    fputs("]}",log_file);return 1;
}
static int snapshot(const char *event,CONTEXT *c)
{
    DWORD errors[3],stream[8],used,argv[4],i;unsigned char text[256],guard[16],pending[4096];char name[512];
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||!read_memory(0x85070,stream,32)||!read_memory(memory,text,256)||!read_memory(memory+256,guard,16))return 0;
    used=stream[0]-stream[2];if(used>4096||(used&&!read_memory(stream[2],pending,used)))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"arguments\":",event,phase,target);words(args,count);
    fprintf(log_file,",\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"errors\":",c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(errors,3);
    fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes(&c->FloatSave,sizeof(c->FloatSave));fputs(",\"stdout_file\":",log_file);words(stream,8);fputs(",\"pending\":",log_file);bytes(pending,used);fputs(",\"input\":",log_file);bytes(text,256);fputs(",\"guards\":",log_file);bytes(guard,16);
    fputs(",\"modules\":",log_file);if(!modules())return 0;fputs(",\"libraries\":",log_file);if(!libraries())return 0;
    if(phase==0){
        if(!read_memory(memory+512,argv,16))return 0;
        fputs(",\"argv\":",log_file);words(argv,4);fputs(",\"argv_names\":[",log_file);
        for(i=0;i<3;++i){if(i)fputc(',',log_file);if(!string_at(memory+(i?i==1?1024:1088:768),name,sizeof(name)))return 0;bytes(name,strlen(name)+1);}fputc(']',log_file);
    }
    fputs("}\n",log_file);fflush(log_file);return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[4],i;count=0;
    if(phase==0){target=0x26b40;count=2;args[0]=3;args[1]=memory+512;}
    else if(phase==1){target=0x17000;count=1;args[0]=memory+256;}
    else if(phase==2||phase==3){target=0x35760;count=2;args[0]=memory+(phase==2?32:160);args[1]=image+offsetof(Image,libraries);}
    else if(phase==4)target=0x35520;
    else if(phase==5){target=0x50e00;count=1;args[0]=0x85070;}
    else return 0;
    frame[0]=sentinel;for(i=0;i<count;++i)frame[i+1]=args[i];
    if(!write_memory(stack_top,frame,(count+1)*4)||!read_memory(target,&entry_byte,1)||!code(target,0xcc))return 0;
    c->Eip=target;c->Esp=stack_top;return SetThreadContext(thread,c);
}
static int write_report(void)
{
    DWORD header[4],i,length,next;ModuleDebugInfo row;Image value;DefaultLibrary library;char text[512];unsigned char changed[256];
    header[0]=0x31444d4c;header[1]=1;header[2]=main_result;header[3]=module_count;
    if(fwrite(header,4,4,report)!=4||!read_memory(memory,changed,256)||fwrite(changed,1,256,report)!=256)return 0;
    for(i=0;i<module_count;++i){
        if(!read_memory(module_owner+i*76,&row,76))return 0;
#define NAME_REPORT(pointer) if(!string_at((DWORD)(pointer),text,sizeof(text)))return 0;length=strlen(text)+1;if(fwrite(&length,4,1,report)!=1||fwrite(text,1,length,report)!=length)return 0
        NAME_REPORT(row.library_name);NAME_REPORT(row.module_name);NAME_REPORT(row.source_name);
#undef NAME_REPORT
    }
    if(!read_memory(image,&value,sizeof(value)))return 0;next=(DWORD)value.libraries.default_libraries;
    for(i=0;next;++i){if(i>16||!read_memory(next,&library,sizeof(library)))return 0;next=(DWORD)library.next;}
    if(fwrite(&i,4,1,report)!=1)return 0;next=(DWORD)value.libraries.default_libraries;
    while(next){if(!read_memory(next,&library,sizeof(library))||!string_at((DWORD)library.name,text,sizeof(text)))return 0;length=strlen(text)+1;if(fwrite(&library.enabled,1,1,report)!=1||fwrite(&length,4,1,report)!=1||fwrite(text,1,length,report)!=length)return 0;next=(DWORD)library.next;}
    return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;DEBUG_EVENT ev;CONTEXT c;
    DWORD pid,site,status,n,rva,size,old,again,failed=0,stopped=0,exit_code=999,at_main=0,argv_data[4],clock_value;
    unsigned char *mask,guard[16];char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    /* program,textplan,stdout,stderr,events,report,cwd,libraries */
    if(argc!=9)return 2;
    f=fopen(argv[2],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    f=fopen(argv[8],"rb");if(!f||fread(payload,1,sizeof(payload),f)!=sizeof(payload)||fgetc(f)!=EOF||fclose(f))return 3;
    log_file=fopen(argv[5],"wt");report=fopen(argv[6],"wb");if(!log_file||!report)return 3;
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[7],&startup,&launch))return 4;
    pid=launch.dwProcessId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    for(;;){
        if(!WaitForDebugEvent(&ev,15000)){failed=5;break;}status=DBG_CONTINUE;if(ev.dwProcessId!=pid){failed=6;break;}
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){process=ev.u.CreateProcessInfo.hProcess;thread=ev.u.CreateProcessInfo.hThread;teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x10000||!code(0x26b40,0xcc)){failed=7;break;}if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        }else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        }else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS;if(!GetThreadContext(thread,&c)){failed=9;break;}
            if(last_error_armed&&((ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_SINGLE_STEP&&(c.Dr6&1))||(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x5b224))){
                DWORD access_stack[8],last_value;unsigned char access_code[32];
                memset(access_stack,0,sizeof(access_stack));memset(access_code,0,sizeof(access_code));last_value=0;read_memory(c.Esp,access_stack,sizeof(access_stack));read_memory(c.Eip-16,access_code,sizeof(access_code));read_memory(teb+0x34,&last_value,4);
                fprintf(log_file,"{\"event\":\"last_error_access\",\"phase\":%lu,\"site\":%lu,\"eip\":%lu,\"esp\":%lu,\"eflags\":%lu,\"last_error\":%lu,\"dr6\":%lu,\"stack\":",phase,site,c.Eip,c.Esp,c.EFlags,last_value,c.Dr6);words(access_stack,8);fputs(",\"code_before_after\":",log_file);bytes(access_code,32);fputs("}\n",log_file);fflush(log_file);failed=29;break;
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x26b40&&!at_main){
                at_main=1;initial=c;if(!VirtualProtectEx(process,(void *)(0x10000+rva),size,PAGE_EXECUTE_READWRITE,&old)||!write_memory(0x10000+rva,mask+8,size)||!FlushInstructionCache(process,(void *)(0x10000+rva),size)||!VirtualProtectEx(process,(void *)(0x10000+rva),size,old,&again)||!read_memory(c.Esp,&main_return,4)||main_return!=0x50a85){failed=10;break;}
                memory=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                memset(guard,0xa5,sizeof(guard));argv_data[0]=memory+768;argv_data[1]=memory+1024;argv_data[2]=memory+1088;argv_data[3]=0;
                if(!memory||stack_top==0xf000||!sentinel||!write_memory(memory,payload,256)||!write_memory(memory+256,guard,16)||!write_memory(memory+512,argv_data,16)||!write_memory(memory+768,argv[1],strlen(argv[1])+1)||!write_memory(memory+1024,"-link",6)||!write_memory(memory+1088,"@modules.rsp",13)||!code(sentinel,0xcc)||!read_memory(0x1b099,&clock_byte,1)||!read_memory(0x1b09e,&clock_return_byte,1)||!code(0x1b099,0xcc)||!code(0x1b09e,0xcc)||!prepare(&c)){failed=11;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==0x1b099&&!clock_active){if(!read_memory(c.Esp,&clock_pointer,4)||!clock_pointer||!code(site,clock_byte)||!SetThreadContext(thread,&c)){failed=25;break;}clock_active=1;
                }else if(site==0x1b09e&&clock_active){if(!read_memory(clock_pointer,&clock_value,4)||clock_value!=c.Eax||!code(site,clock_return_byte)||!SetThreadContext(thread,&c)){failed=26;break;}fprintf(log_file,"{\"event\":\"clock_boundary\",\"caller\":\"link_passes\",\"pointer\":%lu,\"result\":%lu,\"output\":%lu}\n",clock_pointer,c.Eax,clock_value);fflush(log_file);clock_active=0;
                }else if(site==target&&!active){
                    if((phase==0||phase==2||phase==3||phase==4)&&!snapshot(entry_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(entry_byte==0xcc){fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
                    else{before=c;active=1;if(!code(target,entry_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&active){
                    if(c.Esp!=before.Esp+4||c.Ebx!=before.Ebx||c.Esi!=before.Esi||c.Edi!=before.Edi||c.Ebp!=before.Ebp){failed=14;break;}
                    if(phase==0){
                        main_result=c.Eax;main_done=c;
                        if((c.Dr7&255)||!code(0x5b224,0xcc)){failed=28;break;}
                        fprintf(log_file,"{\"event\":\"debug_context_before_watch\",\"dr0\":%lu,\"dr1\":%lu,\"dr2\":%lu,\"dr3\":%lu,\"dr6\":%lu,\"dr7\":%lu}\n",c.Dr0,c.Dr1,c.Dr2,c.Dr3,c.Dr6,c.Dr7);
                        c.Dr0=teb+0x34;c.Dr6=0;c.Dr7=0xf0001;last_error_armed=1;
                        fprintf(log_file,"{\"event\":\"last_error_watch_armed\",\"address\":%lu,\"width\":4,\"access\":\"readwrite\",\"dr7\":%lu}\n",teb+0x34,c.Dr7);fflush(log_file);
                    }
                    if(phase==1&&!read_memory(memory+256,&image,4)){failed=15;break;}
                    if((phase==0||phase==2||phase==3||phase==4)&&!snapshot("return",&c)){failed=16;break;}
                    active=0;if(phase==0&&main_result){failed=17;break;}++phase;
                    if(phase==6){
                        if(!write_report()){failed=18;break;}
                        /* Actual CRT continuation consumes EAX and its caller-owned stack. */
                        if(!snapshot("before_crt",&c)){failed=27;break;}
                        stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=main_result;
                        if(c.Ebx!=initial.Ebx||c.Esi!=initial.Esi||c.Edi!=initial.Edi||c.Ebp!=initial.Ebp||!SetThreadContext(thread,&c)){failed=19;break;}
                        fprintf(log_file,"{\"event\":\"continuation\",\"main_result\":%lu,\"restored_eip\":%lu,\"restored_esp\":%lu,\"nonvolatiles_preserved\":true}\n",main_result,main_return,c.Esp);fflush(log_file);
                    }else if(!prepare(&c)){failed=20;break;}
                }else{fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!at_main){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"phase\":%lu,\"va\":%lu,\"code\":%lu}\n",phase,site,ev.u.Exception.ExceptionRecord.ExceptionCode);failed=21;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=22;break;}
    }
    if(failed&&process)TerminateProcess(process,124);fclose(report);fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"phase\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,phase,GetFileSize(output,NULL),GetFileSize(error,NULL));fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
