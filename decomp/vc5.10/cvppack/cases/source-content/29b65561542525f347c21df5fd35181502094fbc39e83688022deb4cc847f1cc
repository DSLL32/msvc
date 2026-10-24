/* Explicit reference-only stream and temporary-lifetime observations. */
static DWORD namehash_stream_arena, namehash_stream_pending, namehash_first_fill;
static DWORD namehash_free_pending, namehash_free_count;
static void namehash_hex(FILE *log, HANDLE process, const char *label, DWORD address, DWORD length)
{
    unsigned char *bytes;
    DWORD i;
    if (!length || length > 65536) { fprintf(log, "namehash-observation-failed\n"); return; }
    bytes = malloc(length);
    if (!bytes || !read_bytes(process, address, bytes, length)) { free(bytes); fprintf(log, "namehash-observation-failed\n"); return; }
    fprintf(log, "%s %08lx %lu ", label, address, length);
    for (i = 0; i < length; ++i) fprintf(log, "%02x", bytes[i]);
    fputc('\n', log); free(bytes);
}
static void namehash_complete_observation(FILE *log, HANDLE process, CONTEXT *context, DWORD site)
{
    DWORD args[3], words[6], i, kind, length;
    int copy_call = site==0x40ebb0 || site==0x40ec07 || site==0x40ec60 || site==0x40ecdc || site==0x40ed1d;
    int copy_return = site==0x40ebb5 || site==0x40ec0c || site==0x40ec65 || site==0x40ece1 || site==0x40ed22;
    int fill_call = site==0x40ebbb || site==0x40ec6a;
    int fill_return = site==0x40ebc0 || site==0x40ec6f;
    if (site == 0x410010 && !namehash_first_fill) {
        namehash_first_fill=1;
        name_hash_observation(log, process, context, site);
        namehash_hex(log, process, "namehash-first-fill-stack", context->Esp, 256);
    } else if (copy_call || fill_call) {
        if (namehash_stream_pending || !read_bytes(process, context->Esp, args, fill_call?12:4)) { fprintf(log, "namehash-observation-failed\n"); return; }
        kind=copy_call?0:1;
        namehash_stream_arena=copy_call?context->Ecx:args[0];
        length=copy_call?args[0]:args[2];
        if (!length || length>96 || !read_bytes(process,namehash_stream_arena,words,24)) { fprintf(log,"namehash-observation-failed\n"); return; }
        fprintf(log,"namehash-stream-call %lu %08lx %08lx %08lx %08lx %lu %08lx %08lx %08lx %08lx\n",kind,site,context->Esp,namehash_stream_arena,copy_call?context->Edx:args[1],length,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"namehash-stream-before",namehash_stream_arena,24);
        if(copy_call) namehash_hex(log,process,"namehash-stream-input",context->Edx,length);
        namehash_stream_pending=site;
    } else if (copy_return || fill_return) {
        if (!namehash_stream_pending || site!=namehash_stream_pending+5 || !read_bytes(process,namehash_stream_arena,words,24)) { fprintf(log,"namehash-observation-failed\n"); return; }
        fprintf(log,"namehash-stream-return %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",site,context->Esp,context->Eax,context->Ebx,context->Esi,context->Edi,context->Ebp);
        namehash_hex(log,process,"namehash-stream-after",namehash_stream_arena,24);
        if(words[1]==1 && read_bytes(process,words[2],args,12) && args[0]<=4096) namehash_hex(log,process,"namehash-stream-output",args[1],args[0]);
        else fprintf(log,"namehash-observation-failed\n");
        namehash_stream_pending=0;
    } else if (site==0x40ed87 || site==0x40ed97 || site==0x40eda8) {
        if(namehash_free_pending || !read_bytes(process,context->Esp,args,4)) { fprintf(log,"namehash-observation-failed\n");return; }
        for(i=0;i<3;++i) if(args[0]==name_hash_addresses[i]) break;
        if(args[0] && i==3) { fprintf(log,"namehash-observation-failed\n");return; }
        fprintf(log,"namehash-temp-free-call %08lx %08lx %08lx %lu %08lx %08lx %08lx %08lx %08lx\n",site,context->Esp,args[0],i,context->Edi,context->Ebx,context->Esi,context->Edi,context->Ebp);
        if(args[0]) namehash_hex(log,process,"namehash-temp-before-free",args[0],name_hash_sizes[i]);
        namehash_free_pending=site;
    } else if(site==0x40ed89 || site==0x40ed99 || site==0x40edaa) {
        if(!namehash_free_pending || site!=namehash_free_pending+2) { fprintf(log,"namehash-observation-failed\n");return; }
        fprintf(log,"namehash-temp-free-return %08lx %08lx %08lx %08lx %08lx %08lx\n",site,context->Esp,context->Ebx,context->Esi,context->Edi,context->Ebp);
        ++namehash_free_count;namehash_free_pending=0;
    } else if(site==0x40ede0) {
        if(namehash_free_count!=12 || namehash_stream_pending || namehash_free_pending || !read_bytes(process,context->Esp+60,words,24)) { fprintf(log,"namehash-observation-failed\n");return; }
        namehash_hex(log,process,"namehash-complete-arena",context->Esp+60,24);
        if(words[1]!=1 || !read_bytes(process,words[2],args,12) || args[0]!=564) { fprintf(log,"namehash-observation-failed\n");return; }
        namehash_hex(log,process,"namehash-complete-block",words[2],12);
        namehash_hex(log,process,"namehash-complete-output",args[1],args[0]);
    }
}
