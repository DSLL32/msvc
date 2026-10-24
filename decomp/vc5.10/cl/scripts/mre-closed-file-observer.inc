/* Only after a genuine Release/Close return reports fd3 closed. Its former
 * authenticated FileId identifies the path, never a guessed live descriptor. */
static int mre_closed_state(int enabled,const char *phase,DWORD slot,DWORD handle,BYTE flags)
{
    return enabled && (!strcmp(phase,"mre-5-after") || !strcmp(phase,"mre-6-after")) &&
        mre_file_have_live && slot==mre_file_last_slot && handle==0xffffffffUL && !(flags&1);
}
static int mre_same_file_id(const BY_HANDLE_FILE_INFORMATION *a,const BY_HANDLE_FILE_INFORMATION *b)
{
    return a->dwVolumeSerialNumber==b->dwVolumeSerialNumber &&
        a->nFileIndexHigh==b->nFileIndexHigh && a->nFileIndexLow==b->nFileIndexLow;
}
static void mre_closed_file_copy(const char *phase,DWORD teb,DWORD tid,HANDLE thread,
    HANDLE input,HANDLE out,HANDLE err,DWORD actual_path)
{
    DWORD slot,handle,header[8],peb,parameters,cwd_pointer,opened[6];WORD cwd_length;
    BYTE lowio[36],after_lowio[36];WCHAR cwd[MAX_PATH];
    char actual_name[9],expected[MAX_PATH],kind[100],post_phase[80];unsigned length,i;
    HANDLE independent,reopened;BY_HANDLE_FILE_INFORMATION info,after;
    CONTEXT before_context,after_context;
    if (!debug_event_active || debug_event_pid!=debuggee_process_id || debug_event_tid!=tid ||
        !mre_file_directory(directory)) ExitProcess(197);
    memory(actual_path,actual_name,9);if (memcmp(actual_name,"case.idb",9)) ExitProcess(197);
    memory(exported(crt_base,"__pioinfo"),&slot,4);memory(slot+108,lowio,36);memcpy(&handle,lowio,4);
    if (!mre_closed_state(mre_jobs_observer,phase,slot,handle,lowio[4])) ExitProcess(197);
    memory(teb+0x30,&peb,4);memory(peb+0x10,&parameters,4);
    memory(parameters+0x24,&cwd_length,2);memory(parameters+0x28,&cwd_pointer,4);
    if (!cwd_length || (cwd_length&1) || cwd_length>sizeof(cwd)-2 || !cwd_pointer) ExitProcess(197);
    memory(cwd_pointer,cwd,cwd_length);cwd[cwd_length/2]=0;
    length=cwd_length/2;if (length && cwd[length-1]=='\\') --length;
    if (length!=strlen(directory)) ExitProcess(197);
    for (i=0;i<length;++i)
        if (cwd[i]!=(WCHAR)(unsigned char)directory[i] &&
            !(cwd[i]>='a' && cwd[i]<='z' && cwd[i]-'a'+'A'==(WCHAR)directory[i]) &&
            !(cwd[i]>='A' && cwd[i]<='Z' && cwd[i]-'A'+'a'==(WCHAR)directory[i])) ExitProcess(197);
    sprintf(expected,"%s\\case.idb",directory);
    independent=CreateFileA(expected,0,7,NULL,OPEN_EXISTING,0,NULL);
    opened[0]=(DWORD)independent;opened[1]=GetLastError();opened[2]=0;opened[3]=0;opened[4]=0;opened[5]=7;
    memset(&info,0,sizeof(info));
    if (independent!=INVALID_HANDLE_VALUE) {opened[2]=GetFileInformationByHandle(independent,&info);opened[3]=GetLastError();}
    sprintf(kind,"idb-%s-path-open.bin",phase);dependency_save("mre-files",kind,opened,sizeof(opened));
    if (independent==INVALID_HANDLE_VALUE || !opened[2] || !mre_same_file_id(&info,&mre_file_last_live) ||
        info.dwFileAttributes&0x410 /* directory or reparse point; old SDK lacks the latter name */ ||
        info.nFileSizeHigh || info.nFileSizeLow>16UL*1024*1024) ExitProcess(198);
    header[0]=2;header[1]=event_number;header[2]=tid;header[3]=slot;header[4]=handle;
    header[5]=0;header[6]=0;header[7]=debuggee_process_id; /* Reserved: no live cursor. */
    sprintf(kind,"idb-%s-request.bin",phase);dependency_save("mre-files",kind,header,32);
    sprintf(kind,"idb-%s-last-live.bin",phase);dependency_save("mre-files",kind,&mre_file_last_live,sizeof(mre_file_last_live));
    sprintf(kind,"idb-%s-path.bin",phase);dependency_save("mre-files",kind,&info,sizeof(info));
    sprintf(kind,"idb-%s-cwd.bin",phase);dependency_save("mre-files",kind,cwd,cwd_length+2);
    sprintf(kind,"idb-%s-dos-path.bin",phase);dependency_save("mre-files",kind,expected,strlen(expected)+1);
    sprintf(kind,"idb-%s-lowio.bin",phase);dependency_save("mre-files",kind,lowio,36);
    memset(&before_context,0,sizeof(before_context));before_context.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    if (!GetThreadContext(thread,&before_context)) ExitProcess(199);
    printf("MRE_FILE %s\n",phase);fflush(stdout);mre_file_ack(phase,header);
    memset(&after_context,0,sizeof(after_context));after_context.ContextFlags=before_context.ContextFlags;
    if (!GetThreadContext(thread,&after_context) || memcmp(&before_context,&after_context,sizeof(before_context))) ExitProcess(199);
    memory(slot+108,after_lowio,36);
    if (memcmp(lowio,after_lowio,36) || !GetFileInformationByHandle(independent,&after) || memcmp(&info,&after,sizeof(info))) ExitProcess(199);
    sprintf(kind,"idb-%s-path-after.bin",phase);dependency_save("mre-files",kind,&after,sizeof(after));
    reopened=CreateFileA(expected,0,7,NULL,OPEN_EXISTING,0,NULL);
    if (reopened==INVALID_HANDLE_VALUE || !GetFileInformationByHandle(reopened,&after) || memcmp(&info,&after,sizeof(info))) ExitProcess(199);
    sprintf(kind,"idb-%s-path-reopened.bin",phase);dependency_save("mre-files",kind,&after,sizeof(after));
    CloseHandle(reopened);CloseHandle(independent);
    sprintf(kind,"idb-%s-lowio-after.bin",phase);dependency_save("mre-files",kind,after_lowio,36);
    sprintf(kind,"idb-%s-context-before.bin",phase);dependency_save("mre-files",kind,&before_context,sizeof(before_context));
    sprintf(kind,"idb-%s-context-after.bin",phase);dependency_save("mre-files",kind,&after_context,sizeof(after_context));
    sprintf(post_phase,"%s-copy-after",phase);dependency_state(post_phase,teb,tid,&after_context,input,out,err);
    fprintf(dependency_events,"file-copy-closed phase=%s event=%lu tid=%lu all-threads-held=debug-event cursor=none\n",phase,event_number,tid);fflush(dependency_events);
}
