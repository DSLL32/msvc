/* Original-only genuine suffix entry/RET and pending vector append inputs. */
static unsigned suffix_calls,suffix_returns;
static DWORD suffix_esp,suffix_return;
static BYTE suffix_saved;
static int suffix_active;

static void suffix_enter(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[4];char value[100];
    if(!option_suffix_probe)return;
    if(suffix_active || suffix_calls>=1)fail("suffix actual count/nesting");
    readmem(context->Esp,stack,sizeof(stack));
    if(stack[0]!=base+0x394cf || stack[3]!=35)fail("suffix actual caller/action");
    remote_string(stack[1],value,sizeof(value));if(strcmp(value,"*"))fail("suffix actual pattern");
    save_blob("suffix-0-pattern.bin",value,strlen(value)+1);
    remote_string(stack[2],value,sizeof(value));if(strcmp(value,"e"))fail("suffix actual argument");
    save_blob("suffix-0-argument.bin",value,strlen(value)+1);
    suffix_esp=context->Esp;suffix_return=stack[0];
    suffix_saved=snapshot[0x394cf-code_rva];if(suffix_saved!=0x83)fail("suffix return byte");
    action_registers("suffix-entry",suffix_calls++,address,context,thread,tid);
    suffix_active=1;patch(suffix_return,0xcc);
}

static int suffix_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    if(!option_suffix_probe || !suffix_active || address!=suffix_return)return 0;
    if(context->Esp!=suffix_esp+4 || context->Eax!=1)fail("suffix actual RET0/result");
    action_registers("suffix-return",suffix_returns++,address,context,thread,tid);save_state();
    patch(address,suffix_saved);suffix_active=0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("suffix observer EIP");
    return 1;
}

static void check_suffix_stop(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[3],vector[3],arena[8],root;char value[100];
    if(!option_suffix_probe)return;
    if(address!=base+0x100ea || suffix_calls!=1 || suffix_returns!=1 || suffix_active)
        fail("complete suffix/append boundary");
    readmem(context->Esp,words,sizeof(words));readmem(base+0xc06c4,&root,4);
    if(words[0]!=base+0x396bc || words[1]!=root)fail("append actual caller/vector owner");
    remote_string(words[2],value,sizeof(value));if(strcmp(value,"_MSC_EXTENSIONS"))fail("append actual value");
    save_blob("suffix-append-input.bin",value,strlen(value)+1);
    action_registers("append-entry",0,address,context,thread,tid);
    readmem(words[1],vector,sizeof(vector));
    if(vector[0]!=(40UL<<16) || vector[1]!=1)fail("append known vector header");
    save_blob("suffix-append-header.bin",vector,sizeof(vector));
    vector_remote("suffix-append-items.bin",vector[2],160);
    readmem(base+0xc6520,arena,sizeof(arena));if(arena[3]!=32768)fail("append owned arena extent");
    vector_remote("suffix-append-committed.bin",arena[2],32768);
    fprintf(events,"suffix-complete calls=1 returned=1 actual-frontier=000100ea\n");
}
