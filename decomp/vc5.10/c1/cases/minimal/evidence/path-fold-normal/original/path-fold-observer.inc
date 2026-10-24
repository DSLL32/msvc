/* Original-only observer of the genuine public _mbctolower boundary.
 * No provider calls or semantic target-code substitutions are performed. */
static int path_fold_probe, path_fold_api_active, path_fold_return_active;
static int path_fold_helper_active;
static DWORD path_fold_api, path_fold_calls, path_fold_argument;
static DWORD path_fold_api_esp, path_fold_helper_esp;
static BYTE path_fold_api_saved, path_fold_return_saved, path_fold_helper_saved;

static void path_fold_prepare(void)
{
    FILE *probe=fopen("path-fold-observers.txt","r");
    DWORD entry,iat,provider;
    if(!probe)return;
    if(fscanf(probe,"%lx %lx %lx",&entry,&iat,&provider)!=3 ||
       entry!=0x12f3e || iat!=0x5d05c || provider!=0xa5d3 || fgetc(probe)!='\n' || fgetc(probe)!=EOF)
        fail("path fold observer plan");
    fclose(probe);
    readmem(base+iat,&path_fold_api,4);
    if(path_fold_api!=crt_base+provider || path_fold_api!=exported(crt_base,"_mbctolower"))
        fail("canonical _mbctolower import/export");
    readmem(path_fold_api,&path_fold_api_saved,1);
    if(path_fold_api_saved!=0x83)fail("canonical _mbctolower first byte");
    save_remote_state("mbctolower-provider",0,path_fold_api,54);
    path_fold_return_saved=snapshot[0x12f67-code_rva];
    path_fold_helper_saved=snapshot[0x12b80-code_rva];
    if(path_fold_return_saved!=0x83 || path_fold_helper_saved!=0x83)
        fail("path fold return instruction ownership");
    path_fold_probe=1;
    fprintf(events,"path-fold-observer entry=00012f3e iat=0005d05c provider=0000a5d3 table=00055928 bytes=512 no-query-calls=1\n");
}

static void path_fold_seed(const char *phase,WORD first)
{
    WORD words[256];unsigned i;char name[100];
    readmem(base+0x55928,words,sizeof(words));sprintf(name,"path-fold-%s-before-seed.bin",phase);save_blob(name,words,sizeof(words));
    for(i=0;i<256;++i)words[i]=(WORD)(first+37*i);
    write_mem(base+0x55928,words,sizeof(words));
    fprintf(events,"path-fold-seed phase=%s bytes=512 first=%04x step=37\n",phase,first);
}

static void path_fold_record(unsigned phase,DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    /* Serialized pieces have explicit extents; no observer-struct padding is
     * interpreted. Mutable provider state/tables are read anew each time. */
    DWORD words[20],state[18],teb,ptd,tls;BYTE raw_teb[4096],code[9];
    BYTE mb[257],lower[256],classes[256];WORD ctype[257],old[257],fold[256];
    char name[100],result[64];FILE *file;LDT_ENTRY selector;
    if(!GetThreadSelectorEntry(thread,context->SegFs,&selector))fail("path fold TEB selector");
    teb=selector.BaseLow|((DWORD)selector.HighWord.Bytes.BaseMid<<16)|((DWORD)selector.HighWord.Bytes.BaseHi<<24);
    readmem(teb,raw_teb,sizeof(raw_teb));
    if(*(DWORD *)(raw_teb+0x18)!=teb || *(DWORD *)(raw_teb+0x24)!=tid)fail("path fold TEB identity");
    readmem(crt_base+0x15ce,code,4);if(*(DWORD *)code!=crt_base+0x3d004)fail("path fold TLS literal");
    readmem(crt_base+0x3d004,&tls,4);if(tls>=64)fail("path fold TLS slot");
    memcpy(&ptd,raw_teb+0xe10+tls*4,4);if(!ptd)fail("path fold PTD");
    memset(words,0,sizeof(words));words[0]=phase;words[1]=path_fold_calls;words[2]=sequence;
    words[3]=address;words[4]=context->Esp;words[5]=context->Eax;
    words[6]=context->Ebx;words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=base+0x12f67;words[11]=path_fold_argument;
    readmem(ptd+8,words+12,8);memcpy(words+14,raw_teb+0x34,4);
    words[15]=crt_base;words[16]=base;words[17]=path_fold_api_esp;words[18]=teb;words[19]=ptd;
    memset(state,0,sizeof(state));state[0]=crt_base;
    readmem(locale_export("__mb_cur_max"),state+1,4);readmem(locale_export("__lc_codepage"),state+2,4);
    readmem(locale_export("__lc_handle"),state+3,24);readmem(crt_base+0x3f97c,state+9,4);
    readmem(crt_base+0x3f1c0,state+10,4);readmem(locale_export("_pctype"),state+11,4);
    state[12]=locale_export("_ctype");readmem(locale_export("__argc"),state+13,4);
    readmem(locale_export("__argv"),state+14,4);readmem(locale_export("_environ"),state+15,4);
    state[16]=locale_old_pctype;state[17]=locale_result;
    readmem(locale_export("_mbctype"),mb,sizeof(mb));readmem(crt_base+0x3f7f8,lower,sizeof(lower));
    readmem(state[11]-2,ctype,sizeof(ctype));readmem(locale_old_pctype-2,old,sizeof(old));
    memset(result,0,sizeof(result));remote_string(locale_result,result,sizeof(result));
    readmem(base+0x55928,fold,sizeof(fold));readmem(base+0x5e008,classes,sizeof(classes));
    sprintf(name,"path-fold-%03lu-%s.bin",path_fold_calls,phase?"return":"entry");file=fopen(name,"wb");
    if(!file || fwrite(words,1,sizeof(words),file)!=sizeof(words) ||
       fwrite(state,1,sizeof(state),file)!=sizeof(state) ||
       fwrite(&context->FloatSave,1,sizeof(context->FloatSave),file)!=sizeof(context->FloatSave) ||
       fwrite(mb,1,sizeof(mb),file)!=sizeof(mb) || fwrite(lower,1,sizeof(lower),file)!=sizeof(lower) ||
       fwrite(ctype,1,sizeof(ctype),file)!=sizeof(ctype) || fwrite(old,1,sizeof(old),file)!=sizeof(old) ||
       fwrite(result,1,sizeof(result),file)!=sizeof(result) || fwrite(fold,1,sizeof(fold),file)!=sizeof(fold) ||
       fwrite(classes,1,sizeof(classes),file)!=sizeof(classes) || fclose(file))fail("path fold API record");
    fprintf(events,"path-fold-api phase=%s index=%lu seq=%u pc=%08lx esp=%08lx argument=%lu result=%08lx\n",phase?"return":"entry",path_fold_calls,sequence,address,context->Esp,path_fold_argument,context->Eax);
}

static int path_fold_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD stack[2];
    if(!path_fold_probe)return 0;
    if(path_fold_api_active && address==path_fold_api) {
        readmem(context->Esp,stack,sizeof(stack));
        if(!mb_complete || stack[0]!=base+0x12f67 || stack[1]>255 || path_fold_return_active || path_fold_calls>=256)
            fail("actual _mbctolower ABI/caller/inventory");
        path_fold_argument=stack[1];path_fold_api_esp=context->Esp;
        path_fold_record(0,address,context,thread,tid);
        patch(base+0x12f67,0xcc);path_fold_return_active=1;
        patch(address,path_fold_api_saved);path_fold_api_active=0;
    } else if(path_fold_return_active && address==base+0x12f67) {
        path_fold_record(1,address,context,thread,tid);
        patch(address,path_fold_return_saved);path_fold_return_active=0;
        patch(path_fold_api,0xcc);path_fold_api_active=1;++path_fold_calls;
    } else if(path_fold_helper_active && address==base+0x12b80) {
        char name[80];
        fprintf(events,"path-fold-helper-return seq=%u pc=%08lx esp=%08lx entry-esp=%08lx calls=%lu\n",sequence,address,context->Esp,path_fold_helper_esp,path_fold_calls);
        sprintf(name,"fpu-%03u.bin",sequence);save_blob(name,&context->FloatSave,sizeof(context->FloatSave));
        save_frontend_state("path-fold-complete");save_state();
        patch(address,path_fold_helper_saved);path_fold_helper_active=0;
    } else return 0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("path fold observer context");
    return 1;
}

static void path_fold_enter(CONTEXT *context)
{
    DWORD caller;
    if(!path_fold_probe)return;
    if(path_fold_helper_active || path_fold_calls || !mb_complete)fail("path fold helper inventory");
    readmem(context->Esp,&caller,4);if(caller!=base+0x12b80)fail("path fold helper caller");
    if(restore_probe)path_fold_seed("helper",0x4303);
    path_fold_helper_esp=context->Esp;save_frontend_state("path-fold-enter");
    patch(base+0x12b80,0xcc);path_fold_helper_active=1;
    patch(path_fold_api,0xcc);path_fold_api_active=1;
}
