/* Explicit original-only debugger. Never linked into native reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_RVA 0x1000
#define TEXT_SIZE 0x59e
#define TARGET_ENTRY 0x1220
#define TARGET_SIZE 0x4000
#define MAX_RANGES 1024

typedef struct Range { DWORD start,end,kind; } Range;
typedef struct Point { DWORD at; BYTE original; int active,kind; } Point;
static HANDLE child,threads[64];
static DWORD thread_ids[64],thread_count,base,exe_base;
static FILE *events;
static BYTE snapshot[TEXT_SIZE],masked[TEXT_SIZE],readback[TEXT_SIZE];
static Range ranges[MAX_RANGES];
static unsigned range_count;
static Point points[2048];
static unsigned point_count;
static int baseline,guarded,ready;
static DWORD callback_text,callback_bytes,crt_base;
static DWORD args_slots[5];
static char child_environment[] = "PATH=R:\\toolchains\\5.10\\bin\0SystemRoot=C:\\windows\0TEMP=.\0TMP=.\0WINEDLLOVERRIDES=msvcrt=n\0WINEPATH=R:\\toolchains\\5.10\\bin\0windir=C:\\windows\0";

static void fail(const char *what)
{
    fprintf(events,"error %s %lu\n",what,GetLastError()); fflush(events);
    if(child) TerminateProcess(child,125);
    ExitProcess(125);
}
static void readmem(DWORD at,void *data,DWORD size)
{
    DWORD got;
    if(!ReadProcessMemory(child,(void *)at,data,size,&got)||got!=size) fail("read");
}
static void writebytes(DWORD at,const void *data,DWORD size)
{
    DWORD got,old;
    if(!VirtualProtectEx(child,(void *)at,size,PAGE_EXECUTE_READWRITE,&old)) fail("protect");
    if(!WriteProcessMemory(child,(void *)at,(void *)data,size,&got)||got!=size) fail("write");
    FlushInstructionCache(child,(void *)at,size);
}
static DWORD string_bytes(DWORD at)
{
    DWORD n; BYTE c;
    for(n=0;n<16384;++n){readmem(at+n,&c,1);if(!c)return n+1;}
    fail("unterminated-string");return 0;
}
static void hexbytes(const char *tag,DWORD at,DWORD size)
{
    DWORD n; BYTE byte;
    fprintf(events,"%s %lu ",tag,size);
    for(n=0;n<size;++n){readmem(at+n,&byte,1);fprintf(events,"%02x",byte);}
    fprintf(events,"\n");fflush(events);
}
static void save_file(HANDLE input,const char *name)
{
    HANDLE output; BYTE buffer[16384];DWORD got,written,total=0;
    if(!input||input==INVALID_HANDLE_VALUE)fail("module-hfile");
    if(SetFilePointer(input,0,NULL,FILE_BEGIN)==0xffffffffUL)fail("module-seek");
    output=CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if(output==INVALID_HANDLE_VALUE)fail("module-output");
    for(;;){
        if(!ReadFile(input,buffer,sizeof(buffer),&got,NULL))fail("module-read");
        if(!got)break;
        if(!WriteFile(output,buffer,got,&written,NULL)||got!=written)fail("module-write");
        total+=got;
    }
    CloseHandle(output);fprintf(events,"module-file %s %lu\n",name,total);fflush(events);
}
static unsigned point(DWORD at,int kind)
{
    BYTE trap=0xcc;Point *p;
    if(point_count==2048)fail("point-overflow");
    p=points+point_count;
    p->at=at;p->kind=kind;p->active=1;
    readmem(at,&p->original,1);writebytes(at,&trap,1);
    return point_count++;
}
static void load(DWORD at,HANDLE file)
{
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS nt;IMAGE_EXPORT_DIRECTORY ex;
    char name[128];DWORD n;
    readmem(at,&dos,sizeof(dos));readmem(at+dos.e_lfanew,&nt,sizeof(nt));
    strcpy(name,"<none>");
    if(nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress){
        readmem(at+nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,&ex,sizeof(ex));
        for(n=0;n<sizeof(name)-1;++n){readmem(at+ex.Name+n,name+n,1);if(!name[n])break;}
        name[sizeof(name)-1]=0;
    }
    fprintf(events,"module %s actual=%08lx preferred=%08lx entry=%08lx size=%08lx\n",name,at,nt.OptionalHeader.ImageBase,nt.OptionalHeader.AddressOfEntryPoint,nt.OptionalHeader.SizeOfImage);
    if(!_stricmp(name,"MSVCRT.dll")){crt_base=at;save_file(file,"loaded-crt.bin");}
    if(!_stricmp(name,"RCDLL.dll"))save_file(file,"loaded-rcdll.bin");
    fflush(events);
}
static void vector_at(const char *tag,DWORD at)
{
    DWORD n,item;
    for(n=0;n<4096;++n){
        readmem(at+n*4,&item,4);
        if(!item){fprintf(events,"%s-end %lu\n",tag,n);return;}
        hexbytes(tag,item,string_bytes(item));
    }
    fail("vector-overflow");
}
static void snapshot_and_mask(void)
{
    FILE *file;unsigned n;
    readmem(base+TEXT_RVA,snapshot,sizeof(snapshot));
    file=fopen("relocated-text.bin","wb");if(!file)fail("snapshot-open");
    if(fwrite(snapshot,1,sizeof(snapshot),file)!=sizeof(snapshot))fail("snapshot-write");
    fclose(file);
    if(!baseline){
        memset(masked,0xcc,sizeof(masked));
        /* Verified PE IAT and debug-directory bytes, not executable bodies. */
        memcpy(masked,snapshot,0x58);
        memcpy(masked+0x60,snapshot+0x60,0x1c);
        memcpy(masked+0x3ca,snapshot+0x3ca,TEXT_SIZE-0x3ca);
        for(n=0;n<range_count;++n){
            Range *r=ranges+n;
            if(r->start<0x1000||r->end>0x13ca||r->start>=r->end)fail("range-bounds");
            memcpy(masked+r->start-TEXT_RVA,snapshot+r->start-TEXT_RVA,r->end-r->start);
        }
        writebytes(base+TEXT_RVA,masked,sizeof(masked));
        readmem(base+TEXT_RVA,readback,sizeof(readback));
        if(memcmp(masked,readback,sizeof(masked)))fail("mask-readback");
        file=fopen("masked-text.bin","wb");if(!file)fail("mask-open");
        if(fwrite(readback,1,sizeof(readback),file)!=sizeof(readback))fail("mask-write");
        fclose(file);
        for(n=0;n<range_count;++n)
            if(ranges[n].kind==1&&ranges[n].start!=TARGET_ENTRY)point(base+ranges[n].start,2);
    }else{
        point(base+0x13be,3);point(base+0x11fd,4);
        point(base+0x1100,7);
        point(base+0x12e5,20);point(base+0x12eb,21);
        point(base+0x11d2,22);
    }
    ready=1;fprintf(events,"snapshot-ready mode=%s\n",baseline?"baseline":"masked");fflush(events);
}
static int trap(HANDLE thread,DWORD address)
{
    CONTEXT c;DWORD words[12];unsigned n;Point *p=NULL;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&c))fail("get-context");
    for(n=0;n<point_count;++n)if(points[n].active&&points[n].at==address){p=points+n;break;}
    if(p){
        writebytes(address,&p->original,1);p->active=0;c.Eip=address;
        if(p->kind==1){
            BYTE byte;
            if(!guarded||ready)fail("guard-state");
            snapshot_and_mask();
            if(!baseline){
                readmem(address,&byte,1);
                if(byte==0xcc)p=NULL;
                else fprintf(events,"entry %08lx\n",TARGET_ENTRY);
            }
        }else if(p->kind==2)fprintf(events,"entry %08lx\n",address-base);
        else if(p->kind==3){
            readmem(c.Esp,words,7*4);
            fprintf(events,"export-args %08lx %08lx %08lx %08lx %lu %08lx\n",words[1],words[2],words[3],words[4],words[5],words[6]);
        }else if(p->kind==4)fprintf(events,"export-result %ld\n",(long)c.Eax);
        else if(p->kind==5){fail("unused-point");}
        else if(p->kind==6){fail("unused-point");
        }else if(p->kind==7){
            readmem(c.Esp,words,16);callback_text=words[3];callback_bytes=string_bytes(callback_text);
            fprintf(events,"callback %lu %lu\n",words[1],words[2]);hexbytes("before",callback_text,callback_bytes);
            point(words[0],8);
        }else if(p->kind==8){hexbytes("after",callback_text,callback_bytes);fprintf(events,"callback-return %ld\n",(long)c.Eax);point(exe_base+0x1100,7);}
        else if(p->kind==20){
            readmem(c.Esp,args_slots,sizeof(args_slots));
            fprintf(events,"mainargs-input wildcard=%lu\n",args_slots[3]);
            hexbytes("mainargs-info-before",args_slots[4],4);
        }else if(p->kind==21){
            DWORD ac,av,ep;
            readmem(args_slots[0],&ac,4);readmem(args_slots[1],&av,4);readmem(args_slots[2],&ep,4);
            fprintf(events,"mainargs-output argc=%lu\n",ac);
            hexbytes("mainargs-info-after",args_slots[4],4);
            vector_at("argv",av);vector_at("env",ep);
        }else if(p->kind==22){
            readmem(c.Esp,words,16);
            fprintf(events,"main-entry argc=%lu\n",words[1]);
            vector_at("main-argv",words[2]);vector_at("main-env",words[3]);
            hexbytes("startup-data",base+0x2000,0x38);
            if(!crt_base)fail("missing-crt");
            hexbytes("crt-app-type",crt_base+0x3fa90,4);
            hexbytes("crt-fmode",crt_base+0x3fa88,4);
            hexbytes("crt-commode",crt_base+0x3fa94,4);
            hexbytes("crt-newmode",crt_base+0x3f198,4);
            hexbytes("crt-adjust-fdiv",crt_base+0x3f928,4);
        }
        if(p){
            if(!SetThreadContext(thread,&c))fail("set-context");
            fflush(events);return 1;
        }
    }
    if(base&&ready&&address>=base+TEXT_RVA&&address<base+TEXT_RVA+TEXT_SIZE){
        readmem(c.Esp,words,sizeof(words));
        fprintf(events,"frontier %08lx eax=%08lx ecx=%08lx edx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n",address-base,c.Eax,c.Ecx,c.Edx,c.Ebx,c.Esi,c.Edi,c.Ebp,c.Esp);
        fprintf(events,"stack");for(n=0;n<12;++n)fprintf(events," %08lx",words[n]);fprintf(events,"\n");
        hexbytes("data",base+0x2000,0x38);
        if(address==base+0x11d2){vector_at("frontier-argv",words[2]);vector_at("frontier-env",words[3]);}
        if(address==base+0x1100)hexbytes("frontier-message",words[3],string_bytes(words[3]));
        if(address==base+0x13be)vector_at("frontier-rc-argv",words[6]);
        if(address==base+0x1376){
            if(words[2]<words[1]||words[2]-words[1]>64)fail("initializer-range");
            hexbytes("frontier-initializers",words[1],words[2]-words[1]);
        }
        fflush(events);TerminateProcess(child,124);return 2;
    }
    fprintf(events,"unhandled-break %08lx\n",address);fflush(events);return 0;
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;PROCESS_INFORMATION process;DEBUG_EVENT event;
    DWORD status,i;HANDLE thread;FILE *file;char command[2048];int stop=0;
    if(argc!=4)return 125;
    baseline=!strcmp(argv[1],"baseline");
    if(!baseline&&strcmp(argv[1],"masked"))return 125;
    file=fopen(argv[3],"rb");if(!file)return 125;
    while(range_count<MAX_RANGES&&fscanf(file,"%lx %lx %lu",&ranges[range_count].start,&ranges[range_count].end,&ranges[range_count].kind)==3)++range_count;
    if(!feof(file)||range_count==MAX_RANGES){fclose(file);return 125;}fclose(file);
    events=fopen("events.txt","wb");if(!events)return 125;
    sprintf(command,"\"%s\" /help",argv[2]);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    SetEnvironmentVariableA("TMP",".");SetEnvironmentVariableA("TEMP",".");
    if(!CreateProcessA(argv[2],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,child_environment,"S:\\vc5.10\\rc\\original",&startup,&process))fail("create");
    for(;;){
        if(!WaitForDebugEvent(&event,30000))fail("debug-wait");
        status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            child=event.u.CreateProcessInfo.hProcess;base=exe_base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            if(base!=0x1000000)fail("exe-base");
            save_file(event.u.CreateProcessInfo.hFile,"loaded-target.bin");
            {BYTE byte;readmem(base+TARGET_ENTRY,&byte,1);if(byte!=0x64)fail("entry-byte");}
            point(base+TARGET_ENTRY,1);guarded=1;
            thread_ids[thread_count]=event.dwThreadId;threads[thread_count++]=event.u.CreateProcessInfo.hThread;
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);
        }else if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){
            if(thread_count==64)fail("threads");
            thread_ids[thread_count]=event.dwThreadId;threads[thread_count++]=event.u.CreateThread.hThread;
        }else if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){
            load((DWORD)event.u.LoadDll.lpBaseOfDll,event.u.LoadDll.hFile);
            if(event.u.LoadDll.hFile)CloseHandle(event.u.LoadDll.hFile);
        }else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            DWORD code=event.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD at=(DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            thread=NULL;for(i=0;i<thread_count;++i)if(thread_ids[i]==event.dwThreadId)thread=threads[i];
            if(!thread)fail("exception-thread");
            if(code==EXCEPTION_BREAKPOINT){int r=trap(thread,at);if(r==2)stop=1;else if(!r&&ready)status=DBG_EXCEPTION_NOT_HANDLED;}
            else {fprintf(events,"exception %08lx at %08lx first=%lu\n",code,at,event.u.Exception.dwFirstChance);fflush(events);status=DBG_EXCEPTION_NOT_HANDLED;}
        }else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){
            fprintf(events,"exit %lu\n",event.u.ExitProcess.dwExitCode);fclose(events);
            ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status);
            return stop?124:event.u.ExitProcess.dwExitCode;
        }
        if(!ContinueDebugEvent(event.dwProcessId,event.dwThreadId,status))fail("continue");
    }
}
