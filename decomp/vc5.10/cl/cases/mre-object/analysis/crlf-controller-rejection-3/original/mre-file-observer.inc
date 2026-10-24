/* Original-only hybrid copy. Called synchronously inside an outstanding
 * EXCEPTION_DEBUG_EVENT: Windows suspends every debuggee thread until the
 * enclosing debugger loop calls ContinueDebugEvent. No duplicate is read. */
static int mre_file_label(const char *name)
{
    unsigned i;if (!name[0] || strlen(name)>80) return 0;
    for (i=0;name[i];++i)
        if (!((name[i]>='a' && name[i]<='z') || (name[i]>='0' && name[i]<='9') || name[i]=='-')) return 0;
    return 1;
}
static int mre_file_directory(const char *name)
{
    return !strncmp(name,"S:\\vc5.10\\cl\\",13) && mre_file_label(name+13);
}
static void mre_file_ack(const char *phase)
{
    HANDLE pipe=GetStdHandle(STD_INPUT_HANDLE);DWORD start=GetTickCount(),available,got;
    char line[128],expected[128];unsigned used=0;
    sprintf(expected,"MRE_FILE_DONE %s\n",phase);
    while (GetTickCount()-start<10000) {
        if (!PeekNamedPipe(pipe,NULL,0,NULL,&available,NULL)) ExitProcess(176);
        if (!available) { Sleep(1);continue; }
        if (used+1>=sizeof(line) || !ReadFile(pipe,line+used,1,&got,NULL) || got!=1) ExitProcess(176);
        if (line[used++]=='\n') {
            line[used]=0;if (strcmp(line,expected)) ExitProcess(176);return;
        }
    }
    ExitProcess(176);
}
static void mre_file_position(HANDLE file,DWORD *position)
{
    position[1]=0;SetLastError(0);position[0]=SetFilePointer(file,0,position+1,FILE_CURRENT);
    if (position[0]==0xffffffffUL && GetLastError()) ExitProcess(177);
}
static void mre_file_copy(const char *phase,DWORD teb,DWORD tid,HANDLE thread,
    HANDLE input,HANDLE out,HANDLE err,DWORD actual_path)
{
    typedef LONG (WINAPI *QueryObject)(HANDLE,ULONG,void *,ULONG,ULONG *);
    DWORD slot,handle,position[2],after_position[2],header[8],peb,parameters;
    DWORD cwd_pointer,open_result[6],metadata_access=0,metadata_share=FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;WORD cwd_length;WCHAR cwd[MAX_PATH];char expected[MAX_PATH],path[MAX_PATH],kind[100];
    BYTE lowio[36],after_lowio[36],object_info[128];ULONG object_bytes=0;
    BY_HANDLE_FILE_INFORMATION descriptor_info,path_info,after_info;
    HANDLE duplicate,independent,reopened;CONTEXT before_context,after_context;
    QueryObject query;unsigned i,length;char actual_name[9];
    if (!debug_event_active || debug_event_pid!=debuggee_process_id || debug_event_tid!=tid ||
        !mre_file_directory(directory) || !mre_file_label(phase)) ExitProcess(178);
    memory(actual_path,actual_name,sizeof(actual_name));
    if (memcmp(actual_name,"case.idb",sizeof(actual_name))) ExitProcess(178);
    sprintf(expected,"%s\\case.idb",directory);
    /* Prove that the child's actual CWD, not just the helper's host CWD,
     * selects the fixed owned S: path. Retain the raw UTF16 chain. */
    memory(teb+0x30,&peb,4);memory(peb+0x10,&parameters,4);
    memory(parameters+0x24,&cwd_length,2);memory(parameters+0x28,&cwd_pointer,4);
    if (!cwd_length || (cwd_length&1) || cwd_length>sizeof(cwd)-2 || !cwd_pointer) ExitProcess(179);
    memory(cwd_pointer,cwd,cwd_length);cwd[cwd_length/2]=0;
    length=cwd_length/2;if (length && cwd[length-1]=='\\') --length;
    if (length!=strlen(directory)) ExitProcess(179);
    for (i=0;i<length;++i)
        if (cwd[i]!=(WCHAR)(unsigned char)directory[i] &&
            !(cwd[i]>='a' && cwd[i]<='z' && cwd[i]-'a'+'A'==(WCHAR)directory[i]) &&
            !(cwd[i]>='A' && cwd[i]<='Z' && cwd[i]-'A'+'a'==(WCHAR)directory[i])) ExitProcess(179);
    sprintf(kind,"idb-%s-cwd.bin",phase);dependency_save("mre-files",kind,cwd,cwd_length+2);
    memory(exported(crt_base,"__pioinfo"),&slot,4);memory(slot+3*36,lowio,sizeof(lowio));
    memcpy(&handle,lowio,4);
    if (lowio[4]!=0x11 || !DuplicateHandle(child,(HANDLE)handle,GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS)) ExitProcess(180);
    if (GetFileType(duplicate)!=FILE_TYPE_DISK || !GetFileInformationByHandle(duplicate,&descriptor_info)) ExitProcess(181);
    mre_file_position(duplicate,position);
    if (descriptor_info.nFileSizeHigh || !descriptor_info.nFileSizeLow || descriptor_info.nFileSizeLow>16UL*1024*1024) ExitProcess(182);
    sprintf(kind,"idb-%s-descriptor-initial.bin",phase);dependency_save("mre-files",kind,&descriptor_info,sizeof(descriptor_info));
    sprintf(kind,"idb-%s-dos-path.bin",phase);dependency_save("mre-files",kind,expected,strlen(expected)+1);
    /* Explicit metadata-only identity observation: the provider denies data
     * sharing. This requests no read/write access and makes no read-sharing
     * claim. File contents are independently copied under the reviewed host
     * observer contract, with no child/duplicate data reads. */
    independent=CreateFileA(expected,metadata_access,metadata_share,NULL,OPEN_EXISTING,0,NULL);
    open_result[0]=(DWORD)independent;open_result[1]=GetLastError();open_result[2]=0;open_result[3]=0;
    open_result[4]=metadata_access;open_result[5]=metadata_share;
    memset(&path_info,0,sizeof(path_info));
    if (independent!=INVALID_HANDLE_VALUE) {
        open_result[2]=GetFileInformationByHandle(independent,&path_info);open_result[3]=GetLastError();
    }
    sprintf(kind,"idb-%s-path-open.bin",phase);dependency_save("mre-files",kind,open_result,sizeof(open_result));
    sprintf(kind,"idb-%s-path-initial.bin",phase);dependency_save("mre-files",kind,&path_info,sizeof(path_info));
    if (independent==INVALID_HANDLE_VALUE || !open_result[2] ||
        memcmp(&descriptor_info,&path_info,sizeof(path_info))) ExitProcess(183);
    query=(QueryObject)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtQueryObject");
    memset(object_info,0,sizeof(object_info));
    if (!query || query(duplicate,0,object_info,sizeof(object_info),&object_bytes)<0 || object_bytes<8 || object_bytes>sizeof(object_info)) ExitProcess(184);
    sprintf(kind,"idb-%s-object-basic.bin",phase);dependency_save("mre-files",kind,object_info,object_bytes);
    header[0]=1;header[1]=event_number;header[2]=tid;header[3]=slot;header[4]=handle;
    header[5]=position[0];header[6]=position[1];header[7]=debuggee_process_id;
    sprintf(kind,"idb-%s-request.bin",phase);dependency_save("mre-files",kind,header,sizeof(header));
    sprintf(kind,"idb-%s-descriptor.bin",phase);dependency_save("mre-files",kind,&descriptor_info,sizeof(descriptor_info));
    sprintf(kind,"idb-%s-path.bin",phase);dependency_save("mre-files",kind,&path_info,sizeof(path_info));
    sprintf(kind,"idb-%s-lowio.bin",phase);dependency_save("mre-files",kind,lowio,sizeof(lowio));
    memset(&before_context,0,sizeof(before_context));before_context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread,&before_context)) ExitProcess(185);
    printf("MRE_FILE %s\n",phase);fflush(stdout);mre_file_ack(phase);
    memset(&after_context,0,sizeof(after_context));after_context.ContextFlags=before_context.ContextFlags;
    if (!GetThreadContext(thread,&after_context) || memcmp(&before_context,&after_context,sizeof(before_context))) ExitProcess(186);
    memory(slot+3*36,after_lowio,sizeof(after_lowio));mre_file_position(duplicate,after_position);
    if (memcmp(lowio,after_lowio,sizeof(lowio)) || memcmp(position,after_position,sizeof(position)) ||
        !GetFileInformationByHandle(duplicate,&after_info) || memcmp(&descriptor_info,&after_info,sizeof(after_info))) ExitProcess(187);
    sprintf(kind,"idb-%s-descriptor-after.bin",phase);dependency_save("mre-files",kind,&after_info,sizeof(after_info));
    if (!GetFileInformationByHandle(independent,&after_info) || memcmp(&path_info,&after_info,sizeof(after_info))) ExitProcess(187);
    sprintf(kind,"idb-%s-path-after.bin",phase);dependency_save("mre-files",kind,&after_info,sizeof(after_info));
    /* Reopen the pathname too: an unchanged held file handle alone would not
     * detect an external pathname replacement during the host copy. */
    reopened=CreateFileA(expected,metadata_access,metadata_share,NULL,OPEN_EXISTING,0,NULL);
    if (reopened==INVALID_HANDLE_VALUE || !GetFileInformationByHandle(reopened,&after_info) ||
        memcmp(&path_info,&after_info,sizeof(after_info))) ExitProcess(187);
    sprintf(kind,"idb-%s-path-reopened.bin",phase);dependency_save("mre-files",kind,&after_info,sizeof(after_info));
    CloseHandle(reopened);
    sprintf(kind,"idb-%s-lowio-after.bin",phase);dependency_save("mre-files",kind,after_lowio,sizeof(after_lowio));
    sprintf(kind,"idb-%s-position-after.bin",phase);dependency_save("mre-files",kind,after_position,sizeof(after_position));
    sprintf(kind,"idb-%s-context-before.bin",phase);dependency_save("mre-files",kind,&before_context,sizeof(before_context));
    sprintf(kind,"idb-%s-context-after.bin",phase);dependency_save("mre-files",kind,&after_context,sizeof(after_context));
    CloseHandle(independent);CloseHandle(duplicate);
    sprintf(path,"%s-copy-after",phase);dependency_state(path,teb,tid,&after_context,input,out,err);
    fprintf(dependency_events,"file-copy phase=%s event=%lu tid=%lu all-threads-held=debug-event position=%lu:%lu\n",phase,event_number,tid,position[1],position[0]);fflush(dependency_events);
}
