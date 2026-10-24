/* Original debugger only: complete address sort/stream/lifetime observations. */
static DWORD ah_sort_count, ah_callback_count, ah_inflight, ah_callback_esp, ah_callback_return;
static DWORD ah_stream_pending, ah_stream_arena, ah_free_pending;
static void ah_environment(FILE *log,HANDLE process,CONTEXT *context,const char *phase)
{
    address_sort_environment_mode=1;
    sort_environment(log,process,context,phase);
    address_sort_environment_mode=0;
}
static void ah_complete(FILE *log,HANDLE process,CONTEXT *context,DWORD site)
{
    DWORD args[4],words[6],index,length,kind;
    int copy=site==0x40ef7b || site==0x40efa5 || site==0x40efda || site==0x40f01e || site==0x40f030;
    int fill=site==0x40ef86 || site==0x40efe4;
    int returned=site==0x40ef80 || site==0x40efaa || site==0x40efdf || site==0x40f023 || site==0x40f035 || site==0x40ef8b || site==0x40efe9;
    if(site==0x40ef4b) {
        if(ah_sort_count>=3 || ah_inflight || !read_bytes(process,context->Esp,args,16)) goto failed;
        fprintf(log,"address-sort-call %lu %08lx %08lx %lu %lu %08lx %08lx\n",ah_sort_count,context->Esp,args[0],args[1],args[2],args[3],context->Ebx);
        address_hash_owned(log,process,ah_sort_count==0?"sort0-before":ah_sort_count==1?"sort1-before":"sort2-before");
        ah_environment(log,process,context,"before");
    } else if(site==0x40ef4d) {
        if(ah_sort_count>=3 || ah_inflight) goto failed;
        fprintf(log,"address-sort-return %lu %08lx %lu\n",ah_sort_count,context->Esp,ah_callback_count);
        address_hash_owned(log,process,ah_sort_count==0?"sort0-after":ah_sort_count==1?"sort1-after":"sort2-after");
        ah_environment(log,process,context,"after");++ah_sort_count;
    } else if(site==0x40edc4) {
        unsigned char rows[16];
        if(ah_sort_count>=3 || ah_inflight || ah_callback_count>=4096 || !read_bytes(process,context->Esp,args,12) ||
            (args[0]!=loaded_crt_base+0x2551e && args[0]!=loaded_crt_base+0x25534 && args[0]!=loaded_crt_base+0x2560c) ||
            !read_bytes(process,args[1],rows,8) || !read_bytes(process,args[2],rows+8,8)) goto failed;
        ah_inflight=1;ah_callback_esp=context->Esp;ah_callback_return=args[0];
        fprintf(log,"address-callback-call %lu %lu %08lx %08lx %08lx %08lx ",ah_sort_count,ah_callback_count,context->Esp,args[0],args[1],args[2]);
        for(index=0;index<16;++index) fprintf(log,"%02x",rows[index]);fputc('\n',log);
        ah_environment(log,process,context,"entry");
    } else if(site==0x40edd6) {
        if(!ah_inflight || context->Esp!=ah_callback_esp || !read_bytes(process,context->Esp,args,4) || args[0]!=ah_callback_return) goto failed;
        fprintf(log,"address-callback-return %lu %lu %08lx %08lx %ld\n",ah_sort_count,ah_callback_count,context->Esp,args[0],(LONG)context->Eax);
        ah_environment(log,process,context,"return");ah_inflight=0;++ah_callback_count;
    } else if(site==0x40ef6b) {
        if(ah_sort_count!=3 || !read_bytes(process,context->Esp,args,8)) goto failed;
        ah_stream_arena=args[0];fprintf(log,"address-arena-init-call %08lx %08lx %lu %08lx %08lx %08lx %08lx\n",context->Esp,args[0],args[1],context->Ebx,context->Esi,context->Edi,context->Ebp);
    } else if(site==0x40ef70) {
        fprintf(log,"address-arena-init-return %08lx %08lx %08lx %08lx %08lx\n",context->Esp,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"address-arena-initialized",ah_stream_arena,24);
    } else if(copy || fill) {
        if(ah_stream_pending || ah_sort_count!=3 || !read_bytes(process,context->Esp,args,fill?12:4)) goto failed;
        kind=copy?0:1;length=copy?args[0]:args[2];
        if((copy?context->Ecx:args[0])!=ah_stream_arena || !length || length>4) goto failed;
        fprintf(log,"address-stream-call %lu %08lx %08lx %08lx %08lx %lu %08lx %08lx %08lx %08lx\n",kind,site,context->Esp,ah_stream_arena,copy?context->Edx:args[1],length,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"address-stream-before",ah_stream_arena,24);
        if(copy) namehash_hex(log,process,"address-stream-input",context->Edx,length);
        ah_stream_pending=site;
    } else if(returned) {
        if(!ah_stream_pending || site!=ah_stream_pending+5 || !read_bytes(process,ah_stream_arena,words,24)) goto failed;
        fprintf(log,"address-stream-return %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",site,context->Esp,context->Eax,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"address-stream-after",ah_stream_arena,24);
        if(words[1]!=1 || !read_bytes(process,words[2],args,12) || !args[0] || args[0]>4096) goto failed;
        namehash_hex(log,process,"address-stream-output",args[1],args[0]);ah_stream_pending=0;
    } else if(site==0x40f09b || site==0x40f0b9) {
        if(ah_free_pending || ah_free_count>=4 || !read_bytes(process,context->Esp,args,4)) goto failed;
        index=ah_free_count<3?ah_free_count+1:0;
        if(args[0]!=address_hash_addresses[index]) goto failed;
        fprintf(log,"address-temp-free-call %lu %08lx %08lx %08lx %lu %08lx %08lx %08lx %08lx %08lx\n",ah_free_count,site,context->Esp,args[0],index,context->Ebx,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"address-temp-before-free",args[0],address_hash_sizes[index]);ah_free_pending=site;
    } else if(site==0x40f09d || site==0x40f0bb) {
        if(!ah_free_pending || site!=ah_free_pending+2) goto failed;
        fprintf(log,"address-temp-free-return %08lx %08lx %08lx %08lx %08lx %08lx\n",site,context->Esp,context->Ebx,context->Esi,context->Edi,context->Ebp);ah_free_pending=0;++ah_free_count;
    } else if(site==0x4100c0) {
        if(ah_free_count!=4 || ah_stream_pending || ah_free_pending || !read_bytes(process,ah_stream_arena,words,24)) goto failed;
        namehash_hex(log,process,"address-complete-arena",ah_stream_arena,24);
        if(words[1]!=1 || !read_bytes(process,words[2],args,12) || args[0]!=540) goto failed;
        namehash_hex(log,process,"address-complete-block",words[2],12);
        namehash_hex(log,process,"address-complete-output",args[1],args[0]);
        ah_environment(log,process,context,"frontier");
    }
    return;
failed:
    fprintf(log,"address-complete-observation-failed\n");
}
