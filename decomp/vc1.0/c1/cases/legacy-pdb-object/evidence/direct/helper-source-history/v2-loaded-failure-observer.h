/* Debugger-only external API calls. Never compiled into the application. */
#ifndef CAPTURE_PDB_UNIT_H
#define CAPTURE_PDB_UNIT_H
static int pdb_unit=-1,pdb_stage,pdb_heap_step,pdb_heap_active;
static DWORD pdb_self,pdb_marker,pdb_stack,pdb_signature,pdb_teb,pdb_external_base;
static DWORD pdb_heap_site,pdb_heap_return,pdb_heap_size;
static unsigned char pdb_heap_byte,pdb_heap_return_byte;
static DWORD pdb_allocations[32],pdb_allocation_bytes[32];
static unsigned pdb_allocation_count,pdb_call_count,pdb_call_pop,pdb_call_args;
static CONTEXT pdb_saved;
static DWORD pdb_heap_error;
static DWORD pdb_constructor_site,pdb_constructor_stack,pdb_constructor_ebx;
static DWORD pdb_heap_stack,pdb_heap_ebx,pdb_heap_esi,pdb_heap_edi,pdb_heap_ebp;
static DWORD pdb_heap_track;
static int pdb_constructor_step;
static int pdb_unit_initialize(HANDLE process,DWORD base)
{
    DWORD got;
    unsigned char byte;
    pdb_constructor_site=base+0x7b70;
    if(!ReadProcessMemory(process,(void *)pdb_constructor_site,&byte,1,&got)||got!=1||byte!=0x53||
       !patch_debug_byte(process,pdb_constructor_site,0xcc))return 0;
    pdb_heap_site=base+0x40af0;
    if(!ReadProcessMemory(process,(void *)pdb_heap_site,&pdb_heap_byte,1,&got)||got!=1||pdb_heap_byte!=0x53)return 0;
    return 1;
}
static void pdb_unit_errors(HANDLE process,HANDLE log,DWORD base,const char *boundary)
{
    DWORD error,dos,last,got,written;
    char line[120];
    if(!ReadProcessMemory(process,(void *)(base+0x639cc),&error,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(base+0x639d0),&dos,4,&got)||got!=4||
       !ReadProcessMemory(process,(void *)(pdb_teb+0x34),&last,4,&got)||got!=4) {pdb_heap_error=1;return;}
    sprintf(line,"pdb-errors %s %08lx %08lx %08lx\n",boundary,error,dos,last);
    WriteFile(log,line,strlen(line),&written,0);
}
static int pdb_heap_event(HANDLE process,HANDLE thread,HANDLE log,DWORD code,DWORD site)
{
    CONTEXT c;
    DWORD values[2],got,written,pointer;
    char line[100],label[100];
    if(!pdb_heap_site)return 0;
    if(pdb_heap_step && code==EXCEPTION_SINGLE_STEP) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(!GetThreadContext(thread,&c)||c.Eip!=pdb_heap_site+1||c.Esp!=pdb_heap_stack-4||
           !ReadProcessMemory(process,(void *)c.Esp,&pointer,4,&got)||got!=4||pointer!=pdb_heap_ebx||
           !patch_debug_byte(process,pdb_heap_site,0xcc))return -1;
        c.EFlags&=~0x100UL;pdb_heap_step=0;
        return SetThreadContext(thread,&c)?1:-1;
    }
    if(code!=EXCEPTION_BREAKPOINT)return 0;
    if(site==pdb_heap_site) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(pdb_heap_active||!GetThreadContext(thread,&c)||!ReadProcessMemory(process,(void *)c.Esp,values,8,&got)||got!=8)return -1;
        pdb_heap_return=values[0];pdb_heap_size=values[1];
        pdb_heap_track=pdb_heap_return==pdb_constructor_site+11||pdb_heap_return==pdb_constructor_site+48||
            pdb_heap_return==pdb_constructor_site+108||
            (pdb_heap_return>=pdb_constructor_site-0x7e0&&pdb_heap_return<pdb_constructor_site-0x544);
        pdb_heap_stack=c.Esp;pdb_heap_ebx=c.Ebx;pdb_heap_esi=c.Esi;pdb_heap_edi=c.Edi;pdb_heap_ebp=c.Ebp;
        if(pdb_heap_track&&(!ReadProcessMemory(process,(void *)pdb_heap_return,&pdb_heap_return_byte,1,&got)||got!=1||
           pdb_heap_return_byte==0xcc||!patch_debug_byte(process,pdb_heap_return,0xcc)))return -1;
        if(!patch_debug_byte(process,pdb_heap_site,pdb_heap_byte))return -1;
        pdb_heap_active=pdb_heap_track;pdb_heap_step=1;c.Eip=site;c.EFlags|=0x100;
        return SetThreadContext(thread,&c)?1:-1;
    }
    if(pdb_heap_active&&site==pdb_heap_return) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(!GetThreadContext(thread,&c)||c.Esp!=pdb_heap_stack+4||c.Ebx!=pdb_heap_ebx||c.Esi!=pdb_heap_esi||
           c.Edi!=pdb_heap_edi||c.Ebp!=pdb_heap_ebp||!patch_debug_byte(process,site,pdb_heap_return_byte)||pdb_allocation_count>=32)return -1;
        pointer=c.Eax;if(!pointer)return -1;
        sprintf(line,"pdb-allocation %u %lu %08lx\n",pdb_allocation_count,pdb_heap_size,pointer);WriteFile(log,line,strlen(line),&written,0);
        pdb_allocations[pdb_allocation_count]=pointer;pdb_allocation_bytes[pdb_allocation_count]=pdb_heap_size;
        sprintf(label,"allocation-before-%u",pdb_allocation_count++);
        pdb_memory(&pdb_observer,label,pointer,pdb_heap_size<256?pdb_heap_size:256);
        pdb_heap_active=0;c.Eip=site;
        return SetThreadContext(thread,&c)?1:-1;
    }
    return 0;
}
static int pdb_launch(HANDLE process,HANDLE thread,HANDLE log,DWORD target,DWORD self,
                      unsigned args,DWORD a,DWORD b,DWORD c,unsigned pop)
{
    CONTEXT state=pdb_saved;
    DWORD frame[12],written;
    char line[160];
    unsigned i;
    if(pdb_stack<pdb_external_base+8192||pdb_stack+sizeof(frame)>pdb_external_base+65536||args>3||pop>12)return -1;
    for(i=0;i<12;++i)frame[i]=0xa5a5a5a5;
    frame[0]=pdb_marker;frame[1]=a;frame[2]=b;frame[3]=c;
    if(!WriteProcessMemory(process,(void *)pdb_stack,frame,sizeof(frame),&written)||written!=sizeof(frame))return -1;
    state.Eip=target;state.Esp=pdb_stack;state.Ecx=self;
    pdb_call_pop=pop;pdb_call_args=args;
    sprintf(line,"pdb-call %u %08lx %08lx %08lx %u %08lx %08lx %08lx %u\n",pdb_call_count++,target,self,pdb_stack,args,a,b,c,pop);
    WriteFile(log,line,strlen(line),&written,0);
    return SetThreadContext(thread,&state)?1:-1;
}
static int pdb_unit_event(HANDLE process,HANDLE thread,HANDLE log,DWORD code,DWORD site,DWORD base)
{
    CONTEXT c;
    DWORD words[4],got,written,allocation,canary[4],handle;
    int heap;
    unsigned i;
    char line[160],label[100];
    static char arguments[]="unit.pdb\0wc\0rc\0";
    if(pdb_unit<0)return 0;
    if(pdb_constructor_step&&code==EXCEPTION_SINGLE_STEP) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(!GetThreadContext(thread,&c)||c.Eip!=pdb_constructor_site+1||c.Esp!=pdb_constructor_stack-4||
           !ReadProcessMemory(process,(void *)c.Esp,&allocation,4,&got)||got!=4||allocation!=pdb_constructor_ebx||
           !patch_debug_byte(process,pdb_heap_site,0xcc))return -1;
        c.EFlags&=~0x100UL;pdb_constructor_step=0;
        return SetThreadContext(thread,&c)?1:-1;
    }
    if(code==EXCEPTION_BREAKPOINT&&site==pdb_constructor_site) {
        memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
        if(pdb_stage||pdb_allocation_count||!GetThreadContext(thread,&c)||
           !patch_debug_byte(process,site,0x53))return -1;
        pdb_process=process;pdb_log=log;memset(&pdb_observer,0,sizeof(pdb_observer));
        pdb_observer.read=pdb_original_read;pdb_observer.emit=pdb_original_emit;
        pdb_constructor_stack=c.Esp;pdb_constructor_ebx=c.Ebx;
        pdb_constructor_step=1;c.Eip=site;c.EFlags|=0x100;
        return SetThreadContext(thread,&c)?1:-1;
    }
    heap=pdb_heap_event(process,thread,log,code,site);if(heap)return heap;
    if(code!=EXCEPTION_BREAKPOINT||site!=base+0x7300)return 0;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if(!GetThreadContext(thread,&c))return -1;
    if(!pdb_stage) {
        typedef LPVOID (WINAPI *AllocateInProcess)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
        AllocateInProcess allocate;
        pdb_process=process;pdb_log=log;memset(&pdb_observer,0,sizeof(pdb_observer));pdb_observer.read=pdb_original_read;pdb_observer.emit=pdb_original_emit;
        pdb_self=c.Ecx;pdb_saved=c;pdb_marker=site;
        if(pdb_allocation_count!=3||pdb_allocation_bytes[0]!=116||pdb_allocations[0]!=pdb_self||
           pdb_allocation_bytes[1]!=12||pdb_allocation_bytes[2]!=12||
           !ReadProcessMemory(process,(void *)(pdb_self+72),words,12,&got)||got!=12||
           words[0]!=pdb_allocations[1]||words[1]!=words[0]||words[2]||
           !ReadProcessMemory(process,(void *)(pdb_self+84),words,12,&got)||got!=12||
           words[0]!=pdb_allocations[2]||words[1]!=words[0]||words[2])return -1;
        if(!ReadProcessMemory(process,(void *)c.Esp,words,12,&got)||got!=12)return -1;
        sprintf(line,"pdb-constructor %08lx %08lx %08lx %08lx\n",pdb_self,c.Esp,words[1],words[2]);WriteFile(log,line,strlen(line),&written,0);
        pdb_database(&pdb_observer,"constructor",pdb_self,0);
        pdb_unit_errors(process,log,base,"constructor");
        allocate=(AllocateInProcess)GetProcAddress(GetModuleHandleA("kernel32.dll"),"VirtualAllocEx");if(!allocate)return -1;
        allocation=(DWORD)allocate(process,0,65536,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!allocation)return -1;
        pdb_external_base=allocation;
        pdb_stack=allocation+65536-512;
        if(!WriteProcessMemory(process,(void *)(allocation+4096),arguments,sizeof(arguments),&written)||written!=sizeof(arguments))return -1;
        pdb_memory(&pdb_observer,"arguments",allocation+4096,sizeof(arguments));
        if(pdb_observer.failed||pdb_heap_error)return -1;
        pdb_stage=1;
        return pdb_launch(process,thread,log,base+0x7150,pdb_self,3,allocation+4096,allocation+4096+((pdb_unit==1||pdb_unit==3)?12:9),pdb_signature,12);
    }
    sprintf(line,"pdb-return %u %08lx %08lx %08lx %u %08lx %08lx\n",pdb_stage,c.Eax,c.Edx,c.Esp,
        c.Ebx==pdb_saved.Ebx&&c.Esi==pdb_saved.Esi&&c.Edi==pdb_saved.Edi&&c.Ebp==pdb_saved.Ebp,c.FloatSave.ControlWord,pdb_saved.FloatSave.ControlWord);
    WriteFile(log,line,strlen(line),&written,0);
    if(c.Esp!=pdb_stack+4+pdb_call_pop||!ReadProcessMemory(process,(void *)(pdb_stack+4+4*pdb_call_args),canary,16,&got)||got!=16)return -1;
    sprintf(line,"pdb-canary %08lx %08lx %08lx %08lx\n",canary[0],canary[1],canary[2],canary[3]);WriteFile(log,line,strlen(line),&written,0);
    for(i=0;i<4;++i)if(canary[i]!=0xa5a5a5a5)return -1;
    if(c.Ebx!=pdb_saved.Ebx||c.Esi!=pdb_saved.Esi||c.Edi!=pdb_saved.Edi||c.Ebp!=pdb_saved.Ebp||
       c.FloatSave.ControlWord!=pdb_saved.FloatSave.ControlWord)return -1;
    if(!ReadProcessMemory(process,(void *)(pdb_self+100),&handle,4,&got)||got!=4)return -1;
    if(pdb_stage==1&&pdb_unit>=4) {
        if(c.Eax!=1)return -1;
        pdb_database(&pdb_observer,"reload-before",pdb_self,1);
        if(pdb_unit==5) {pdb_stage=2;return pdb_launch(process,thread,log,base+0x44030,0,2,handle,32,0,0);}
        pdb_stage=3;return pdb_launch(process,thread,log,base+0x41570,0,3,handle,0,0,0);
    }
    if(pdb_stage==2) {
        if(c.Eax)return -1;
        pdb_stage=3;return pdb_launch(process,thread,log,base+0x41570,0,3,handle,0,0,0);
    }
    if(pdb_stage==3) {
        if(c.Eax)return -1;
        pdb_stage=4;return pdb_launch(process,thread,log,base+0x72c0,pdb_self,0,0,0,0,0);
    }
    if(pdb_stage!=5) {
        pdb_unit_errors(process,log,base,"result");
        pdb_database(&pdb_observer,"result",pdb_self,c.Eax==1||pdb_unit>=4);
        for(i=0;i<pdb_allocation_count;++i) {
            sprintf(label,"allocation-after-%u",i);pdb_memory(&pdb_observer,label,pdb_allocations[i],pdb_allocation_bytes[i]<256?pdb_allocation_bytes[i]:256);
        }
        if((int)handle>=0) {pdb_stage=5;return pdb_launch(process,thread,log,base+0x41570,0,3,handle,0,1,0);}
    } else {
        sprintf(line,"pdb-offset %lu\n",c.Eax);WriteFile(log,line,strlen(line),&written,0);
    }
    if(pdb_observer.failed||pdb_heap_error)return -1;
    TerminateProcess(process,99);return 1;
}
#endif
