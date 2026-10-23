/* Original-debugger transport only. Exact current type-storage lifetime. */
static DWORD buffer_base,buffer_node,buffer_sequence,buffer_pending,buffer_initialized;
static DWORD buffer_step,buffer_step_site,buffer_tf,buffer_thread;
static unsigned char buffer_before[0x7f00];

static int buffer_hex(HANDLE child,HANDLE log,const char *label,DWORD address,DWORD size)
{
    DWORD bytes,written,i;int used;
    if(!address || size>sizeof(array_bytes) || !ReadProcessMemory(child,(void *)address,array_bytes,size,&bytes) || bytes!=size)return 0;
    used=sprintf(object_line,"buffer-bytes %s %08lx ",label,address);
    for(i=0;i<size;++i)used+=sprintf(object_line+used,"%02x",array_bytes[i]);
    object_line[used++]='\n';
    return WriteFile(log,object_line,used,&written,0) && written==(DWORD)used;
}

static int observe_buffer(HANDLE child,HANDLE original,HANDLE log,HANDLE thread,DWORD site,DWORD tid)
{
    CONTEXT c;DWORD words[2],raw[6],bytes,written,request;unsigned char instruction;
    char line[256];
    if(site!=0x22f40 && site!=0x22f6d && site!=0x19a65)return 0;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(buffer_step || !GetThreadContext(thread,&c) || c.Eip!=site+1)return -1;
    if(site==0x22f40) {
        if(buffer_pending || buffer_sequence>=6 || c.Ecx!=0x3301c ||
            !ReadProcessMemory(child,(void *)c.Esp,words,8,&bytes) || bytes!=8 || words[0]!=0x19a65 ||
            (words[1]!=8 && words[1]!=12 && words[1]!=16) ||
            !ReadProcessMemory(child,(void *)c.Ecx,raw,24,&bytes) || bytes!=24 || raw[0]!=0x7f00)return -1;
        ++buffer_sequence;buffer_pending=1;
        sprintf(line,"buffer-entry %lu %08lx %08lx %lu %08lx %08lx %08lx\n",buffer_sequence,c.Ecx,c.Edx,words[1],c.Ebx,c.Esp,words[0]);
        if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) ||
            !buffer_hex(child,log,"entry-root",c.Ecx,24) || !buffer_hex(child,log,"source",c.Edx,words[1]))return -1;
        if(!buffer_initialized) {
            if(restore_runtime(child,original,log,thread,site)!=1)return -1;
            buffer_initialized=1;
        }
        instruction=0x53;buffer_step=site+1;
    } else if(site==0x22f6d) {
        if(!buffer_pending || buffer_sequence!=1 || buffer_base || !c.Eax || !c.Edi || c.Ebp!=0x3301c ||
            !ReadProcessMemory(child,(void *)c.Esp,&request,4,&bytes) || bytes!=4 || request!=0x7f00 ||
            !ReadProcessMemory(child,(void *)c.Edi,raw,12,&bytes) || bytes!=12 || raw[0] || raw[1] || raw[2] ||
            !ReadProcessMemory(child,(void *)c.Eax,buffer_before,sizeof(buffer_before),&bytes) || bytes!=sizeof(buffer_before))return -1;
        buffer_base=c.Eax;buffer_node=c.Edi;
        sprintf(line,"buffer-allocation %08lx %08lx %lu %08lx %08lx\n",buffer_base,buffer_node,request,c.Ebp,c.Esp);
        if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) ||
            !buffer_hex(child,log,"birth-payload",buffer_base,request) || !buffer_hex(child,log,"birth-node",buffer_node,12) ||
            !buffer_hex(child,log,"birth-root",c.Ebp,24))return -1;
        instruction=0x83;buffer_step=site+3;
    } else {
        if(!buffer_pending || !buffer_base || !buffer_node || !c.Eax)return -1;
        sprintf(line,"buffer-return %lu %08lx %08lx\n",buffer_sequence,c.Eax,c.Esp);
        if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line) ||
            !buffer_hex(child,log,"return-root",0x3301c,24) || !buffer_hex(child,log,"return-node",buffer_node,12) ||
            !buffer_hex(child,log,"return-payload",buffer_base,sizeof(buffer_before)))return -1;
        buffer_pending=0;instruction=0x8b;buffer_step=site+2;
    }
    buffer_step_site=site;buffer_thread=tid;buffer_tf=c.EFlags&0x100;
    if(!WriteProcessMemory(child,(void *)site,&instruction,1,&bytes) || bytes!=1)return -1;
    FlushInstructionCache(child,(void *)site,1);c.Eip=site;c.EFlags|=0x100;
    return SetThreadContext(thread,&c)?1:-1;
}

static int rearm_buffer(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;char line[128];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(address!=buffer_step || tid!=buffer_thread || !GetThreadContext(thread,&c) || c.Eip!=buffer_step ||
        !WriteProcessMemory(child,(void *)buffer_step_site,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)buffer_step_site,1);
    sprintf(line,"buffer-rearm %08lx %08lx\n",buffer_step_site,buffer_step);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return 0;
    c.EFlags=(c.EFlags&~0x100UL)|buffer_tf;buffer_step=0;
    return SetThreadContext(thread,&c);
}

static int dump_buffer(HANDLE child,HANDLE log,DWORD frontier)
{
    DWORD bytes,pointer,record[3],bucket,j,total=0,seen[6];char label[64];
    if(!buffer_initialized)return 1;
    if((frontier!=0x262d0 && frontier!=0x1ad40 && frontier!=0x1af10) || buffer_step || buffer_pending || buffer_sequence!=(frontier==0x262d0?3:6) ||
        !buffer_hex(child,log,"frontier-root",0x3301c,24) || !buffer_hex(child,log,"frontier-node",buffer_node,12) ||
        !buffer_hex(child,log,"frontier-payload",buffer_base,sizeof(buffer_before)))return 0;
    if(!ReadProcessMemory(child,(void *)0x37170,&pointer,4,&bytes) || bytes!=4 ||
        !ReadProcessMemory(child,(void *)pointer,walk_bytes,16384,&bytes) || bytes!=16384)return 0;
    for(bucket=0;bucket<4096;++bucket) {
        memcpy(&pointer,walk_bytes+bucket*4,4);
        while(pointer) {
            if(total==6)return 0;
            for(j=0;j<total;++j)if(seen[j]==pointer)return 0;
            seen[total++]=pointer;
            if(!ReadProcessMemory(child,(void *)pointer,record,12,&bytes) || bytes!=12)return 0;
            sprintf(label,"hash-%lu-%u",bucket,(unsigned)(record[0]&0xffff));
            if(!buffer_hex(child,log,label,pointer,12))return 0;
            pointer=record[2];
        }
    }
    return total==(frontier==0x262d0?3:6);
}
