/* Original-only public ctype/atol and complete numeric call observations. */
static DWORD integer_esp,integer_return;
static unsigned integer_calls,integer_returns,integer_api_calls,integer_api_returns;
static int integer_active,integer_api_active;
static DWORD integer_argument,integer_api_esp,integer_api_return;
static DWORD integer_providers[3],integer_api_argument;
static BYTE integer_saved,integer_provider_saved[3],integer_api_saved;
static const char *integer_api_names[3]={"isspace","isdigit","atol"};
static DWORD integer_api_rvas[3]={0x1c0be,0x1c065,0x5d30};
static DWORD integer_iats[3]={0x5d12c,0x5d018,0x5d07c};
static DWORD integer_return_rvas[3]={0x13313,0x13329,0x1333b};

static void integer_record(const char *phase,DWORD address,CONTEXT *context,
                           HANDLE thread,DWORD tid)
{
    char tag[80],name[100],input[100];
    DWORD words[16];unsigned index=integer_calls-1;
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    readmem(context->Esp,words+10,16);words[14]=integer_argument;words[15]=index;
    sprintf(name,"integer-%u-%s-registers.bin",index,phase);save_blob(name,words,sizeof(words));
    remote_string(integer_argument,input,sizeof(input));
    sprintf(name,"integer-%u-%s-input.bin",index,phase);save_blob(name,input,strlen(input)+1);
    sprintf(tag,"integer-%u-%s",index,phase);locale_snapshot(tag,context,thread,tid);
    fprintf(events,"integer phase=%s index=%u seq=%u pc=%08lx esp=%08lx\n",phase,index,sequence,address,context->Esp);
}

/* Called only at the genuine target entry after locale initialization. */
static void integer_enter(CONTEXT *context,HANDLE thread,DWORD tid)
{
    DWORD words[2],provider;unsigned i;char input[100],name[100];
    if(!option_integer_probe)return;
    if(integer_active || integer_calls>=3)fail("numeric entry count/nesting");
    readmem(context->Esp,words,sizeof(words));
    if(words[0]!=base+0x1320c)fail("numeric action caller");
    if(!integer_calls) {
        for(i=0;i<3;++i) {
            readmem(base+integer_iats[i],&provider,4);
            if(provider!=crt_base+integer_api_rvas[i] || provider!=exported(crt_base,integer_api_names[i]))
                fail("numeric canonical public import");
            integer_providers[i]=provider;
            readmem(provider,&integer_provider_saved[i],1);
            if(integer_provider_saved[i]!=(i==2?0x53:0x83))fail("numeric public entry byte");
            sprintf(name,"integer-provider-%s",integer_api_names[i]);
            save_remote_state(name,0,provider,i==2?16:42);patch(provider,0xcc);
        }
    }
    integer_esp=context->Esp;integer_return=words[0];integer_argument=words[1];
    remote_string(integer_argument,input,sizeof(input));
    if(strlen(input)!=1 || input[0]!=(integer_calls==0?'1':integer_calls==1?'5':'0'))
        fail("numeric actual complete input");
    integer_saved=snapshot[0x1320c-code_rva];if(integer_saved!=0x8b)fail("numeric return byte");
    ++integer_calls;integer_active=1;patch(integer_return,0xcc);
    integer_record("entry",base+0x132f9,context,thread,tid);
}

static void integer_api_record(const char *phase,DWORD address,CONTEXT *context,
                               HANDLE thread,DWORD tid)
{
    DWORD words[17];char name[100],tag[80],input[100];
    unsigned kind=(integer_api_calls-1)%3,index=(integer_api_calls-1)/3;
    words[0]=sequence;words[1]=address;words[2]=context->Esp;words[3]=context->Eax;
    words[4]=context->Ecx;words[5]=context->Edx;words[6]=context->Ebx;
    words[7]=context->Esi;words[8]=context->Edi;words[9]=context->Ebp;
    words[10]=integer_api_return;words[11]=integer_api_argument;
    words[12]=kind;words[13]=index;words[14]=integer_argument;words[15]=crt_base;
    readmem(context->Esp+(!strcmp(phase,"entry")?4:0),words+16,4);
    sprintf(name,"integer-%u-api-%s-%s-registers.bin",index,integer_api_names[kind],phase);save_blob(name,words,sizeof(words));
    if(kind==2) {
        remote_string(integer_api_argument,input,sizeof(input));
        sprintf(name,"integer-%u-api-atol-%s-input.bin",index,phase);save_blob(name,input,strlen(input)+1);
    }
    sprintf(tag,"integer-%u-api-%s-%s",index,integer_api_names[kind],phase);
    locale_snapshot(tag,context,thread,tid);
    fprintf(events,"integer-api phase=%s index=%u kind=%s seq=%u pc=%08lx esp=%08lx arg=%08lx eax=%08lx\n",phase,index,integer_api_names[kind],sequence,address,context->Esp,integer_api_argument,context->Eax);
}

static int integer_boundary(DWORD address,CONTEXT *context,HANDLE thread,DWORD tid)
{
    unsigned kind;DWORD stack[2];
    if(!option_integer_probe)return 0;
    for(kind=0;kind<3;++kind)if(integer_providers[kind] && address==integer_providers[kind])break;
    if(kind<3) {
        if(!integer_active || integer_api_active || integer_api_calls>=9 || kind!=integer_api_calls%3)
            fail("numeric API actual order/count");
        readmem(context->Esp,stack,sizeof(stack));
        if(stack[0]!=base+integer_return_rvas[kind] ||
           stack[1]!=(kind==2?integer_argument:(DWORD)(integer_calls==1?'1':integer_calls==2?'5':'0')))
            fail("numeric API actual caller/input");
        integer_api_return=stack[0];integer_api_argument=stack[1];integer_api_esp=context->Esp;
        integer_api_saved=snapshot[integer_return_rvas[kind]-code_rva];
        if(integer_api_saved!=(kind==2?0x8b:0x83))fail("numeric API return ownership");
        ++integer_api_calls;integer_api_active=1;
        integer_api_record("entry",address,context,thread,tid);save_state();
        patch(integer_api_return,0xcc);patch(address,integer_provider_saved[kind]);
    } else if(integer_api_active && address==integer_api_return) {
        kind=(integer_api_calls-1)%3;
        if(context->Esp!=integer_api_esp+4)fail("numeric API RET0");
        integer_api_record("return",address,context,thread,tid);save_state();
        patch(address,integer_api_saved);patch(integer_providers[kind],0xcc);
        integer_api_active=0;++integer_api_returns;
    } else if(integer_active && address==integer_return) {
        if(integer_api_calls!=integer_calls*3 || integer_api_returns!=integer_api_calls ||
           context->Esp!=integer_esp+4)fail("numeric RET0/call closure");
        integer_record("return",address,context,thread,tid);save_state();
        patch(address,integer_saved);integer_active=0;++integer_returns;
    } else return 0;
    context->Eip=address;if(!SetThreadContext(thread,context))fail("numeric observer EIP");
    return 1;
}

static void check_integer_stop(void)
{
    if(option_integer_probe && (integer_calls!=3 || integer_returns!=3 ||
       integer_api_calls!=9 || integer_api_returns!=9 || integer_active || integer_api_active))
        fail("complete numeric/public call closure");
}
