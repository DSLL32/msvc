/* Original debugger only: the reached registered, unbuffered FILE branch. */
static DWORD freebuf_enabled, freebuf_step, freebuf_return, freebuf_esp;
static DWORD freebuf_thread, freebuf_calls, freebuf_saved_tf;
static BYTE freebuf_return_byte;
static DWORD freebuf_file[8], freebuf_errors[3], freebuf_handle, freebuf_osfile;
static FLOATING_SAVE_AREA freebuf_fp;

static int freebuf_snapshot(HANDLE child, HANDLE thread, CONTEXT *context,
                           DWORD *file, DWORD *errors, DWORD *handle, DWORD *flags)
{
    DWORD bytes;
    memset(context, 0, sizeof(*context));
    context->ContextFlags = CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    *flags = 0;
    return GetThreadContext(thread, context) && child_teb &&
        ReadProcessMemory(child, (void *)0x393f0, file, 32, &bytes) && bytes == 32 &&
        ReadProcessMemory(child, (void *)0x39614, errors, 8, &bytes) && bytes == 8 &&
        ReadProcessMemory(child, (void *)(child_teb+0x34), errors+2, 4, &bytes) && bytes == 4 &&
        file[4] < 64 &&
        ReadProcessMemory(child, (void *)(0x39740+4*file[4]), handle, 4, &bytes) && bytes == 4 &&
        ReadProcessMemory(child, (void *)(0x39700+file[4]), flags, 1, &bytes) && bytes == 1;
}

static void log_freebuf(HANDLE log, const char *stage, DWORD thread, CONTEXT *context,
                        DWORD *file, DWORD *errors, DWORD handle, DWORD flags)
{
    char line[1024];
    BYTE *raw;
    DWORD i, written;
    int used;
    used = sprintf(line, "freebuf-%s thread %lu esp %08lx file ", stage, thread, context->Esp);
    raw=(BYTE *)file;
    for(i=0;i<32;++i)used+=sprintf(line+used,"%02x",raw[i]);
    used+=sprintf(line+used," errno %lu doserrno %lu last_error %lu handle %08lx osfile %lu fp ",
        errors[0],errors[1],errors[2],handle,flags);
    raw=(BYTE *)&context->FloatSave;
    for(i=0;i<sizeof(context->FloatSave);++i)used+=sprintf(line+used,"%02x",raw[i]);
    line[used++]='\n';WriteFile(log,line,used,&written,0);
}

static int begin_freebuf(HANDLE child, HANDLE original, HANDLE thread, HANDLE log, DWORD tid)
{
    CONTEXT context;
    DWORD args[2], bytes, written, i;
    BYTE trap=0xcc;
    if(freebuf_return || freebuf_step || freebuf_calls || sopen_completed!=3 ||
       !freebuf_snapshot(child,thread,&context,freebuf_file,freebuf_errors,&freebuf_handle,&freebuf_osfile) ||
       !ReadProcessMemory(child,(void *)context.Esp,args,8,&bytes) || bytes!=8)return 0;
    log_freebuf(log,"entry",tid,&context,freebuf_file,freebuf_errors,freebuf_handle,freebuf_osfile);
    if(args[0]!=0x2e155 || args[1]!=0x393f0 || freebuf_file[3]!=0x80 ||
       freebuf_file[4]!=last_sopen_fd || freebuf_osfile!=1 || (context.EFlags&0x100))return 0;
    for(i=0;i<8;++i)if(i!=3 && i!=4 && freebuf_file[i])return 0;
    if(!ReadProcessMemory(child,(void *)args[0],&freebuf_return_byte,1,&bytes) || bytes!=1 || freebuf_return_byte==trap ||
       SetFilePointer(original,0x1f990,0,FILE_BEGIN)==0xffffffff ||
       !ReadFile(original,original_bytes,54,&bytes,0) || bytes!=54 || original_bytes[0]!=0x56)return 0;
    if(!WriteProcessMemory(child,(void *)0x2ff90,original_bytes,54,&bytes) || bytes!=54 ||
       !WriteProcessMemory(child,(void *)args[0],&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x2ff90,54);FlushInstructionCache(child,(void *)args[0],1);
    freebuf_return=args[0];freebuf_esp=context.Esp;freebuf_thread=tid;freebuf_step=0x2ff91;
    freebuf_fp=context.FloatSave;freebuf_saved_tf=context.EFlags&0x100;
    context.Eip=0x2ff90;context.EFlags|=0x100;
    if(!SetThreadContext(thread,&context))return 0;
    WriteFile(log,"freebuf-restored 0002ff90 0002ffc6\n",35,&written,0);
    return 1;
}

static int rearm_freebuf(HANDLE child,HANDLE thread,HANDLE log,DWORD tid,DWORD address)
{
    CONTEXT context;DWORD bytes,written;BYTE trap=0xcc;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL;
    if(tid!=freebuf_thread || address!=freebuf_step || !GetThreadContext(thread,&context) || context.Eip!=freebuf_step ||
       !WriteProcessMemory(child,(void *)0x2ff90,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x2ff90,1);
    context.EFlags=(context.EFlags&~0x100UL)|freebuf_saved_tf;
    if(!SetThreadContext(thread,&context))return 0;
    freebuf_step=0;
    WriteFile(log,"freebuf-rearmed 0002ff90 0002ff91\n",34,&written,0);
    return 1;
}

static int finish_freebuf(HANDLE child,HANDLE thread,HANDLE log,DWORD tid)
{
    CONTEXT context;DWORD file[8],errors[3],handle,flags,bytes;
    if(freebuf_step || tid!=freebuf_thread ||
       !freebuf_snapshot(child,thread,&context,file,errors,&handle,&flags))return 0;
    log_freebuf(log,"return",tid,&context,file,errors,handle,flags);
    if(context.Esp!=freebuf_esp+4 || memcmp(file,freebuf_file,32) || memcmp(errors,freebuf_errors,12) ||
       memcmp(&context.FloatSave,&freebuf_fp,sizeof(freebuf_fp)) || handle!=freebuf_handle || flags!=freebuf_osfile ||
       !WriteProcessMemory(child,(void *)freebuf_return,&freebuf_return_byte,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)freebuf_return,1);
    context.Eip=freebuf_return;freebuf_return=0;++freebuf_calls;
    return SetThreadContext(thread,&context);
}
