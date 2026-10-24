/* Original-only arena/vector allocation observations. No target calls/seeds. */
static DWORD vector_returns[3]={0x12beb,0x12bfc,0x12c0d};
static BYTE vector_return_saved;
static DWORD vector_return_address,vector_entry_esp,vector_count;
static int vector_return_active,signature_entry_active;
static DWORD vector_member_return,vector_member_esp;
static BYTE vector_member_saved;
static int vector_member_active;
static DWORD signature_entry;
static int signature_reference,signature_return_active,signature_reference_complete;
static DWORD signature_api_esp,signature_input;
static BYTE signature_return_saved;

static void prepare_vector_observer(void)
{
    FILE *file=fopen("vector-observers.txt","r");DWORD first,capacity,arena;
    BYTE byte;
    if(!file)return;
    if(fscanf(file,"%lx %lx %lx",&first,&capacity,&arena)!=3 || first!=0x11e3c || capacity!=40 || arena!=1 || fgetc(file)!='\n' || fgetc(file)!=EOF)
        fail("vector observer plan");
    fclose(file);vector_probe=1;
    if(!mspdb_base)fail("MSPDB loaded identity absent");
    readmem(base+0x5d1d4,&signature_entry,4);
    if(signature_entry!=mspdb_base+0xf22d || signature_entry!=exported(mspdb_base,"SigForPbCb"))fail("actual signature IAT/export");
    readmem(signature_entry,&byte,1);if(byte!=0x55)fail("signature entry ownership");
    save_remote_state("signature-provider-entry",0,signature_entry,3);
    file=fopen("signature-reference.txt","r");
    if(file) {
        DWORD entry,returned,frontier;unsigned i;
        if(restore_probe || baseline || fscanf(file,"%lx %lx %lx",&entry,&returned,&frontier)!=3 || entry!=0xf22d || returned!=0x12c31 || frontier!=0x12f9c || fgetc(file)!='\n' || fgetc(file)!=EOF)
            fail("signature reference plan");
        fclose(file);
        for(i=0;i<count;++i)if(ranges[i].first<=0x12f9c && 0x12f9c<ranges[i].last)fail("reference next_argument must remain masked");
        if(snapshot[0x12f9c-code_rva]!=0xa1 || snapshot[0x12c31-code_rva]!=0x83)fail("signature reference caller/stop ownership");
        signature_return_saved=snapshot[0x12c31-code_rva];signature_reference=1;
        save_remote_state("signature-provider-body",0,signature_entry,53);
        save_remote_state("signature-provider-table",0,mspdb_base+0x21a30,1024);
        fprintf(events,"signature-reference-plan accepted73=1 next_argument-masked=1 provider-execution-analysis-only=1\n");
    }
    patch(signature_entry,0xcc);signature_entry_active=1;
    fprintf(events,"vector-observer entry=00011e3c capacity=40 arena=1 header=12 items=160 signature-iat=0005d1d4 provider=0000f22d\n");
}

static void vector_record(const char *phase,DWORD address,CONTEXT *context)
{
    DWORD words[14],stack[3];char name[100];
    readmem(context->Esp,stack,sizeof(stack));
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
    words[8]=stack[0];words[9]=stack[1];words[10]=stack[2];words[11]=vector_entry_esp;
    words[12]=base;words[13]=vector_count;
    sprintf(name,"vector-%lu-%s-registers.bin",vector_count,phase);save_blob(name,words,sizeof(words));
    sprintf(name,"vector-%lu-%s-fp.bin",vector_count,phase);save_blob(name,&context->FloatSave,sizeof(context->FloatSave));
    sprintf(name,"vector-%lu-%s",vector_count,phase);save_frontend_state(name);
    fprintf(events,"vector phase=%s index=%lu seq=%u pc=%08lx esp=%08lx entry-esp=%08lx\n",phase,vector_count,sequence,address,context->Esp,vector_entry_esp);
}

static void vector_enter(CONTEXT *context)
{
    DWORD stack[3];
    if(!vector_probe)return;
    if(vector_count>=3 || vector_return_active)fail("vector entry inventory");
    readmem(context->Esp,stack,sizeof(stack));
    if(stack[0]!=base+vector_returns[vector_count] || stack[1]!=40 || stack[2]!=1)fail("actual vector input/caller");
    vector_started=1;vector_entry_esp=context->Esp;
    vector_record("entry",base+0x11e3c,context);
    vector_return_address=stack[0];vector_return_saved=snapshot[stack[0]-base-code_rva];
    if(vector_return_saved!=(vector_count==2?0xa3:0x83))fail("vector return instruction");
    patch(vector_return_address,0xcc);vector_return_active=1;
}

static void observe_vector_commit(CONTEXT *context,DWORD address,HANDLE thread)
{
    DWORD args[4],provider,cursor,reserve;MEMORY_BASIC_INFORMATION region;unsigned i;
    if(address==base+0x98b9) {
        readmem(context->Esp,args,sizeof(args));readmem(base+0x5d1c4,&provider,4);
        pool_commit_address=args[0];pool_commit_bytes=args[1];pool_commit_esp=context->Esp;pool_commit_error=thread_error(context,thread);
        fprintf(events,"vector-commit-before seq=%u esp=%08lx address=%08lx bytes=%08lx kind=%08lx protection=%08lx provider=%08lx error=%lu\n",sequence,context->Esp,args[0],args[1],args[2],args[3],provider,pool_commit_error);
    } else if(address==base+0x98bf) {
        if(context->Eax!=pool_commit_address || pool_commit_bytes!=0x8000)fail("actual vector commit result/size");
        fprintf(events,"vector-commit-after seq=%u esp=%08lx before-esp=%08lx result=%08lx error=%lu before-error=%lu\n",sequence,context->Esp,pool_commit_esp,context->Eax,thread_error(context,thread),pool_commit_error);
        readmem(base+0x58fb0+32+20,&reserve,4);cursor=pool_commit_address;
        for(i=0;i<2;++i) {
            if(VirtualQueryEx(child,(LPVOID)cursor,&region,sizeof(region))!=sizeof(region))fail("vector commit region");
            fprintf(events,"vector-commit-region part=%u base=%08lx allocation=%08lx allocation-protect=%08lx bytes=%08lx state=%08lx protect=%08lx type=%08lx\n",i,(DWORD)region.BaseAddress,(DWORD)region.AllocationBase,region.AllocationProtect,region.RegionSize,region.State,region.Protect,region.Type);
            cursor+=region.RegionSize;
        }
        if(cursor!=pool_commit_address+reserve)fail("vector reservation closure");
        save_remote_state("vector-commit-zero",0,pool_commit_address,pool_commit_bytes);
    }
}

static void vector_member_record(const char *phase,DWORD address,CONTEXT *context)
{
    DWORD words[12],caller;char name[100];
    readmem(context->Esp,&caller,4);
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ebx;words[5]=context->Esi;words[6]=context->Edi;words[7]=context->Ebp;
    words[8]=context->Ecx;words[9]=caller;words[10]=vector_member_esp;words[11]=vector_count;
    sprintf(name,"vector-member-%s-registers.bin",phase);save_blob(name,words,sizeof(words));
    sprintf(name,"vector-member-%s-fp.bin",phase);save_blob(name,&context->FloatSave,sizeof(context->FloatSave));
    sprintf(name,"vector-member-%s",phase);save_frontend_state(name);
    fprintf(events,"vector-member phase=%s seq=%u pc=%08lx esp=%08lx entry-esp=%08lx\n",phase,sequence,address,context->Esp,vector_member_esp);
}
static void vector_member_enter(CONTEXT *context)
{
    DWORD caller;
    if(!vector_probe || !vector_started)return;
    readmem(context->Esp,&caller,4);
    if(vector_count || vector_member_active || caller!=base+0x11ea2 || context->Ecx!=base+0x58fd0)fail("vector member ABI/inventory");
    vector_member_esp=context->Esp;vector_member_return=caller;vector_member_saved=snapshot[caller-base-code_rva];
    if(vector_member_saved!=0xeb)fail("member return opcode");
    vector_member_record("entry",base+0x9883,context);
    patch(caller,0xcc);vector_member_active=1;
}

static void signature_reference_record(const char *phase,DWORD address,CONTEXT *context,int at_return)
{
    DWORD words[16],arguments[3],arena[8];char name[100];
    readmem(context->Esp+(at_return?0:4),arguments,sizeof(arguments));
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=base+0x12c31;words[11]=mspdb_base;
    memcpy(words+12,arguments,sizeof(arguments));words[15]=context->EFlags;
    sprintf(name,"signature-reference-%s-registers.bin",phase);save_blob(name,words,sizeof(words));
    sprintf(name,"signature-reference-%s-input",phase);save_remote_state(name,0,signature_input,2);
    readmem(base+0x58fd0,arena,sizeof(arena));
    if(!arena[2] || arena[3]!=0x8000)fail("signature reference owned vector span");
    sprintf(name,"signature-reference-%s-vector",phase);save_remote_state(name,0,arena[2],arena[3]);
    fprintf(events,"signature-reference phase=%s seq=%u pc=%08lx esp=%08lx eax=%08lx arguments=%08lx,%08lx,%08lx\n",phase,sequence,address,context->Esp,context->Eax,arguments[0],arguments[1],arguments[2]);
}

static int vector_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD header[3],words[12],stack[4],arena[8];char name[100];
    if(!vector_probe)return 0;
    if(signature_return_active && address==base+0x12c31) {
        if(!signature_reference || signature_reference_complete || context->Esp!=signature_api_esp+4)fail("signature actual RET0/order");
        signature_reference_record("return",address,context,1);
        save_remote_state("signature-provider-table-after",0,mspdb_base+0x21a30,1024);
        locale_snapshot("signature-return",context,thread,tid);save_state();
        patch(address,signature_return_saved);signature_return_active=0;signature_reference_complete=1;
    } else if(vector_member_active && address==vector_member_return) {
        vector_member_record("return",address,context);save_state();
        patch(address,vector_member_saved);vector_member_active=0;
    } else if(vector_return_active && address==vector_return_address) {
        vector_record("return",address,context);save_state();
        readmem(context->Eax,header,sizeof(header));
        save_remote_state("vector-header",vector_count,context->Eax,12);
        save_remote_state("vector-items",vector_count,header[2],160);
        readmem(base+0x58fb0+32,arena,sizeof(arena));
        save_remote_state("vector-committed",vector_count,arena[2],arena[3]);
        patch(address,vector_return_saved);vector_return_active=0;++vector_count;
    } else if(signature_entry_active && address==signature_entry) {
        if(vector_count!=3 || vector_return_active || restore_probe)fail("signature entry branch/order");
        readmem(context->Esp,stack,sizeof(stack));
        if(stack[0]!=base+0x12c31 || stack[1]!=base+0x6318c || stack[2]!=1 || stack[3]!=0xffffffff)fail("actual signature cdecl inputs");
        words[0]=sequence;words[1]=address;words[2]=context->Esp;
        words[3]=context->Ebx;words[4]=context->Esi;words[5]=context->Edi;words[6]=context->Ebp;
        memcpy(words+7,stack,sizeof(stack));words[11]=mspdb_base;
        save_blob("signature-entry-registers.bin",words,sizeof(words));
        save_remote_state("signature-input",0,stack[1],2);
        if(signature_reference) {
            signature_api_esp=context->Esp;signature_input=stack[1];
            signature_reference_record("entry",address,context,0);
        }
        locale_snapshot("signature-entry",context,thread,tid);save_state();
        if(signature_reference) {
            patch(base+0x12c31,0xcc);signature_return_active=1;
            patch(address,0x55);signature_entry_active=0;
        } else {
            fprintf(events,"dependency-frontier module=MSPDB50.dll rva=0000f22d seq=%u\n",words[0]);fflush(events);
            TerminateProcess(child,125);ExitProcess(125);
        }
    } else return 0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("vector return context");return 1;
}
