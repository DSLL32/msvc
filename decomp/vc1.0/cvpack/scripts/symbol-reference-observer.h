/* Original capture only: reached36-byte contained reference reservations. */
static DWORD symbol_reference_count,symbol_reference_active;
static DWORD symbol_reference_bases[3],symbol_reference_arguments[5],symbol_reference_hash_buckets[3];
static DWORD symbol_reference_step,symbol_reference_site,symbol_reference_thread,symbol_reference_tf;

static int symbol_reference_bytes(HANDLE child,HANDLE log,const char *label,DWORD index)
{
    unsigned char raw[36];DWORD bytes,written,i;int used;
    if(index>=symbol_reference_count || !ReadProcessMemory(child,(void *)symbol_reference_bases[index],raw,36,&bytes) || bytes!=36)return 0;
    used=sprintf(object_line,"symbol-reference-bytes %s %lu %08lx ",label,index,symbol_reference_bases[index]);
    for(i=0;i<36;++i)used+=sprintf(object_line+used,"%02x",raw[i]);object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int observe_symbol_reference(HANDLE child,HANDLE log,HANDLE thread,DWORD tid,DWORD site)
{
    CONTEXT c;DWORD args[5],locals[5],head,bytes,written,size,i,state[2];unsigned char instruction;int used;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(symbol_reference_step || !GetThreadContext(thread,&c) || c.Eip!=site+1)return 0;
    if(site==0x1d7d3) {
        if(symbol_reference_active || symbol_reference_count>=3 || !c.Eax ||
           !ReadProcessMemory(child,(void *)c.Esp,locals,20,&bytes) || bytes!=20 || locals[0]!=36 ||
           !ReadProcessMemory(child,(void *)(c.Esp+24),args,20,&bytes) || bytes!=20 || args[2]!=3 || args[4]!=0x400 ||
           args[0]<symbol_base || args[0]>symbol_base+608-38 ||
           !ReadProcessMemory(child,(void *)(args[1]+(c.Esi&0xffff)*4),&head,4,&bytes) || bytes!=4 ||
           !ReadProcessMemory(child,(void *)args[0],array_bytes,2,&bytes) || bytes!=2)return 0;
        size=(DWORD)array_bytes[0]+((DWORD)array_bytes[1]<<8)+2;
        if(size<38 || size>64 || args[0]+size>symbol_base+608 || !ReadProcessMemory(child,(void *)args[0],array_bytes,size,&bytes) || bytes!=size)return 0;
        if(!ReadProcessMemory(child,(void *)0x373c8,state,8,&bytes) || bytes!=8 || state[1]!=c.Eax+36)return 0;
        memcpy(symbol_reference_arguments,args,sizeof(args));symbol_reference_active=1;symbol_reference_hash_buckets[symbol_reference_count]=c.Esi&0xffff;symbol_reference_bases[symbol_reference_count++]=c.Eax;
        used=sprintf(object_line,"symbol-reference-reservation %lu %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx ",
            symbol_reference_count-1,c.Eax,c.Esp,c.Esi,args[0],args[1],args[2],args[3],args[4],locals[3],locals[2]>>16,locals[2]&0xffff,locals[4],head);
        for(i=0;i<size;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);object_line[used++]='\n';
        if(!WriteFile(log,object_line,used,&written,0) || written!=(DWORD)used || !symbol_reference_bytes(child,log,"birth",symbol_reference_count-1))return 0;
        sprintf(object_line,"symbol-reference-reserved-state %lu %lu %08lx\n",symbol_reference_count-1,state[0],state[1]);
        if(!WriteFile(log,object_line,strlen(object_line),&written,0) || written!=strlen(object_line))return 0;
        instruction=0x8b;symbol_reference_step=site+4;
    } else if(site==0x1d943) {
        if(!symbol_reference_active || !ReadProcessMemory(child,(void *)c.Esp,args,20,&bytes) || bytes!=20 ||
           memcmp(args,symbol_reference_arguments,20) || c.Eax!=symbol_reference_bases[symbol_reference_count-1] ||
           !symbol_reference_bytes(child,log,"return",symbol_reference_count-1))return 0;
        if(!ReadProcessMemory(child,(void *)(args[1]+symbol_reference_hash_buckets[symbol_reference_count-1]*4),&head,4,&bytes) || bytes!=4 || head!=c.Eax ||
           !ReadProcessMemory(child,(void *)0x373c8,state,8,&bytes) || bytes!=8 || state[1]!=c.Eax+36)return 0;
        sprintf(object_line,"symbol-reference-published %lu %08lx %08lx %08lx %lu %08lx\n",symbol_reference_count-1,c.Esp,c.Eax,head,state[0],state[1]);
        if(!WriteFile(log,object_line,strlen(object_line),&written,0) || written!=strlen(object_line))return 0;
        symbol_reference_active=0;
        if(symbol_reference_completion && symbol_reference_count==3) { symbol_reference_checkpoint=1;return 1; }
        instruction=0x83;symbol_reference_step=site+3;
    } else return 0;
    if(!WriteProcessMemory(child,(void *)site,&instruction,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)site,1);symbol_reference_site=site;symbol_reference_thread=tid;symbol_reference_tf=c.EFlags&0x100;c.Eip=site;c.EFlags|=0x100;
    return SetThreadContext(thread,&c);
}

static int rearm_symbol_reference(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;char line[96];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=symbol_reference_step || tid!=symbol_reference_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)symbol_reference_site,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)symbol_reference_site,1);
    sprintf(line,"symbol-reference-rearm %08lx %08lx\n",symbol_reference_site,address);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return 0;
    symbol_reference_step=0;c.EFlags=(c.EFlags&~0x100UL)|symbol_reference_tf;return SetThreadContext(thread,&c);
}

static int dump_symbol_references(HANDLE child,HANDLE log)
{
    DWORD i;if(symbol_reference_active || symbol_reference_step)return 0;
    for(i=0;i<symbol_reference_count;++i)if(!symbol_reference_bytes(child,log,"frontier",i))return 0;
    return 1;
}
