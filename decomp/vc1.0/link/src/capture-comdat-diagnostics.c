/* Explicit original-only external diagnostic caller. Not reconstruction code.
 * The input address is controlled caller data, not an output normalization. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "comdat.h"
#include <recon.h>
typedef char node20[(sizeof(ComdatNode)==20)?1:-1];
typedef char edge12[(sizeof(ComdatEdge)==12)?1:-1];
typedef char contribution64[(sizeof(Contribution)==64)?1:-1];
typedef char module56[(sizeof(Module)==56)?1:-1];
typedef char group40[(sizeof(Group)==40)?1:-1];
typedef char section72[(sizeof(Section)==72)?1:-1];
typedef char contribution_fields[(offsetof(Contribution,flags)==8 && offsetof(Contribution,module)==0x34 && offsetof(Contribution,group)==0x38)?1:-1];
typedef char group_section[(offsetof(Group,section)==0x1c)?1:-1];
typedef char module_path[(offsetof(Module,path)==4)?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static AllocateRemote allocate_remote;
static HANDLE process,thread;static FILE *log_file;
static DWORD base,teb,stack_top,sentinel,text_rva,text_size,requested=0x12000000;
static unsigned char *text,input[4096];
static DWORD entries[]={0x17600,0x3a880,0x17540,0x17520,0x175e0};
static const char *names[]={"dump_comdat_graph","debug_printf","print_comdat_node","comdat_usage_label","print_comdat_edge"};
typedef struct Breakpoint { DWORD address,kind,index;unsigned char byte; } Breakpoint;
static Breakpoint points[128];static DWORD point_count,pending=0xffffffff;
typedef struct Frame { DWORD index,ret;CONTEXT context; } Frame;
static Frame frames[128];static DWORD depth;
static int read_memory(DWORD a,void *b,DWORD n){DWORD got;return ReadProcessMemory(process,(void *)a,b,n,&got) && got==n;}
static int write_memory(DWORD a,const void *b,DWORD n){DWORD got;return WriteProcessMemory(process,(void *)a,(void *)b,n,&got) && got==n;}
static int code(DWORD a,const void *b,DWORD n){DWORD old,restored;return VirtualProtectEx(process,(void *)a,n,PAGE_EXECUTE_READWRITE,&old) && write_memory(a,b,n) && FlushInstructionCache(process,(void *)a,n) && VirtualProtectEx(process,(void *)a,n,old,&restored);}
static void words(DWORD *p,DWORD n){DWORD i;fputc('[',log_file);for(i=0;i<n;++i)fprintf(log_file,"%s%lu",i?",":"",p[i]);fputc(']',log_file);}
static void bytes(unsigned char *p,DWORD n){DWORD i;fputc('"',log_file);for(i=0;i<n;++i)fprintf(log_file,"%02x",p[i]);fputc('"',log_file);}
static int string(DWORD a,char *buffer,DWORD capacity){DWORD i;for(i=0;i<capacity;++i){if(!read_memory(a+i,buffer+i,1))return 0;if(!buffer[i])return 1;}return 0;}
static int snapshot(const char *event,DWORD index,CONTEXT *c,int entering)
{
    DWORD stream[8],errors[3],args[3],ctype,mbmax,used,count=entering?1:0;
    unsigned char region[4096],flags[100],table[514],buffer[4096];
    char format[256],value[256];DWORD length,log;
    format[0]=value[0]=0;
    if(entering && index==1){
        if(!read_memory(c->Esp,args,8) || !string(args[1],format,sizeof(format)))return 0;
        if(strchr(format,'%')){
            if(strcmp(format,"%s") && strcmp(format,"|sec=%.8s") && strcmp(format,"|grp=%.8s") && strcmp(format,"|con=0x%p") && strcmp(format,"|mod=%s") && strcmp(format,"|%s()"))return 0;
            count=2;
        }
    }
    if(!read_memory(c->Esp,args,(count+1)*4) || !read_memory(requested,region,4096) ||
       !read_memory(0x85070,stream,32) || !read_memory(0x852d8,errors,8) || !read_memory(teb+0x34,errors+2,4) ||
       !read_memory(0x5c06c,flags,100) || !read_memory(0x7f07c,&log,4) || !read_memory(0x8554c,&mbmax,4) ||
       !read_memory(0x85340,&ctype,4) || !read_memory(ctype-2,table,514))return 0;
    used=stream[0]-stream[2];if(used>sizeof(buffer) || (used && !read_memory(stream[2],buffer,used)))return 0;
    if(entering && index==1 && count==2 && strcmp(format,"|con=0x%p") && !string(args[2],value,sizeof(value)))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"name\":\"%s\",\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,names[index],c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(args,count+1);
    fprintf(log_file,",\"format\":");bytes((unsigned char *)format,strlen(format));fprintf(log_file,",\"string_value\":");bytes((unsigned char *)value,strlen(value));
    fprintf(log_file,",\"input_base\":%lu,\"region\":",requested);bytes(region,4096);
    fprintf(log_file,",\"stdout_file\":");words(stream,8);fprintf(log_file,",\"pending\":");bytes(buffer,used);
    fprintf(log_file,",\"errors\":");words(errors,3);fprintf(log_file,",\"debug_flags\":");bytes(flags,100);
    fprintf(log_file,",\"debug_log\":%lu,\"mb_cur_max\":%lu,\"ctype\":",log,mbmax);bytes(table,514);fprintf(log_file,"}\n");fflush(log_file);return 1;
}
static int install(DWORD address,DWORD kind,DWORD index)
{
    DWORD i;unsigned char trap=0xcc;
    for(i=0;i<point_count;++i)if(points[i].address==address)return i;
    if(point_count==128 || !read_memory(address,&points[point_count].byte,1) || (points[point_count].byte==0xcc && address!=sentinel))return -1;
    i=point_count++;points[i].address=address;points[i].kind=kind;points[i].index=index;
    return code(address,&trap,1)?(int)i:-1;
}
static unsigned char *load(const char *path,DWORD *length)
{
    FILE *file;long n;unsigned char *data;file=fopen(path,"rb");if(!file)return NULL;
    if(fseek(file,0,SEEK_END) || (n=ftell(file))<0 || fseek(file,0,SEEK_SET))return NULL;
    data=malloc(n?n:1);if(!data || fread(data,1,n,file)!=(unsigned long)n || fclose(file))return NULL;*length=n;return data;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES security;STARTUPINFOA startup;PROCESS_INFORMATION launch;DEBUG_EVENT event;CONTEXT context;
    MEMORY_BASIC_INFORMATION before_mapping,after_mapping;
    HANDLE output,error;DWORD pid,tid,status,site,exception,at_main=0,failed=0,stopped=0,exit_code=999;
    DWORD i,n,mask_size,actual,frame[2],restore=0;unsigned char trap=0xcc,original_main,*payload,after[4096];char command[4096];
    /* program,input,text,stdout,stderr,events,cwd */
    if(argc!=8)return 2;
    payload=load(argv[2],&n);text=load(argv[3],&mask_size);if(!payload || n!=4096 || !text || mask_size<8)return 3;memcpy(input,payload,4096);
    text_rva=*(DWORD *)text;text_size=*(DWORD *)(text+4);if(text_size+8!=mask_size)return 3;
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");if(!allocate_remote)return 3;
    log_file=fopen(argv[6],"wt");if(!log_file)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[5],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE || error==INVALID_HANDLE_VALUE)return 3;
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[7],&startup,&launch))return 4;
    pid=launch.dwProcessId;tid=launch.dwThreadId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    for(;;){
        if(!WaitForDebugEvent(&event,15000)){failed=5;break;}status=DBG_CONTINUE;
        if(event.dwProcessId!=pid){failed=6;break;}
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=event.u.CreateProcessInfo.hProcess;thread=event.u.CreateProcessInfo.hThread;base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;
            if(base!=0x10000 || !read_memory(0x26b40,&original_main,1) || !code(0x26b40,&trap,1)){failed=7;break;}
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        }else if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;exception=event.u.Exception.ExceptionRecord.ExceptionCode;
            memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;if(!GetThreadContext(thread,&context)){failed=9;break;}
            if(exception==EXCEPTION_SINGLE_STEP && pending!=0xffffffff){
                if(!code(points[pending].address,&trap,1)){failed=10;break;}pending=0xffffffff;context.EFlags&=~0x100;
                if(!SetThreadContext(thread,&context)){failed=10;break;}
            }else if(exception==EXCEPTION_BREAKPOINT && site==0x26b40 && !at_main){
                at_main=1;if(!code(base+text_rva,text+8,text_size)){failed=11;break;}
                if(VirtualQueryEx(process,(void *)requested,&before_mapping,sizeof(before_mapping))!=sizeof(before_mapping) || before_mapping.State!=MEM_FREE){failed=26;break;}
                actual=(DWORD)allocate_remote(process,(void *)requested,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                fprintf(log_file,"{\"event\":\"owned_mapping\",\"requested\":%lu,\"returned\":%lu,\"size\":4096}\n",requested,actual);fflush(log_file);
                if(actual!=requested){failed=12;break;}
                if(VirtualQueryEx(process,(void *)requested,&after_mapping,sizeof(after_mapping))!=sizeof(after_mapping) ||
                    (DWORD)after_mapping.BaseAddress!=requested || (DWORD)after_mapping.AllocationBase!=requested ||
                    after_mapping.RegionSize!=4096 || after_mapping.State!=MEM_COMMIT ||
                    after_mapping.Protect!=PAGE_READWRITE || after_mapping.Type!=MEM_PRIVATE){failed=27;break;}
                fprintf(log_file,"{\"event\":\"owned_mapping_proof\",\"before_state\":%lu,\"after_state\":%lu,\"allocation_base\":%lu,\"base\":%lu,\"size\":%lu,\"protect\":%lu,\"type\":%lu}\n",before_mapping.State,after_mapping.State,(DWORD)after_mapping.AllocationBase,(DWORD)after_mapping.BaseAddress,after_mapping.RegionSize,after_mapping.Protect,after_mapping.Type);fflush(log_file);
                stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                frame[0]=sentinel;frame[1]=requested+0x40;
                if(stack_top==0xf000 || !sentinel || !write_memory(requested,input,4096) || !write_memory(stack_top,frame,8) || !code(sentinel,&trap,1)){failed=13;break;}
                for(i=0;i<5;++i)if(text[8+entries[i]-base-text_rva]!=0xcc && install(entries[i],1,i)<0){failed=14;break;}
                if(failed)break;
                context.Eip=entries[0];context.Esp=stack_top;
                if(!SetThreadContext(thread,&context)){failed=15;break;}
            }else if(exception==EXCEPTION_BREAKPOINT && at_main){
                context.Eip=site;
                for(i=0;i<point_count && points[i].address!=site;++i);
                if(i==point_count){
                    if(site==sentinel && depth==0){failed=16;break;}
                    for(n=0;n<5 && entries[n]!=site;++n);
                    if(!snapshot("frontier",n<5?n:0,&context,n<5)){failed=25;break;}fprintf(log_file,"{\"event\":\"trap\",\"va\":%lu,\"code\":%lu}\n",site,exception);stopped=2;TerminateProcess(process,17);
                }else if(points[i].kind==1){
                    if(depth==128 || !snapshot("entry",points[i].index,&context,1) || !read_memory(context.Esp,&restore,4)){failed=17;break;}
                    frames[depth].index=points[i].index;frames[depth].ret=restore;frames[depth].context=context;++depth;
                    if(install(restore,2,0)<0 || !code(site,&points[i].byte,1)){failed=18;break;}
                    pending=i;context.EFlags|=0x100;if(!SetThreadContext(thread,&context)){failed=18;break;}
                }else{
                    Frame *before;
                    if(!depth || frames[depth-1].ret!=site){failed=19;break;}before=&frames[--depth];
                    if(context.Esp!=before->context.Esp+4 || context.Ebx!=before->context.Ebx || context.Esi!=before->context.Esi || context.Edi!=before->context.Edi || context.Ebp!=before->context.Ebp || !snapshot("return",before->index,&context,0)){failed=20;break;}
                    if(site==sentinel){
                        if(depth || !read_memory(requested,after,4096) || memcmp(after,input,4096)){failed=21;break;}
                        stopped=1;TerminateProcess(process,0);
                    }else{
                        if(!code(site,&points[i].byte,1)){failed=22;break;}pending=i;context.EFlags|=0x100;if(!SetThreadContext(thread,&context)){failed=22;break;}
                    }
                }
            }else if(exception==EXCEPTION_BREAKPOINT && !at_main && (site<base+text_rva || site>=base+text_rva+text_size)){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"va\":%lu,\"code\":%lu}\n",site,exception);failed=23;break;}
        }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=event.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,event.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,event.dwThreadId,status)){failed=24;break;}
    }
    if(failed && process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,stopped,exit_code,GetFileSize(output,NULL),GetFileSize(error,NULL));
    fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
