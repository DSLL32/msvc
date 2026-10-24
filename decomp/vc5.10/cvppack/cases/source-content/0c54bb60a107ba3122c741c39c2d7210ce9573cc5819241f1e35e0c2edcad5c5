/* Original-only four-generation address-hash inputs and first qsort boundary. */
static DWORD address_hash_sizes[4], address_hash_addresses[4], address_hash_count;
static DWORD address_hash_pending, address_hash_pending_site, address_hash_started, address_hash_sort_seen;
static void address_hash_owned(FILE *log,HANDLE process,const char *phase)
{
    DWORD index,offset;
    unsigned char *bytes;
    if(address_hash_count!=4 || address_hash_pending) { fprintf(log,"addresshash-observation-failed\n");return; }
    for(index=0;index<4;++index) {
        bytes=malloc(address_hash_sizes[index]);
        if(!bytes || !read_bytes(process,address_hash_addresses[index],bytes,address_hash_sizes[index])) { free(bytes);fprintf(log,"addresshash-observation-failed\n");return; }
        fprintf(log,"addresshash-owned %s %08lx %lu ",phase,address_hash_addresses[index],address_hash_sizes[index]);
        for(offset=0;offset<address_hash_sizes[index];++offset) fprintf(log,"%02x",bytes[offset]);
        fputc('\n',log);free(bytes);
    }
}
static void address_hash_observation(FILE *log,HANDLE process,CONTEXT *context,DWORD site)
{
    DWORD args[6],index,size;
    if(site==0x40ede0) {
        if(address_hash_started || namehash_free_count!=12 || !read_bytes(process,context->Esp,args,24)) { fprintf(log,"addresshash-observation-failed\n");return; }
        address_hash_started=1;
        fprintf(log,"addresshash-entry %08lx",context->Esp);for(index=0;index<6;++index) fprintf(log," %08lx",args[index]);fputc('\n',log);
        namehash_hex(log,process,"addresshash-parent-stack",context->Esp,128);
    } else if(site==0x40ee42 || site==0x40eeaf) {
        if(!address_hash_started || address_hash_pending || address_hash_count>=4 || namehash_free_count!=12 || !read_bytes(process,context->Esp,&size,4) || !size || size>65536) { fprintf(log,"addresshash-observation-failed\n");return; }
        address_hash_pending=size;address_hash_pending_site=site;
        fprintf(log,"addresshash-allocation-call %08lx %lu %lu %08lx %08lx %08lx %08lx %08lx\n",site,size,namehash_free_count,context->Esp,context->Ebx,context->Esi,context->Edi,context->Ebp);
    } else if(site==0x40ee47 || site==0x40eeb4) {
        if(!address_hash_pending || site!=address_hash_pending_site+5 || !context->Eax) { fprintf(log,"addresshash-observation-failed\n");return; }
        address_hash_sizes[address_hash_count]=address_hash_pending;address_hash_addresses[address_hash_count++]=context->Eax;
        fprintf(log,"addresshash-allocation-return %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",site,context->Eax,context->Esp,context->Ebx,context->Esi,context->Edi,context->Ebp);address_hash_pending=0;
    } else if(site==0x40ef4b) {
        if(address_hash_sort_seen && address_hash_complete_mode) return;
        if(address_hash_sort_seen || !read_bytes(process,context->Esp,args,16)) { fprintf(log,"addresshash-observation-failed\n");return; }
        {
            WORD maximum;
            DWORD absolute,position,groups;
            if(!read_bytes(process,context->Esp+32,&maximum,2) || !read_bytes(process,context->Esp+36,&position,4) ||
                !read_bytes(process,context->Esp+44,&groups,4) || !read_bytes(process,context->Esp+48,&absolute,4)) { fprintf(log,"addresshash-observation-failed\n");return; }
            fprintf(log,"addresshash-sort-locals %u %lu %lu %lu %08lx\n",maximum,absolute,position,context->Edi,groups);
        }
        address_hash_sort_seen=1;
        fprintf(log,"addresshash-sort-call %08lx %08lx %lu %lu %08lx %08lx\n",context->Esp,args[0],args[1],args[2],args[3],context->Ebx);
        address_hash_owned(log,process,"before-sort");
    } else if(site==0x40edc0) {
        if(!address_hash_sort_seen) { fprintf(log,"addresshash-observation-failed\n");return; }
        address_hash_owned(log,process,"first-callback");
    }
}
