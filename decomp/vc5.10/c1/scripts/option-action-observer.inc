/* Original-only complete ordered option/action/callback/public-call inventory.
 * No target value is seeded here and every mutable point is read afresh. */
static int option_integer_probe;
static void integer_enter(CONTEXT *,HANDLE,DWORD);
static unsigned action_matches, action_calls, action_returns;
static unsigned action_outer_calls, action_outer_returns;
static unsigned action_inner_calls, action_inner_returns;
static unsigned action_api_calls, action_api_returns;
static DWORD action_match_esp, action_handler_esp, action_next_esp;
static DWORD action_match_return, action_handler_return, action_next_return;
static BYTE action_match_saved, action_handler_saved, action_next_saved;
static int action_match_active, action_handler_active, action_next_active;
static unsigned action_next_kind;
static DWORD action_api_esp, action_api_return, action_api_input, action_api_length;
static BYTE action_api_saved;
static int action_api_active;

static void prepare_option_action_observer(void)
{
    FILE *file=fopen("option-action-observers.txt","r");
    DWORD entry,frontier;
    if(!file)return;
    if(baseline || !option_match_probe || !vector_probe || signature_reference ||
       fscanf(file,"%lx %lx",&entry,&frontier)!=2 || entry!=0x131aa ||
       (frontier!=0x132f9 && frontier!=0x1335b) || fgetc(file)!='\n' || fgetc(file)!=EOF)
        fail("option action observer plan");
    fclose(file);option_action_probe=1;signature_continue=0;
    option_integer_probe=frontier==0x1335b;
    fprintf(events,"option-actions-plan ordinary=1 no-seeds=1 action=000131aa frontier=%08lx\n",frontier);
}

static void action_registers(const char *tag,unsigned index,DWORD address,
                             CONTEXT *context)
{
    DWORD words[16];char name[100];
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    readmem(context->Esp,words+10,20);words[15]=base;
    sprintf(name,"actions-%s-%u-registers.bin",tag,index);save_blob(name,words,sizeof(words));
    sprintf(name,"actions-%s-%u-fp.bin",tag,index);save_blob(name,&context->FloatSave,sizeof(context->FloatSave));
    sprintf(name,"actions-%s-%u",tag,index);save_frontend_state(name);
    fprintf(events,"option-action phase=%s index=%u seq=%u pc=%08lx esp=%08lx\n",tag,index,sequence,address,context->Esp);
}

/* Keep actual first-prefix filenames as aliases of an observed first phase;
 * later loop iterations receive distinct files and never overwrite them. */
static void action_first_match(const char *phase,DWORD address,CONTEXT *context)
{
    if(action_matches==1)option_match_record(phase,address,context);
}

static void action_next_value(const char *tag,unsigned index,CONTEXT *context)
{
    char text[1024],name[100];
    if(!context->Eax) {
        if(!option_integer_probe || strcmp(tag,"outer") || index!=7)
            fail("unexpected NULL option argument before numeric frontier");
        sprintf(name,"actions-%s-%u-value.bin",tag,index);save_blob(name,"",0);return;
    }
    remote_string(context->Eax,text,sizeof(text));
    sprintf(name,"actions-%s-%u-value.bin",tag,index);save_blob(name,text,strlen(text)+1);
}

static void observe_option_action_entry(DWORD address,CONTEXT *context)
{
    DWORD stack[5];unsigned index;
    if(!option_action_probe || (address!=base+0x12fbe && address!=base+0x12f9c && address!=base+0x131aa))return;
    readmem(context->Esp,stack,sizeof(stack));
    if(address==base+0x12fbe) {
        if(action_match_active || action_matches>=(option_integer_probe?8U:3U) || stack[0]!=base+(action_matches?0x12c6e:0x12c50))
            fail("matcher ordered caller/count");
        ++action_matches;action_match_active=1;action_match_esp=context->Esp;
        action_match_return=stack[0];action_match_saved=snapshot[action_match_return-base-code_rva];
        if(action_match_saved!=0x83)fail("matcher caller return byte");
        action_registers("match-entry",action_matches-1,address,context);
        action_first_match("entry",address,context);patch(action_match_return,0xcc);
    } else if(address==base+0x12f9c) {
        if(action_next_active)fail("nested option argument callback");
        if(stack[0]==base+(action_outer_calls?0x12c63:0x12c45)) {
            if(action_outer_calls>=(option_integer_probe?8U:3U))fail("outer option argument count");
            action_next_kind=0;index=action_outer_calls++;
            if(!index){argument_cursor_esp=context->Esp;argument_cursor_record("entry",address,context);}
            action_registers("outer-entry",index,address,context);
        } else if(stack[0]==base+0x130ae || stack[0]==base+0x13137) {
            if(!action_match_active || action_inner_calls>=3)fail("inner option argument count");
            action_next_kind=1;index=action_inner_calls++;
            action_registers("inner-entry",index,address,context);
            action_first_match("next-entry",address,context);
        } else fail("unknown option argument caller");
        action_next_esp=context->Esp;action_next_return=stack[0];
        action_next_saved=snapshot[action_next_return-base-code_rva];
        if(action_next_saved!=(action_next_kind?0x8b:0x50))fail("argument callback return byte");
        action_next_active=1;patch(action_next_return,0xcc);
    } else if(address==base+0x131aa) {
        if(!action_match_active || action_handler_active || action_calls>=(option_integer_probe?7U:3U) ||
           (stack[0]!=base+0x130bc && stack[0]!=base+0x13145 && (!option_integer_probe || stack[0]!=base+0x130f8)))
            fail("option action caller/count");
        index=action_calls++;
        action_registers("action-entry",index,address,context);
        action_first_match("action",address,context);
        action_handler_esp=context->Esp;action_handler_return=stack[0];
        action_handler_saved=snapshot[action_handler_return-base-code_rva];
        if(action_handler_saved!=(action_handler_return==base+0x130f8?0x83:0x8b))
            fail("action caller return byte");
        action_handler_active=1;patch(action_handler_return,0xcc);
    }
}

static void action_signature_record(const char *phase,DWORD address,
                                    CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[17],arena[8];char tag[100],name[100];unsigned index=action_api_calls-1;
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=action_api_return;words[11]=mspdb_base;
    readmem(context->Esp+(!strcmp(phase,"entry")?4:0),words+12,12);
    words[15]=context->EFlags;words[16]=index;
    sprintf(name,"actions-signature-%u-%s-registers.bin",index,phase);save_blob(name,words,sizeof(words));
    sprintf(name,"actions-signature-%u-%s-input",index,phase);save_remote_state(name,0,action_api_input,action_api_length+1);
    sprintf(name,"actions-signature-%u-%s-table",index,phase);save_remote_state(name,0,mspdb_base+0x21a30,1024);
    readmem(base+0x58fd0,arena,sizeof(arena));
    if(!arena[2] || arena[3]!=32768)fail("signature owned vector extent");
    sprintf(name,"actions-signature-%u-%s-vector",index,phase);save_remote_state(name,0,arena[2],32768);
    sprintf(tag,"actions-signature-%u-%s",index,phase);locale_snapshot(tag,context,thread,tid);
    if(!restore_probe && !index) {
        /* Actual first C boundary, retaining the prior real prefix schema. */
        signature_input=action_api_input;signature_api_esp=action_api_esp;
        signature_reference_record(phase,address,context,!strcmp(phase,"return"));
        if(!strcmp(phase,"return"))save_remote_state("signature-provider-table-after",0,mspdb_base+0x21a30,1024);
        locale_snapshot(!strcmp(phase,"entry")?"signature-entry":"signature-return",context,thread,tid);
    }
    fprintf(events,"option-signature phase=%s index=%u seq=%u pc=%08lx caller=%08lx bytes=%lu\n",phase,index,sequence,address,action_api_return,action_api_length);
}

static int option_action_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[4],expected;BYTE guard;unsigned index;
    char name[100],text[1024];
    if(!option_action_probe)return 0;
    if(address==signature_entry) {
        if(action_api_active || action_api_calls>=(option_integer_probe?(restore_probe?8U:9U):(restore_probe?2U:3U)))fail("signature repeated count/nesting");
        readmem(context->Esp,stack,sizeof(stack));
        index=action_api_calls;
        expected=(!restore_probe&&!index)?0x12c31:
                 (index==(restore_probe?0U:1U) || (option_integer_probe && index==(restore_probe?2U:3U)))?0x130e2:0x13121;
        if(stack[0]!=base+expected || stack[2]>1023)fail("signature repeated caller/length");
        remote_string(stack[1],text,sizeof(text));
        if(strlen(text)!=stack[2])fail("signature complete defined input span");
        action_api_input=stack[1];action_api_length=stack[2];action_api_esp=context->Esp;
        action_api_return=stack[0];action_api_saved=snapshot[expected-code_rva];
        if(action_api_saved!=0x83)fail("signature caller ADD12 byte");
        ++action_api_calls;action_api_active=1;
        readmem(signature_entry,&guard,1);if(guard!=0xcc)fail("signature owned entry guard");
        sprintf(name,"actions-signature-%u-provider-body",index);save_remote_state(name,0,signature_entry,53);
        if(!restore_probe&&!index) {
            DWORD w[12];w[0]=sequence;w[1]=address;w[2]=context->Esp;
            w[3]=context->Ebx;w[4]=context->Esi;w[5]=context->Edi;w[6]=context->Ebp;
            memcpy(w+7,stack,16);w[11]=mspdb_base;save_blob("signature-entry-registers.bin",w,sizeof(w));
            save_remote_state("signature-input",0,stack[1],2);
        }
        action_signature_record("entry",address,context,thread,tid);save_state();
        patch(action_api_return,0xcc);patch(signature_entry,0x55);
    } else if(action_api_active && address==action_api_return) {
        if(context->Esp!=action_api_esp+4)fail("signature repeated RET0");
        action_signature_record("return",address,context,thread,tid);save_state();
        patch(address,action_api_saved);patch(signature_entry,0xcc);
        action_api_active=0;++action_api_returns;
    } else if(action_handler_active && address==action_handler_return) {
        if(context->Esp!=action_handler_esp+4 || action_returns>=(option_integer_probe?7U:2U))fail("option action RET0/count");
        action_registers("action-return",action_returns,address,context);save_state();
        patch(address,action_handler_saved);action_handler_active=0;++action_returns;
    } else if(action_match_active && address==action_match_return) {
        if(context->Esp!=action_match_esp+4 || context->Eax!=(DWORD)(option_integer_probe && action_matches==8?0:1))fail("option match RET0/result");
        action_registers("match-return",action_matches-1,address,context);save_state();
        patch(address,action_match_saved);action_match_active=0;
    } else if(action_next_active && address==action_next_return) {
        if(context->Esp!=action_next_esp+4)fail("option argument RET0");
        if(action_next_kind) {
            index=action_inner_returns++;action_registers("inner-return",index,address,context);
            action_next_value("inner",index,context);action_first_match("next-return",address,context);
        } else {
            index=action_outer_returns++;action_registers("outer-return",index,address,context);
            action_next_value("outer",index,context);
            if(!index){argument_cursor_record("return",address,context);save_remote_state("argument-cursor-result",0,context->Eax,4);}
        }
        save_state();patch(address,action_next_saved);action_next_active=0;
    } else return 0;
    context->Eip=address;
    if(!SetThreadContext(thread,context))fail("option action observer EIP");
    return 1;
}

static void check_option_action_stop(DWORD address)
{
    if(!option_action_probe)return;
    if(option_integer_probe) {
        if(address!=base+0x1335b || action_matches!=8 || action_calls!=7 || action_returns!=7 ||
           action_outer_calls!=8 || action_outer_returns!=8 || action_inner_calls!=3 || action_inner_returns!=3 ||
           action_api_calls!=(restore_probe?8U:9U) || action_api_returns!=action_api_calls ||
           action_api_active || action_next_active || action_match_active || action_handler_active)
            fail("complete integer option-loop frontier inventory");
        fprintf(events,"option-actions-complete matches=8 actions=7 returned=7 outer=8 inner=3 signatures=%u actual-frontier=0001335b\n",action_api_calls);
        return;
    }
    if(address!=base+0x132f9 || action_matches!=3 || action_calls!=3 || action_returns!=2 ||
       action_outer_calls!=3 || action_outer_returns!=3 || action_inner_calls!=3 || action_inner_returns!=3 ||
       action_api_calls!=(restore_probe?2U:3U) || action_api_returns!=action_api_calls ||
       action_api_active || action_next_active || !action_match_active || !action_handler_active)
        fail("complete ordered action/integer frontier inventory");
    fprintf(events,"option-actions-complete matches=3 actions=3 returned=2 outer=3 inner=3 signatures=%u actual-frontier=000132f9\n",action_api_calls);
}
