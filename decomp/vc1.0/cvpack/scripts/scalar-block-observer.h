/* Exact first begin_symbol_block callback: observe its real caller return.
   No heap bytes are read by this controller or credited on suppression==0. */
static int begin_scalar_block(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD bytes,written,ret,words[32],conversion[2];unsigned char trap=0xcc,instruction;char line[256];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(scalar_active || scalar_checkpoint || !GetThreadContext(thread,&c) || c.Eip!=0x25450 || c.Eax!=0x25450 || c.Ecx!=0x381b0 ||
       !ReadProcessMemory(child,(void *)c.Ecx,conversion,8,&bytes) || bytes!=8 || conversion[0]!=0x206 || conversion[1]!=0x25450 ||
       (c.EFlags&0x100) || !ReadProcessMemory(child,(void *)c.Esp,&ret,4,&bytes) || bytes!=4 || ret!=0x25172 ||
       !ReadProcessMemory(child,(void *)0x335e4,scalar_before,16,&bytes) || bytes!=16 || scalar_before[1] || scalar_before[2] ||
       !ReadProcessMemory(child,(void *)ret,&instruction,1,&bytes) || bytes!=1 || instruction!=0xa1 ||
       !WriteProcessMemory(child,(void *)ret,&trap,1,&bytes) || bytes!=1)return 0;
    if(!ReadProcessMemory(child,(void *)c.Esp,words,sizeof(words),&bytes) || bytes!=sizeof(words) ||
       !dump_progress_input(child,log,0x25450,words))return 0;
    sprintf(line,"scalar-block target %08lx %08lx %08lx %08lx\n",c.Eax,c.Ecx,conversion[0],conversion[1]);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return 0;
    scalar_thread=tid;scalar_esp=c.Esp;scalar_ebp=c.Ebp;scalar_active=1;
    sprintf(line,"scalar-block entry %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",tid,c.Eip,c.Esp,c.Ebp,scalar_before[0],scalar_before[1],scalar_before[2],scalar_before[3]);
    FlushInstructionCache(child,(void *)ret,1);
    return WriteFile(log,line,strlen(line),&written,0) && written==strlen(line);
}

static int finish_scalar_block(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD bytes,written,after[4];char line[256];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(!scalar_active || scalar_checkpoint || tid!=scalar_thread || !GetThreadContext(thread,&c) || c.Eip!=0x25173 ||
       c.Esp!=scalar_esp+4 || c.Ebp!=scalar_ebp ||
       !ReadProcessMemory(child,(void *)0x335e4,after,16,&bytes) || bytes!=16 ||
       after[0]!=scalar_before[0] || after[1] || after[2]!=1 || after[3]!=scalar_before[3])return 0;
    sprintf(line,"scalar-block return %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",tid,0x25172UL,c.Esp,c.Ebp,after[0],after[1],after[2],after[3]);
    scalar_active=0;scalar_checkpoint=1;
    return WriteFile(log,line,strlen(line),&written,0) && written==strlen(line);
}
