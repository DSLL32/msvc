/* Original capture only: two actual allocate_symbol malloc8100 lifetimes. */
static DWORD symbol_block_base[2],symbol_block_count;
static DWORD symbol_block_step,symbol_block_tf,symbol_block_thread;
static int symbol_block_bytes(HANDLE child,HANDLE log,const char *label,DWORD index)
{
    DWORD bytes,written,i;int used;
    if(index>=symbol_block_count || !ReadProcessMemory(child,(void *)symbol_block_base[index],array_bytes,8100,&bytes) || bytes!=8100)return 0;
    used=sprintf(object_line,"symbol-block-bytes %s %lu %08lx ",label,index,symbol_block_base[index]);
    for(i=0;i<8100;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}
static int observe_symbol_block(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD args[4],state[2],bytes,written;unsigned char instruction=0x83;char line[240];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(symbol_block_count>=2 || symbol_block_step || !GetThreadContext(thread,&c) || c.Eip!=0x19550 || !c.Eax ||
       !ReadProcessMemory(child,(void *)c.Esp,args,16,&bytes) || bytes!=16 || args[0]!=8100 ||
       (args[2]!=0x1b3e5 && args[2]!=0x1b5cf) || c.Esi!=(args[3]&0xffff) ||
       !ReadProcessMemory(child,(void *)0x373c8,state,8,&bytes) || bytes!=8 || state[0]>=c.Esi || state[0]>8100)return 0;
    symbol_block_base[symbol_block_count++]=c.Eax;
    sprintf(line,"symbol-block-allocation %lu %08lx %lu %08lx %lu %08lx %lu\n",symbol_block_count-1,c.Eax,args[0],args[2],c.Esi,state[1],state[0]);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) || !symbol_block_bytes(child,log,"birth",symbol_block_count-1) ||
       !WriteProcessMemory(child,(void *)0x1954f,&instruction,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x1954f,1);symbol_block_step=0x19552;symbol_block_tf=c.EFlags&0x100;symbol_block_thread=tid;c.Eip=0x1954f;c.EFlags|=0x100;
    return SetThreadContext(thread,&c);
}
static int rearm_symbol_block(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;const char line[]="symbol-block-rearm 0001954f 00019552\n";
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=symbol_block_step || tid!=symbol_block_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)0x1954f,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x1954f,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1)return 0;
    symbol_block_step=0;c.EFlags=(c.EFlags&~0x100UL)|symbol_block_tf;return SetThreadContext(thread,&c);
}
static int dump_symbol_blocks(HANDLE child,HANDLE log,DWORD frontier)
{
    DWORD i;if(!symbol_block_count)return 1;
    if(symbol_block_step || (frontier!=0x257a0 && frontier!=0x25920) || symbol_block_count!=2)return 0;
    for(i=0;i<symbol_block_count;++i)if(!symbol_block_bytes(child,log,"frontier",i))return 0;
    return 1;
}
