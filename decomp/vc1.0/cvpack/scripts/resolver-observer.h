/* Original debugger only: actual type resolver arguments and defined outputs. */
static DWORD resolver_enabled,resolver_initialized,resolver_sequence,resolver_depth;
static DWORD resolver_step,resolver_step_site,resolver_tf,resolver_thread;
typedef struct ResolverCall { DWORD sequence,esp,type,flag,returned; } ResolverCall;
static ResolverCall resolver_calls[32];

static int observe_resolver(HANDLE child,HANDLE original,HANDLE log,HANDLE thread,DWORD site,DWORD tid)
{
    CONTEXT c;DWORD words[3],bytes,written,table,relative;unsigned short value;
    unsigned char byte=0x83,cleanup[2],record[28];unsigned i;int used;char line[256];ResolverCall *call;
    if(site!=0x1e700 && (!resolver_depth || site!=resolver_calls[resolver_depth-1].returned))return 0;
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_FULL;
    if(resolver_step || !GetThreadContext(thread,&c) || c.Eip!=site+1 ||
       !ReadProcessMemory(child,(void *)c.Esp,words,sizeof(words),&bytes) || bytes!=sizeof(words))return -1;
    if(site==0x1e700) {
        if(resolver_depth==32 || resolver_sequence==256 || !words[2] ||
           !ReadProcessMemory(child,(void *)words[2],&value,2,&bytes) || bytes!=2 ||
           !ReadProcessMemory(child,(void *)(words[0]+1),cleanup,2,&bytes) || bytes!=2 ||
           cleanup[0]!=0xc4 || (cleanup[1]!=8 && cleanup[1]!=12))return -1;
        call=&resolver_calls[resolver_depth++];call->sequence=++resolver_sequence;
        call->esp=c.Esp;call->type=words[1];call->flag=words[2];call->returned=words[0];
        sprintf(line,"resolver-entry %lu %08lx %08lx %08lx %08lx %04x\n",call->sequence,c.Esp,words[1],words[2],words[0],value);
        if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return -1;
        if(!resolver_initialized) {
            if(restore_runtime(child,original,log,thread,site)!=1)return -1;
            resolver_initialized=1;
        }
        byte=0xcc;
        if(!WriteProcessMemory(child,(void *)call->returned,&byte,1,&bytes) || bytes!=1)return -1;
        FlushInstructionCache(child,(void *)call->returned,1);
        byte=0x83;
    } else {
        call=&resolver_calls[resolver_depth-1];
        if(c.Esp!=call->esp+4 || words[0]!=call->type || words[1]!=call->flag ||
           !ReadProcessMemory(child,(void *)call->flag,&value,2,&bytes) || bytes!=2 ||
           !ReadProcessMemory(child,(void *)0x373bc,&table,4,&bytes) || bytes!=4 ||
           c.Eax<table || (c.Eax-table)%28 || (c.Eax-table)/28>=12000)return -1;
        relative=(c.Eax-table)/28;
        if(!ReadProcessMemory(child,(void *)c.Eax,record,28,&bytes) || bytes!=28)return -1;
        used=sprintf(line,"resolver-return %lu %08lx %08lx %08lx %04x %lu ",call->sequence,c.Esp,c.Eax,call->flag,value,relative);
        for(i=0;i<28;++i)used+=sprintf(line+used,"%02x",record[i]);
        line[used++]='\n';line[used]=0;
        if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return -1;
        --resolver_depth;
    }
    if(!WriteProcessMemory(child,(void *)site,&byte,1,&bytes) || bytes!=1)return -1;
    FlushInstructionCache(child,(void *)site,1);
    resolver_step=site+3;resolver_step_site=site;resolver_thread=tid;resolver_tf=c.EFlags&0x100;
    c.Eip=site;c.EFlags|=0x100;
    return SetThreadContext(thread,&c)?1:-1;
}

static int rearm_resolver(HANDLE child,HANDLE log,HANDLE thread,DWORD address,DWORD tid)
{
    CONTEXT c;DWORD bytes,written;unsigned char trap=0xcc;char line[96];
    memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
    if(address!=resolver_step || tid!=resolver_thread || !GetThreadContext(thread,&c) || c.Eip!=address ||
       !WriteProcessMemory(child,(void *)resolver_step_site,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)resolver_step_site,1);
    sprintf(line,"resolver-rearm %08lx %08lx\n",resolver_step_site,address);
    if(!WriteFile(log,line,strlen(line),&written,0) || written!=strlen(line))return 0;
    resolver_step=0;c.EFlags=(c.EFlags&~0x100UL)|resolver_tf;
    return SetThreadContext(thread,&c);
}
