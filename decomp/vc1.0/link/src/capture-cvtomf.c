/* Explicit original-only external cvtomf caller. This transport establishes
 * the adapter's real public setjmp contract; it never seeds a converter mode. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;
static FILE *log_file;
static DWORD teb,map_call_active,map_index,pointer_call_active,record_slot,type_map_active,auxiliary_call_active,coff_type_slot,auxiliary_slot,find_fixup_active,symbols_call_active,finish_call_active,debug_inputs_released;
static const char *log_path;
typedef struct CaptureTracePoint {DWORD address;char name[64];unsigned char saved,seen;} CaptureTracePoint;
static CaptureTracePoint trace_points[32];
static DWORD trace_count;
static int trace_index(DWORD address)
{
    DWORD i;for(i=0;i<trace_count;++i)if(trace_points[i].address==address&&!trace_points[i].seen)return i+1;
    return 0;
}
static int read_memory(DWORD a,void *b,DWORD n)
{ DWORD got;return ReadProcessMemory(process,(void *)a,b,n,&got)&&got==n; }
static int write_memory(DWORD a,const void *b,DWORD n)
{ DWORD got;return WriteProcessMemory(process,(void *)a,(void *)b,n,&got)&&got==n; }
static int code(DWORD a,unsigned char byte)
{
    DWORD old,again;
    return VirtualProtectEx(process,(void *)a,1,PAGE_EXECUTE_READWRITE,&old)&&
        write_memory(a,&byte,1)&&FlushInstructionCache(process,(void *)a,1)&&
        VirtualProtectEx(process,(void *)a,1,old,&again);
}
static void hex(const unsigned char *p,DWORD n)
{ DWORD i;fputc('"',log_file);for(i=0;i<n;++i)fprintf(log_file,"%02x",p[i]);fputc('"',log_file); }
static void words(const DWORD *p,DWORD n)
{ DWORD i;fputc('[',log_file);for(i=0;i<n;++i)fprintf(log_file,"%s%lu",i?",":"",p[i]);fputc(']',log_file); }
static int buffer_state(const char *key,const char *event,DWORD pointer_address,DWORD size_address)
{
    DWORD pointer,size,offset,used;unsigned char bytes[4096];char path[1024];FILE *file;
    if(!read_memory(pointer_address,&pointer,4)||!read_memory(size_address,&size,4)||size>0x100000)return 0;
    fprintf(log_file,",\"%s\":{\"pointer\":%lu,\"size\":%lu}",key,pointer,size);
    if(!pointer)return 1;
    sprintf(path,"%s.%s.%s.bin",log_path,event,key);file=fopen(path,"wb");if(!file)return 0;
    for(offset=0;offset<size;offset+=used){used=size-offset;if(used>4096)used=4096;if(!read_memory(pointer+offset,bytes,used)||fwrite(bytes,1,used,file)!=used)return 0;}
    return fclose(file)==0;
}
static int converter_state(int symbols_released)
{
    DWORD pointer,count,input,stream[8],offset,defined,groups,records,type_map,type_count=0,type_bytes;unsigned short remaining,segment;unsigned char checksum,bytes[4096];
    if(!read_memory(0x5c5dc,&pointer,4)||!read_memory(0x807c8,&count,4)||count>512||!read_memory(0x6f19c,&remaining,2)||!read_memory(0x6f1a0,&checksum,1)||!read_memory(0x77dc8,&offset,4)||!read_memory(0x77dc4,&segment,2)||!read_memory(0x77e0c,&input,4))return 0;
    fprintf(log_file,",\"record_remaining\":%u,\"record_checksum\":%u,\"data_offset\":%lu,\"data_segment\":%u,\"debug_fixups\":{\"pointer\":%lu,\"count\":%lu,\"records\":",remaining,checksum,offset,segment,pointer,count);
    if(count&&!symbols_released&&!read_memory(pointer,bytes,count*8))return 0;hex(bytes,symbols_released?0:count*8);fprintf(log_file,",\"records_live\":%s},\"input_file\":",symbols_released?"false":"true");
    if(input){if(!read_memory(input,stream,32)||stream[1]>4096)return 0;words(stream,8);fprintf(log_file,",\"input_unread\":");if(stream[1]&&!read_memory(stream[0],bytes,stream[1]))return 0;hex(bytes,stream[1]);}else fprintf(log_file,"null,\"input_unread\":\"\"");
    defined=groups=0;
    if(!read_memory(0x65210,&defined,2)||!read_memory(0x735b4,&groups,2)||defined>=256||groups>=256)return 0;
    fprintf(log_file,",\"defined_segments\":%lu,\"groups\":%lu,\"segments\":",defined,groups);
    if(!read_memory(0x65214,bytes,(defined+1)*16))return 0;hex(bytes,(defined+1)*16);
    fprintf(log_file,",\"segment_indices\":");if(!read_memory(0x735c0,bytes,(defined+1)*4))return 0;hex(bytes,(defined+1)*4);
    fprintf(log_file,",\"group_indices\":");if(!read_memory(0x739c0,bytes,(groups+1)*4))return 0;hex(bytes,(groups+1)*4);
    fprintf(log_file,",\"thread_symbols\":");if(!read_memory(0x77df8,bytes,16))return 0;hex(bytes,16);
    if(!read_memory(0x77ecc,&records,4)||!read_memory(0x77ec0,&type_map,4)||!read_memory(0x77ed0,&type_count,2)||!read_memory(0x77ec8,&type_bytes,4)||type_count>256)return 0;
    fprintf(log_file,",\"type_directory\":{\"records_pointer\":%lu,\"map_pointer\":%lu,\"count\":%lu,\"records\":",records,type_map,type_count);
    if(type_bytes&&type_count&&!read_memory(records,bytes,type_count*4))return 0;words((DWORD *)bytes,type_bytes?type_count:0);fprintf(log_file,",\"records_live\":%s}",type_bytes&&records?"true":"false");
    if(!read_memory(0x5c5cc,&count,4)||!read_memory(0x80f90,&pointer,4)||count>200)return 0;
    fprintf(log_file,",\"coff_symbol_count\":%lu,\"coff_symbols\":",count);
    if(count&&!read_memory(pointer,bytes,count*18))return 0;hex(bytes,count*18);
    return 1;
}
static int line_lifetime_state(const char *event,CONTEXT *c)
{
    DWORD pointer,count,lines,argument,target,displacement;
    unsigned char bytes[72];unsigned i;DWORD offset;unsigned short line;
    if(!read_memory(0x5c5d8,&pointer,4)||!read_memory(0x5c5d4,&count,4)||count!=9||!read_memory(pointer,bytes,72)||!read_memory(0x77e28,&lines,4)||!read_memory(c->Esp,&argument,4)||!read_memory(0x49b96,&displacement,4))return 0;
    target=0x49b9a+displacement;
    fprintf(log_file,"{\"event\":\"%s\",\"eip\":%lu,\"esp\":%lu,\"pointer\":%lu,\"count\":%lu,\"coff_line_count\":%lu,\"stack_word0\":%lu,\"free_target\":%lu,\"raw\":",event,c->Eip,c->Esp,pointer,count,lines,argument,target);hex(bytes,72);fprintf(log_file,",\"rows\":[");
    for(i=0;i<count;++i){memcpy(&offset,bytes+i*8,4);memcpy(&line,bytes+i*8+4,2);fprintf(log_file,"%s[%lu,%u]",i?",":"",offset,line);}fprintf(log_file,"]}\n");fflush(log_file);return target==0x50fa0;
}
static int snapshot(const char *event,CONTEXT *c,DWORD arguments)
{
    DWORD stack[8],jump[8],err[3],files[16],used,i,j,head,registration[4],ctype,mb,symbol_pointer,symbol_capacity;
    DWORD addresses[]={0x651f0,0x651f4,0x651f8,0x651fc,0x80e5c,0x80e6c},globals[6];
    unsigned char pending[4096],text[514];char path[1024];FILE *buffer_file;
    if(!read_memory(c->Esp,stack,32)||!read_memory(0x5c43c,jump,32)||
       !read_memory(0x852d8,err,8)||!read_memory(teb+0x34,err+2,4)||
       !read_memory(0x85070,files,64)||!read_memory(teb,&head,4)||
       !read_memory(head,registration,16)||!read_memory(0x85340,&ctype,4)||
       !read_memory(0x8554c,&mb,4))return 0;
    for(i=0;i<6;++i)if(!read_memory(addresses[i],globals+i,4))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"eip\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"stack\":",event,c->Eip,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);
    words(stack,8);fprintf(log_file,",\"jump\":");words(jump,8);
    fprintf(log_file,",\"fs_head\":%lu,\"registration\":",head);words(registration,4);
    fprintf(log_file,",\"parser_globals\":");words(globals,6);
    fprintf(log_file,",\"errors\":");words(err,3);
    fprintf(log_file,",\"files\":[");
    for(i=0;i<2;++i){if(i)fputc(',',log_file);words(files+i*8,8);}
    fprintf(log_file,"],\"pending\":[");
    for(i=0;i<2;++i){if(i)fputc(',',log_file);used=files[i*8]-files[i*8+2];if(used>4096||(used&&!read_memory(files[i*8+2],pending,used)))return 0;hex(pending,used);}
    fprintf(log_file,"],\"argv\":[");
    for(i=0;i<6;++i){if(!read_memory(arguments+i*4,&j,4))return 0;if(i==5){if(j)return 0;break;}if(i)fputc(',',log_file);used=0;do{if(used==512||!read_memory(j+used,text+used,1))return 0;}while(text[used++]);hex(text,used);}
    if(!read_memory(ctype-2,text,514))return 0;
    fprintf(log_file,"],\"ctype\":");hex(text,514);fprintf(log_file,",\"mb_cur_max\":%lu",mb);
    if(c->Eip==0x49e60||c->Eip==0x4aba0)fprintf(log_file,",\"formals\":{\"type\":%lu}",stack[1]);
    if(c->Eip==0x47b90){find_fixup_active=1;fprintf(log_file,",\"formals\":{\"offset\":%lu}",stack[1]);}
    if(find_fixup_active&&!strcmp(event,"return")){
        DWORD base;if(!read_memory(0x5c5dc,&base,4))return 0;
        if(c->Eax){if(!read_memory(c->Eax,pending,8))return 0;fprintf(log_file,",\"returned_offset\":%lu,\"returned_row\":",c->Eax-base);hex(pending,8);}
        else fprintf(log_file,",\"returned_offset\":null,\"returned_row\":null");find_fixup_active=0;
    }
    if(c->Eip==0x49d00)fprintf(log_file,",\"formals\":{\"number\":%lu}",stack[1]);
    if(c->Eip==0x49300){
        DWORD row[2];
        fprintf(log_file,",\"formals\":{\"left\":%lu,\"right\":%lu},\"rows\":[",stack[1],stack[2]);
        for(i=1;i<3;++i){if(!read_memory(stack[i],row,8))return 0;fprintf(log_file,"%s{\"offset\":%lu,\"external\":%u,\"section\":%d}",i==1?"":",",row[0],(unsigned short)row[1],(short)(row[1]>>16));}fputc(']',log_file);
    }
    if(c->Eip==0x49310){
        DWORD count,pointer,node[6],bucket,line[2];unsigned short section;
        symbols_call_active=1;
        if(!read_memory(0x5c5d4,&count,4)||!read_memory(0x5c5d8,&pointer,4)||count>512||!read_memory(0x5c5d0,&section,2))return 0;
        if(!read_memory(pointer,pending,8))return 0;fprintf(log_file,",\"line_first_allocated_row\":");hex(pending,8);
        fprintf(log_file,",\"formals\":[],\"line_count\":%lu,\"text_section\":%u,\"lines\":[",count,section);
        for(i=0;i<count;++i){if(!read_memory(pointer+i*8,line,8))return 0;fprintf(log_file,"%s[%lu,%u]",i?",":"",line[0],(unsigned short)line[1]);}fputc(']',log_file);
        if(!read_memory(0x77e38,pending,128))return 0;fprintf(log_file,",\"debug_record\":");hex(pending,128);
        fprintf(log_file,",\"symbol_indices\":\"");for(i=0;i<16384;i+=sizeof(pending)){if(!read_memory(0x5c5e0+i,pending,sizeof(pending)))return 0;for(j=0;j<sizeof(pending);++j)fprintf(log_file,"%02x",pending[j]);}fputc('"',log_file);
        fprintf(log_file,",\"symbol_nodes\":[");used=0;
        if(!read_memory(0x6d218,&pointer,4))return 0;
        for(bucket=0;bucket<211;++bucket){
            if(!read_memory(pointer+bucket*4,&count,4))return 0;
            while(count){if(++used>128||!read_memory(count,node,24))return 0;if(used>1)fputc(',',log_file);
                fprintf(log_file,"{\"bucket\":%lu,\"value\":%lu,\"kind\":%u,\"section\":%d,\"ordinal\":%u,\"type\":%u,\"weak_default\":%u,\"name\":",bucket,node[1],(unsigned short)node[2],(short)(node[2]>>16),(unsigned short)node[3],(unsigned short)(node[3]>>16),(unsigned short)node[4]);
                j=0;do{if(j==260||!read_memory(node[0]+j,pending+j,1))return 0;}while(pending[j++]);hex(pending,j);fputc('}',log_file);count=node[5];
            }
        }fputc(']',log_file);
    }
    if(c->Eip==0x4aab0){auxiliary_call_active=1;coff_type_slot=stack[2];auxiliary_slot=stack[3];
        fprintf(log_file,",\"formals\":{\"type\":%u,\"coff_type\":%lu,\"auxiliary\":%lu}",(unsigned short)stack[1],stack[2],stack[3]);}
    if(auxiliary_call_active&&!strcmp(event,"return")){
        unsigned short result;if(!read_memory(coff_type_slot,&result,2)||!read_memory(auxiliary_slot,pending,18))return 0;
        fprintf(log_file,",\"coff_type\":%u,\"auxiliary\":",result);hex(pending,18);auxiliary_call_active=0;
    }
    if(c->Eip==0x4ad40)finish_call_active=1;
    if(c->Eip==0x4ad40||c->Eip==0x492c0||(symbols_call_active&&!strcmp(event,"return")))debug_inputs_released=1;
    if(finish_call_active){DWORD counter;if(!read_memory(0x8111c,&counter,4))return 0;fprintf(log_file,",\"anonymous_counter\":%lu",counter);}
    if(c->Eip==0x4ada0||c->Eip==0x4aab0||c->Eip==0x4ad40)type_map_active=1;
    if(type_map_active){
        DWORD map,count=0,head,slot[3];unsigned short kind;
        if(!read_memory(0x77ee4,&head,4)||!read_memory(0x77ec0,&map,4)||!read_memory(0x77ed0,&count,2)||count>256)return 0;
        fprintf(log_file,",\"deferred_head\":%lu,\"type_map\":[",head);
        for(i=0;i<count;++i){if(i)fputc(',',log_file);if(!read_memory(map+i*12,slot,12))return 0;kind=(unsigned short)slot[0];
            fprintf(log_file,"{\"type\":%u,\"reserved\":%u,\"size\":%lu,\"value\":%lu",kind,(unsigned short)(slot[0]>>16),slot[1],slot[2]);
            if((kind&0x30)==0x30){if(!read_memory(slot[2],pending,18))return 0;fprintf(log_file,",\"array\":");hex(pending,18);}fputc('}',log_file);
        }fputc(']',log_file);if(!strcmp(event,"return")||c->Eip==0x4ad40)type_map_active=0;
    }
    if(c->Eip==0x4ac00)fprintf(log_file,",\"formals\":{\"types\":%lu,\"names\":%lu}",stack[1],stack[2]);
    if(c->Eip==0x49db0||c->Eip==0x49e40){
        DWORD cursor;pointer_call_active=1;record_slot=stack[1];if(!read_memory(record_slot,&cursor,4))return 0;
        fprintf(log_file,",\"formals\":{\"record_pointer\":%lu},\"cursor\":%lu",stack[1],cursor);
    }
    if(pointer_call_active&&!strcmp(event,"return")){
        DWORD cursor;if(!read_memory(record_slot,&cursor,4))return 0;fprintf(log_file,",\"cursor\":%lu",cursor);
    }
    if(c->Eip==0x49f00){
        map_call_active=1;map_index=stack[4];
        if(!read_memory(stack[3],pending,12))return 0;
        fprintf(log_file,",\"formals\":{\"kind\":%lu,\"record\":%lu,\"type\":%lu,\"index\":%lu},\"type_before\":",stack[1],stack[2],stack[3],stack[4]);hex(pending,12);
    }
    if(c->Eip==0x4a4c0){map_call_active=1;map_index=stack[1];fprintf(log_file,",\"formals\":{\"index\":%lu,\"enclosing\":%lu}",stack[1],stack[2]);}
    if(map_call_active&&!strcmp(event,"return")){
        DWORD map;if(!read_memory(0x77ec0,&map,4)||!read_memory(map+map_index*12,pending,12))return 0;
        fprintf(log_file,",\"map_slot_index\":%lu,\"map_slot\":",map_index);hex(pending,12);
    }
    if(c->Eip==0x49c20||c->Eip==0x4acb0){
        if(stack[3]>4096||!read_memory(stack[2],pending,stack[3]))return 0;
        fprintf(log_file,",\"formals\":{\"offset\":%lu,\"data\":%lu,\"length\":%lu},\"payload\":",stack[1],stack[2],stack[3]);hex(pending,stack[3]);
    }
    if(!read_memory(0x77eb8,&symbol_pointer,4)||!read_memory(0x77e30,&symbol_capacity,4))return 0;
    fprintf(log_file,",\"symbol_buffer\":{\"pointer\":%lu,\"capacity\":%lu",symbol_pointer,symbol_capacity);
    if(symbol_pointer){
        if(symbol_capacity>0x100000)return 0;
        sprintf(path,"%s.%s.symbols.bin",log_path,event);buffer_file=fopen(path,"wb");if(!buffer_file)return 0;
        for(i=0;i<symbol_capacity;i+=used){used=symbol_capacity-i;if(used>4096)used=4096;if(!read_memory(symbol_pointer+i,pending,used)||fwrite(pending,1,used,buffer_file)!=used)return 0;}
        if(fclose(buffer_file))return 0;
    }
    fprintf(log_file,"}");if(strcmp(event,"omf_main_return") && (!converter_state(debug_inputs_released)||!buffer_state("type_buffer",event,0x77ec4,0x77ec8)))return 0;fprintf(log_file,"}\n");fflush(log_file);return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES security;STARTUPINFOA startup;PROCESS_INFORMATION launch;
    DEBUG_EVENT ev;CONTEXT c,main_entry,before,watched;AllocateRemote allocate_remote;
    DWORD pid,site,status,base=0,frame=0,arguments=0,main_return=0,failed=0,phase=0,exit_code=999;
    DWORD values[6],at,i,trace_at,result=999,held=0,text_rva=0,text_size=0,watch=0,watch_return=0;unsigned char saved_main,saved_longjmp,watch_byte,watch_return_byte,line_before_byte,line_after_byte,*mask=NULL;FILE *mask_file;
    HANDLE output,error;IMAGE_AUX_SYMBOL auxiliary_layout;char command[1024];const char *names[]={"cvtomf","-g","-o","converted.obj","legacy.obj"};
    if(argc<6||argc>9 || sizeof(auxiliary_layout)!=18 || (char *)&auxiliary_layout.Sym.FcnAry.Function.PointerToNextFunction-(char *)&auxiliary_layout!=12)return 2;
    if(argc>=8)watch=strtoul(argv[7],NULL,0);
    if(argc==9){
        mask_file=fopen(argv[8],"rb");if(!mask_file||fread(&trace_count,4,1,mask_file)!=1||trace_count>32||!trace_count||watch)return 2;
        for(i=0;i<trace_count;++i)if(fread(&trace_points[i].address,4,1,mask_file)!=1||fread(trace_points[i].name,1,64,mask_file)!=64||!memchr(trace_points[i].name,0,64))return 2;
        if(fgetc(mask_file)!=EOF||fclose(mask_file))return 2;
    }
    if(argc>=7){
        mask_file=fopen(argv[6],"rb");if(!mask_file||fread(&text_rva,4,1,mask_file)!=1||fread(&text_size,4,1,mask_file)!=1)return 2;
        mask=malloc(text_size);if(!mask||fread(mask,1,text_size,mask_file)!=text_size||fgetc(mask_file)!=EOF||fclose(mask_file))return 2;
    }
    log_path=argv[4];log_file=fopen(argv[4],"wt");if(!log_file)return 3;
    memset(&security,0,sizeof(security));security.nLength=sizeof(security);security.bInheritHandle=TRUE;
    output=CreateFileA(argv[2],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);
    if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[5],&startup,&launch))return 4;
    pid=launch.dwProcessId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);
    allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    for(;;){
        if(!WaitForDebugEvent(&ev,15000)){failed=5;break;}status=DBG_CONTINUE;
        if(ev.dwProcessId!=pid){failed=6;break;}
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){
            process=ev.u.CreateProcessInfo.hProcess;thread=ev.u.CreateProcessInfo.hThread;teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;
            if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x10000||!read_memory(0x26b40,&saved_main,1)||!read_memory(0x52777,&saved_longjmp,1)||!code(0x26b40,0xcc)){failed=7;break;}
            if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        }else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        }else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;
            memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;if(!GetThreadContext(thread,&c)){failed=9;break;}
            if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x26b40&&phase==0){
                main_entry=c;main_entry.Eip=site;
                if(mask){
                    DWORD old,again;
                    if(!VirtualProtectEx(process,(void *)(0x10000+text_rva),text_size,PAGE_EXECUTE_READWRITE,&old)||!write_memory(0x10000+text_rva,mask,text_size)||!FlushInstructionCache(process,(void *)(0x10000+text_rva),text_size)||!VirtualProtectEx(process,(void *)(0x10000+text_rva),text_size,old,&again)){failed=23;break;}
                }
                base=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
                if(!base||!read_memory(c.Esp,&main_return,4)){failed=10;break;}
                if(watch&&(!read_memory(watch,&watch_byte,1)||watch_byte==0xcc||!code(watch,0xcc))){failed=25;break;}
                if(watch==0x492c0&&(!read_memory(0x49b95,&line_before_byte,1)||!read_memory(0x49b9a,&line_after_byte,1)||line_before_byte!=0xe8||line_after_byte!=0x83||!code(0x49b95,0xcc)||!code(0x49b9a,0xcc))){failed=28;break;}
                for(i=0;i<trace_count;++i)if(!read_memory(trace_points[i].address,&trace_points[i].saved,1)||trace_points[i].saved==0xcc||!code(trace_points[i].address,0xcc)){failed=32;break;}if(failed)break;
                /* Owned writable stack, argv and strings; return sentinel lies in
                 * a separately allocated page, containing only debugger INT3. */
                frame=base+0xf000;arguments=base+0x1000;at=base+0x1100;
                for(i=0;i<5;++i){values[i]=at;if(!write_memory(at,names[i],strlen(names[i])+1)){failed=11;break;}at+=strlen(names[i])+1;}if(failed)break;values[5]=0;
                if(!write_memory(arguments,values,24)){failed=12;break;}
                values[0]=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);values[1]=0x5c43c;
                if(!values[0]||!code(values[0],0xcc)||!write_memory(frame,values,8)||!code(0x52777,0xcc)){failed=13;break;}
                result=values[0];c.Eip=0x52738;c.Esp=frame;phase=1;
                if(!SetThreadContext(thread,&c)){failed=14;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==result&&phase==1){
                c.Eip=site;if(c.Eax||!snapshot("setjmp_return",&c,arguments)){failed=15;break;}
                values[0]=result;values[1]=5;values[2]=arguments;
                if(!write_memory(frame-0x100,values,12)){failed=16;break;}
                c.Eip=0x48010;c.Esp=frame-0x100;before=c;phase=2;
                if(!snapshot("omf_main_entry",&c,arguments)||!SetThreadContext(thread,&c)){failed=17;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==result&&phase==2){
                c.Eip=site;
                if(c.Esp!=before.Esp+4||c.Ebx!=before.Ebx||c.Esi!=before.Esi||c.Edi!=before.Edi||c.Ebp!=before.Ebp||!snapshot("omf_main_return",&c,arguments)){failed=18;break;}
                phase=3;result=c.Eax;c.Eip=main_return;c.Esp=main_entry.Esp+4;
                if(!code(0x52777,saved_longjmp)||!SetThreadContext(thread,&c)){failed=19;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&phase==2&&(trace_at=trace_index(site))!=0){
                CaptureTracePoint *point=&trace_points[trace_at-1];
                c.Eip=site;point->seen=1;
                if(!read_memory(c.Esp,values,24)){failed=33;break;}
                fprintf(log_file,"{\"event\":\"first_entry\",\"name\":\"%s\",\"eip\":%lu,\"stack\":",point->name,site);words(values,6);fprintf(log_file,"}\n");fflush(log_file);
                if((site==0x49310&&!snapshot("symbols_trace_entry",&c,arguments))||!code(site,point->saved)||!SetThreadContext(thread,&c)){failed=34;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&watch==0x492c0&&phase==2&&(site==0x49b95||site==0x49b9a)){
                c.Eip=site;
                if(!line_lifetime_state(site==0x49b95?"line_release_entry":"line_release_return",&c)||!code(site,site==0x49b95?line_before_byte:line_after_byte)||!SetThreadContext(thread,&c)){failed=29;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==watch&&phase==2){
                c.Eip=site;watched=c;
                if(watch==0x492c0&&!line_lifetime_state("line_reader_entry",&c)){failed=30;break;}
                if(!snapshot("entry",&c,arguments)||!read_memory(c.Esp,&watch_return,4)||!read_memory(watch_return,&watch_return_byte,1)||!code(watch,watch_byte)||!code(watch_return,0xcc)||!SetThreadContext(thread,&c)){failed=26;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==watch_return&&phase==2){
                c.Eip=site;
                if(c.Esp!=watched.Esp+4||c.Ebx!=watched.Ebx||c.Esi!=watched.Esi||c.Edi!=watched.Edi||c.Ebp!=watched.Ebp||!snapshot("return",&c,arguments)||!code(watch_return,watch_return_byte)||!SetThreadContext(thread,&c)){failed=27;break;}
                if(watch==0x492c0&&!line_lifetime_state("line_reader_return",&c)){failed=31;break;}
                watch_return=0;
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x52777&&phase==2){
                c.Eip=site;if(!snapshot("held_longjmp",&c,arguments)){failed=20;break;}held=1;TerminateProcess(process,17);
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&phase==2&&mask){
                c.Eip=site;if(!snapshot("frontier",&c,arguments)){failed=24;break;}held=2;TerminateProcess(process,17);
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&phase==0){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"va\":%lu,\"code\":%lu,\"phase\":%lu}\n",site,ev.u.Exception.ExceptionRecord.ExceptionCode,phase);failed=21;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=22;break;}
    }
    if(failed&&process)TerminateProcess(process,124);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"held\":%lu,\"phase\":%lu,\"exit\":%lu,\"result\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu}\n",failed,held,phase,exit_code,result,GetFileSize(output,NULL),GetFileSize(error,NULL));fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
