/* Original capture only: one reached 1024-record packed hash slab. */
static DWORD packed_arena_base,packed_arena_entry;
static DWORD packed_arena_step,packed_arena_tf,packed_arena_thread;

static int packed_arena_bytes(HANDLE child,HANDLE log,const char *label)
{
    DWORD bytes,written,i;int used;
    if(!packed_arena_base || !ReadProcessMemory(child,(void *)packed_arena_base,array_bytes,0x3000,&bytes) || bytes!=0x3000)return 0;
    used=sprintf(object_line,"packed-hash-bytes %s %08lx ",label,packed_arena_base);
    for(i=0;i<0x3000;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int observe_packed_arena(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD size,globals[2],bytes,written;unsigned char instruction=0x83;char line[160];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(packed_arena_base || packed_arena_step || !GetThreadContext(thread,&c) || c.Eip!=0x19c3c ||
       !ReadProcessMemory(child,(void *)c.Esp,&size,4,&bytes) || bytes!=4 || size!=0x3000 || !c.Eax ||
       !ReadProcessMemory(child,(void *)0x350a8,globals,8,&bytes) || bytes!=8 || globals[0] || globals[1])return 0;
    packed_arena_base=c.Eax;packed_arena_entry=c.Edi;
    sprintf(line,"packed-hash-allocation %08lx %lu %08lx %08lx %lu\n",c.Eax,size,c.Edi,c.Esp,c.Ebx&0xffff);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) || !packed_arena_bytes(child,log,"birth") ||
       !WriteProcessMemory(child,(void *)0x19c3b,&instruction,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x19c3b,1);
    packed_arena_step=0x19c3e;packed_arena_tf=c.EFlags&0x100;packed_arena_thread=tid;
    c.Eip=0x19c3b;c.EFlags|=0x100;
    return SetThreadContext(thread,&c);
}

static int rearm_packed_arena(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;
    const char line[]="packed-hash-rearm 00019c3b 00019c3e\n";
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=packed_arena_step || tid!=packed_arena_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)0x19c3b,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x19c3b,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1)return 0;
    packed_arena_step=0;c.EFlags=(c.EFlags&~0x100UL)|packed_arena_tf;
    return SetThreadContext(thread,&c);
}

static int dump_packed_arena(HANDLE child,HANDLE log,DWORD frontier)
{
    if(!packed_arena_base)return 1;
    if(packed_arena_step || frontier!=0x257a0)return 0;
    return packed_arena_bytes(child,log,"frontier");
}
