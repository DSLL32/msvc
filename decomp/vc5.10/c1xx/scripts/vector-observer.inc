/* Original-only vector/arena1 observations, following C1's transport design.
 * This target's addresses, 32KiB commit and CPP/3 input are independent. */
static DWORD vector_returns[3]={0x39059,0x3906a,0x3907b};
static DWORD vector_count,vector_entry_esp,vector_return_address;
static DWORD vector_member_esp,vector_member_return,signature_entry;
static BYTE vector_return_saved,vector_member_saved;
static int vector_return_active,vector_member_active,signature_entry_active;
static DWORD argument_entry_esp;
static BYTE argument_return_saved;
static int argument_active;

static void argument_record(const char *phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[12];char name[100];
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
    readmem(context->Esp,words+8,4);words[9]=argument_entry_esp;words[10]=base;words[11]=0;
    sprintf(name,"argument-%s-registers.bin",phase);save_blob(name,words,sizeof(words));
    sprintf(name,"argument-%s",phase);locale_snapshot(name,context,thread,tid);
    fprintf(events,"argument-observer phase=%s seq=%u pc=%08lx esp=%08lx\n",phase,sequence,address,context->Esp);
}

static void argument_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD caller;
    if(!getenv("C1XX_OBSERVE_ARGUMENT"))return;
    readmem(context->Esp,&caller,4);
    if(!getenv("C1XX_TEST_INIT_RESTORE") || argument_active || vector_count!=3 || caller!=base+0x390b3)fail("argument restore caller/order");
    argument_entry_esp=context->Esp;argument_return_saved=snapshot[0x390b3-code_rva];
    if(argument_return_saved!=0x50)fail("argument actual caller instruction");
    argument_record("entry",base+0x39428,context,thread,tid);
    patch(caller,0xcc);argument_active=1;
}


static void vector_remote(const char *name,DWORD address,DWORD bytes)
{
    BYTE *data=(BYTE *)malloc(bytes);if(!data)fail("vector observer allocation");
    readmem(address,data,bytes);save_blob(name,data,bytes);free(data);
}

static void prepare_vector_observer(void)
{
    BYTE first,entry[53];
    if(!getenv("C1XX_OBSERVE_VECTORS") || baseline)return;
    if(!mspdb_base)fail("vector MSPDB actual identity absent");
    readmem(base+0xc9234,&signature_entry,4);
    if(signature_entry!=mspdb_base+0xf22d)fail("vector actual SigForPbCb IAT");
    readmem(signature_entry,entry,sizeof(entry));save_blob("signature-provider-entry.bin",entry,sizeof(entry));
    first=entry[0];if(first!=0x55)fail("signature entry original byte");
    patch(signature_entry,0xcc);signature_entry_active=1;vector_probe=1;
    fprintf(events,"vector-observer entry=0000682f capacity=40 arena=1 header=12 items=160 signature-iat=000c9234 provider=0000f22d commit=32768\n");
}

static void vector_record(const char *phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[14],stack[3],arena[8];char name[100],label[64];
    readmem(context->Esp,stack,sizeof(stack));readmem(base+0xc6520,arena,sizeof(arena));
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
    words[8]=stack[0];words[9]=stack[1];words[10]=stack[2];words[11]=vector_entry_esp;
    words[12]=base;words[13]=vector_count;
    sprintf(name,"vector-%lu-%s-registers.bin",vector_count,phase);save_blob(name,words,sizeof(words));
    sprintf(label,"vector-%lu-%s",vector_count,phase);locale_snapshot(label,context,thread,tid);
    sprintf(name,"vector-%lu-%s-arena.bin",vector_count,phase);save_blob(name,arena,sizeof(arena));
    fprintf(events,"vector phase=%s index=%lu seq=%u pc=%08lx esp=%08lx entry-esp=%08lx\n",phase,vector_count,sequence,address,context->Esp,vector_entry_esp);
}

static void vector_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[3];
    if(!vector_probe)return;
    if(vector_count>=3 || vector_return_active)fail("vector entry inventory");
    readmem(context->Esp,stack,sizeof(stack));
    if(stack[0]!=base+vector_returns[vector_count] || stack[1]!=40 || stack[2]!=1)fail("vector actual input/caller");
    vector_started=1;vector_entry_esp=context->Esp;vector_record("entry",base+0x682f,context,thread,tid);
    vector_return_address=stack[0];vector_return_saved=snapshot[stack[0]-base-code_rva];
    if(vector_return_saved!=(vector_count==2?0xa3:0x83))fail("vector caller return opcode");
    patch(vector_return_address,0xcc);vector_return_active=1;
}

static void vector_member_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD caller,words[12];
    if(!vector_started)return;
    readmem(context->Esp,&caller,4);
    if(vector_count || vector_member_active || caller!=base+0x6895 || context->Ecx!=base+0xc6520)fail("vector commit member actual ABI");
    vector_member_esp=context->Esp;vector_member_return=caller;vector_member_saved=snapshot[caller-base-code_rva];
    if(vector_member_saved!=0xeb)fail("vector member return instruction");
    words[0]=sequence;words[1]=base+0x1d738;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
    words[8]=context->Ecx;words[9]=caller;words[10]=vector_member_esp;words[11]=vector_count;
    save_blob("vector-member-entry-registers.bin",words,sizeof(words));locale_snapshot("vector-member-entry",context,thread,tid);
    patch(caller,0xcc);vector_member_active=1;
}

static void observe_vector_commit(CONTEXT *context,DWORD address,HANDLE thread)
{
    DWORD args[4],provider,cursor,reserve;MEMORY_BASIC_INFORMATION region;unsigned i;
    if(address==base+0x1d76e) {
        readmem(context->Esp,args,sizeof(args));readmem(base+0xc9204,&provider,4);
        vector_commit_address=args[0];vector_commit_bytes=args[1];vector_commit_esp=context->Esp;
        fprintf(events,"vector-commit-before seq=%u esp=%08lx address=%08lx bytes=%08lx kind=%08lx protection=%08lx provider=%08lx error=%lu\n",sequence,context->Esp,args[0],args[1],args[2],args[3],provider,thread_error(context,thread));
    } else if(address==base+0x1d774) {
        if(context->Eax!=vector_commit_address || vector_commit_bytes!=0x8000 || context->Esp!=vector_commit_esp+16)fail("vector commit actual result/size/RET16");
        fprintf(events,"vector-commit-after seq=%u esp=%08lx before-esp=%08lx result=%08lx error=%lu\n",sequence,context->Esp,vector_commit_esp,context->Eax,thread_error(context,thread));
        readmem(base+0xc6534,&reserve,4);cursor=vector_commit_address;
        for(i=0;i<2;++i) {
            if(VirtualQueryEx(child,(LPVOID)cursor,&region,sizeof(region))!=sizeof(region))fail("vector commit region");
            fprintf(events,"vector-commit-region part=%u base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n",i,(DWORD)region.BaseAddress,(DWORD)region.AllocationBase,region.AllocationProtect,region.RegionSize,region.State,region.Protect,region.Type);cursor+=region.RegionSize;
        }
        if(cursor!=vector_commit_address+reserve)fail("vector reserve closure");
        vector_remote("vector-commit-zero.bin",vector_commit_address,vector_commit_bytes);
    }
}

static int vector_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD header[3],arena[8],words[12],stack[4];char name[100];
    if(!vector_probe)return 0;
    if(argument_active && address==base+0x390b3) {
        BYTE result[256];DWORD length=0;
        if(context->Esp!=argument_entry_esp+4)fail("argument actual RET0");
        argument_record("return",address,context,thread,tid);
        do {if(length==sizeof(result))fail("argument result unbounded");readmem(context->Eax+length,result+length,1);}while(result[length++]);
        save_blob("argument-result.bin",result,length);patch(address,argument_return_saved);argument_active=0;
    } else if(vector_member_active && address==vector_member_return) {
        words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
        words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
        words[8]=context->Ecx;readmem(context->Esp,words+9,4);words[10]=vector_member_esp;words[11]=vector_count;
        save_blob("vector-member-return-registers.bin",words,sizeof(words));locale_snapshot("vector-member-return",context,thread,tid);
        patch(address,vector_member_saved);vector_member_active=0;
    } else if(vector_return_active && address==vector_return_address) {
        vector_record("return",address,context,thread,tid);readmem(context->Eax,header,sizeof(header));
        sprintf(name,"vector-%lu-header.bin",vector_count);vector_remote(name,context->Eax,12);
        sprintf(name,"vector-%lu-items.bin",vector_count);vector_remote(name,header[2],160);
        readmem(base+0xc6520,arena,sizeof(arena));sprintf(name,"vector-%lu-committed.bin",vector_count);vector_remote(name,arena[2],arena[3]);
        patch(address,vector_return_saved);vector_return_active=0;++vector_count;
    } else if(signature_entry_active && address==signature_entry) {
        if(vector_count!=3 || vector_return_active || getenv("C1XX_TEST_INIT_RESTORE"))fail("signature entry branch/order");
        readmem(context->Esp,stack,sizeof(stack));
        if(stack[0]!=base+0x3909f || stack[1]!=base+0xd6620 || stack[2]!=3 || stack[3]!=0xffffffff)fail("actual CPP signature cdecl inputs");
        words[0]=sequence;words[1]=address;words[2]=context->Esp;
        words[3]=context->Ebx;words[4]=context->Esi;words[5]=context->Edi;words[6]=context->Ebp;
        memcpy(words+7,stack,sizeof(stack));words[11]=mspdb_base;save_blob("signature-entry-registers.bin",words,sizeof(words));
        vector_remote("signature-input.bin",stack[1],4);locale_snapshot("signature-entry",context,thread,tid);save_state();
        fprintf(events,"dependency-frontier module=MSPDB50.dll rva=0000f22d seq=%u\n",words[0]);fflush(events);
        TerminateProcess(child,125);ExitProcess(125);
    } else return 0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("vector return context");return 1;
}
