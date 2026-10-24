/* Original debugger transport for the first, existing mapped input open. */
static char mapped_slot_name[260];
static DWORD mapped_open_active, mapped_open_step, mapped_open_tf;
static DWORD mapped_open_slot, mapped_open_esp, mapped_open_thread, mapped_open_handle;
static BYTE mapped_open_return_byte;

static int arm_mapped_open(HANDLE child)
{
    BYTE trap=0xcc;DWORD bytes;
    return WriteProcessMemory(child,(void *)0x2aa90,&trap,1,&bytes) && bytes==1 &&
        FlushInstructionCache(child,(void *)0x2aa90,1);
}

static int begin_mapped_open(HANDLE child,HANDLE thread,HANDLE log,DWORD tid)
{
    CONTEXT context;DWORD args[7],bytes,written,name_pointer,i;BYTE call=0xe8,trap=0xcc;
    char name[260],line[1024];int used;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(mapped_open_active || mapped_open_step || !GetThreadContext(thread,&context) || (context.EFlags&0x100) ||
       !ReadProcessMemory(child,(void *)context.Esp,args,28,&bytes) || bytes!=28 ||
       !ReadProcessMemory(child,(void *)context.Esi,&name_pointer,4,&bytes) || bytes!=4 || name_pointer!=args[0])return 0;
    for(i=0;i<sizeof(name);++i) {
        if(!ReadProcessMemory(child,(void *)(args[0]+i),name+i,1,&bytes) || bytes!=1)return 0;
        if(!name[i])break;
    }
    if(i==sizeof(name) || strcmp(name,mapped_slot_name) || args[1]!=0xc0000000 || args[2] || args[3] ||
       args[4]!=OPEN_ALWAYS || args[5]!=FILE_ATTRIBUTE_NORMAL || args[6])return 0;
    used=sprintf(line,"mapped-open-request thread %lu slot %08lx esp %08lx args",tid,context.Esi,context.Esp);
    for(i=0;i<7;++i)used+=sprintf(line+used," %08lx",args[i]);
    used+=sprintf(line+used," name ");for(i=0;name[i];++i)used+=sprintf(line+used,"%02x",(BYTE)name[i]);
    line[used++]='\n';WriteFile(log,line,used,&written,0);
    if(!ReadProcessMemory(child,(void *)0x2aa95,&mapped_open_return_byte,1,&bytes) || bytes!=1 || mapped_open_return_byte!=0x89 ||
       !WriteProcessMemory(child,(void *)0x2aa90,&call,1,&bytes) || bytes!=1 ||
       !WriteProcessMemory(child,(void *)0x2aa95,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,(void *)0x2aa90,6);
    mapped_open_active=1;mapped_open_step=0x2be52;mapped_open_tf=context.EFlags&0x100;
    mapped_open_slot=context.Esi;mapped_open_esp=context.Esp;mapped_open_thread=tid;
    context.Eip=0x2aa90;context.EFlags|=0x100;
    return SetThreadContext(thread,&context);
}

static int step_mapped_open(HANDLE child,HANDLE thread,DWORD tid,DWORD address)
{
    CONTEXT context;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_CONTROL;
    if(tid!=mapped_open_thread || address!=mapped_open_step || !GetThreadContext(thread,&context) ||
       context.Eip!=mapped_open_step || !arm_mapped_open(child))return 0;
    context.EFlags=(context.EFlags&~0x100UL)|mapped_open_tf;mapped_open_step=0;
    return SetThreadContext(thread,&context);
}

static int finish_mapped_open(HANDLE child,HANDLE thread,HANDLE log,DWORD tid)
{
    CONTEXT context;DWORD bytes,written;char line[128];
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(mapped_open_step || tid!=mapped_open_thread || !GetThreadContext(thread,&context) ||
       context.Esi!=mapped_open_slot || context.Esp!=mapped_open_esp+28 || context.Eax==0xffffffff || !context.Eax ||
       !WriteProcessMemory(child,(void *)0x2aa95,&mapped_open_return_byte,1,&bytes) || bytes!=1)return 0;
    mapped_open_handle=context.Eax;mapped_open_active=0;
    sprintf(line,"mapped-open-return thread %lu slot %08lx handle %08lx\n",tid,mapped_open_slot,mapped_open_handle);
    WriteFile(log,line,strlen(line),&written,0);FlushInstructionCache(child,(void *)0x2aa95,1);
    context.Eip=0x2aa95;return SetThreadContext(thread,&context);
}

static int dump_mapped_source(HANDLE child,HANDLE log,DWORD pointer,DWORD *slot)
{
    HANDLE duplicate;BY_HANDLE_FILE_INFORMATION info;DWORD type,position,after,written;char line[256];
    if(mapped_open_active || mapped_open_step || pointer!=mapped_open_slot || slot[4]!=mapped_open_handle ||
       !DuplicateHandle(child,(HANDLE)slot[4],GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS))return 0;
    type=GetFileType(duplicate);position=SetFilePointer(duplicate,0,0,FILE_CURRENT);
    if(!GetFileInformationByHandle(duplicate,&info)) { CloseHandle(duplicate);return 0; }
    after=SetFilePointer(duplicate,0,0,FILE_CURRENT);CloseHandle(duplicate);
    if(type!=FILE_TYPE_DISK || position!=0 || after!=position || info.nFileSizeHigh || info.nFileSizeLow!=slot[3])return 0;
    sprintf(line,"mapped-slot-source slot %08lx handle %08lx type %lu size %lu position %lu after %lu volume %lu index %08lx%08lx attrs %lu\n",
        pointer,slot[4],type,info.nFileSizeLow,position,after,info.dwVolumeSerialNumber,
        info.nFileIndexHigh,info.nFileIndexLow,info.dwFileAttributes);
    return WriteFile(log,line,strlen(line),&written,0);
}

static DWORD mapped_api_active,mapped_api_thread,mapped_api_esp,mapped_api_callback;
static DWORD mapped_api_mapping,mapped_api_view;
static BYTE mapped_api_return_byte;

static int arm_mapping_calls(HANDLE child)
{
    BYTE trap=0xcc;DWORD bytes;
    return WriteProcessMemory(child,(void *)0x2ad55,&trap,1,&bytes) && bytes==1 &&
        WriteProcessMemory(child,(void *)0x2ad9d,&trap,1,&bytes) && bytes==1 && FlushInstructionCache(child,0,0);
}

static int begin_mapping_call(HANDLE child,HANDLE thread,HANDLE log,DWORD tid,DWORD address)
{
    CONTEXT context;DWORD args[6],slot[10],bytes,written,i,count,returned,callback;
    BYTE call=0xff,trap=0xcc;char line[256];int used;
    count=address==0x2ad55?6:5;returned=address+6;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(mapped_api_active || !GetThreadContext(thread,&context) || context.Esi!=mapped_open_slot ||
       !ReadProcessMemory(child,(void *)context.Esp,args,count*4,&bytes) || bytes!=count*4 ||
       !ReadProcessMemory(child,(void *)context.Esi,slot,40,&bytes) || bytes!=40 ||
       !ReadProcessMemory(child,(void *)(address==0x2ad55?0x38ce4:0x38ce8),&callback,4,&bytes) || bytes!=4 || !callback)return 0;
    if(slot[4]!=mapped_open_handle || slot[3]!=0x200000 || slot[7]!=0x200000 || slot[1]!=3)return 0;
    if(count==6) {
        if(mapped_api_mapping || args[0]!=slot[4] || args[1] || args[2]!=PAGE_READWRITE || args[3] || args[4]!=slot[7] || args[5])return 0;
    } else if(!mapped_api_mapping || mapped_api_view || args[0]!=mapped_api_mapping || slot[5]!=mapped_api_mapping ||
        args[1]!=FILE_MAP_ALL_ACCESS || args[2] || args[3] || args[4])return 0;
    used=sprintf(line,"mapped-api-request stage %lu thread %lu slot %08lx esp %08lx callback %08lx args",count==6?1UL:2UL,tid,context.Esi,context.Esp,callback);
    for(i=0;i<count;++i)used+=sprintf(line+used," %08lx",args[i]);line[used++]='\n';WriteFile(log,line,used,&written,0);
    if(!ReadProcessMemory(child,(void *)returned,&mapped_api_return_byte,1,&bytes) || bytes!=1 || mapped_api_return_byte==trap ||
       !WriteProcessMemory(child,(void *)address,&call,1,&bytes) || bytes!=1 ||
       !WriteProcessMemory(child,(void *)returned,&trap,1,&bytes) || bytes!=1)return 0;
    FlushInstructionCache(child,0,0);mapped_api_active=address;mapped_api_thread=tid;mapped_api_esp=context.Esp;mapped_api_callback=callback;
    context.Eip=address;return SetThreadContext(thread,&context);
}

static int finish_mapping_call(HANDLE child,HANDLE thread,HANDLE log,DWORD tid,DWORD address)
{
    CONTEXT context;DWORD bytes,written,count;BYTE trap=0xcc;char line[160];
    count=mapped_api_active==0x2ad55?6:5;
    memset(&context,0,sizeof(context));context.ContextFlags=CONTEXT_FULL;
    if(!mapped_api_active || address!=mapped_api_active+6 || tid!=mapped_api_thread || !GetThreadContext(thread,&context) ||
       context.Esi!=mapped_open_slot || context.Esp!=mapped_api_esp+count*4 || !context.Eax ||
       !WriteProcessMemory(child,(void *)address,&mapped_api_return_byte,1,&bytes) || bytes!=1 ||
       !WriteProcessMemory(child,(void *)mapped_api_active,&trap,1,&bytes) || bytes!=1)return 0;
    if(count==6)mapped_api_mapping=context.Eax;else mapped_api_view=context.Eax;
    sprintf(line,"mapped-api-return stage %lu thread %lu slot %08lx esp %08lx value %08lx\n",count==6?1UL:2UL,tid,context.Esi,context.Esp,context.Eax);
    WriteFile(log,line,strlen(line),&written,0);FlushInstructionCache(child,0,0);mapped_api_active=0;
    context.Eip=address;return SetThreadContext(thread,&context);
}
