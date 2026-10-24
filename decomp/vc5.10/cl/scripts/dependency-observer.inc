/* Original-only analysis observer; no native fixture includes or invokes it. */
static int dependency_analysis;
static int dependency_link_response;
static int dependency_clock_analysis;
static int dependency_clock_response;
static int dependency_complete;
static int dependency_response;
static FILE *dependency_events;
static DWORD dependency_argv,dependency_pass,dependency_c1,dependency_c1_entry;
static BYTE dependency_c1_entry_byte;
static DWORD dependency_invocations,dependency_unloads;
static unsigned dependency_putenv_calls,dependency_unlink_calls;
static char dependency_ir_prefix[MAX_PATH];
static DWORD dependency_sites[] = {0x563d,0x5655,0x5660,0x56c9,0x54f9,0x572e,0x55d2,0x4fd1,0x4fe6};
static BYTE dependency_bytes[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static BYTE dependency_after_bytes[9];
static DWORD dependency_lengths[] = {6,2,2,3,6,6,6,6,6};
static int dependency_pending[9];
static int dependency_installed[9];

static void dependency_save(const char *phase,const char *kind,const void *data,DWORD size)
{
    char name[MAX_PATH],path[MAX_PATH];FILE *file;
    sprintf(name,"observer\\%s-%s",phase,kind);output_path(path,name);
    file=fopen(path,"wb");if (!file || fwrite(data,1,size,file)!=size) ExitProcess(82);
    fclose(file);
}
static void dependency_unicode(const char *phase,const char *kind,const BYTE *descriptor)
{
    WORD length,capacity;DWORD address;BYTE *bytes;
    memcpy(&length,descriptor,2);memcpy(&capacity,descriptor+2,2);memcpy(&address,descriptor+4,4);
    if ((length&1) || capacity<length || length>32760 || !address) ExitProcess(83);
    bytes=malloc(length+2);if (!bytes) ExitProcess(84);
    memory(address,bytes,length+2);
    if (bytes[length] || bytes[length+1]) ExitProcess(85);
    dependency_save(phase,kind,bytes,length+2);free(bytes);
}
static void dependency_os_environment(const char *phase,DWORD address)
{
    WORD value,previous=1;DWORD length=0;BYTE *bytes=malloc(131072);
    if (!bytes || !address) ExitProcess(86);
    do {
        if (length+2>131072) ExitProcess(87);
        memory(address+length,&value,2);memcpy(bytes+length,&value,2);length+=2;
        if (!value && !previous) break;
        previous=value;
    } while (1);
    dependency_save(phase,"os-environment.bin",bytes,length);free(bytes);
}
static void dependency_files(const char *phase)
{
    WIN32_FIND_DATAA found;HANDLE search;char name[MAX_PATH],path[MAX_PATH];FILE *file;
    sprintf(name,"observer\\%s-directory.bin",phase);output_path(path,name);
    file=fopen(path,"wb");if (!file) ExitProcess(102);
    search=FindFirstFileA("*",&found);
    if (search==INVALID_HANDLE_VALUE) ExitProcess(103);
    do {
        if (!(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
            if (fwrite(&found,1,sizeof(found),file)!=sizeof(found)) ExitProcess(104);
        }
    } while (FindNextFileA(search,&found));
    FindClose(search);fclose(file);
}
static void dependency_state(const char *phase,DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD words[32],pointers[64],pointer,i,j,stack[16],environment,execution[5];
    BYTE parameters[96],mb[261],ctype[514],lowio[1152],iob[640];
    DWORD peb,process_parameters,ptr,base,flags,count,capacity,length;
    char name[MAX_PATH],path[MAX_PATH];FILE *file;
    /* Raw context/errors precede debugger-side DuplicateHandle/file queries. */
    dependency_save(phase,"context.bin",context,sizeof(*context));
    memory(context->Esp,stack,sizeof(stack));dependency_save(phase,"stack.bin",stack,sizeof(stack));
    error_state_to(teb,tid,phase,dependency_events);fflush(dependency_events);
    stream_state(teb,phase,input,out,err);
    memory(teb+0x30,&peb,4);memory(peb+0x10,&process_parameters,4);
    memory(process_parameters,parameters,sizeof(parameters));
    dependency_save(phase,"parameters.bin",parameters,sizeof(parameters));
    dependency_unicode(phase,"cwd-utf16.bin",parameters+0x24);
    dependency_unicode(phase,"image-utf16.bin",parameters+0x38);
    dependency_unicode(phase,"command-utf16.bin",parameters+0x40);
    memcpy(&environment,parameters+0x48,4);dependency_os_environment(phase,environment);
    memset(words,0,sizeof(words));words[0]=crt_base;
    memory(exported(crt_base,"_environ"),words+1,4);
    memory(exported(crt_base,"_wenviron"),words+2,4);
    memory(exported(crt_base,"_fmode"),words+3,4);
    memory(exported(crt_base,"_commode"),words+4,4);
    memory(exported(crt_base,"__mb_cur_max"),words+5,4);
    memory(crt_base+0x3f1c0,words+6,4);
    memory(exported(crt_base,"__lc_codepage"),words+7,4);
    memory(exported(crt_base,"__lc_handle"),words+8,24);
    memory(exported(crt_base,"_pctype"),words+14,4);
    words[15]=exported(crt_base,"__pioinfo");words[16]=exported(crt_base,"_iob");
    words[17]=peb;words[18]=process_parameters;words[19]=environment;
    words[20]=dependency_argv;words[21]=dependency_pass;words[22]=dependency_c1;
    dependency_save(phase,"crt-words.bin",words,sizeof(words));
    memory(target_base+0x97a8,execution,12);
    memory(target_base+0x9020,execution+3,4);
    memory(execution[3]+8,execution+4,4);
    dependency_save(phase,"execution-words.bin",execution,sizeof(execution));
    sprintf(name,"observer\\%s-crt-environment.bin",phase);raw_vector(name,words[1],-1);
    if (dependency_argv) { sprintf(name,"observer\\%s-caller-argv.bin",phase);raw_vector(name,dependency_argv,-1); }
    if (dependency_pass) { memory(dependency_pass,stack,36);dependency_save(phase,"caller-pass.bin",stack,36); }
    memory(crt_base+0x3f1c0,mb,4);memory(crt_base+0x3f1c8,mb+4,257);
    dependency_save(phase,"mb-state.bin",mb,sizeof(mb));
    memory(words[14]-2,ctype,sizeof(ctype));dependency_save(phase,"ctype.bin",ctype,sizeof(ctype));
    memory(words[15],pointers,sizeof(pointers));dependency_save(phase,"pioinfo-pointers.bin",pointers,sizeof(pointers));
    sprintf(name,"observer\\%s-open-descriptors.json",phase);output_path(path,name);file=fopen(path,"wb");
    if (!file) ExitProcess(88);fputs("[",file);j=0;
    for (i=0;i<64;++i) if (pointers[i]) {
        unsigned k;
        memory(pointers[i],lowio,sizeof(lowio));sprintf(name,"lowio-%lu.bin",i);
        dependency_save(phase,name,lowio,sizeof(lowio));
        for (k=0;k<32;++k) if (lowio[k*36+4]&1) {
            if (j++) fputs(",",file);
            memcpy(&pointer,lowio+k*36,4);fprintf(file,"{\"fd\":%lu,\"query\":",i*32+k);
            handle_query(file,(HANDLE)pointer,1);fputs("}",file);
        }
    }
    fputs("]\n",file);fclose(file);
    memory(words[16],iob,sizeof(iob));dependency_save(phase,"static-iob.bin",iob,sizeof(iob));
    for (i=0;i<20;++i) {
        memcpy(&ptr,iob+i*32,4);memcpy(&count,iob+i*32+4,4);memcpy(&base,iob+i*32+8,4);
        memcpy(&flags,iob+i*32+12,4);memcpy(&capacity,iob+i*32+24,4);
        length=(flags&1)?count:((flags&2) && ptr && base ? ptr-base:0);
        if ((flags&3) && length) {
            BYTE *live;
            if (!base || !ptr || length>capacity || length>65536) ExitProcess(89);
            live=malloc(length);if (!live) ExitProcess(90);
            memory((flags&1)?ptr:base,live,length);sprintf(name,"iob-%lu-live.bin",i);
            dependency_save(phase,name,live,length);free(live);
        }
    }
    dependency_files(phase);
    fprintf(dependency_events,"snapshot phase=%s event=%lu tid=%lu eax=%08lx esp=%08lx ebp=%08lx\n",phase,event_number,tid,context->Eax,context->Esp,context->Ebp);fflush(dependency_events);
}
static void dependency_copy_ir(const char *folder)
{
    char pattern[MAX_PATH],source[MAX_PATH],destination[MAX_PATH];
    WIN32_FIND_DATAA found;HANDLE search;
    if (!dependency_ir_prefix[0]) ExitProcess(91);
    sprintf(pattern,"%s*",dependency_ir_prefix);search=FindFirstFileA(pattern,&found);if (search==INVALID_HANDLE_VALUE) ExitProcess(92);
    CreateDirectoryA(folder,NULL);
    do {
        if (!(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
            sprintf(source,".\\%s",found.cFileName);sprintf(destination,"%s\\%s",folder,found.cFileName);
            if (!CopyFileA(source,destination,TRUE)) ExitProcess(93);
            fprintf(dependency_events,"c1-output name=%s size=%lu:%lu\n",found.cFileName,found.nFileSizeHigh,found.nFileSizeLow);
        }
    } while (FindNextFileA(search,&found));
    FindClose(search);fflush(dependency_events);
}
/* Validate the exact observed six-character _mktemp prefix before appending. */
static int dependency_link_paths(char *response,DWORD capacity,char *destination,DWORD destination_capacity)
{
    DWORD length=(DWORD)strlen(response),i;
    const char *folder="observer\\link-input\\";
    if (length!=8 || response[0]!='.' || response[1]!='\\') return 0;
    for (i=2;i<length;++i)
        if (!((response[i]>='a' && response[i]<='z') ||
              (response[i]>='A' && response[i]<='Z') ||
              (response[i]>='0' && response[i]<='9'))) return 0;
    if (length+2>=capacity || strlen(folder)+length>=destination_capacity) return 0;
    strcat(response,"lk");sprintf(destination,"%s%s",folder,response+2);
    return 1;
}
static void dependency_install(int masked)
{
    unsigned i;BYTE call[6];char path[MAX_PATH],line[128];FILE *file;
    output_path(path,"dependency-observer.txt");file=fopen(path,"rb");if (!file) return;
    if (!fgets(line,sizeof(line),file)) ExitProcess(94);
    if (!strcmp(line,"CL_LINK_CLOCK_ANALYSIS_V1\n")) { dependency_link_response=1;dependency_clock_analysis=1;dependency_complete=1; }
    else if (!strcmp(line,"CL_LINK_CLOCK_RESPONSE_V1\n")) { dependency_link_response=1;dependency_clock_response=1;dependency_response=1;dependency_complete=1; }
    else if (!strcmp(line,"CL_LINK_RESPONSE_V1\n")) { dependency_link_response=1;dependency_response=1;dependency_complete=1; }
    else if (!strcmp(line,"CL_DEPENDENCY_RESPONSE_V1\n")) { dependency_response=1;dependency_complete=1; }
    else if (!strcmp(line,"CL_DEPENDENCY_ANALYSIS_V2_COMPLETE\n")) dependency_complete=1;
    else if (strcmp(line,"CL_DEPENDENCY_ANALYSIS_V1\n")) ExitProcess(94);
    if (!!masked!=!!dependency_response) ExitProcess(110);
    fclose(file);dependency_analysis=1;output_path(path,"observer");CreateDirectoryA(path,NULL);
    output_path(path,"observer\\events.txt");dependency_events=fopen(path,"wb");if (!dependency_events) ExitProcess(95);
    for (i=0;i<(dependency_complete?9U:6U);++i) {
        if (dependency_link_response && i!=5 && i!=7 && i!=8) continue;
        memory(target_base+dependency_sites[i],call,dependency_lengths[i]);
        /* A fresh masked trial must never restore an unaccepted helper body.
         * In particular unload_pass can still be the next named frontier. */
        if (dependency_response && call[0]==0xcc) continue;
        if (call[0]!=0xff || (i==1 || i==2 ? call[1]!=0xd7 : i==3 ? call[1]!=0x56 || call[2]!=0x18 : call[1]!=0x15)) ExitProcess(96);
        memory(target_base+dependency_sites[i]+dependency_lengths[i],dependency_after_bytes+i,1);
        patch_byte(target_base+dependency_sites[i],0xcc);
        dependency_installed[i]=1;
    }
}
static void dependency_loaded(DWORD base,HANDLE file,IMAGE_NT_HEADERS *nt)
{
    if (!dependency_analysis) return;
    if (nt->FileHeader.TimeDateStamp==861912360UL && nt->OptionalHeader.SizeOfImage==634880UL &&
        nt->OptionalHeader.ImageBase==0x10600000UL && nt->OptionalHeader.AddressOfEntryPoint==0x1202aUL) {
        dependency_c1=base;dependency_c1_entry=base+nt->OptionalHeader.AddressOfEntryPoint;
        save_file(file,"observer\\loaded-c1.bin");memory(dependency_c1_entry,&dependency_c1_entry_byte,1);
        fprintf(dependency_events,"c1-load event=%lu base=%08lx entry=%08lx entry_byte=%02x\n",event_number,base,dependency_c1_entry,dependency_c1_entry_byte);
        patch_byte(dependency_c1_entry,0xcc);fflush(dependency_events);
    }
}
static int dependency_point(DWORD address)
{
    unsigned i;
    if (!dependency_analysis) return -1;
    if (address==dependency_c1_entry) return 9;
    for (i=0;i<(dependency_complete?9U:6U);++i)
        if (dependency_installed[i] && (address==target_base+dependency_sites[i] || (dependency_pending[i] && address==target_base+dependency_sites[i]+dependency_lengths[i]))) return i;
    return -1;
}
static void dependency_trap(int point,DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    char phase[64];DWORD args[3],actual;
    static const char *names[] = {"load","get-invoke","get-abort","invoke","free","spawn","putenv","unlink"};
    if (point==9) {
        memory(context->Esp+4,args,12);
        fprintf(dependency_events,"c1-entry event=%lu module=%08lx reason=%lu reserved=%08lx\n",event_number,args[0],args[1],args[2]);
        sprintf(phase,"c1-entry-%lu",args[1]);dependency_state(phase,teb,tid,context,input,out,err);
        patch_byte(address,dependency_c1_entry_byte);context->Eip=address;return;
    }
    if (point==8) {
        DWORD state,status;
        dependency_argv=0;
        memory(target_base+0x9020,&state,4);memory(state+8,&dependency_pass,4);
        memory(context->Esp,&status,4);memory(target_base+0xa254,&actual,4);
        if (actual!=exported(crt_base,"exit")) ExitProcess(114);
        dependency_state("exit-before",teb,tid,context,input,out,err);
        fprintf(dependency_events,"exit-call status=%lu function=%08lx esp=%08lx\n",status,actual,context->Esp);fflush(dependency_events);
        patch_byte(address,dependency_bytes[point]);context->Eip=address;return;
    }
    sprintf(phase,"%s-%s",names[point],dependency_pending[point]?"after":"before");
    if (point==6) {
        if (!dependency_pending[point]) ++dependency_putenv_calls;
        if (dependency_putenv_calls>2) ExitProcess(106);
        sprintf(phase,"putenv-%u-%s",dependency_putenv_calls,dependency_pending[point]?"after":"before");
    }
    if (point==7) {
        DWORD stack[16],argument;char filename[1024];
        if (!dependency_pending[point]) ++dependency_unlink_calls;
        if (dependency_unlink_calls>7) ExitProcess(112);
        sprintf(phase,"unlink-%u-%s",dependency_unlink_calls,dependency_pending[point]?"after":"before");
        dependency_save(phase,"context.bin",context,sizeof(*context));
        memory(context->Esp,stack,sizeof(stack));dependency_save(phase,"stack.bin",stack,sizeof(stack));
        error_state_to(teb,tid,phase,dependency_events);fflush(dependency_events);
        if (!dependency_pending[point]) {
            argument=stack[0];remote_string(argument,filename,sizeof(filename));
            memory(target_base+0xa290,&actual,4);
            if (actual!=exported(crt_base,"_unlink")) ExitProcess(113);
            dependency_save(phase,"filename.bin",filename,strlen(filename)+1);
            fprintf(dependency_events,"unlink-call sequence=%u function=%08lx argument=%08lx esp=%08lx\n",dependency_unlink_calls,actual,argument,context->Esp);
        } else
            fprintf(dependency_events,"unlink-return sequence=%u result=%08lx esp=%08lx\n",dependency_unlink_calls,context->Eax,context->Esp);
        dependency_files(phase);fflush(dependency_events);
        if (!dependency_pending[point]) {
            dependency_pending[point]=1;patch_byte(address,dependency_bytes[point]);
            patch_byte(address+dependency_lengths[point],0xcc);
        } else {
            dependency_pending[point]=0;patch_byte(address,dependency_after_bytes[point]);
            patch_byte(target_base+dependency_sites[point],0xcc);
        }
        context->Eip=address;return;
    }
    if (!dependency_pending[point]) {
        if (point==0 || point==6) { memory(context->Ebp+0x18,&dependency_argv,4);memory(context->Ebp+0xc,&dependency_pass,4); }
        if (point==6) {
            DWORD assignment;char text[32768];
            memory(context->Esp,&assignment,4);remote_string(assignment,text,sizeof(text));
            dependency_save(phase,"assignment.bin",text,strlen(text)+1);
        }
        if (point==0 || point==1 || point==2) {
            DWORD parameters[2],function;char text[1024],kind[80];
            memory(context->Esp,parameters,point==0 ? 4 : 8);
            if (point) {
                memory(target_base+0xa1d8,&function,4);
                if (context->Edi!=function || parameters[0]!=dependency_c1) ExitProcess(105);
            }
            remote_string(parameters[point ? 1 : 0],text,sizeof(text));
            sprintf(kind,"argument.bin");dependency_save(phase,kind,text,strlen(text)+1);
        }
        if (point==3) {
            memory(context->Esp,args,12);memory(dependency_pass+24,&actual,4);
            if (actual!=exported(dependency_c1,"_InvokeCompilerPass@12")) ExitProcess(97);
            {
                DWORD index,pointer;char token[MAX_PATH];int next=0;
                for (index=0;index<4096;++index) {
                    memory(dependency_argv+index*4,&pointer,4);if (!pointer) break;
                    remote_string(pointer,token,sizeof(token));
                    if (next) { strcpy(dependency_ir_prefix,token);break; }
                    next=!strcmp(token,"-il");
                }
                if (!next || index==4096) ExitProcess(101);
            }
            ++dependency_invocations;fprintf(dependency_events,"invoke-call argc=%lu argv=%08lx last=%lu function=%08lx esp=%08lx\n",args[0],args[1],args[2],actual,context->Esp);
        }
        if (point==5) { memory(context->Ebp+0x18,&dependency_argv,4);memory(context->Ebp+0xc,&dependency_pass,4); }
        if (point==4 && dependency_c1_entry) patch_byte(dependency_c1_entry,0xcc);
        dependency_state(phase,teb,tid,context,input,out,err);
        if (point==5) {
            if (!dependency_complete) {
                fprintf(dependency_events,"analysis-stop-before-c2 invocations=%lu unloads=%lu\n",dependency_invocations,dependency_unloads);fflush(dependency_events);
                TerminateProcess(child,0x7003);return;
            } else {
                DWORD parameters[3],path_pointer,terminator;char executable[MAX_PATH],argument[MAX_PATH];HANDLE image;
                memory(context->Esp,parameters,12);memory(parameters[2],&path_pointer,4);
                memory(parameters[2]+4,&terminator,4);
                if (parameters[0]!=0 || (!dependency_link_response && terminator)) ExitProcess(107);
                remote_string(parameters[1],executable,sizeof(executable));
                remote_string(path_pointer,argument,sizeof(argument));
                if (strcmp(executable,argument)) ExitProcess(107);
                image=CreateFileA(executable,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
                if (image==INVALID_HANDLE_VALUE) ExitProcess(108);
                save_file(image,"observer\\spawn-executable.bin");CloseHandle(image);
                if (dependency_link_response) {
                    DWORD state,prefix;char response[MAX_PATH],destination[MAX_PATH];
                    CreateDirectoryA("observer\\link-input",NULL);
                    if (!CopyFileA("test.obj","observer\\link-input\\test.obj",TRUE)) ExitProcess(115);
                    memory(target_base+0x9020,&state,4);memory(state+32,&prefix,4);
                    remote_string(prefix,response,sizeof(response));
                    if (!dependency_link_paths(response,sizeof(response),destination,sizeof(destination))) ExitProcess(118);
                    if (!CopyFileA(response,destination,TRUE)) ExitProcess(116);
                } else dependency_copy_ir("observer\\c2-input");
            }
        }
        dependency_pending[point]=1;patch_byte(address,dependency_bytes[point]);
        patch_byte(address+dependency_lengths[point],0xcc);
    } else {
        dependency_state(phase,teb,tid,context,input,out,err);
        if (point==3) dependency_copy_ir("observer\\c1-output");
        if (point==5) {
            if (dependency_link_response) {
                CreateDirectoryA("observer\\link-output",NULL);
                if (!CopyFileA("test.exe","observer\\link-output\\test.exe",TRUE)) ExitProcess(117);
            } else {
                CreateDirectoryA("observer\\c2-output",NULL);
                if (!CopyFileA("test.obj","observer\\c2-output\\test.obj",TRUE)) ExitProcess(109);
            }
            if (dependency_link_response)
                fprintf(dependency_events,"observed-child family=link result=%08lx putenv_calls=%u\n",context->Eax,dependency_putenv_calls);
            else
                fprintf(dependency_events,"analysis-c2-completed result=%08lx putenv_calls=%u\n",context->Eax,dependency_putenv_calls);
        }
        fprintf(dependency_events,"return api=%s result=%08lx esp=%08lx\n",names[point],context->Eax,context->Esp);fflush(dependency_events);
        dependency_pending[point]=0;
        patch_byte(address,dependency_after_bytes[point]);
        patch_byte(target_base+dependency_sites[point],0xcc);
    }
    context->Eip=address;
}
