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
static DWORD thread_ids[64],thread_count,base,exe_base,initial_teb,target_loader_entry,crt_base;
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
static DWORD probe_return,cpinfo_pointer,cpinfo_error,local_pointer,local_bytes,line_file,line_format_pointer,towupper_value;
static DWORD cpp_count,cpp_vector,cpp_allocation_size,source_format_dest,output_write_file;
static unsigned macro_copy_count;
static DWORD macro_copy_length,macro_copy_sp,macro_copy_site,macro_trim_pointer;
static int path_errors;
static DWORD path_api_pointer,path_api_call;
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
static DWORD crt_errno_value(void)
{
    DWORD index,context,value;
    /* Canonical MSVCRT _errno (RVA6436) returns _getptd (RVA15c3)+8. That
     * provider's _getptd reads its TLS index at RVA3d004 through TlsGetValue.
     * Inspect the existing i386 TEB slot without calling into the child. */
    if(!crt_base)fail("errno-provider");
    readmem(crt_base+0x3d004,&index,4);
    if(index>=64)fail("errno-tls-expansion-unobserved");
    readmem(initial_teb+0xe10+4*index,&context,4);
    if(!context)fail("errno-thread-context");
    readmem(context+8,&value,4);return value;
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
static DWORD wide_string_bytes(DWORD at)
{
    DWORD n;WCHAR c;
    for(n=0;n<4177;++n){readmem(at+2*n,&c,2);if(!c)return 2*(n+1);}
    fail("unterminated-wide-string");return 0;
}
static void hexbytes(const char *tag,DWORD at,DWORD size)
{
    DWORD n,take,i; BYTE raw[512];char encoded[1024];
    const char *digits="0123456789abcdef";
    fprintf(events,"%s %lu ",tag,size);
    for(n=0;n<size;n+=take){
        take=size-n;if(take>sizeof(raw))take=sizeof(raw);readmem(at+n,raw,take);
        for(i=0;i<take;++i){encoded[2*i]=digits[raw[i]>>4];encoded[2*i+1]=digits[raw[i]&15];}
        if(fwrite(encoded,2,take,events)!=take)fail("hex-write");
    }
    fprintf(events,"\n");fflush(events);
}
static void crt_file_inventory(const char *phase)
{
    DWORD count,index,block,handle;BYTE flags;char label[80];
    /* Canonical provider's lowio allocation (36-byte entries) and exported
     * _iob at RVA3f570. Capture live slots, not unused capacity equivalence. */
    readmem(crt_base+0x3f7f0,&count,4);if(count>2048)fail("lowio-capacity");
    fprintf(events,"lowio-capacity %s %lu\n",phase,count);
    for(index=0;index<count;++index){
        readmem(crt_base+0x3f460+4*(index>>5),&block,4);if(!block)continue;
        readmem(block+36*(index&31)+4,&flags,1);
        if(flags&1){
            readmem(block+36*(index&31),&handle,4);
            fprintf(events,"lowio-open %s %lu flags=%02x handle=%08lx\n",phase,index,flags,handle);
            sprintf(label,"lowio-%s-%lu",phase,index);hexbytes(label,block+36*(index&31),36);
        }
    }
    sprintf(label,"iob-%s",phase);hexbytes(label,crt_base+0x3f570,20*32);
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
static void crt_file_state(DWORD pointer,const char *label,int full_buffer)
{
    DWORD fields[8],array,handle,position;HANDLE duplicate;
    readmem(pointer,fields,32);fprintf(events,"%s-pointer %08lx\n",label,pointer);{char name[80];sprintf(name,"%s-file",label);hexbytes(name,pointer,32);}
    if(fields[2]){char name[80];DWORD bytes=full_buffer?fields[6]:fields[0]-fields[2];if(!fields[6]||fields[6]>65536||bytes>fields[6])fail("file-buffer-size");sprintf(name,"%s-buffer",label);hexbytes(name,fields[2],bytes);}
    if(fields[4]>=256)fail("source-lowio-scope");readmem(crt_base+0x3f460+4*(fields[4]>>5),&array,4);readmem(array+36*(fields[4]&31),&handle,4);
    if(!DuplicateHandle(child,(HANDLE)handle,GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS))fail("source-duplicate-handle");
    position=SetFilePointer(duplicate,0,NULL,FILE_CURRENT);CloseHandle(duplicate);if(position==0xffffffffUL)fail("source-file-position");
    fprintf(events,"%s-position %lu\n",label,position);
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
    if(!_stricmp(name,"MSVCRT.dll")){crt_base=at;save_file(file,"loaded-crt.bin");}
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
        for(n=0;n<range_count;++n)if(ranges[n].start==0x10bcf&&ranges[n].kind){point(base+0x10bf3,40);point(base+0x10bff,40);point(base+0x8765,42);point(base+0x894f,43);}
        for(n=0;n<range_count;++n)if(ranges[n].start==0x2b84&&ranges[n].kind)point(base+0x2bc1,45);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x44fd&&ranges[n].kind)point(base+0x45d5,55);
        for(n=0;n<range_count;++n)if(ranges[n].start==0xbee2&&ranges[n].kind){point(base+0x4d2a,53);point(base+0x4d63,53);}
        for(n=0;n<range_count;++n)if(ranges[n].start==0x26d9&&ranges[n].kind)point(base+0x11b71,64);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x4240&&ranges[n].kind)point(base+0x4253,59);
        for(n=0;n<range_count;++n)if(ranges[n].start==0x91a4&&ranges[n].kind)point(base+0x91b3,61);
        if(path_errors){
            point(base+0x7efb,47);point(base+0x7f49,49);point(base+0x7f7c,49);
            point(base+0x7f93,51);point(base+0x85b6,51);point(base+0x85ed,51);
            fprintf(events,"errno-boundary dll-entry %ld\n",(long)crt_errno_value());
            crt_file_inventory("dll-entry");
        }
        for(n=0;n<range_count;++n)if(ranges[n].start==0x9041&&ranges[n].kind){point(base+0x90e5,33);point(base+0x9182,35);point(base+0x873e,37);}
        for(n=0;n<range_count;++n)if(ranges[n].start==0x8c9e&&ranges[n].kind){point(base+0x8ea2,30);point(base+0x108e7,32);}
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
static DWORD listing_pointer,listing_sp,class_sp,class_count,lookup_sp,logical_sp,integer_sp,digit_sp,kind_sp,storage_number_bp,storage_number_cursor,intern_sp,intern_pointer,intern_node,intern_result,string_sp,expect_sp,compare_sp,angle_sp,search_open_sp,search_attempt,match_sp,block_sp;
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
        }else if(p->kind==2){
            fprintf(events,"entry %08lx\n",address-base);if(address==base+0x8b46)export_inputs(&c);
            if(path_errors&&address==base+0x8b46){fprintf(events,"errno-boundary RC-entry %ld\n",(long)crt_errno_value());crt_file_inventory("RC-entry");}
            if(address==base+0x3db4){DWORD depth;readmem(c.Esp,words,12);readmem(base+0x14a10,&depth,4);fprintf(events,"source-open-entry caller=%08lx path=%08lx first=%ld depth=%ld\n",words[0],words[1],(long)words[2],(long)depth);hexbytes("source-stack-before",base+0x18e80,36*1024);}
            if(address==base+0x4298){DWORD depth,cursor,encoding,frame[9],state[8];readmem(c.Esp,words,4);readmem(base+0x14a10,&depth,4);readmem(base+0x18120,&cursor,4);readmem(base+0x21e84,&encoding,4);readmem(base+0x18e80,frame,36);readmem(frame[1],state,32);fprintf(events,"refill-entry caller=%08lx depth=%ld cursor=%08lx encoding=%ld\n",words[0],(long)depth,cursor,(long)encoding);hexbytes("refill-entry-frame",base+0x18e80,36);hexbytes("refill-entry-buffer",frame[5]-8,8206);hexbytes("refill-entry-file",frame[1],32);hexbytes("refill-entry-file-buffer",state[2],state[6]);point(base+0x3eee,57);}
            if(address==base+0xbebd){readmem(c.Esp,words,8);fprintf(events,"source-name-entry caller=%08lx path=%08lx\n",words[0],words[1]);hexbytes("source-name-input",words[1],wide_string_bytes(words[1]));point(base+0x3ef9,58);}
            if(address==base+0xc036){DWORD marker[4],enabled;readmem(c.Esp,words,8);listing_pointer=words[1];listing_sp=c.Esp;readmem(listing_pointer,marker,16);readmem(base+0x25058,&enabled,4);fprintf(events,"listing-entry caller=%08lx marker=%08lx esp=%08lx enabled=%lu\n",words[0],listing_pointer,c.Esp,enabled);hexbytes("listing-record-before",listing_pointer,16);hexbytes("listing-name",marker[1],wide_string_bytes(marker[1]));hexbytes("listing-text",marker[2],wide_string_bytes(marker[2]));point(words[0],63);}
            if(address==base+0x6b80)point(base+0x6c1a,78);
            if(address==base+0x12a2b){DWORD cursor,line,file;readmem(c.Esp,words,4);block_sp=c.Esp;readmem(base+0x18120,&cursor,4);readmem(base+0x16ce8,&line,4);readmem(base+0x18e74,&file,4);fprintf(events,"block-comment-entry cursor=%08lx line=%lu esp=%08lx caller=%08lx\n",cursor,line,c.Esp,words[0]);crt_file_state(file,"block-before",0);point(words[0],81);}
            if(address==base+0x128e9){DWORD cursor;readmem(c.Esp,words,8);match_sp=c.Esp;readmem(base+0x18120,&cursor,4);fprintf(events,"character-match-entry character=%lu cursor=%08lx esp=%08lx caller=%08lx\n",words[1],cursor,c.Esp,words[0]);point(words[0],80);}
            if(address==base+0x629a){DWORD cursor,frame[9];readmem(c.Esp,words,4);angle_sp=c.Esp;readmem(base+0x18120,&cursor,4);readmem(base+0x18e80,frame,36);fprintf(events,"angle-entry cursor=%08lx esp=%08lx caller=%08lx\n",cursor,c.Esp,words[0]);hexbytes("angle-input-source",frame[5]-8,8206);hexbytes("angle-input-macro",base+0x18128,1024);hexbytes("angle-input-marker",base+0x16cf8,514);point(words[0],75);point(base+0x66b1,76);}
            if(address==base+0x7341){readmem(c.Esp,words,12);compare_sp=c.Esp;fprintf(events,"compare-entry left=%08lx right=%08lx esp=%08lx caller=%08lx\n",words[1],words[2],c.Esp,words[0]);hexbytes("compare-input-left",words[1],20);hexbytes("compare-input-right",words[2],20);point(words[0],74);}
            if(address==base+0x6c71){DWORD cursor;readmem(c.Esp,words,8);expect_sp=c.Esp;readmem(base+0x18120,&cursor,4);fprintf(events,"expect-entry word=%08lx cursor=%08lx esp=%08lx caller=%08lx\n",words[1],cursor,c.Esp,words[0]);hexbytes("expect-input-word",words[1],wide_string_bytes(words[1]));point(words[0],73);}
            if(address==base+0x11f3b){DWORD cursor,token,header,active,directive;WORD word;readmem(c.Esp,words,4);string_sp=c.Esp;readmem(base+0x18120,&cursor,4);readmem(cursor,&word,2);readmem(base+0x16cd0,&token,4);readmem(base+0x16efc,&header,4);readmem(base+0x1811c,&active,4);readmem(base+0x18530,&directive,4);fprintf(events,"string-entry cursor=%08lx word=%u token=%08lx header=%lu active=%lu directive=%lu esp=%08lx caller=%08lx\n",cursor,word,token,header,active,directive,c.Esp,words[0]);hexbytes("string-buffer-before",base+0x16f08,4090);hexbytes("string-extra-before",base+0x16cd4,2);point(words[0],72);}
            if(address==base+0x72a2){readmem(c.Esp,words,8);intern_sp=c.Esp;intern_pointer=words[1];intern_node=c.Esi;fprintf(events,"intern-entry type=%08lx node=%08lx esp=%08lx caller=%08lx\n",intern_pointer,intern_node,c.Esp,words[0]);hexbytes("intern-type-input",intern_pointer,20);hexbytes("intern-node-input",intern_node,20);hexbytes("intern-buckets-before",base+0x22830,1024);point(words[0],71);}
            if(address==base+0x736b){readmem(c.Esp,words,12);storage_number_bp=c.Ebp;storage_number_cursor=c.Ebx;fprintf(events,"store-entry token=%08lx value=%08lx number-bp=%08lx cursor=%08lx\n",words[1],words[2],c.Ebp,c.Ebx);hexbytes("store-integer-entry-input",words[2],4);}
            if(address==base+0x126af){readmem(c.Esp,words,8);kind_sp=c.Esp;fprintf(events,"integer-kind-entry value=%lu esp=%08lx\n",words[1],c.Esp);point(words[0],70);}
            if(address==base+0x125aa){readmem(c.Esp,words,12);integer_sp=c.Esp;fprintf(events,"integer-entry text=%08lx radix=%lu esp=%08lx\n",words[1],words[2],c.Esp);hexbytes("integer-conversion-entry-input",words[1],wide_string_bytes(words[1]));point(words[0],68);}
            if(address==base+0x126fb){DWORD previous,radix;readmem(c.Esp,words,8);digit_sp=c.Esp;readmem(c.Ebp-4,&previous,4);readmem(c.Ebp+12,&radix,4);fprintf(events,"digit-value-entry character=%lu esp=%08lx\n",words[1],c.Esp);fprintf(events,"integer-conversion-state cursor=%08lx accumulator=%lu previous=%lu radix=%lu\n",c.Esi,c.Ebx,previous,radix);hexbytes("integer-conversion-input",c.Esi,wide_string_bytes(c.Esi));point(words[0],69);}
            if(address==base+0x12b67){DWORD cursor;WORD value;readmem(c.Esp,words,4);readmem(base+0x18120,&cursor,4);readmem(cursor,&value,2);logical_sp=c.Esp;fprintf(events,"logical-character-entry caller=%08lx cursor=%08lx word=%u esp=%08lx\n",words[0],cursor,value,c.Esp);point(words[0],67);}
            if(address==base+0x478b){readmem(c.Esp,words,8);lookup_sp=c.Esp;fprintf(events,"directive-lookup-entry caller=%08lx word=%08lx esp=%08lx\n",words[0],words[1],c.Esp);hexbytes("directive-lookup-word",words[1],wide_string_bytes(words[1]));point(words[0],66);}
            if(address==base+0xbc80){readmem(c.Esp,words,12);cpp_count=words[1];cpp_vector=words[2];if(cpp_allocation_size)hexbytes("wide-initial-argument-block",cpp_vector-8,cpp_allocation_size);}
        }
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
        else if(p->kind==20){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"heap-alloc-result %08lx last-error=%08lx\n",c.Eax,error);if(c.Eax)hexbytes("heap-alloc-payload",c.Eax,heap_allocation_size);if(heap_observed==4)cpp_allocation_size=heap_allocation_size;if(++heap_observed<heap_limit)point(base+0x109c3,19);}
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
        }else if(p->kind==30){
            readmem(c.Esp,words,8);if(words[0]!=0)fail("cpinfo-page");cpinfo_pointer=words[1];readmem(initial_teb+0x34,&cpinfo_error,4);
            fprintf(events,"cpinfo-call page=0 last-error=%08lx\n",cpinfo_error);hexbytes("cpinfo-before",cpinfo_pointer,20);point(base+0x8ea8,31);
        }else if(p->kind==31){
            DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"cpinfo-result %lu last-error=%08lx\n",c.Eax,error);hexbytes("cpinfo-after",cpinfo_pointer,20);
        }else if(p->kind==32){
            DWORD flags;readmem(c.Ebp-4,&flags,4);fprintf(events,"text-test-result %lu flags=%08lx\n",c.Eax,flags);
        }else if(p->kind==33){
            readmem(c.Esp,words,8);if(words[0]!=0x40||words[1]!=0x1051)fail("line-local-allocate-scope");
            local_bytes=words[1];readmem(c.Ebp+0x14,&line_file,4);fprintf(events,"line-local-allocate %lu %lu\n",words[0],words[1]);point(base+0x90eb,34);
        }else if(p->kind==34){
            local_pointer=c.Eax;if(!local_pointer)fail("line-local-allocate-null");fprintf(events,"line-local-pointer %08lx\n",local_pointer);hexbytes("line-local-initial",local_pointer,local_bytes);
        }else if(p->kind==35){
            readmem(c.Esp,words,4);if(words[0]!=local_pointer)fail("line-local-free-owner");fprintf(events,"line-local-free-pointer %08lx\n",words[0]);hexbytes("line-local-final",local_pointer,local_bytes);point(base+0x9188,36);
        }else if(p->kind==36){fprintf(events,"line-local-free-result %lu\n",c.Eax);
        }else if(p->kind==37){
            readmem(c.Esp,words,8);fprintf(events,"line-ctype %u %u\n",(WORD)words[0],(WORD)words[1]);point(base+0x8744,38);
        }else if(p->kind==38){fprintf(events,"line-ctype-result %lu\n",c.Eax);
        }else if(p->kind==40){
            readmem(c.Esp,words,4);towupper_value=(WORD)words[0];fprintf(events,"prefix-towupper %lu raw=%08lx\n",towupper_value,words[0]);point(address==base+0x10bf3?base+0x10bf5:base+0x10c01,41);
        }else if(p->kind==41){fprintf(events,"prefix-towupper-result %u raw=%08lx\n",(WORD)c.Eax,c.Eax);
        }else if(p->kind==42){fprintf(events,"prefix-first-result %ld\n",(long)c.Eax);
        }else if(p->kind==43){
            readmem(c.Esp,words,12);line_format_pointer=words[0];fprintf(events,"line-format-number %ld\n",(long)words[2]);hexbytes("line-format-template",words[1],wide_string_bytes(words[1]));hexbytes("line-format-before",line_format_pointer,8196);point(base+0x8955,44);
        }else if(p->kind==44){fprintf(events,"line-format-result %ld\n",(long)c.Eax);hexbytes("line-format-after",line_format_pointer,8196);
        }else if(p->kind==45){
            DWORD error;readmem(c.Esp,words,12);readmem(initial_teb+0x34,&error,4);
            if(words[1]||words[2]!=10)fail("number-conversion-scope");
            fprintf(events,"number-conversion end=%08lx base=%lu last-error=%08lx errno=%ld\n",words[1],words[2],error,(long)crt_errno_value());
            hexbytes("number-conversion-input",words[0],wide_string_bytes(words[0]));point(base+0x2bc7,46);
        }else if(p->kind==46){
            DWORD error;readmem(initial_teb+0x34,&error,4);
            fprintf(events,"number-conversion-result %ld last-error=%08lx errno=%ld\n",(long)c.Eax,error,(long)crt_errno_value());
        }else if(p->kind==47||p->kind==49||p->kind==51){
            DWORD error;readmem(c.Esp,words,12);readmem(initial_teb+0x34,&error,4);
            path_api_call=address-base;path_api_pointer=words[0];
            fprintf(events,"path-api-call %08lx errno=%ld last-error=%08lx\n",path_api_call,(long)crt_errno_value(),error);
            if(p->kind==47){
                if(words[2]!=260)fail("fullpath-capacity");
                fprintf(events,"fullpath-capacity %lu\n",words[2]);hexbytes("fullpath-input",words[1],string_bytes(words[1]));hexbytes("fullpath-before",words[0],260);
            }else if(p->kind==49)hexbytes("mktemp-before",words[0],260);
            else{hexbytes("fopen-path",words[0],string_bytes(words[0]));hexbytes("fopen-mode",words[1],string_bytes(words[1]));}
            point(address+((p->kind==47||address==base+0x7f49||address==base+0x7f7c||address==base+0x7f93)?6:2),p->kind+1);
        }else if(p->kind==48||p->kind==50||p->kind==52){
            DWORD error;readmem(initial_teb+0x34,&error,4);
            fprintf(events,"path-api-result %08lx result=%08lx errno=%ld last-error=%08lx\n",path_api_call,c.Eax,(long)crt_errno_value(),error);
            if(p->kind==48){if(c.Eax!=path_api_pointer)fail("fullpath-result-alias");hexbytes("fullpath-after",path_api_pointer,260);}
            else if(p->kind==50){if(c.Eax!=path_api_pointer)fail("mktemp-result-alias");hexbytes("mktemp-after",path_api_pointer,260);}
            else{if(!c.Eax)fail("fopen-result-null");hexbytes("fopen-state",c.Eax,32);}
        }else if(p->kind==53){
            MEMORY_BASIC_INFORMATION memory;char label[80];
            readmem(c.Esp,words,8);if(macro_copy_count>=6||words[1]>512)fail("macro-copy-scope");
            macro_copy_length=words[1];macro_copy_sp=c.Esp;macro_copy_site=address-base;
            if(VirtualQueryEx(child,(void *)words[0],&memory,sizeof(memory))!=sizeof(memory))fail("macro-copy-query");
            fprintf(events,"macro-copy-call %u site=%08lx source=%08lx count=%lu esp=%08lx\n",macro_copy_count,macro_copy_site,words[0],words[1],c.Esp);
            fprintf(events,"macro-copy-region %u base=%08lx allocation=%08lx bytes=%lu state=%08lx protect=%08lx type=%08lx\n",macro_copy_count,(DWORD)memory.BaseAddress,(DWORD)memory.AllocationBase,memory.RegionSize,memory.State,memory.Protect,memory.Type);
            sprintf(label,"macro-copy-source-%u",macro_copy_count);hexbytes(label,words[0],words[1]*2);point(address+5,54);
        }else if(p->kind==54){
            char label[80];if(!c.Eax||c.Esp!=macro_copy_sp+8)fail("macro-copy-return");
            fprintf(events,"macro-copy-return %u result=%08lx esp=%08lx\n",macro_copy_count,c.Eax,c.Esp);
            sprintf(label,"macro-copy-result-%u",macro_copy_count);hexbytes(label,c.Eax-8,macro_copy_length*2+10);
            ++macro_copy_count;point(base+macro_copy_site,53);
        }else if(p->kind==81){DWORD cursor,line,file;readmem(base+0x18120,&cursor,4);readmem(base+0x16ce8,&line,4);readmem(base+0x18e74,&file,4);fprintf(events,"block-comment-return cursor=%08lx line=%lu esp=%08lx\n",cursor,line,c.Esp);if(c.Esp!=block_sp+4)fail("block-comment-return-stack");crt_file_state(file,"block-after",0);
        }else if(p->kind==80){DWORD cursor;readmem(base+0x18120,&cursor,4);fprintf(events,"character-match-return result=%lu cursor=%08lx esp=%08lx\n",c.Eax,cursor,c.Esp);if(c.Esp!=match_sp+8)fail("character-match-return-stack");
        }else if(p->kind==78){DWORD index,error;char label[80];readmem(c.Esp,words,8);readmem(c.Esp+0x1c,&index,4);readmem(initial_teb+0x34,&error,4);search_open_sp=c.Esp;fprintf(events,"search-open attempt=%lu index=%lu path=%08lx first=%lu esp=%08lx errno=%ld last-error=%08lx\n",search_attempt,index,words[0],words[1],c.Esp,(long)crt_errno_value(),error);sprintf(label,"search-open-path-%lu",search_attempt);hexbytes(label,words[0],wide_string_bytes(words[0]));point(base+0x6c1f,79);
        }else if(p->kind==79){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"search-open-return attempt=%lu result=%08lx esp=%08lx errno=%ld last-error=%08lx\n",search_attempt,c.Eax,c.Esp,(long)crt_errno_value(),error);if(c.Esp!=search_open_sp+8)fail("search-open-stack");++search_attempt;if(!c.Eax)point(base+0x6c1a,78);
        }else if(p->kind==75){DWORD cursor;readmem(base+0x18120,&cursor,4);fprintf(events,"angle-return result=%08lx cursor=%08lx esp=%08lx\n",c.Eax,cursor,c.Esp);if(c.Esp!=angle_sp+4)fail("angle-return-stack");hexbytes("angle-result-macro",base+0x18128,1024);hexbytes("angle-result-marker",base+0x16cf8,514);
        }else if(p->kind==76){readmem(c.Esp,words,8);if(words[0]!=base+0x16cf8||words[1]!=base+0x18128)fail("include-copy-alias");fprintf(events,"include-copy destination=%08lx source=%08lx esp=%08lx\n",words[0],words[1],c.Esp);hexbytes("include-copy-source",words[1],wide_string_bytes(words[1]));hexbytes("include-copy-before",words[0],514);point(base+0x66b7,77);
        }else if(p->kind==77){fprintf(events,"include-copy-return result=%08lx esp=%08lx\n",c.Eax,c.Esp);hexbytes("include-copy-after",base+0x16cf8,514);
        }else if(p->kind==74){fprintf(events,"compare-return result=%08lx esp=%08lx\n",c.Eax,c.Esp);if(c.Esp!=compare_sp+12)fail("compare-return-stack");
        }else if(p->kind==73){DWORD cursor;readmem(base+0x18120,&cursor,4);fprintf(events,"expect-return cursor=%08lx esp=%08lx\n",cursor,c.Esp);if(c.Esp!=expect_sp+8)fail("expect-return-stack");
        }else if(p->kind==72){DWORD cursor,token;readmem(base+0x18120,&cursor,4);readmem(base+0x16cd0,&token,4);fprintf(events,"string-return cursor=%08lx token=%08lx esp=%08lx\n",cursor,token,c.Esp);if(c.Esp!=string_sp+4)fail("string-return-stack");hexbytes("string-buffer-after",base+0x16f08,4090);hexbytes("string-extra-after",base+0x16cd4,2);
        }else if(p->kind==71){intern_result=c.Eax;fprintf(events,"intern-return result=%08lx esp=%08lx\n",c.Eax,c.Esp);if(c.Esp!=intern_sp+8)fail("intern-return-stack");hexbytes("intern-result-block",c.Eax-12,32);hexbytes("intern-buckets-after",base+0x22830,1024);
        }else if(p->kind==70){fprintf(events,"integer-kind-return value=%u raw=%08lx esp=%08lx\n",(BYTE)c.Eax,c.Eax,c.Esp);if(c.Esp!=kind_sp+8)fail("integer-kind-stack");
        }else if(p->kind==68){fprintf(events,"integer-return value=%lu esp=%08lx\n",c.Eax,c.Esp);if(c.Esp!=integer_sp+12)fail("integer-return-stack");
        }else if(p->kind==69){fprintf(events,"digit-value-return value=%lu esp=%08lx\n",c.Eax,c.Esp);if(c.Esp!=digit_sp+8)fail("digit-return-stack");
        }else if(p->kind==67){DWORD cursor;readmem(base+0x18120,&cursor,4);fprintf(events,"logical-character-return value=%u raw=%08lx cursor=%08lx esp=%08lx\n",(WORD)c.Eax,c.Eax,cursor,c.Esp);if(c.Esp!=logical_sp+4)fail("logical-character-stack");
        }else if(p->kind==66){fprintf(events,"directive-lookup-return value=%u raw=%08lx esp=%08lx\n",(BYTE)c.Eax,c.Eax,c.Esp);if(c.Esp!=lookup_sp+8)fail("directive-lookup-stack");
        }else if(p->kind==64){WORD value;DWORD first;readmem(c.Esp,words,4);readmem(c.Esp+24,&first,4);if((WORD)words[0]>=256)fail("class-observer-scope");readmem(base+0x14210+2*(WORD)words[0],&value,2);class_sp=c.Esp;fprintf(events,"class-read-call %lu character=%u raw=%08lx table=%u hash=%lu cursor=%08lx first=%u esp=%08lx\n",class_count,(WORD)words[0],words[0],value,c.Ebx,c.Ebp,(WORD)first,c.Esp);point(base+0x11b76,65);
        }else if(p->kind==65){fprintf(events,"class-read-return %lu result=%u raw=%08lx esp=%08lx\n",class_count,(WORD)c.Eax,c.Eax,c.Esp);if(c.Esp!=class_sp+4)fail("class-observer-stack");++class_count;point(base+0x11b71,64);
        }else if(p->kind==63){DWORD enabled;readmem(base+0x25058,&enabled,4);fprintf(events,"listing-return esp=%08lx enabled=%lu\n",c.Esp,enabled);if(c.Esp!=listing_sp+8)fail("listing-return-stack");hexbytes("listing-record-after",listing_pointer,16);
        }else if(p->kind==61){DWORD error;readmem(c.Esp,words,16);output_write_file=words[3];readmem(initial_teb+0x34,&error,4);fprintf(events,"output-write-call data=%08lx size=%lu count=%lu file=%08lx errno=%ld last-error=%08lx\n",words[0],words[1],words[2],words[3],(long)crt_errno_value(),error);hexbytes("output-write-before-file",output_write_file,32);hexbytes("output-write-input",words[0],words[1]*words[2]);point(base+0x91b9,62);
        }else if(p->kind==62){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"output-write-result result=%lu errno=%ld last-error=%08lx\n",c.Eax,(long)crt_errno_value(),error);hexbytes("output-write-after-file",output_write_file,32);
        }else if(p->kind==59){DWORD error;readmem(c.Esp,words,12);source_format_dest=words[0];readmem(initial_teb+0x34,&error,4);fprintf(events,"source-line-format-call dest=%08lx format=%08lx number=%ld errno=%ld last-error=%08lx\n",words[0],words[1],(long)words[2],(long)crt_errno_value(),error);hexbytes("source-line-format-template",words[1],20);hexbytes("source-line-format-before",words[0],1024);point(base+0x4259,60);
        }else if(p->kind==60){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"source-line-format-result result=%ld errno=%ld last-error=%08lx\n",(long)c.Eax,(long)crt_errno_value(),error);hexbytes("source-line-format-after",source_format_dest,1024);
        }else if(p->kind==58){fprintf(events,"source-name-result %08lx\n",c.Eax);hexbytes("source-name-result-block",c.Eax-8,wide_string_bytes(c.Eax)+8);
        }else if(p->kind==57){fprintf(events,"refill-result %ld\n",(long)c.Eax);
        }else if(p->kind==55){
            WORD word;macro_trim_pointer=c.Ebp;readmem(macro_trim_pointer,&word,2);
            fprintf(events,"macro-trim-before pointer=%08lx word=%04x\n",macro_trim_pointer,word);point(base+0x45da,56);
        }else if(p->kind==56){
            WORD word;readmem(macro_trim_pointer,&word,2);fprintf(events,"macro-trim-after pointer=%08lx word=%04x\n",macro_trim_pointer,word);point(base+0x45d5,55);
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
        if(crt_base){DWORD error;readmem(initial_teb+0x34,&error,4);fprintf(events,"frontier-errors errno=%ld last-error=%08lx\n",(long)crt_errno_value(),error);}
        if(export_vector)vector_state("frontier");
        if(address==base+0x8c70){
            DWORD output,state[8],current;readmem(c.Esp+0x1c,&output,4);readmem(output,state,32);hexbytes("progress-output-state",output,32);
            if(state[0]<state[2]||state[0]-state[2]>4096)fail("progress-output-buffer");hexbytes("progress-output-buffer",state[2],state[0]-state[2]);
            readmem(line_file,state,32);hexbytes("progress-input-state",line_file,32);
            if(state[0]<state[2]||state[0]-state[2]+state[1]>4096)fail("progress-input-buffer");hexbytes("progress-input-buffer",state[2],state[0]-state[2]+state[1]);
            readmem(base+0x16ce8,&current,4);fprintf(events,"progress-current-line %lu\n",current);
            hexbytes("progress-line",c.Esp+0x223c,wide_string_bytes(c.Esp+0x223c));
        }
        if(address==base+0x10bcf){
            DWORD state[8];if(!line_file)fail("line-file-checkpoint");readmem(line_file,state,32);hexbytes("line-read-file-state",line_file,32);
            if(state[0]<state[2]||state[0]-state[2]+state[1]>4096)fail("line-read-file-buffer");
            hexbytes("line-read-file-buffer",state[2],state[0]-state[2]+state[1]);
            hexbytes("line-prefix",words[1],wide_string_bytes(words[1]));hexbytes("line-text",words[2],wide_string_bytes(words[2]));
            {DWORD current;readmem(base+0x16ce8,&current,4);fprintf(events,"line-current-number %lu\n",current);}
        }
        if(address==base+0xbc80||address==base+0xbc61||address==base+0x293e||address==base+0x29ef||address==base+0x2b84||address==base+0x44fd||address==base+0x26bb||address==base+0x11b2a||address==base+0x26f8||address==base+0x4c2f||address==base+0x4da5||address==base+0xbee2||address==base+0x3db4||address==base+0x4298||address==base+0xbebd||address==base+0x4240||address==base+0x91a4||address==base+0x41fb||address==base+0x2690||address==base+0xc036||address==base+0x1205d||address==base+0x6345||address==base+0x12a9b||address==base+0x11b44||address==base+0x26d9||address==base+0x478b||address==base+0x30c1||address==base+0x12176||address==base+0x12b67||address==base+0x125aa||address==base+0x126fb||address==base+0x126af||address==base+0x736b||address==base+0x72a2||address==base+0x11f3b||address==base+0x6c71||address==base+0x7341||address==base+0x629a||address==base+0x6b80||address==base+0x12a02||address==base+0x128e9||address==base+0x12a2b||address==base+0x624c){
            DWORD pointer,count,vector,total,length;char label[80];
            if(address==base+0xbc80){count=words[1];vector=words[2];}
            else{count=cpp_count;vector=cpp_vector;}
            if(address==base+0x11f3b||address==base+0x6c71){DWORD frame[9],node,pointer;readmem(base+0x18e80,frame,36);readmem(base+0x16cd0,&node,4);fprintf(events,"string-reader formals=0 token-value=%08lx intern-result=%08lx\n",node,intern_result);hexbytes("string-source-block",frame[5]-8,8206);crt_file_state(frame[1],"cpp-source",1);readmem(base+0x18e74,&pointer,4);crt_file_state(pointer,"cpp-output",0);hexbytes("intern-current-node",intern_node-8,28);hexbytes("intern-current-entry",intern_result-12,32);}
            if(address==base+0x12a02||address==base+0x128e9||address==base+0x12a2b||address==base+0x624c){DWORD depth,frame[9],pointer;readmem(base+0x14a10,&depth,4);if(depth!=1)fail("comment-current-depth");readmem(base+0x18e80+depth*36,frame,36);fprintf(events,"comment-depth %lu\n",depth);hexbytes("comment-frame",base+0x18e80+depth*36,36);hexbytes("comment-parent-frame",base+0x18e80,36);hexbytes("comment-source-block",frame[5]-8,8206);readmem(base+0x149c4,&pointer,4);hexbytes("comment-filename",pointer,514);hexbytes("comment-marker",base+0x16cf8,514);hexbytes("comment-work-buffer",base+0x16f08,4090);crt_file_state(frame[1],"comment-source",1);readmem(base+0x18e74,&pointer,4);crt_file_state(pointer,"cpp-output",0);}
            if(address==base+0x624c){hexbytes("condition-nesting",base+0x149d4,4);hexbytes("condition-state",base+0x227a8,128);hexbytes("condition-word",base+0x18128,1024);}
            if(address==base+0x6b80){DWORD frame[9],pointer,index,list[101];char label[80];readmem(base+0x18e80,frame,36);hexbytes("include-source-block",frame[5]-8,8206);hexbytes("include-macro-current",base+0x18128,1024);hexbytes("include-marker-current",base+0x16cf8,514);hexbytes("include-path-list",base+0x14828,404);readmem(base+0x14828,list,404);for(index=0;index<100;++index)if(list[index+1]){sprintf(label,"include-directory-%lu",index);hexbytes(label,list[index+1],wide_string_bytes(list[index+1]));}readmem(base+0x149bc,&pointer,4);hexbytes("include-path-characters",pointer,wide_string_bytes(pointer));crt_file_state(frame[1],"cpp-source",1);readmem(base+0x18e74,&pointer,4);crt_file_state(pointer,"cpp-output",0);}
            if(address==base+0x629a){DWORD frame[9],pointer;readmem(base+0x18e80,frame,36);hexbytes("angle-source-block",frame[5]-8,8206);hexbytes("angle-macro-before",base+0x18128,1024);hexbytes("angle-marker-before",base+0x16cf8,2);crt_file_state(frame[1],"cpp-source",1);readmem(base+0x18e74,&pointer,4);crt_file_state(pointer,"cpp-output",0);}
            if(address==base+0x7341){DWORD frame[9],pointer;readmem(base+0x18e80,frame,36);hexbytes("type-compare-left",words[1],20);hexbytes("type-compare-right",words[2],20);hexbytes("type-compare-left-allocation",words[1]-8,28);hexbytes("type-compare-right-entry",words[2]-12,32);hexbytes("compare-source-block",frame[5]-8,8206);crt_file_state(frame[1],"cpp-source",1);readmem(base+0x18e74,&pointer,4);crt_file_state(pointer,"cpp-output",0);}
            if(address==base+0x6c71){hexbytes("expect-end-word",words[1],wide_string_bytes(words[1]));hexbytes("string-buffer-current",base+0x16f08,4090);hexbytes("string-extra-current",base+0x16cd4,2);}
            if(address==base+0xc036){DWORD marker[4];readmem(words[1],marker,16);fprintf(events,"source-marker pointer=%08lx parent-bp=%08lx\n",words[1],c.Ebp);hexbytes("source-marker-record",words[1],16);hexbytes("source-marker-name",marker[1],4);hexbytes("source-marker-text",marker[2],2);}
            if(address==base+0x2690){DWORD parent[2];WORD token;readmem(c.Esp+20,parent,8);readmem(base+0x14010+2*(words[1]&0xffff),&token,2);hexbytes("quote-forwarded",c.Esp+20,8);hexbytes("quote-source-input",c.Edi,wide_string_bytes(c.Edi));hexbytes("quote-destination",parent[0],1024);fprintf(events,"quote-position destination=%08lx source=%08lx token=%u\n",c.Esi,c.Edi,token);}
            if(address==base+0x91a4)hexbytes("source-write-payload",words[1],words[2]*words[3]);
            if(address==base+0x12b67||address==base+0x125aa||address==base+0x126fb||address==base+0x126af||address==base+0x736b||address==base+0x72a2){DWORD first,radix,begin,number_bp=c.Ebp,number_cursor=c.Ebx;BYTE token;if(address==base+0x72a2){number_bp=storage_number_bp;number_cursor=storage_number_cursor;}if(address==base+0x126fb){readmem(c.Ebp,&number_bp,4);readmem(c.Ebp-8,&number_cursor,4);}readmem(number_bp+8,&first,4);readmem(number_bp-12,&radix,4);readmem(number_bp-8,&begin,4);readmem(number_bp-4,&token,1);fprintf(events,"number-parser-state first=%u radix=%lu token=%u begin=%08lx cursor=%08lx\n",(WORD)first,radix,token,begin,number_cursor);}
            if(address==base+0x126fb){DWORD previous,radix;readmem(c.Ebp-4,&previous,4);readmem(c.Ebp+12,&radix,4);fprintf(events,"integer-conversion-state cursor=%08lx accumulator=%lu previous=%lu radix=%lu\n",c.Esi,c.Ebx,previous,radix);hexbytes("integer-conversion-input",c.Esi,wide_string_bytes(c.Esi));}
            if(address==base+0x126af||address==base+0x736b||address==base+0x72a2){DWORD value;DWORD number_bp=address==base+0x72a2?storage_number_bp:c.Ebp;readmem(number_bp-20,&value,4);fprintf(events,"number-value value=%lu storage=%08lx initialized=4\n",value,number_bp-20);}
            if(address==base+0x736b)hexbytes("store-integer-input",words[2],4);
            if(address==base+0x72a2){DWORD token,value;readmem(c.Ebp+8,&token,4);readmem(c.Ebp+12,&value,4);fprintf(events,"store-frontier token=%08lx value=%08lx node=%08lx type=%08lx type-code=%08lx number-bp=%08lx\n",token,value,c.Esi,words[1],c.Edi,storage_number_bp);hexbytes("store-integer-input",value,4);hexbytes("integer-node",c.Esi-8,28);hexbytes("integer-type",words[1]-8,28);}

            if(address==base+0x125aa)hexbytes("integer-conversion-input",words[1],wide_string_bytes(words[1]));
            if(address==base+0x293e)hexbytes("option-argument",words[2],wide_string_bytes(words[2]));
            if(address==base+0x29ef){hexbytes("option-operand",words[2],wide_string_bytes(words[2]));hexbytes("option-descriptor",words[1],12);}
            if(address==base+0x2b84){
                DWORD parent[3];readmem(c.Ebp+8,parent,12);
                fprintf(events,"number-parent %08lx %08lx %lu\n",parent[0],parent[1],parent[2]);
                hexbytes("number-operand",words[1],wide_string_bytes(words[1]));
            }
            if(address==base+0x26bb||address==base+0x26f8)hexbytes("setup-forwarded",c.Esp+36,16);
            if(address==base+0x3db4||address==base+0x4298||address==base+0xbebd||address==base+0x4240||address==base+0x91a4||address==base+0x41fb||address==base+0x2690||address==base+0xc036||address==base+0x1205d||address==base+0x6345||address==base+0x12a9b||address==base+0x11b44||address==base+0x26d9||address==base+0x478b||address==base+0x30c1||address==base+0x12176||address==base+0x12b67||address==base+0x125aa||address==base+0x126fb||address==base+0x126af||address==base+0x736b||address==base+0x72a2){
                DWORD bucket,record,fields[4],count=0,pointer;char label[80];
                if(address==base+0x3db4){hexbytes("setup-forwarded",c.Esp+36,16);hexbytes("source-open-input",words[1],wide_string_bytes(words[1]));}
                else{DWORD parent[2],source_bp=c.Ebp;if(address==base+0x72a2){readmem(storage_number_bp+60,&source_bp,4);}if(address==base+0x26d9)readmem(c.Esp+16,&source_bp,4);if(address==base+0x30c1)readmem(c.Esp+12,&source_bp,4);if(address==base+0x12176)readmem(c.Esp+56,&source_bp,4);if(address==base+0x12b67||address==base+0x125aa||address==base+0x126af||address==base+0x736b)readmem(c.Ebp+60,&source_bp,4);if(address==base+0x126fb){DWORD number_bp;readmem(c.Ebp,&number_bp,4);readmem(number_bp+60,&source_bp,4);}fprintf(events,"source-owner-bp %08lx\n",source_bp);readmem(source_bp+8,parent,8);hexbytes("setup-forwarded",source_bp+40,16);hexbytes("source-open-forwarded",source_bp+8,8);hexbytes("source-open-input",parent[0],wide_string_bytes(parent[0]));hexbytes("source-new-buffer",parent[1]-8,0x200e);readmem(base+0x21e88,&pointer,4);crt_file_state(pointer,"cpp-source",1);}
                readmem(base+0x18e74,&pointer,4);if(pointer)crt_file_state(pointer,"cpp-output",0);else fprintf(events,"cpp-output-pointer 00000000\n");
                for(bucket=0;bucket<256;++bucket){
                    readmem(base+0x21ea0+bucket*4,&record,4);
                    while(record){
                        if(++count>3)fail("macro-record-scope");readmem(record,fields,16);
                        fprintf(events,"macro-node %lu bucket=%lu pointer=%08lx next=%08lx name=%08lx text=%08lx kind=%08lx\n",count-1,bucket,record,fields[0],fields[1],fields[2],fields[3]);
                        sprintf(label,"macro-node-record-%lu",count-1);hexbytes(label,record-8,24);
                        sprintf(label,"macro-node-name-%lu",count-1);hexbytes(label,fields[1]-8,wide_string_bytes(fields[1])+10);
                        sprintf(label,"macro-node-text-%lu",count-1);hexbytes(label,fields[2]-8,16);
                        record=fields[0];
                    }
                }
                fprintf(events,"macro-nodes %lu\n",count);
            }
            if(address==base+0xbee2){fprintf(events,"macro-record %08lx\n",c.Esi);hexbytes("pending-macro-block",c.Esi-8,24);hexbytes("macro-copy-value",words[1],wide_string_bytes(words[1]));}
            if(address==base+0x4da5||address==base+0xbee2){DWORD parent[3];readmem(c.Ebp+8,parent,12);hexbytes("definition-forwarded",c.Ebp+8,12);hexbytes("setup-forwarded",c.Ebp+44,16);if(parent[0])hexbytes("macro-definition-value",parent[0],wide_string_bytes(parent[0]));}
            if(address==base+0x4c2f){hexbytes("setup-forwarded",c.Esp+40,16);if(words[1])hexbytes("macro-definition-value",words[1],wide_string_bytes(words[1]));}
            if(address==base+0x11b2a){hexbytes("setup-forwarded",c.Esp+32,16);hexbytes("macro-hash-name",words[1],wide_string_bytes(words[1]));}
            if(count!=20||!vector)fail("wide-preprocessor-scope");total=4*(count+1);
            fprintf(events,"wide-args %lu %08lx\n",count,vector);
            for(n=0;n<count;++n){readmem(vector+4*n,&pointer,4);length=wide_string_bytes(pointer);total+=length;sprintf(label,"wide-argv-%u",n);hexbytes(label,pointer,length);fprintf(events,"wide-pointer %u %08lx\n",n,pointer);}
            readmem(vector+4*count,&pointer,4);fprintf(events,"wide-terminator %08lx\n",pointer);
            if(total>1048576)fail("wide-preprocessor-buffer-size");if(cpp_allocation_size<total+8||cpp_allocation_size>total+10)fail("wide-allocation-observation");hexbytes("wide-argument-block",vector-8,cpp_allocation_size);
            hexbytes("prepared-closed-input",line_file,32);readmem(base+0x149c4,&pointer,4);hexbytes("prepared-filename",pointer,514);
        }
        if(address==base+0x10913){
            DWORD pointer;char label[80];
            if(words[1]!=20||words[2]!=base+0x25590)fail("widen-call-scope");
            fprintf(events,"widen-args %lu %08lx\n",words[1],words[2]);
            for(n=0;n<words[1];++n){readmem(words[2]+4*n,&pointer,4);sprintf(label,"widen-argv-%u",n);hexbytes(label,pointer,string_bytes(pointer));}
            hexbytes("prepared-closed-input",line_file,32);readmem(base+0x149c4,&pointer,4);hexbytes("prepared-filename",pointer,514);
        }
        if(address==base+0x9041){
            hexbytes("line-input-state",words[4],32);
            fprintf(events,"line-input-args capacity=%lu wide=%lu\n",words[2],words[3]);
        }
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
            if(address==base+0x856c)hexbytes("prepare-path",words[1],string_bytes(words[1]));
            else{
                DWORD total,current,pointer;
                if(words[1]!=count||words[2]!=base+0x25590||words[3]!=0)fail("preprocess-call-scope");
                fprintf(events,"preprocessor-args %lu %08lx %lu\n",words[1],words[2],words[3]);
                readmem(base+0x29458,&total,4);readmem(base+0x16ce8,&current,4);fprintf(events,"prepared-lines %lu current=%lu\n",total,current);
                if(!line_file)fail("prepared-file-checkpoint");hexbytes("prepared-closed-input",line_file,32);
                readmem(base+0x149c4,&pointer,4);hexbytes("prepared-filename",pointer,514);
            }
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
        if(GetEnvironmentVariableA("RCDLL_CAPTURE_PATH_ERRORS",value,sizeof(value)))path_errors=atoi(value);
        if(heap_limit>8192||free_limit>8192||character_limit>8192)return 125;
        SetEnvironmentVariableA("RCDLL_CAPTURE_HEAP_LIMIT",NULL);SetEnvironmentVariableA("RCDLL_CAPTURE_FREE_LIMIT",NULL);SetEnvironmentVariableA("RCDLL_CAPTURE_CHARACTER_LIMIT",NULL);
        SetEnvironmentVariableA("RCDLL_CAPTURE_PATH_ERRORS",NULL);
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
