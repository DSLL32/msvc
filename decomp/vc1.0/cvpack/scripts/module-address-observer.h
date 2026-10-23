/* Original capture only: typed AX result at the reviewed procedure callsite. */
static DWORD module_address_step,module_address_tf,module_address_thread,module_address_calls;
static int observe_module_address_return(HANDLE child,HANDLE log,HANDLE thread,DWORD tid)
{
    CONTEXT c;DWORD args[3],input,module,bytes,written,i,size;unsigned char record[64],instruction=0x83;int used;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(module_address_step || module_address_calls>=3 || !GetThreadContext(thread,&c) || c.Eip!=0x257c6 ||
       !ReadProcessMemory(child,(void *)c.Esp,args,12,&bytes) || bytes!=12 ||
       !ReadProcessMemory(child,(void *)0x335e4,&input,4,&bytes) || bytes!=4 ||
       !ReadProcessMemory(child,(void *)0x33050,&module,4,&bytes) || bytes!=4 || args[0]!=module ||
       !ReadProcessMemory(child,(void *)input,record,2,&bytes) || bytes!=2)return 0;
    size=(DWORD)record[0]+((DWORD)record[1]<<8)+2;
    if(size<36 || size>64 || !ReadProcessMemory(child,(void *)input,record,size,&bytes) || bytes!=size ||
       record[2]!=5 || record[3]!=2)return 0;
    used=sprintf(object_line,"module-address-return %08lx %08lx %08lx %08lx %08lx %08lx ",c.Eax,c.Esp,args[0],args[1],args[2],input);
    for(i=0;i<size;++i)used+=sprintf(object_line+used,"%02x",record[i]);object_line[used++]='\n';
    if(!WriteFile(log,object_line,used,&written,0) || written!=(DWORD)used ||
       !WriteProcessMemory(child,(void *)0x257c5,&instruction,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x257c5,1);++module_address_calls;module_address_step=0x257c8;module_address_tf=c.EFlags&0x100;module_address_thread=tid;c.Eip=0x257c5;c.EFlags|=0x100;
    return SetThreadContext(thread,&c);
}
static int rearm_module_address(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;const char line[]="module-address-rearm 000257c5 000257c8\n";
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=module_address_step || tid!=module_address_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)0x257c5,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x257c5,1);
    if(!WriteFile(log,line,sizeof(line)-1,&written,0) || written!=sizeof(line)-1)return 0;
    module_address_step=0;c.EFlags=(c.EFlags&~0x100UL)|module_address_tf;return SetThreadContext(thread,&c);
}
