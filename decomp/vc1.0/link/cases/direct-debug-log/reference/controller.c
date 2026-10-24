/* Explicit original-only diagnostic-log caller/observer; no target graph credit. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "comdat-diagnostics.h"
#include <recon.h>
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static AllocateRemote allocate_remote;
static HANDLE process,thread;static FILE *log_file;
static DWORD base,teb,stack_top,sentinel,text_rva,text_size,memory,phase,main_return;
static unsigned char *text,input[256];
static DWORD entries[]={0x3a750,0x3a900,0x3a880,0x3a970,0x51140,0x51000,0x53630,0x53670};
static const char *names[]={"debug_initialize","open_debug_log","debug_printf","set_debug_flag","fopen","fclose","setbuf","perror"};
static DWORD counts[]={2,1,1,2,2,1,2,1};
static CONTEXT initial;
static FILE *report;
static unsigned char report_rows[9][124];
typedef struct Breakpoint { DWORD address,kind,index;unsigned char byte; } Breakpoint;
static Breakpoint points[128];static DWORD point_count,pending=0xffffffff;
typedef struct Frame { DWORD index,ret,args[2];CONTEXT context; } Frame;
static Frame frames[128];static DWORD depth;
static int read_memory(DWORD a,void *b,DWORD n){DWORD got;return ReadProcessMemory(process,(void *)a,b,n,&got) && got==n;}
static int write_memory(DWORD a,const void *b,DWORD n){DWORD got;return WriteProcessMemory(process,(void *)a,(void *)b,n,&got) && got==n;}
static int code(DWORD a,const void *b,DWORD n){DWORD old,restored;return VirtualProtectEx(process,(void *)a,n,PAGE_EXECUTE_READWRITE,&old) && write_memory(a,b,n) && FlushInstructionCache(process,(void *)a,n) && VirtualProtectEx(process,(void *)a,n,old,&restored);}
static void words(DWORD *p,DWORD n){DWORD i;fputc('[',log_file);for(i=0;i<n;++i)fprintf(log_file,"%s%lu",i?",":"",p[i]);fputc(']',log_file);}
static void bytes(unsigned char *p,DWORD n){DWORD i;fputc('"',log_file);for(i=0;i<n;++i)fprintf(log_file,"%02x",p[i]);fputc('"',log_file);}
static int string(DWORD a,char *buffer,DWORD capacity){DWORD i;for(i=0;i<capacity;++i){if(!read_memory(a+i,buffer+i,1))return 0;if(!buffer[i])return 1;}return 0;}
static int snapshot(const char *event,DWORD index,CONTEXT *c,Frame *before)
{
    DWORD streams[3][8],errors[3],args[2],actual[2],log,i,n,used;
    unsigned char region[272],flags[100],buffer[4096];char text[128];
    if(index>=8)return 0;
    if(before)memcpy(args,before->args,counts[index]*4);
    else if(!read_memory(c->Esp+4,args,counts[index]*4))return 0;
    if(!read_memory((before?before->context.Esp:c->Esp)+4,actual,counts[index]*4)||
       !read_memory(memory,region,sizeof(region))||!read_memory(0x85070,streams,sizeof(streams))||
       !read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||
       !read_memory(0x5c008,flags,100)||!read_memory(0x7f07c,&log,4))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"name\":\"%s\",\"target\":%lu,\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,names[index],entries[index],c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(args,counts[index]);
    fputs(",\"stack_argument_words\":",log_file);words(actual,counts[index]);
    if(index==2){if(!string(args[0],text,sizeof(text))||strchr(text,'%'))return 0;fputs(",\"variadic_words_consumed\":0",log_file);}
    if(index==0||index==1||index==2||index==4||index==7){
        n=(index==0||index==4)?2:1;fputs(",\"argument_strings\":[",log_file);
        for(i=0;i<n;++i){if(i)fputc(',',log_file);if(!args[i])fputs("null",log_file);else{if(!string(args[i],text,sizeof(text)))return 0;bytes((unsigned char *)text,strlen(text)+1);}}fputc(']',log_file);
    }
    fputs(",\"errors\":",log_file);words(errors,3);fprintf(log_file,",\"names_address\":%lu,\"names\":",memory);bytes(region,sizeof(region));
    fputs(",\"debug_flags\":",log_file);bytes(flags,100);fprintf(log_file,",\"debug_flags_address\":%lu,\"debug_log\":%lu,\"iob_address\":%lu,\"streams\":[",0x5c008UL,log,0x85050UL);
    for(i=0;i<3;++i){
        used=0;if((streams[i][3]&0x83)&&!(streams[i][3]&4)){used=streams[i][0]-streams[i][2];if(used>sizeof(buffer)||(used&&!read_memory(streams[i][2],buffer,used)))return 0;}
        if(i)fputc(',',log_file);fprintf(log_file,"{\"index\":%lu,\"raw\":",i+1);words(streams[i],8);fputs(",\"pending\":",log_file);bytes(buffer,used);fputc('}',log_file);
    }
    fprintf(log_file,"],\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));fputs("}\n",log_file);fflush(log_file);return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[3],index,n=1;
    frame[0]=sentinel;
    switch(phase){
    case 0:index=0;n=2;frame[1]=memory;frame[2]=memory+32;break;
    case 1:index=1;frame[1]=memory+64;break;
    case 2:index=2;frame[1]=memory+96;break;
    case 3:index=1;frame[1]=memory+128;break;
    case 4:index=3;n=2;frame[1]=2;frame[2]=1;break;
    case 5:index=2;frame[1]=memory+160;break;
    case 6:index=3;n=2;frame[1]=2;frame[2]=0;break;
    case 7:index=1;frame[1]=0;break;
    case 8:index=2;frame[1]=memory+192;break;
    default:return 0;
    }
    if(!write_memory(stack_top,frame,(n+1)*4))return 0;c->Eip=entries[index];c->Esp=stack_top;return SetThreadContext(thread,c);
}
static int record_row(CONTEXT *c,DWORD index)
{
    DWORD values[2],log,errors[3];unsigned char flags[100],*row=report_rows[phase];
    if(!read_memory(0x5c008,flags,100)||!read_memory(0x7f07c,&log,4)||!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4))return 0;
    values[0]=phase;values[1]=(index==0||index==3)?0:c->Eax;memcpy(row,values,8);memcpy(row+8,flags,100);
    values[0]=log?(log-0x85050)/32:0xffffffffUL;memcpy(row+108,values,4);memcpy(row+112,errors,12);return 1;
}
static int write_report(void)
{
    DWORD header[3]={0x31474c44,1,9};unsigned char payload[256];
    if(!read_memory(memory,payload,256)||memcmp(payload,input,256))return 0;
    return fwrite(header,4,3,report)==3&&fwrite(payload,1,256,report)==256&&fwrite(report_rows,1,sizeof(report_rows),report)==sizeof(report_rows);
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
    HANDLE output,error;DWORD pid,tid,status,site,exception,at_main=0,failed=0,stopped=0,exit_code=999;
    DWORD i,n,mask_size,actual,restore=0;unsigned char trap=0xcc,original_main,*payload,after[272];char command[4096];
    /* program,text,stdout,stderr,events,report,cwd,input */
    if(argc!=9)return 2;
    payload=load(argv[8],&n);text=load(argv[2],&mask_size);if(!payload || n!=256 || !text || mask_size<8)return 3;memcpy(input,payload,256);
    text_rva=*(DWORD *)text;text_size=*(DWORD *)(text+4);if(text_size+8!=mask_size)return 3;
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");if(!allocate_remote)return 3;
    log_file=fopen(argv[5],"wt");report=fopen(argv[6],"wb");if(!log_file||!report)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
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
                at_main=1;initial=context;
                if(!code(base+text_rva,text+8,text_size)||!read_memory(context.Esp,&main_return,4)){failed=11;break;}
                memory=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;
                sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                memset(after,0xa5,sizeof(after));memcpy(after,input,256);
                if(!memory||stack_top==0xf000||!sentinel||!write_memory(memory,after,sizeof(after))||!code(sentinel,&trap,1)){failed=13;break;}
                for(i=0;i<8;++i)if(text[8+entries[i]-base-text_rva]!=0xcc && install(entries[i],1,i)<0){failed=14;break;}
                if(failed||!prepare(&context)){failed=15;break;}

            }else if(exception==EXCEPTION_BREAKPOINT && at_main){
                context.Eip=site;
                for(i=0;i<point_count && points[i].address!=site;++i);
                if(i==point_count){
                    if(site==sentinel && depth==0){failed=16;break;}
                    for(n=0;n<8 && entries[n]!=site;++n);
                    if(n<8&&!snapshot("frontier",n,&context,NULL)){failed=25;break;}fprintf(log_file,"{\"event\":\"trap\",\"va\":%lu,\"code\":%lu}\n",site,exception);stopped=2;TerminateProcess(process,17);
                }else if(points[i].kind==1){
                    if(depth==128 || !snapshot("entry",points[i].index,&context,NULL) || !read_memory(context.Esp,&restore,4)){failed=17;break;}
                    frames[depth].index=points[i].index;frames[depth].ret=restore;frames[depth].context=context;if(!read_memory(context.Esp+4,frames[depth].args,counts[points[i].index]*4)){failed=17;break;}++depth;
                    if(install(restore,2,0)<0 || !code(site,&points[i].byte,1)){failed=18;break;}
                    pending=i;context.EFlags|=0x100;if(!SetThreadContext(thread,&context)){failed=18;break;}
                }else{
                    Frame *before;
                    if(!depth || frames[depth-1].ret!=site){failed=19;break;}before=&frames[--depth];
                    if(context.Esp!=before->context.Esp+4 || context.Ebx!=before->context.Ebx || context.Esi!=before->context.Esi || context.Edi!=before->context.Edi || context.Ebp!=before->context.Ebp || !snapshot("return",before->index,&context,before)){failed=20;break;}
                    if(site==sentinel){
                        if(depth||!record_row(&context,before->index)){failed=21;break;}
                        ++phase;
                        if(phase==9){
                            if(!write_report()){failed=28;break;}
                            stopped=1;context.Eip=main_return;context.Esp=initial.Esp+4;context.Eax=0;
                            if(context.Ebx!=initial.Ebx||context.Esi!=initial.Esi||context.Edi!=initial.Edi||context.Ebp!=initial.Ebp||!SetThreadContext(thread,&context)){failed=29;break;}
                        }else if(!prepare(&context)){failed=30;break;}

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
    fclose(report);fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
