/* Explicit original-only debugger. Never linked into native reconstruction. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_RVA 0x1000
#define TEXT_SIZE 0x12371
#define TARGET_ENTRY 0x12d20
#define TARGET_SIZE 0x30000
#define MAX_RANGES 1024

typedef struct Range { DWORD start,end,kind; } Range;
typedef struct Point { DWORD at; BYTE original; int active,kind; } Point;
static HANDLE child,threads[64];
static DWORD thread_ids[64],thread_count,base,exe_base,initial_teb,target_loader_entry;
static FILE *events;
static BYTE snapshot[TEXT_SIZE],masked[TEXT_SIZE],globals[87148];
static Range ranges[MAX_RANGES];
static unsigned range_count;
static Point points[2048];
static unsigned point_count;
static int baseline,guarded,ready;
static DWORD callback_text,callback_bytes,lifecycle_reason,format_dest,format_capacity,export_count,export_vector;
static DWORD export_storage[256],export_lengths[256];
static DWORD heap_allocation_size;
static DWORD character_pointer;
static CONTEXT probe_context;
static DWORD probe_return;
static unsigned heap_limit=1,free_limit,character_limit,heap_observed,free_observed,characters_observed;

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
static void vector_state(const char *stage)
{
    DWORD n,pointer;char label[80];
    fprintf(events,"export-%s-argc %lu\n",stage,export_count);
    for(n=0;n<export_count;++n){
        readmem(export_vector+n*4,&pointer,4);
        fprintf(events,"export-%s-pointer %lu %08lx\n",stage,n,pointer);
        sprintf(label,"export-%s-argv-%lu",stage,n);hexbytes(label,pointer,string_bytes(pointer));
        if(!strcmp(stage,"before")){export_storage[n]=pointer;export_lengths[n]=string_bytes(pointer);}
        else {sprintf(label,"export-%s-storage-%lu",stage,n);hexbytes(label,export_storage[n],export_lengths[n]);}
    }
    readmem(export_vector+export_count*4,&pointer,4);
    fprintf(events,"export-%s-terminator %08lx\n",stage,pointer);fflush(events);
}
static void process_inputs(void)
{
    DWORD peb,parameters,environment,buffer,n,written;WORD length;WCHAR word,prior=1;
    HANDLE file;
    readmem(initial_teb+0x30,&peb,4);readmem(peb+0x10,&parameters,4);
    readmem(parameters+0x24,&length,2);readmem(parameters+0x28,&buffer,4);
    hexbytes("process-cwd-utf16",buffer,length);
    readmem(parameters+0x48,&environment,4);
    file=CreateFileA("environment-utf16.bin",GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
    if(file==INVALID_HANDLE_VALUE)fail("environment-open");
    for(n=0;n<32768;++n){
        readmem(environment+n*2,&word,2);
        if(!WriteFile(file,&word,2,&written,NULL)||written!=2)fail("environment-write");
        if(!word&&!prior)break;prior=word;
    }
    CloseHandle(file);if(n==32768)fail("environment-limit");
    fprintf(events,"process-environment-bytes %lu\n",(n+1)*2);fflush(events);
}
static void export_inputs(CONTEXT *context)
{
    DWORD words[7];readmem(context->Esp,words,sizeof(words));
    fprintf(events,"export-caller %08lx\n",words[0]);
    fprintf(events,"export-args %08lx %08lx %08lx %08lx %lu %08lx\n",words[1],words[2],words[3],words[4],words[5],words[6]);
    export_count=words[5];export_vector=words[6];
    if(export_count>256)fail("export-argc");vector_state("before");process_inputs();
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
    BYTE trap=0xcc;Point *p;unsigned index;
    for(index=0;index<point_count;++index)if(!points[index].active)break;
    if(index==point_count){if(point_count==2048)fail("point-overflow");++point_count;}
    p=points+index;
    p->at=at;p->kind=kind;p->active=1;
    readmem(at,&p->original,1);writebytes(at,&trap,1);
    return index;
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
    if(!_stricmp(name,"MSVCRT.dll"))save_file(file,"loaded-crt.bin");
    if(!_stricmp(name,"RCDLL.dll")){
        BYTE byte;
        if(base||nt.OptionalHeader.ImageBase!=0x71c20000||nt.OptionalHeader.AddressOfEntryPoint!=TARGET_ENTRY||nt.OptionalHeader.SizeOfImage!=TARGET_SIZE)fail("target-identity");
        if(nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress)fail("target-tls");
        base=at;save_file(file,"loaded-target.bin");
        readmem(base+TARGET_ENTRY,&byte,1);
        if(byte!=0x53)fail("entry-byte");
        point(base+TARGET_ENTRY,1);guarded=1;
        fprintf(events,"entry-guard %08lx original=53\n",base+TARGET_ENTRY);
    }
    fflush(events);
}
static void module_path(DWORD module,const char *label)
{
    DWORD peb,ldr,head,node,next,dll,buffer,n;
    WORD length;WCHAR wide;char path[MAX_PATH];
    readmem(initial_teb+0x30,&peb,4);readmem(peb+0x0c,&ldr,4);
    head=ldr+0x0c;readmem(head,&node,4);
    for(n=0;n<1024 && node!=head;++n){
        readmem(node+0x18,&dll,4);
        if(dll==module){
            readmem(node+0x24,&length,2);readmem(node+0x28,&buffer,4);
            if((length&1)||length/2>=sizeof(path))fail("module-path-length");
            for(n=0;n<(DWORD)(length/2);++n){readmem(buffer+n*2,&wide,2);if(wide>127)fail("module-path-encoding");path[n]=(char)wide;}
            target_loader_entry=node;path[n]=0;fprintf(events,"module-path %s %s\n",label,path);fflush(events);return;
        }
        readmem(node,&next,4);if(next==node)fail("module-list-cycle");node=next;
    }
    fail("module-path-not-found");
}
static void snapshot_and_mask(void)
{
    FILE *file;unsigned n;
    module_path(base,"target");
    readmem(base+TEXT_RVA,snapshot,sizeof(snapshot));
    file=fopen("relocated-text.bin","wb");if(!file)fail("snapshot-open");
    if(fwrite(snapshot,1,sizeof(snapshot),file)!=sizeof(snapshot))fail("snapshot-write");
    fclose(file);
    if(!baseline){
        memset(masked,0xcc,sizeof(masked));
        /* Proven loader metadata: IAT/debug directory and import/export tables. */
        memcpy(masked,snapshot,0x14c);
        memcpy(masked+0x11e2c,snapshot+0x11e2c,0x545);
        for(n=0;n<range_count;++n){
            Range *r=ranges+n;
            if(r->start<TEXT_RVA||r->end>TEXT_RVA+TEXT_SIZE||r->start>=r->end)fail("range-bounds");
            memcpy(masked+r->start-TEXT_RVA,snapshot+r->start-TEXT_RVA,r->end-r->start);
        }
        writebytes(base+TEXT_RVA,masked,sizeof(masked));
        {
            BYTE verified[TEXT_SIZE];
            readmem(base+TEXT_RVA,verified,sizeof(verified));
            if(memcmp(verified,masked,sizeof(masked)))fail("mask-readback");
            file=fopen("masked-text.bin","wb");if(!file)fail("mask-open");
            if(fwrite(verified,1,sizeof(verified),file)!=sizeof(verified))fail("mask-write");
            fclose(file);
        }
        for(n=0;n<range_count;++n)if(ranges[n].kind)point(base+ranges[n].start,2);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x12df0&&ranges[n].end==0x12e1f&&!ranges[n].kind)point(base+0x12df0,28);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x12bb0 && ranges[n].kind)point(base+0x12c30,11);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x7443 && ranges[n].kind)point(base+0x7466,13);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x7443 && ranges[n].kind)point(base+0x7b77,22);
        for(n=0;n<range_count;++n)if(character_limit&&ranges[n].start==0x7443 && ranges[n].kind)point(base+0x8215,26);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x84c4 && ranges[n].kind)point(base+0x850b,17);
        for(n=0;n<range_count;++n)if(heap_limit&&ranges[n].start==0x109b2 && ranges[n].kind)point(base+0x109c3,19);
        for(n=0;n<range_count;++n)if(free_limit&&ranges[n].start==0x109fd && ranges[n].kind)point(base+0x10a11,24);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x8bcd && ranges[n].kind)point(exe_base+0x1100,7);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x8b46 && ranges[n].kind)point(base+0x8b6f,4);
    }else{
        point(base+0x8b46,3);point(base+0x8b6f,4);
        point(base+0x2c64,5);
        point(exe_base+0x1100,7);
        point(base+0x7466,13);point(base+0x2c98,15);point(base+0x850b,17);if(heap_limit)point(base+0x109c3,19);
        point(base+0x7b77,22);
        if(free_limit)point(base+0x10a11,24);if(character_limit)point(base+0x8215,26);
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
            if(!guarded||ready)fail("guard-state");
            {
                readmem(c.Esp,words,16);lifecycle_reason=words[2];
                fprintf(events,"lifecycle-call %lu %08lx %08lx\n",words[2],words[1],words[3]);
                point(words[0],9);
            }
            snapshot_and_mask();
            if(!baseline){
                BYTE byte;readmem(address,&byte,1);
                if(byte==0xcc){
                    for(n=0;n<range_count;++n)if(ranges[n].kind&&ranges[n].start==TARGET_ENTRY)break;
                    if(n==range_count)p=NULL;
                    else {unsigned k;writebytes(address,snapshot+TARGET_ENTRY-TEXT_RVA,1);for(k=0;k<point_count;++k)if(points[k].active&&points[k].kind==2&&points[k].at==address)points[k].active=0;fprintf(events,"entry %08lx\n",TARGET_ENTRY);}
                }
            }
        }else if(p->kind==2){fprintf(events,"entry %08lx\n",address-base);if(address==base+0x8b46)export_inputs(&c);}
        else if(p->kind==3){
            export_inputs(&c);
        }else if(p->kind==4){fprintf(events,"export-result %ld\n",(long)c.Eax);if(export_vector)vector_state("after");}
        else if(p->kind==5){readmem(c.Esp,words,8);fprintf(events,"message-id %lu\n",words[1]);point(base+0x2c89,6);}
        else if(p->kind==6){
            fprintf(events,"message-result %lu\n",c.Eax);
            hexbytes("message-buffer",base+0x15ca0,c.Eax+1);point(base+0x2c64,5);
        }else if(p->kind==7){
            readmem(c.Esp,words,16);callback_text=words[3];callback_bytes=string_bytes(callback_text);
            fprintf(events,"callback %lu %lu\n",words[1],words[2]);hexbytes("before",callback_text,callback_bytes);
            point(words[0],8);
        }else if(p->kind==8){hexbytes("after",callback_text,callback_bytes);fprintf(events,"callback-return %ld\n",(long)c.Eax);point(exe_base+0x1100,7);}
        else if(p->kind==13){
            DWORD registration,level;
            readmem(c.Esp,words,8);readmem(initial_teb,&registration,4);
            level=0xffffffffUL;if(registration!=0xffffffffUL)readmem(registration+12,&level,4);
            fprintf(events,"setjmp-call %08lx %lu registration=%08lx try-level=%08lx\n",words[0],words[1],registration,level);
            hexbytes("setjmp-before",base+0x25998,64);point(base+0x746b,14);
        }else if(p->kind==14){
            fprintf(events,"setjmp-result %ld\n",(long)c.Eax);hexbytes("setjmp-after",base+0x25998,64);
        }else if(p->kind==15){
            DWORD error;
            readmem(c.Esp,words,24);format_dest=words[1];format_capacity=words[2];
            readmem(initial_teb+0x34,&error,4);
            fprintf(events,"format-call capacity=%lu version=%08lx build=%lu last-error=%08lx\n",words[2],words[4],words[5],error);
            hexbytes("format-template",words[3],string_bytes(words[3]));
            if(words[0]==base+0x7aa2||words[0]==base+0x7b15)hexbytes("format-version",words[4],string_bytes(words[4]));
            hexbytes("format-before",format_dest,format_capacity);point(base+0x2cbe,16);
        }else if(p->kind==16){
            DWORD error;readmem(initial_teb+0x34,&error,4);
            fprintf(events,"format-result %lu last-error=%08lx\n",c.Eax,error);hexbytes("format-after",format_dest,format_capacity);
            point(base+0x2c98,15);
        }
        else if(p->kind==17){
            DWORD error;readmem(initial_teb+0x34,&error,4);readmem(c.Esp,words,12);fprintf(events,"heap-create %lu %lu %lu last-error=%08lx\n",words[0],words[1],words[2],error);point(base+0x8511,18);
        }else if(p->kind==18){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"heap-result %08lx last-error=%08lx\n",c.Eax,error);}
        else if(p->kind==19){DWORD error;readmem(initial_teb+0x34,&error,4);readmem(c.Esp,words,12);heap_allocation_size=words[2];if(heap_allocation_size>1048576)fail("heap-observation-size");fprintf(events,"heap-alloc %08lx %lu %lu last-error=%08lx\n",words[0],words[1],words[2],error);point(base+0x109c9,20);}
        else if(p->kind==20){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"heap-alloc-result %08lx last-error=%08lx\n",c.Eax,error);if(c.Eax)hexbytes("heap-alloc-payload",c.Eax,heap_allocation_size);if(++heap_observed<heap_limit)point(base+0x109c3,19);}
        else if(p->kind==11){readmem(c.Esp,words,4);fprintf(events,"crt-malloc-size %lu\n",words[0]);point(base+0x12c36,12);}
        else if(p->kind==12)fprintf(events,"crt-malloc-result %08lx\n",c.Eax);
        else if(p->kind==22){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"getacp-call last-error=%08lx\n",error);point(base+0x7b7d,23);}
        else if(p->kind==23){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"getacp-result %lu last-error=%08lx\n",c.Eax,error);}
        else if(p->kind==24){DWORD error;readmem(initial_teb+0x34,&error,4);readmem(c.Esp,words,12);fprintf(events,"heap-free %08lx %lu %08lx last-error=%08lx\n",words[0],words[1],words[2],error);point(base+0x10a17,25);}
        else if(p->kind==25){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"heap-free-result %lu last-error=%08lx\n",c.Eax,error);if(++free_observed<free_limit)point(base+0x10a11,24);}
        else if(p->kind==26){DWORD error;readmem(initial_teb+0x34,&error,4);readmem(c.Esp,&character_pointer,4);fprintf(events,"charnext-call %08lx last-error=%08lx\n",character_pointer,error);hexbytes("charnext-input",character_pointer,string_bytes(character_pointer));point(base+0x821b,27);}
        else if(p->kind==27){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"charnext-result %08lx delta=%ld last-error=%08lx\n",c.Eax,(long)(c.Eax-character_pointer),error);if(++characters_observed<character_limit)point(base+0x8215,26);}
        else if(p->kind==28){
            readmem(c.Esp,&probe_return,4);probe_context=c;
            fprintf(events,"compiler-probe-entry eax=%08lx esp=%08lx return=%08lx ecx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n",c.Eax,c.Esp,probe_return,c.Ecx,c.Ebx,c.Esi,c.Edi,c.Ebp);
            if(c.Eax!=0x42c4||probe_return!=base+0x8576)fail("compiler-probe-scope");point(probe_return,29);
        }else if(p->kind==29){
            fprintf(events,"compiler-probe-return eax=%08lx esp=%08lx ecx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx\n",c.Eax,c.Esp,c.Ecx,c.Ebx,c.Esi,c.Edi,c.Ebp);
            if(c.Esp!=probe_context.Esp+4-probe_context.Eax||c.Eax!=probe_return||c.Ecx!=probe_context.Ecx||c.Ebx!=probe_context.Ebx||c.Esi!=probe_context.Esi||c.Edi!=probe_context.Edi||c.Ebp!=probe_context.Ebp)fail("compiler-probe-return");
            hexbytes("compiler-probe-lowest-stack",c.Esp,4);
        }
        else if(p->kind==21){
            readmem(c.Esp,words,16);
            fprintf(events,"driver-main %08lx caller=%08lx argc=%lu argv=%08lx envp=%08lx\n",address,words[0],words[1],words[2],words[3]);
        }
        else if(p->kind==9){
            fprintf(events,"lifecycle-return %lu %ld\n",lifecycle_reason,(long)c.Eax);
            if(lifecycle_reason==0){FILE *state;readmem(base+0x14000,globals,sizeof(globals));state=fopen("completion-globals.bin","wb");if(!state)fail("completion-open");if(fwrite(globals,1,sizeof(globals),state)!=sizeof(globals))fail("completion-write");fclose(state);}
            point(base+TARGET_ENTRY,10);
        }
        else if(p->kind==10){readmem(c.Esp,words,16);lifecycle_reason=words[2];fprintf(events,"lifecycle-call %lu %08lx %08lx\n",words[2],words[1],words[3]);point(words[0],9);}
        if(p){
            if(!SetThreadContext(thread,&c))fail("set-context");
            fflush(events);return 1;
        }
    }
    if(base&&ready&&address>=base+TEXT_RVA&&address<base+TEXT_RVA+TEXT_SIZE){
        readmem(c.Esp,words,sizeof(words));
        fprintf(events,"frontier %08lx eax=%08lx ecx=%08lx edx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n",address-base,c.Eax,c.Ecx,c.Edx,c.Ebx,c.Esi,c.Edi,c.Ebp,c.Esp);
        fprintf(events,"stack");for(n=0;n<12;++n)fprintf(events," %08lx",words[n]);fprintf(events,"\n");
        if(export_vector)vector_state("frontier");
        if(address==base+0x8c9e){
            DWORD state[8],position;
            if(words[2]!=250)fail("text-sample-size");
            hexbytes("text-sample",words[1],words[2]+2);hexbytes("text-flags",words[3],4);
            readmem(c.Ebx,state,32);hexbytes("text-file-state",c.Ebx,32);
            if(state[0]<state[2]||state[0]-state[2]+state[1]>4096)fail("text-file-buffer");
            hexbytes("text-file-buffer",state[2],state[0]-state[2]+state[1]);
            readmem(c.Ebp+8,&position,4);fprintf(events,"text-original-position %lu\n",position);
        }
        if(address==base+0x1087c){
            DWORD output,state[8];
            readmem(c.Esp+0x18,&output,4);readmem(output,state,32);
            hexbytes("detect-input-state",words[1],32);hexbytes("detect-output-state",output,32);
            if(state[0]<state[2]||state[0]-state[2]>8196)fail("detect-output-buffer");
            hexbytes("detect-output-buffer",state[2],state[0]-state[2]);
            fprintf(events,"write-result %lu\n",c.Eax);
        }
        if(address==base+0x10a84){
            DWORD pointer;
            if(words[3]>8196)fail("write-checkpoint-size");
            hexbytes("write-payload",words[2],words[3]);
            hexbytes("write-file-state",words[1],32);
            readmem(base+0x149c4,&pointer,4);hexbytes("write-filename",pointer,514);
        }
        if(address==base+0x109fd){
            DWORD include;readmem(base+0x25588,&include,4);
            hexbytes("release-block",words[1]-8,528);
            hexbytes("release-new-block",c.Ebx-8,528);
            hexbytes("release-include",include,string_bytes(include));
        }
        {
            DWORD pointer;
            readmem(base+0x25588,&pointer,4);if(pointer)hexbytes("include-current-block",pointer-8,528);
            readmem(base+0x25f4c,&pointer,4);if(pointer){hexbytes("input-temporary",pointer,string_bytes(pointer));hexbytes("input-temporary-block",pointer-8,268);}
            readmem(base+0x25474,&pointer,4);if(pointer){hexbytes("output-temporary",pointer,string_bytes(pointer));hexbytes("output-temporary-block",pointer-8,268);}
            readmem(base+0x25994,&pointer,4);if(pointer)hexbytes("output-file-state",pointer,32);
        }
        if(address==base+0x856c||address==base+0x8b72){
            DWORD count,pointer;char label[80];
            readmem(c.Ebp-4,&count,4);if(count>256)fail("preprocessor-count");
            fprintf(events,"preprocessor-count %lu\n",count);
            for(n=0;n<count;++n){readmem(base+0x25590+4*n,&pointer,4);sprintf(label,"preprocessor-argv-%u",n);hexbytes(label,pointer,string_bytes(pointer));}
            hexbytes("prepare-path",words[1],string_bytes(words[1]));
        }
        {
            FILE *state;
            readmem(base+0x14000,globals,sizeof(globals));
            state=fopen("globals.bin","wb");if(!state)fail("globals-open");
            if(fwrite(globals,1,sizeof(globals),state)!=sizeof(globals))fail("globals-write");
            fclose(state);
            {
                DWORD exit_pointer;
                memcpy(&exit_pointer,globals+0x15464,4);
                if(exit_pointer)hexbytes("crt-exit-head",exit_pointer,4);
            }
        }
        {
            DWORD flags;
            readmem(target_loader_entry+0x34,&flags,4);
            fprintf(events,"module-loader-flags %08lx\n",flags);
        }
        if(address==base+0x2c98){hexbytes("formatter-template",words[3],string_bytes(words[3]));hexbytes("formatter-version",words[4],string_bytes(words[4]));}
        if(address==base+0x8bcd)hexbytes("emit-message",words[1],string_bytes(words[1]));
        if(address==base+0x8b46 || address==base+0x7443){
            DWORD count,vector,pointer;char label[40];
            count=address==base+0x8b46?words[5]:words[1];
            vector=address==base+0x8b46?words[6]:words[2];
            if(count>256)fail("argument-count");
            fprintf(events,"argument-count %lu\n",count);
            for(n=0;n<count;++n){readmem(vector+n*4,&pointer,4);sprintf(label,"argv-%lu",n);hexbytes(label,pointer,string_bytes(pointer));}
            readmem(vector+count*4,&pointer,4);fprintf(events,"argv-terminator %08lx\n",pointer);
        }
        fflush(events);TerminateProcess(child,124);return 2;
    }
    fprintf(events,"unhandled-break %08lx\n",address);fflush(events);return 0;
}
int main(int argc,char **argv)
{
    STARTUPINFOA startup;PROCESS_INFORMATION process;DEBUG_EVENT event;
    DWORD status,i;HANDLE thread;FILE *file;char command[2048],tail[1800];const char *directory=NULL;int stop=0;
    if(argc!=4&&argc!=6)return 125;
    {
        char value[32];
        if(GetEnvironmentVariableA("RCDLL_CAPTURE_HEAP_LIMIT",value,sizeof(value)))heap_limit=atoi(value);
        if(GetEnvironmentVariableA("RCDLL_CAPTURE_FREE_LIMIT",value,sizeof(value)))free_limit=atoi(value);
        if(GetEnvironmentVariableA("RCDLL_CAPTURE_CHARACTER_LIMIT",value,sizeof(value)))character_limit=atoi(value);
        if(heap_limit>8192||free_limit>8192||character_limit>8192)return 125;
        SetEnvironmentVariableA("RCDLL_CAPTURE_HEAP_LIMIT",NULL);SetEnvironmentVariableA("RCDLL_CAPTURE_FREE_LIMIT",NULL);SetEnvironmentVariableA("RCDLL_CAPTURE_CHARACTER_LIMIT",NULL);
    }
    baseline=!strcmp(argv[1],"baseline")||!strcmp(argv[1],"api-probe");
    if(!baseline&&strcmp(argv[1],"masked"))return 125;
    file=fopen(argv[3],"rb");if(!file)return 125;
    while(range_count<MAX_RANGES&&fscanf(file,"%lx %lx %lu",&ranges[range_count].start,&ranges[range_count].end,&ranges[range_count].kind)==3)++range_count;
    if(!feof(file)||range_count==MAX_RANGES){fclose(file);return 125;}fclose(file);
    events=fopen("events.txt","wb");if(!events)return 125;
    if(argc==6){
        size_t length;file=fopen(argv[4],"rb");if(!file)return 125;
        length=fread(tail,1,sizeof(tail)-1,file);if(!feof(file)){fclose(file);return 125;}fclose(file);tail[length]=0;
        if(strchr(tail,'\r')||strchr(tail,'\n'))return 125;directory=argv[5];
    }else strcpy(tail,"/help");
    sprintf(command,"\"%s\" %s",argv[2],tail);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);
    SetEnvironmentVariableA("TMP",".");SetEnvironmentVariableA("TEMP",".");
    if(!CreateProcessA(argv[2],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,directory,&startup,&process))fail("create");
    for(;;){
        if(!WaitForDebugEvent(&event,30000))fail("debug-wait");
        status=DBG_CONTINUE;
        if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            child=event.u.CreateProcessInfo.hProcess;initial_teb=(DWORD)event.u.CreateProcessInfo.lpThreadLocalBase;exe_base=(DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            thread_ids[thread_count]=event.dwThreadId;threads[thread_count++]=event.u.CreateProcessInfo.hThread;
            save_file(event.u.CreateProcessInfo.hFile,"loaded-driver.bin");
            if(argc==6)point(exe_base+0x11d2,21);
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
