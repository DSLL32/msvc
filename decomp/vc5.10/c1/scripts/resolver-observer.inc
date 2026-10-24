/* Normal-only known input before any path-resolution instruction runs. */
static int resolver_entered;
static DWORD resolver_esp;
static void resolver_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[16];char input[40];
    if(!option_resolver_probe)return;
    if(resolver_entered || restore_probe)fail("resolver case/entry count");
    words[0]=sequence;words[1]=base+0x1335b;words[2]=context->Esp;
    words[3]=context->Eax;words[4]=context->Ecx;words[5]=context->Edx;
    words[6]=context->Ebx;words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    readmem(context->Esp,words+10,16);words[14]=base;words[15]=1;
    if(words[10]!=base+0x12c8b || words[11]!=context->Esp+32 || words[13]!=520)
        fail("actual resolver caller/stack arguments");
    remote_string(words[12],input,sizeof(input));
    if(strcmp(input,"vc50.pdb"))fail("known normal resolver input");
    save_blob("resolver-entry-registers.bin",words,sizeof(words));
    save_blob("resolver-entry-input.bin",input,strlen(input)+1);
    locale_snapshot("resolver-entry",context,thread,tid);
    resolver_esp=context->Esp;resolver_entered=1;
    fprintf(events,"resolver-entry seq=%u pc=%08lx esp=%08lx defined-input=9\n",sequence,base+0x1335b,context->Esp);
}

static void check_resolver_stop(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[2];char path[40];
    if(!option_resolver_probe)return;
    if(!resolver_entered || address!=base+0x1344d || context->Esp!=resolver_esp-284)
        fail("real first normalizer frame");
    readmem(context->Esp,words,sizeof(words));
    if(words[0]!=base+0x133c9 || words[1]!=context->Esp+24)
        fail("real first normalizer local260-byte buffer");
    remote_string(words[1],path,sizeof(path));if(strcmp(path,"vc50.pdb"))fail("actual copied path");
    save_blob("resolver-normalize-input.bin",path,strlen(path)+1);
    locale_snapshot("resolver-normalize",context,thread,tid);
}
