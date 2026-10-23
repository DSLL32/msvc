/* One actual normal _setjmp call in the first Spawn frame. No longjmp or SEH
 * unwind coverage is implied; this is debugger evidence, not CRT source. */
static unsigned spawn_stage;
static DWORD spawn_esp, spawn_environment, spawn_previous, spawn_fsbase, spawn_fs0;
static CONTEXT spawn_setjmp_context, spawn_setjmp_fp;
static BYTE spawn_before[68];

static int spawn_mask(void)
{
    return memory(PREFIX_CODE_BASE,prefix_actual,PREFIX_CODE_SIZE) &&
           !memcmp(prefix_actual,prefix_expected,PREFIX_CODE_SIZE);
}

static int spawn_restore(DWORD address)
{
    BYTE original=prefix_source[address-PREFIX_CODE_BASE];
    if(!patch(address,original)) return 0;
    prefix_expected[address-PREFIX_CODE_BASE]=original;
    return spawn_mask();
}

static int spawn_float(CONTEXT *fp)
{
    memset(fp,0,sizeof(*fp)); fp->ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    return GetThreadContext(primary,fp);
}

static void spawn_local(const char *kind, const BYTE *data, unsigned size)
{
    unsigned i;
    fprintf(events,"local-bytes kind=%s count=%u hex=",kind,size);
    for(i=0;i<size;++i) fprintf(events,"%02x",data[i]);
    fprintf(events,"\n");
}

static int spawn_fs(CONTEXT *c, DWORD *base, DWORD *head)
{
    LDT_ENTRY selector;
    if(!GetThreadSelectorEntry(primary,c->SegFs,&selector)) return 0;
    *base=(DWORD)selector.BaseLow | ((DWORD)selector.HighWord.Bytes.BaseMid<<16) |
          ((DWORD)selector.HighWord.Bytes.BaseHi<<24);
    return *base && memory(*base,head,4);
}

static int spawn_fp_registers_unchanged(const CONTEXT *c)
{
    const CONTEXT *before=&spawn_setjmp_context;
    return c->Eax==0 && c->Esp==before->Esp+4 &&
           c->Ebx==before->Ebx && c->Ecx==before->Ecx && c->Edx==before->Edx &&
           c->Esi==before->Esi && c->Edi==before->Edi && c->Ebp==before->Ebp &&
           c->SegCs==before->SegCs && c->SegSs==before->SegSs &&
           c->SegDs==before->SegDs && c->SegEs==before->SegEs &&
           c->SegFs==before->SegFs && c->SegGs==before->SegGs &&
           (c->EFlags&0x8d5UL)==0x44UL &&
           (c->EFlags&~0x8d5UL)==(before->EFlags&~0x8d5UL);
}

static int spawn_event(DWORD address, CONTEXT *c)
{
    DWORD value,caller,stack_base,stack_limit,fsbase,fs0;
    BYTE actual[68],expected[68];
    WORD segment;
    CONTEXT fp;
    if(spawn_stage==0 && address==0x416d3c) {
        if(!prefix_spawn(c) || !memory(0x45018c,&spawn_previous,4) || spawn_previous) return 0;
        spawn_esp=c->Esp;
        fprintf(events,"app-entry name=Spawn address=%08lx previous-SpawnStack=%08lx\n",address,spawn_previous);
        if(!spawn_restore(address)) return 0;
        spawn_stage=1; c->Eip=address;
        return SetThreadContext(primary,c)?1:0;
    }
    if(spawn_stage==1 && address==0x433b14) {
        if(!at_break(address,c) || !spawn_mask() || c->Eax!=spawn_esp-64 ||
           c->Esp!=c->Eax-4 || !memory(c->Esp,&caller,4) || caller!=0x416d57 ||
           c->Ecx!=spawn_previous || c->Edx!=prefix_argv ||
           !memory(0x45018c,&value,4) || value!=c->Eax) return 0;
        spawn_environment=c->Eax;
        if(!spawn_fs(c,&spawn_fsbase,&spawn_fs0) ||
           !memory(spawn_fsbase+4,&stack_base,4) || !memory(spawn_fsbase+8,&stack_limit,4) ||
           spawn_environment-4<stack_limit || spawn_environment>0xffffffffUL-64 ||
           spawn_environment+64>stack_base ||
           !memory(spawn_environment-4,spawn_before,sizeof(spawn_before)) ||
           !memory(spawn_environment+52,&value,4) || value!=0x4243c8 || !spawn_float(&spawn_setjmp_fp)) return 0;
        spawn_setjmp_context=*c;
        log_context("actual-setjmp-entry",c);
        fprintf(events,"setjmp-live-owner env=%08lx bytes=52 SpawnStack=%08lx stack-limit=%08lx stack-base=%08lx fs-base=%08lx fs0=%08lx return=%08lx callback=%08lx\n",spawn_environment,spawn_environment,stack_limit,stack_base,spawn_fsbase,spawn_fs0,caller,value);
        spawn_local("actual-setjmp-frame-before",spawn_before,sizeof(spawn_before));
        spawn_local("actual-setjmp-x87-before",(BYTE *)&spawn_setjmp_fp.FloatSave,sizeof(spawn_setjmp_fp.FloatSave));
        if(!spawn_restore(address)) return 0;
        spawn_stage=2; c->Eip=address;
        return SetThreadContext(primary,c)?1:0;
    }
    if(spawn_stage==2 && address==0x416d57) {
        if(!at_break(address,c) || !spawn_mask() || !spawn_float(&fp) ||
           !spawn_fs(c,&fsbase,&fs0) || fsbase!=spawn_fsbase || fs0!=spawn_fs0 ||
           !memory(spawn_environment-4,actual,sizeof(actual))) return 0;
        log_context("actual-setjmp-return",c);
        spawn_local("actual-setjmp-frame-after",actual,sizeof(actual));
        spawn_local("actual-setjmp-x87-after",(BYTE *)&fp.FloatSave,sizeof(fp.FloatSave));
        memcpy(expected,spawn_before,sizeof(expected));
#define SAVE_DWORD(offset,field) value=spawn_setjmp_context.field; memcpy(expected+4+(offset),&value,4)
        SAVE_DWORD(0,Ebx); SAVE_DWORD(4,Ecx); SAVE_DWORD(8,Edx);
        SAVE_DWORD(12,Esi); SAVE_DWORD(16,Edi); SAVE_DWORD(20,Ebp);
#undef SAVE_DWORD
        value=0x416d57; memcpy(expected+4+24,&value,4);
        value=spawn_setjmp_context.Esp+4; memcpy(expected+4+28,&value,4);
#define SAVE_WORD(offset,field) segment=(WORD)spawn_setjmp_context.field; memcpy(expected+4+(offset),&segment,2)
        SAVE_WORD(32,SegEs); SAVE_WORD(34,SegDs); SAVE_WORD(36,SegCs);
        SAVE_WORD(38,SegFs); SAVE_WORD(40,SegGs); SAVE_WORD(42,SegSs);
#undef SAVE_WORD
        memcpy(expected+4+44,&spawn_fs0,4);
        if(!spawn_fp_registers_unchanged(c) || memcmp(&fp.FloatSave,&spawn_setjmp_fp.FloatSave,sizeof(fp.FloatSave)) ||
           memcmp(actual,expected,sizeof(actual)) || !memory(0x45018c,&value,4) || value!=spawn_environment) return 0;
        fprintf(events,"setjmp-normal-return VERIFIED result=0 GP=preserved-except-EAX segments=unchanged flags=exact-SUB x87=unchanged fs0=unchanged writes=48 last-word=untouched adjacent-frame=unchanged owner=live\n");
        if(!spawn_restore(address)) return 0;
        spawn_stage=3; c->Eip=address;
        return SetThreadContext(primary,c)?1:0;
    }
    if(spawn_stage==3 && address==0x4243c8) {
        if(!at_break(address,c) || !spawn_mask() || c->Eax || c->Edx ||
           c->Esp!=spawn_environment-4 || !memory(c->Esp,&caller,4) || caller!=0x416d61 ||
           !memory(0x45018c,&value,4) || value!=spawn_environment || !prefix_application_state(1)) return 0;
        log_context("frontier-DoLinkFromCommandLine",c);
        fprintf(events,"matched-callback-frontier name=DoLinkFromCommandLine address=%08lx return=%08lx active-SpawnStack=%08lx whole-mask-verified=%lu longjmp-observed=0\n",address,caller,value,PREFIX_CODE_SIZE);
        return 2;
    }
    return 0;
}
