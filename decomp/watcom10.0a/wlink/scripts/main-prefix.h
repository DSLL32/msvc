/* Target-specific first WLINK application prefix, included by the established
 * debugger helper. This is test transport, not reconstructed implementation.
 * Entire executable section is masked at actual main; only main is restored. */
#define PREFIX_CODE_BASE 0x410000UL
#define PREFIX_CODE_SIZE 0x29800UL
static BYTE prefix_source[PREFIX_CODE_SIZE];
static BYTE prefix_expected[PREFIX_CODE_SIZE];
static BYTE prefix_actual[PREFIX_CODE_SIZE];
static DWORD prefix_main_esp, prefix_argv;
static int prefix_masked;
static const DWORD prefix_initializers[5]={0x4365ce,0x4376ce,0x43776d,0x437274,0x4364d9};
static const BYTE prefix_priorities[5]={32,32,32,32,2};
static unsigned prefix_seen[5];

static int prefix_write(DWORD address, const BYTE *data, DWORD size)
{
    DWORD old,ignored,wrote;
    int result;
    if(!VirtualProtectEx(child,(void *)address,size,PAGE_EXECUTE_READWRITE,&old)) return 0;
    result=WriteProcessMemory(child,(void *)address,data,size,&wrote) && wrote==size;
    if(!FlushInstructionCache(child,(void *)address,size)) result=0;
    if(!VirtualProtectEx(child,(void *)address,size,old,&ignored)) result=0;
    return result;
}

static int prefix_source_file(const char *path)
{
    FILE *f;
    long size;
    int result;
    if(configured_base!=0x400000 || main_rva!=0x242e0 || entry_rva!=0x34656 || main_kind!=1) return 0;
    f=fopen(path,"rb"); if(!f) return 0;
    result=!fseek(f,0,SEEK_END);
    size=ftell(f);
    if(size!=209744 || fseek(f,0x400,SEEK_SET)) result=0;
    if(result && fread(prefix_source,1,PREFIX_CODE_SIZE,f)!=PREFIX_CODE_SIZE) result=0;
    fclose(f);
    /* The broker binds the full unchanged GS-compatible source file hash.
     * Later compare every loaded executable byte against these exact bytes. */
    return result;
}

static int prefix_table(unsigned expected_type)
{
    BYTE table[30];
    DWORD pointer;
    unsigned i;
    if(!memory(0x4427d4,table,sizeof(table))) return 0;
    for(i=0;i<5;++i) {
        memcpy(&pointer,table+i*6+2,4);
        if(table[i*6]!=expected_type || table[i*6+1]!=prefix_priorities[i] || pointer!=prefix_initializers[i]) return 0;
    }
    return 1;
}

static int prefix_pins(void)
{
    static const DWORD slots[4]={0x442614,0x4425dc,0x4425f8,0x442600};
    static const DWORD values[4]={0x4342d7,0x435813,0x435818,0x435818};
    DWORD value;
    unsigned i;
    for(i=0;i<4;++i) if(!memory(slots[i],&value,4) || value!=values[i]) return 0;
    return 1;
}

static int prefix_arm_initializers(void)
{
    unsigned i;
    BYTE byte;
    if(!prefix_table(0) || !prefix_pins()) return 0;
    for(i=0;i<5;++i) {
        if(!memory(prefix_initializers[i],&byte,1) || byte!=prefix_source[prefix_initializers[i]-PREFIX_CODE_BASE] || !patch(prefix_initializers[i],0xcc)) return 0;
    }
    return 1;
}

static int prefix_initializer(DWORD address, CONTEXT *c)
{
    unsigned i;
    for(i=0;i<5;++i) if(prefix_initializers[i]==address) break;
    if(i==5) return 0;
    if(!entry_seen || main_seen || prefix_seen[i] || !at_break(address,c) || !patch(address,prefix_source[address-PREFIX_CODE_BASE])) return -1;
    prefix_seen[i]=1;
    fprintf(events,"CRT-bootstrap-initializer index=%u address=%08lx\n",i,address);
    c->Eip=address;
    return SetThreadContext(primary,c)?1:-1;
}

static int prefix_application_state(int entered)
{
    static const DWORD addresses[5]={0x45015c,0x450164,0x450160,0x4500e8,0x45021c};
    static const char *names[5]={"MapFile","MapFName","OutFiles","LibPath","Argv"};
    DWORD value;
    WORD state;
    unsigned i;
    for(i=0;i<5;++i) {
        if(!memory(addresses[i],&value,4)) return 0;
        fprintf(events,"app-state stage=%s name=%s value=%08lx\n",entered?"Spawn":"main",names[i],value);
        if(value!=((i==4 && entered)?prefix_argv:0)) return 0;
    }
    if(!memory(0x450186,&state,2)) return 0;
    fprintf(events,"app-state stage=%s name=LinkState value=%04x\n",entered?"Spawn":"main",(unsigned)state);
    return state==0;
}

static int prefix_at_main(CONTEXT *c)
{
    DWORD caller;
    unsigned i;
    if(depth || step.active || !prefix_table(2) || !prefix_pins() || !memory(c->Esp,&caller,4) || caller!=0x43819e) return 0;
    for(i=0;i<5;++i) if(!prefix_seen[i]) return 0;
    prefix_main_esp=c->Esp; prefix_argv=c->Edx;
    if(c->Eax!=2 || !prefix_application_state(0)) return 0;
    /* Main's known observer byte has already been restored. Every remaining
     * debugger modification must be exactly one owned IAT-thunk INT3. */
    memcpy(prefix_expected,prefix_source,PREFIX_CODE_SIZE);
    for(i=0;i<thunk_count;++i) {
        if(!thunks[i].armed || thunks[i].address<PREFIX_CODE_BASE || thunks[i].address>=PREFIX_CODE_BASE+PREFIX_CODE_SIZE) return 0;
        prefix_expected[thunks[i].address-PREFIX_CODE_BASE]=0xcc;
    }
    if(!memory(PREFIX_CODE_BASE,prefix_actual,PREFIX_CODE_SIZE) || memcmp(prefix_actual,prefix_expected,PREFIX_CODE_SIZE)) return 0;
    fprintf(events,"bootstrap-code-preimage verified-bytes=%lu main-caller=%08lx\n",PREFIX_CODE_SIZE,caller);
    memset(prefix_expected,0xcc,PREFIX_CODE_SIZE);
    memcpy(prefix_expected+0x142e0,prefix_source+0x142e0,232);
    if(!prefix_write(PREFIX_CODE_BASE,prefix_expected,PREFIX_CODE_SIZE) || !memory(PREFIX_CODE_BASE,prefix_actual,PREFIX_CODE_SIZE) || memcmp(prefix_actual,prefix_expected,PREFIX_CODE_SIZE)) return 0;
    prefix_masked=1;
    fprintf(events,"whole-code-mask verified-bytes=%lu restored-main=004242e0..004243c8 restored-CRT=0\n",PREFIX_CODE_SIZE);
    return 1;
}

static int prefix_spawn(CONTEXT *c)
{
    DWORD caller;
    if(!prefix_masked || !at_break(0x416d3c,c) || c->Eax!=0x4243c8 || c->Esp!=prefix_main_esp-4 || !memory(c->Esp,&caller,4) || caller!=0x4242f0 || depth || step.active || !prefix_application_state(1)) return 0;
    if(!memory(PREFIX_CODE_BASE,prefix_actual,PREFIX_CODE_SIZE) || memcmp(prefix_actual,prefix_expected,PREFIX_CODE_SIZE)) return 0;
    log_context("frontier-Spawn",c);
    fprintf(events,"matched-first-frontier name=Spawn address=00416d3c caller=%08lx callback=%08lx argv=%08lx whole-mask-verified=%lu\n",caller,c->Eax,prefix_argv,PREFIX_CODE_SIZE);
    return 1;
}
