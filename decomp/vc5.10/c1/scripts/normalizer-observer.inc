/* Original-only two real path calls and the reached public fullpath boundary. */
static unsigned normalize_calls,normalize_returns,fullpath_calls,fullpath_returns;
static int normalize_active,fullpath_active,resolver_returned;
static DWORD normalize_argument,normalize_esp,normalize_return,fullpath_entry,fullpath_esp;
static DWORD fullpath_arguments[3];
static BYTE normalize_saved,fullpath_saved,resolver_saved;
static DWORD packing_memory,packing_allocator_esp;
static int packing_allocator_active,packing_allocator_returned;

static void packing_allocation_record(const char *phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[16],arena[8];BYTE memory[8];char name[120],tag[100];
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;words[4]=context->Ecx;words[5]=context->Edx;
    words[6]=context->Ebx;words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    readmem(context->Esp,words+10,8);words[12]=packing_memory;words[13]=base;words[14]=packing_allocator_esp;words[15]=8;
    sprintf(name,"packing-allocation-%s-registers.bin",phase);save_blob(name,words,sizeof(words));
    readmem(base+0x58fb0,arena,sizeof(arena));
    if(!arena[2] || arena[3]!=0x10000 || packing_memory<arena[2] || packing_memory-arena[2]>arena[3]-8)fail("packing allocation owned8B");
    sprintf(name,"packing-allocation-%s-arena.bin",phase);save_blob(name,arena,sizeof(arena));
    readmem(packing_memory,memory,8);sprintf(name,"packing-allocation-%s-memory.bin",phase);save_blob(name,memory,sizeof(memory));
    sprintf(tag,"packing-allocation-%s",phase);locale_snapshot(tag,context,thread,tid);
}

static void packing_allocation_enter(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[2];
    if(option_action_frontier!=0xd458)return;
    if(address==base+0x134c9) {
        if(packing_allocator_active || packing_allocator_returned)fail("packing allocator repeats");
        readmem(context->Esp,words,8);if(words[0]!=base+0x13494 || words[1]!=8)fail("packing allocator actual ABI");
        readmem(base+0x58fb0,&packing_memory,4);packing_allocator_esp=context->Esp;
        if(snapshot[0x13494-code_rva]!=0x83)fail("packing allocator return byte");
        packing_allocator_active=1;packing_allocation_record("entry",address,context,thread,tid);patch(base+0x13494,0xcc);
    } else if(address==base+0xd458) {
        if(!packing_allocator_returned || packing_allocator_active || context->Ecx!=packing_memory)fail("packing actual receiver");
        readmem(context->Esp,words,4);if(words[0]!=base+0x134a3)fail("packing member actual caller");
        readmem(base+0x5e410,words,4);if(words[0]!=packing_memory)fail("packing published receiver");
        packing_allocation_record("member",address,context,thread,tid);
    }
}

static void normalizer_cwd(const char *phase,CONTEXT *context,HANDLE thread)
{
    LDT_ENTRY selector;DWORD values[6];WORD length,maximum;BYTE text[1042];char name[100];
    if(!GetThreadSelectorEntry(thread,context->SegFs,&selector))fail("fullpath TEB selector");
    values[0]=selector.BaseLow|((DWORD)selector.HighWord.Bytes.BaseMid<<16)|((DWORD)selector.HighWord.Bytes.BaseHi<<24);
    readmem(values[0]+0x30,values+1,4);readmem(values[1]+0x10,values+2,4);
    readmem(values[2]+0x24,values+3,12);memcpy(&length,values+3,2);memcpy(&maximum,(BYTE *)(values+3)+2,2);
    if(!length || length%2 || length>1040 || maximum<length+2)fail("actual PEB current directory bounds");
    readmem(values[4],text,length+2);if(text[length] || text[length+1])fail("actual PEB cwd terminator");
    sprintf(name,"fullpath-%s-cwd-identity.bin",phase);save_blob(name,values,sizeof(values));
    sprintf(name,"fullpath-%s-cwd.bin",phase);save_blob(name,text,length+2);
}

static void prepare_normalizer(CONTEXT *context,HANDLE thread,DWORD tid)
{
    BYTE code[32];
    if(!option_normalizer_probe || restore_probe || fullpath_entry)fail("normalizer preparation");
    readmem(base+0x5d040,&fullpath_entry,4);
    if(fullpath_entry!=crt_base+0x1dabe || fullpath_entry!=exported(crt_base,"_fullpath"))fail("canonical fullpath import/export");
    readmem(fullpath_entry,code,sizeof(code));if(code[0]!=0x53)fail("canonical fullpath first byte");
    fullpath_saved=code[0];save_blob("fullpath-provider-entry.bin",code,sizeof(code));patch(fullpath_entry,0xcc);
    resolver_saved=snapshot[0x12c8b-code_rva];if(resolver_saved!=0x83)fail("resolver return byte");patch(base+0x12c8b,0xcc);
}

static void normalizer_record(const char *phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[16];char tag[80],name[100],text[521];unsigned index=normalize_calls-1;
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;words[4]=context->Ecx;words[5]=context->Edx;
    words[6]=context->Ebx;words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=normalize_return;words[11]=normalize_argument;words[12]=index;words[13]=base;
    readmem(context->Esp+(!strcmp(phase,"entry")?4:0),words+14,4);words[15]=index?520:260;
    sprintf(name,"normalizer-%u-%s-registers.bin",index,phase);save_blob(name,words,sizeof(words));
    remote_string(normalize_argument,text,index?521:261);sprintf(name,"normalizer-%u-%s-path.bin",index,phase);save_blob(name,text,strlen(text)+1);
    sprintf(tag,"normalizer-%u-%s",index,phase);locale_snapshot(tag,context,thread,tid);
    fprintf(events,"normalizer phase=%s index=%u seq=%u pc=%08lx esp=%08lx\n",phase,index,sequence,address,context->Esp);
}

static void normalizer_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[2];
    if(!option_normalizer_probe)return;
    if(normalize_active || normalize_calls>=2)fail("normalizer call count/nesting");
    readmem(context->Esp,stack,sizeof(stack));
    if(stack[0]!=base+(normalize_calls?0x1343f:0x133c9))fail("normalizer actual caller");
    normalize_return=stack[0];normalize_argument=stack[1];normalize_esp=context->Esp;
    normalize_saved=snapshot[normalize_return-base-code_rva];if(normalize_saved!=(normalize_calls?0x83:0x0f))fail("normalizer return source byte");
    ++normalize_calls;normalize_active=1;patch(normalize_return,0xcc);normalizer_record("entry",base+0x1344d,context,thread,tid);
}

static void fullpath_record(const char *phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[18];BYTE buffer[520];char name[100],tag[80],input[40];
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;words[4]=context->Ecx;words[5]=context->Edx;
    words[6]=context->Ebx;words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=base+0x1342e;memcpy(words+11,fullpath_arguments,12);words[14]=crt_base;
    readmem(context->Esp+(!strcmp(phase,"entry")?4:0),words+15,12);
    sprintf(name,"fullpath-%s-registers.bin",phase);save_blob(name,words,sizeof(words));
    readmem(fullpath_arguments[0],buffer,sizeof(buffer));sprintf(name,"fullpath-%s-buffer.bin",phase);save_blob(name,buffer,sizeof(buffer));
    remote_string(fullpath_arguments[1],input,sizeof(input));if(strcmp(input,"vc50.pdb"))fail("fullpath actual known input");
    sprintf(name,"fullpath-%s-input.bin",phase);save_blob(name,input,strlen(input)+1);
    sprintf(tag,"fullpath-%s",phase);locale_snapshot(tag,context,thread,tid);normalizer_cwd(phase,context,thread);
    fprintf(events,"fullpath phase=%s seq=%u pc=%08lx esp=%08lx result=%08lx\n",phase,sequence,address,context->Esp,context->Eax);
}

static int normalizer_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[4];
    if(!option_normalizer_probe)return 0;
    if(option_action_frontier==0xd458 && packing_allocator_active && address==base+0x13494) {
        if(context->Esp!=packing_allocator_esp+4 || context->Eax!=packing_memory)fail("packing allocator real RET0/result");
        packing_allocation_record("return",address,context,thread,tid);save_state();patch(address,snapshot[0x13494-code_rva]);
        packing_allocator_active=0;packing_allocator_returned=1;
    } else if(fullpath_entry && address==fullpath_entry) {
        if(fullpath_calls || normalize_calls!=1 || normalize_returns!=1 || normalize_active)fail("fullpath actual ordering");
        readmem(context->Esp,stack,sizeof(stack));
        if(stack[0]!=base+0x1342e || stack[3]!=520)fail("fullpath true cdecl input/caller");
        memcpy(fullpath_arguments,stack+1,12);fullpath_esp=context->Esp;fullpath_active=1;++fullpath_calls;
        fullpath_record("entry",address,context,thread,tid);save_state();patch(base+0x1342e,0xcc);patch(address,fullpath_saved);
    } else if(fullpath_active && address==base+0x1342e) {
        if(context->Esp!=fullpath_esp+4)fail("fullpath RET0");
        fullpath_record("return",address,context,thread,tid);save_state();patch(address,snapshot[0x1342e-code_rva]);
        fullpath_active=0;++fullpath_returns;
    } else if(normalize_active && address==normalize_return) {
        if(context->Esp!=normalize_esp+4)fail("normalizer RET0");
        normalizer_record("return",address,context,thread,tid);save_state();patch(address,normalize_saved);normalize_active=0;++normalize_returns;
    } else if(resolver_entered && !resolver_returned && address==base+0x12c8b) {
        if(context->Esp!=resolver_esp+4 || normalize_returns!=2 || fullpath_returns!=1)fail("resolver real return/closure");
        stack[0]=sequence;stack[1]=context->Esp;stack[2]=context->Eax;stack[3]=base;
        save_blob("resolver-return-registers.bin",stack,sizeof(stack));locale_snapshot("resolver-return",context,thread,tid);save_state();
        patch(address,resolver_saved);resolver_returned=1;
    } else return 0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("normalizer observer EIP");return 1;
}

static void check_normalizer_stop(DWORD address)
{
    if(option_normalizer_probe && (address!=base+option_action_frontier || normalize_calls!=2 || normalize_returns!=2 || fullpath_calls!=1 || fullpath_returns!=1 || !resolver_returned || normalize_active || fullpath_active))
        fail("whole normalizer/fullpath/resolver closure");
}
