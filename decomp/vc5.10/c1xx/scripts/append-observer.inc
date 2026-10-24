/* Original-only exact append calls and genuine resolver input observation. */
static unsigned append_calls,append_returns;
static int append_active;
static DWORD append_esp,append_return,append_vector,append_value;
static BYTE append_saved;

static void append_record(const char *phase,DWORD address,CONTEXT *context,
                          HANDLE thread,DWORD tid)
{
    DWORD words[18],header[3],arena[8];char name[100],tag[80],value[1024];
    unsigned index=append_calls-1;
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=append_return;words[11]=append_vector;words[12]=append_value;
    words[13]=index;words[14]=base;words[15]=context->EFlags;
    readmem(context->Esp+(!strcmp(phase,"entry")?4:0),words+16,8);
    sprintf(name,"append-%u-%s-registers.bin",index,phase);save_blob(name,words,sizeof(words));
    remote_string(append_value,value,sizeof(value));
    sprintf(name,"append-%u-%s-input.bin",index,phase);save_blob(name,value,strlen(value)+1);
    readmem(append_vector,header,sizeof(header));
    if((header[0]>>16)!=40 || header[1]!=1)fail("append observed header capacity/arena");
    sprintf(name,"append-%u-%s-header.bin",index,phase);save_blob(name,header,sizeof(header));
    sprintf(name,"append-%u-%s-items.bin",index,phase);vector_remote(name,header[2],160);
    readmem(base+0xc6520,arena,sizeof(arena));if(arena[3]!=32768)fail("append observed owned extent");
    sprintf(name,"append-%u-%s-committed.bin",index,phase);vector_remote(name,arena[2],32768);
    sprintf(tag,"append-%u-%s",index,phase);locale_snapshot(tag,context,thread,tid);
    fprintf(events,"append-call phase=%s index=%u seq=%u pc=%08lx esp=%08lx\n",phase,index,sequence,address,context->Esp);
}

static void append_enter(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[3],root,header[3],arena[8];char value[1024];
    if(!option_append_probe)return;
    if(append_active || append_calls>=6)fail("append count/nesting");
    readmem(context->Esp,stack,sizeof(stack));
    readmem(base+(append_calls<5?0xc06c4:0xbf6f0),&root,4);
    if(stack[0]!=base+0x396bc || stack[1]!=root || !stack[2])fail("append actual caller/owner/value");
    append_esp=context->Esp;append_return=stack[0];append_vector=stack[1];append_value=stack[2];
    append_saved=snapshot[0x396bc-code_rva];if(append_saved!=0x83)fail("append caller ADD8 byte");
    if(!append_calls) {
        /* This is the same actual first pre-body checkpoint as the previous
         * pending frontier; later calls never overwrite its prefix aliases. */
        action_registers("append-entry",0,address,context,thread,tid);
        remote_string(append_value,value,sizeof(value));save_blob("suffix-append-input.bin",value,strlen(value)+1);
        readmem(append_vector,header,sizeof(header));save_blob("suffix-append-header.bin",header,sizeof(header));
        vector_remote("suffix-append-items.bin",header[2],160);
        readmem(base+0xc6520,arena,sizeof(arena));if(arena[3]!=32768)fail("append prefix owned extent");
        vector_remote("suffix-append-committed.bin",arena[2],32768);
    }
    ++append_calls;append_active=1;append_record("entry",address,context,thread,tid);
    patch(append_return,0xcc);
}

static int append_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    if(!option_append_probe || !append_active || address!=append_return)return 0;
    if(context->Esp!=append_esp+4)fail("append actual RET0");
    append_record("return",address,context,thread,tid);save_state();
    patch(address,append_saved);append_active=0;++append_returns;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("append return observer EIP");
    return 1;
}

static void check_append_stop(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[4],input,arena[8];char value[100];
    if(!option_append_probe)return;
    if(address!=base+0x397db || append_calls!=6 || append_returns!=6 || append_active)
        fail("complete append/resolver inventory");
    readmem(context->Esp,stack,sizeof(stack));readmem(base+0xcd534,&input,4);
    if(stack[0]!=base+0x390f9 || stack[1]!=context->Esp+32 || stack[2]!=input || stack[3]!=520)
        fail("resolver actual caller/output/input/capacity");
    remote_string(input,value,sizeof(value));if(strcmp(value,"vc50.pdb"))fail("resolver actual input string");
    save_blob("append-resolver-input.bin",value,strlen(value)+1);
    action_registers("resolver-entry",0,address,context,thread,tid);
    readmem(base+0xc6520,arena,sizeof(arena));if(arena[3]!=32768)fail("resolver owned extent");
    vector_remote("append-final-committed.bin",arena[2],32768);
    fprintf(events,"append-complete calls=6 returned=6 actual-frontier=000397db output=uninitialized-caller-stack520\n");
}
