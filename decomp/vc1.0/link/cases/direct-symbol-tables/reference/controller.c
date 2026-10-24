/* Explicit original-only external caller of the custom LINK symbol table.
 * Setup/iteration use genuine accepted APIs. No driver operation is a vertex. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "symbols.h"
#include <recon.h>
typedef char SymbolTable40[sizeof(SymbolTable)==40?1:-1];
typedef char HashTable44[sizeof(HashTable)==44?1:-1];
typedef char External56[sizeof(External)==56?1:-1];
typedef char ExternalFlags36[offsetof(External,flags)==36?1:-1];
typedef void *(WINAPI *AllocateRemote)(HANDLE,void *,DWORD,DWORD,DWORD);
static HANDLE process,thread;
static FILE *log_file,*report;
static DWORD teb,memory,stack_top,sentinel,main_return;
static DWORD phase,index,current_external,tables[4],target,count,args[5];
static DWORD report_count,report_counts[4],enum_table;
static unsigned char payload[4096],entry_byte;
static CONTEXT before,initial;
static int active;
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
static void words(DWORD *data,DWORD size)
{ DWORD i;fputc('[',log_file);for(i=0;i<size;++i)fprintf(log_file,"%s%lu",i?",":"",data[i]);fputc(']',log_file); }
static void bytes(unsigned char *data,DWORD size)
{ DWORD i;fputc('"',log_file);for(i=0;i<size;++i)fprintf(log_file,"%02x",data[i]);fputc('"',log_file); }
static int name_of(SymbolTable *table,External *external,char *name)
{
    DWORD i,n;
    if(external->symbol.N.Name.Short){memcpy(name,&external->symbol,8);name[8]=0;return 1;}
    if(external->symbol.N.Name.Long<4 || external->symbol.N.Name.Long>=table->strings.length)return 0;
    n=table->strings.length-external->symbol.N.Name.Long;if(n>64)n=64;
    if(!read_memory((DWORD)table->strings.data+external->symbol.N.Name.Long,name,n))return 0;
    for(i=0;i<n;++i)if(!name[i])return 1;
    return 0;
}
static int table_state(DWORD address)
{
    SymbolTable table;HashTable hash;External external;HashNode node;
    DWORD bucket,segment,buckets,entry,heads[4096],n,seen=0;unsigned char text[4096];char name[64];
    if(!address){fputs("null",log_file);return 1;}
    if(!read_memory(address,&table,sizeof(table))||!read_memory((DWORD)table.hash,&hash,sizeof(hash))||hash.bucket_count>4096||hash.element_count>64||table.strings.length>4096)return 0;
    fprintf(log_file,"{\"address\":%lu,\"header\":",address);bytes((unsigned char *)&table,sizeof(table));fprintf(log_file,",\"hash_header\":");bytes((unsigned char *)&hash,sizeof(hash));
    fprintf(log_file,",\"strings\":");if(table.strings.length&&!read_memory((DWORD)table.strings.data,text,table.strings.length))return 0;bytes(text,table.strings.length);
    fprintf(log_file,",\"entries\":[");
    for(bucket=0;bucket<hash.bucket_count;++bucket){
        if(!(bucket%hash.buckets_per_segment)){
            n=hash.bucket_count-bucket;if(n>hash.buckets_per_segment)n=hash.buckets_per_segment;
            if(n>4096||!read_memory((DWORD)hash.directory+4*(bucket/hash.buckets_per_segment),&segment,4)||!read_memory(segment,&buckets,4)||!read_memory(buckets,heads,n*4))return 0;
        }
        entry=heads[bucket%hash.buckets_per_segment];
        while(entry){
            if(++seen>64||!read_memory(entry,&node,sizeof(node))||!read_memory((DWORD)node.value,&external,sizeof(external))||!name_of(&table,&external,name))return 0;
            if(seen>1)fputc(',',log_file);fprintf(log_file,"{\"bucket\":%lu,\"node\":%lu,\"next\":%lu,\"external\":%lu,\"raw\":",bucket,entry,(DWORD)node.next,(DWORD)node.value);bytes((unsigned char *)&external,sizeof(external));fprintf(log_file,",\"name\":");bytes((unsigned char *)name,strlen(name)+1);fputc('}',log_file);entry=(DWORD)node.next;
        }
    }
    fprintf(log_file,"]}");return seen==hash.element_count;
}
static int snapshot(char *event,CONTEXT *c)
{
    DWORD errors[3],stream[8],used,i;unsigned char pending[4096],short_name[9],guard[64],inputs[4096];
    if(!read_memory(0x852d8,errors,8)||!read_memory(teb+0x34,errors+2,4)||!read_memory(0x85070,stream,32)||!read_memory(0x5c0dc,short_name,9)||!read_memory(memory,guard,64)||!read_memory(memory+1024,inputs,4096))return 0;
    used=stream[0]-stream[2];if(used>4096||(used&&!read_memory(stream[2],pending,used))||memcmp(inputs,payload,4096))return 0;
    fprintf(log_file,"{\"event\":\"%s\",\"phase\":%lu,\"target\":%lu,\"esp\":%lu,\"eax\":%lu,\"ebx\":%lu,\"esi\":%lu,\"edi\":%lu,\"ebp\":%lu,\"arguments\":",event,phase,target,c->Esp,c->Eax,c->Ebx,c->Esi,c->Edi,c->Ebp);words(args,count);
    fprintf(log_file,",\"errors\":");words(errors,3);fprintf(log_file,",\"fp_control\":%lu,\"fp_status\":%lu,\"fp_tag\":%lu,\"fp_raw\":",c->FloatSave.ControlWord,c->FloatSave.StatusWord,c->FloatSave.TagWord);bytes((unsigned char *)&c->FloatSave,sizeof(c->FloatSave));
    fprintf(log_file,",\"stdout_file\":");words(stream,8);fprintf(log_file,",\"pending\":");bytes(pending,used);fprintf(log_file,",\"short_name\":");bytes(short_name,9);fprintf(log_file,",\"guards\":");bytes(guard,64);fprintf(log_file,",\"inputs_unchanged\":true,\"tables\":[");
    for(i=0;i<4;++i){if(i)fputc(',',log_file);if(!table_state(tables[i]))return 0;}
    fprintf(log_file,"]}\n");fflush(log_file);return 1;
}
static int prepare(CONTEXT *c)
{
    DWORD frame[6],i;char name[64];count=1;
    if(phase==0||phase==6){target=0x26190;args[0]=memory+(phase?48:32);}
    else if(phase==1){
        if(!read_memory(memory+1024+index*128+32,name,64)||!memchr(name,0,64))return 0;
        target=0x25cd0;count=5;args[0]=tables[0];args[1]=strlen(name)<=8;args[2]=memory+1024+index*128+32;args[3]=memory+80;args[4]=memory+96;
    }else if(phase==2){target=0x25e20;count=3;args[0]=current_external;args[1]=1;args[2]=tables[0];}
    else if(phase==3||phase==5||phase==7||phase==9){target=0x26510;args[0]=tables[phase==3?0:(phase==5?1:(phase==7?2:3))];}
    else if(phase==4||phase==8){target=0x2dcf0;args[0]=tables[phase==4?0:2];}
    else if(phase>=10 && phase<=21){
        enum_table=(phase-10)/3;args[0]=tables[enum_table];target=(phase-10)%3==0?0x261e0:((phase-10)%3==1?0x261f0:0x26210);
    }else if(phase==22){target=0x50e00;args[0]=0x85070;}
    else return 0;
    frame[0]=sentinel;for(i=0;i<count;++i)frame[i+1]=args[i];
    if(!write_memory(stack_top,frame,(count+1)*4)||!read_memory(target,&entry_byte,1)||!code(target,0xcc))return 0;
    c->Eip=target;c->Esp=stack_top;return SetThreadContext(thread,c);
}
static int advance(CONTEXT *c)
{
    SymbolTable table;External external;char name[64];DWORD data[3];
    if(phase==0){if(!read_memory(memory+32,tables,4)||!tables[0])return 0;phase=1;}
    else if(phase==1){if(!c->Eax)return 0;current_external=c->Eax;if(index%3)phase=2;else if(++index==32)phase=3;}
    else if(phase==2){if(++index==32)phase=3;else phase=1;}
    else if(phase==4){tables[1]=c->Eax;if(!tables[1]||tables[1]==tables[0])return 0;++phase;}
    else if(phase==6){if(!read_memory(memory+48,tables+2,4)||!tables[2])return 0;++phase;}
    else if(phase==8){tables[3]=c->Eax;if(!tables[3]||tables[3]==tables[2])return 0;++phase;}
    else if(phase>=10 && phase<=21 && (phase-10)%3==0){
        if(!read_memory(tables[enum_table],&table,sizeof(table))||!read_memory((DWORD)table.hash+12,&report_count,4)||fwrite(&report_count,4,1,report)!=1)return 0;report_counts[enum_table]=report_count;++phase;
    }else if(phase>=10 && phase<=21 && (phase-10)%3==1){
        if(c->Eax){
            if(!report_count--||!read_memory(tables[enum_table],&table,sizeof(table))||!read_memory(c->Eax,&external,sizeof(external))||!name_of(&table,&external,name))return 0;
            data[0]=external.flags;data[1]=external.symbol.N.Name.Short!=0;data[2]=strlen(name)+1;
            if(fwrite(data,4,3,report)!=3||fwrite(name,1,data[2],report)!=data[2])return 0;
        }else{if(report_count)return 0;++phase;}
    }else ++phase;
    return 1;
}
int main(int argc,char **argv)
{
    SECURITY_ATTRIBUTES sa;STARTUPINFOA startup;PROCESS_INFORMATION launch;DEBUG_EVENT ev;CONTEXT c;
    DWORD pid,site,status,n,rva,size,old,again,failed=0,stopped=0,exit_code=999,at_main=0,header[4];
    unsigned char *mask,guard[256];char command[1024];FILE *f;HANDLE output,error;AllocateRemote allocate_remote;
    /* program,textplan,stdout,stderr,events,report,cwd,names */
    if(argc!=9)return 2;
    f=fopen(argv[2],"rb");if(!f)return 3;fseek(f,0,SEEK_END);n=ftell(f);rewind(f);mask=malloc(n);if(!mask||fread(mask,1,n,f)!=n||fclose(f)||n<8)return 3;rva=*(DWORD *)mask;size=*(DWORD *)(mask+4);if(size+8!=n)return 3;
    f=fopen(argv[8],"rb");if(!f||fread(payload,1,sizeof(payload),f)!=sizeof(payload)||fgetc(f)!=EOF||fclose(f))return 3;
    log_file=fopen(argv[5],"wt");report=fopen(argv[6],"wb");if(!log_file||!report)return 3;
    header[0]=0x314d5953;header[1]=1;header[2]=4;header[3]=0;if(fwrite(header,4,4,report)!=4)return 3;
    memset(&sa,0,sizeof(sa));sa.nLength=sizeof(sa);sa.bInheritHandle=TRUE;
    output=CreateFileA(argv[3],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    error=CreateFileA(argv[4],GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);startup.hStdOutput=output;startup.hStdError=error;
    sprintf(command,"\"%s\"",argv[1]);if(!CreateProcessA(argv[1],command,NULL,NULL,TRUE,DEBUG_ONLY_THIS_PROCESS,NULL,argv[7],&startup,&launch))return 4;
    pid=launch.dwProcessId;CloseHandle(launch.hProcess);CloseHandle(launch.hThread);allocate_remote=(AllocateRemote)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");
    for(;;){
        if(!WaitForDebugEvent(&ev,15000)){failed=5;break;}status=DBG_CONTINUE;if(ev.dwProcessId!=pid){failed=6;break;}
        if(ev.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT){process=ev.u.CreateProcessInfo.hProcess;thread=ev.u.CreateProcessInfo.hThread;teb=(DWORD)ev.u.CreateProcessInfo.lpThreadLocalBase;if((DWORD)ev.u.CreateProcessInfo.lpBaseOfImage!=0x10000||!code(0x26b40,0xcc)){failed=7;break;}if(ev.u.CreateProcessInfo.hFile)CloseHandle(ev.u.CreateProcessInfo.hFile);
        }else if(ev.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT){if(ev.u.LoadDll.hFile)CloseHandle(ev.u.LoadDll.hFile);
        }else if(ev.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT){failed=8;break;
        }else if(ev.dwDebugEventCode==EXCEPTION_DEBUG_EVENT){
            site=(DWORD)ev.u.Exception.ExceptionRecord.ExceptionAddress;memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;if(!GetThreadContext(thread,&c)){failed=9;break;}
            if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&site==0x26b40&&!at_main){
                at_main=1;initial=c;if(!VirtualProtectEx(process,(void *)(0x10000+rva),size,PAGE_EXECUTE_READWRITE,&old)||!write_memory(0x10000+rva,mask+8,size)||!FlushInstructionCache(process,(void *)(0x10000+rva),size)||!VirtualProtectEx(process,(void *)(0x10000+rva),size,old,&again)||!read_memory(c.Esp,&main_return,4)){failed=10;break;}
                memory=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);stack_top=(DWORD)allocate_remote(process,NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)+0xf000;sentinel=(DWORD)allocate_remote(process,NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
                memset(guard,0xa5,sizeof(guard));if(!memory||stack_top==0xf000||!sentinel||!write_memory(memory,guard,sizeof(guard))||!write_memory(memory+1024,payload,sizeof(payload))||!code(sentinel,0xcc)||!prepare(&c)){failed=11;break;}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&at_main){
                c.Eip=site;
                if(site==target&&!active){
                    if((target==0x26510||target==0x2dcf0)&&!snapshot(entry_byte==0xcc?"frontier":"entry",&c)){failed=12;break;}
                    if(entry_byte==0xcc){fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
                    else{before=c;active=1;if(!code(target,entry_byte)||!SetThreadContext(thread,&c)){failed=13;break;}}
                }else if(site==sentinel&&active){
                    if(c.Esp!=before.Esp+4||c.Ebx!=before.Ebx||c.Esi!=before.Esi||c.Edi!=before.Edi||c.Ebp!=before.Ebp){failed=14;break;}
                    if(phase==4)tables[1]=c.Eax;if(phase==8)tables[3]=c.Eax;
                    if((target==0x26510||target==0x2dcf0)&&!snapshot("return",&c)){failed=15;break;}
                    active=0;if(!advance(&c)){failed=16;break;}
                    if(phase==23){stopped=1;c.Eip=main_return;c.Esp=initial.Esp+4;c.Eax=0;if(!SetThreadContext(thread,&c)){failed=17;break;}}
                    else if(!prepare(&c)){failed=18;break;}
                }else{fprintf(log_file,"{\"event\":\"trap\",\"phase\":%lu,\"va\":%lu}\n",phase,site);fflush(log_file);stopped=2;TerminateProcess(process,17);}
            }else if(ev.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT&&!at_main){
            }else{fprintf(log_file,"{\"event\":\"unexpected\",\"phase\":%lu,\"va\":%lu,\"code\":%lu}\n",phase,site,ev.u.Exception.ExceptionRecord.ExceptionCode);failed=19;break;}
        }else if(ev.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT){exit_code=ev.u.ExitProcess.dwExitCode;ContinueDebugEvent(pid,ev.dwThreadId,status);break;}
        if(!ContinueDebugEvent(pid,ev.dwThreadId,status)){failed=20;break;}
    }
    if(failed&&process)TerminateProcess(process,124);fclose(report);
    fprintf(log_file,"{\"event\":\"summary\",\"failed\":%lu,\"stopped\":%lu,\"exit\":%lu,\"phase\":%lu,\"stdout_bytes\":%lu,\"stderr_bytes\":%lu,\"counts\":",failed,stopped,exit_code,phase,GetFileSize(output,NULL),GetFileSize(error,NULL));words(report_counts,4);fprintf(log_file,"}\n");fclose(log_file);CloseHandle(output);CloseHandle(error);return failed?1:0;
}
