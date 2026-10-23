/* Original capture only: one reached 1024-record recursive hash slab. */
static DWORD recursive_arena_base,recursive_arena_entry;
static DWORD recursive_arena_step,recursive_arena_tf,recursive_arena_thread;

static int recursive_arena_bytes(HANDLE child,HANDLE log,const char *label)
{
    DWORD bytes,written,i;int used;
    if(!recursive_arena_base || !ReadProcessMemory(child,(void *)recursive_arena_base,array_bytes,0x3000,&bytes) || bytes!=0x3000)return 0;
    used=sprintf(object_line,"recursive-hash-bytes %s %08lx ",label,recursive_arena_base);
    for(i=0;i<0x3000;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int observe_recursive_arena(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD size,globals[2],bytes,written;unsigned char instruction=0x83;char line[160];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(recursive_arena_base || recursive_arena_step || !GetThreadContext(thread,&c) || c.Eip!=0x19cb7 ||
       !ReadProcessMemory(child,(void *)c.Esp,&size,4,&bytes) || bytes!=4 || size!=0x3000 || !c.Eax ||
       !ReadProcessMemory(child,(void *)0x352b4,globals,8,&bytes) || bytes!=8 || globals[0] || globals[1])return 0;
    recursive_arena_base=c.Eax;recursive_arena_entry=c.Esi;
    sprintf(line,"recursive-hash-allocation %08lx %lu %08lx %08lx\n",c.Eax,size,c.Esi,c.Esp);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) || !recursive_arena_bytes(child,log,"birth") ||
       !WriteProcessMemory(child,(void *)0x19cb6,&instruction,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x19cb6,1);
    recursive_arena_step=0x19cb9;recursive_arena_tf=c.EFlags&0x100;recursive_arena_thread=tid;
    c.Eip=0x19cb6;c.EFlags|=0x100;
    return SetThreadContext(thread,&c);
}

static int rearm_recursive_arena(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;
    const char line[]="recursive-hash-rearm 00019cb6 00019cb9\n";
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=recursive_arena_step || tid!=recursive_arena_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)0x19cb6,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x19cb6,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1)return 0;
    recursive_arena_step=0;c.EFlags=(c.EFlags&~0x100UL)|recursive_arena_tf;
    return SetThreadContext(thread,&c);
}

static int dump_recursive_arena(HANDLE child,HANDLE log,DWORD frontier)
{
    if(!recursive_arena_base)return 1;
    if(recursive_arena_step || frontier!=0x1c880 && frontier!=0x19c20)return 0;
    return recursive_arena_bytes(child,log,"frontier");
}
