/* Original-only bounded Open/Query/Init observation, ending before signature logic. */
static int mre_analysis_observer,mre_analysis_stopped,mre_response_observer,mre_signature_observer;
static DWORD mre_provider_base,mre_analysis_calls,mre_analysis_ebp;
static int mre_analysis_pending[3];
static BYTE mre_analysis_returns[3];
static DWORD mre_analysis_stack[3],mre_analysis_arguments[3][6];
static DWORD mre_analysis_sites[3]={0x3597,0x35c1,0x35ca};
static DWORD mre_analysis_slots[3]={0xa300,0xa314,0xa304};
static DWORD mre_analysis_bytes[3]={24,8,4};
static const char *mre_analysis_exports[3]={"_MREFOpenByName@24","_MREQueryMreDrv@8","_MREDrvOneTimeInit@4"};

static int mre_observation_mode(int masked,int other_family)
{
    return masked==0 && other_family==0;
}
static int mre_observation_call(const BYTE *call,DWORD base,DWORD slot)
{
    DWORD actual;
    memcpy(&actual,call+2,4);
    return call[0]==0xff && call[1]==0x15 && actual==base+slot;
}
static int mre_observation_response_mode(int masked,int other_family)
{
    return masked==1 && other_family==0;
}
static int mre_observation_response_marker(const char *line)
{
    return !strcmp(line,"CL_MRE_PREFIX_RESPONSE_V2\n") ||
        !strcmp(line,"CL_MRE_SIGNATURE_RESPONSE_V1\n");
}
static int mre_observation_frontier_mode(int signature,BYTE signature_entry,BYTE selector_entry)
{
    return (signature==0 && signature_entry==0xcc) ||
        (signature==1 && signature_entry==0x55 && selector_entry==0xcc);
}
static void mre_observation_install(int masked)
{
    char path[MAX_PATH],line[80];FILE *file;unsigned i;BYTE call[6],entry,selector;
    output_path(path,"mre-observer.txt");file=fopen(path,"rb");if (!file) return;
    if (!fgets(line,sizeof(line),file) || fgetc(file)!=EOF) ExitProcess(160);
    mre_response_observer=mre_observation_response_marker(line);
    mre_signature_observer=!strcmp(line,"CL_MRE_SIGNATURE_RESPONSE_V1\n");
    if (mre_response_observer) {
        if (!mre_observation_response_mode(masked,dependency_analysis) || !mre_file_directory(directory)) ExitProcess(160);
    } else if (strcmp(line,"CL_MRE_PREFIX_ANALYSIS_V2\n") ||
        !mre_observation_mode(masked,dependency_analysis)) ExitProcess(160);
    fclose(file);mre_analysis_observer=1;
    /* The ordinary named-entry guard also blocks every later dependency path. */
    for (i=0;i<entry_count;++i)
        if (entry_guards[i].rva==0x550d && !strcmp(entry_guards[i].name,"execute_pass")) break;
    if (i==entry_count) ExitProcess(175);
    output_path(path,"observer");CreateDirectoryA(path,NULL);
    output_path(path,"observer\\mre-events.txt");dependency_events=fopen(path,"wb");
    if (!dependency_events) ExitProcess(161);
    for (i=0;i<3;++i) {
        memory(target_base+mre_analysis_sites[i],call,sizeof(call));
        if (!mre_observation_call(call,target_base,mre_analysis_slots[i])) ExitProcess(162);
        memory(target_base+mre_analysis_sites[i]+6,mre_analysis_returns+i,1);
        patch_byte(target_base+mre_analysis_sites[i],0xcc);
    }
    if (!mre_response_observer) {
        memory(target_base+0x393a,&entry,1);if (entry!=0x55) ExitProcess(163);
        patch_byte(target_base+0x393a,0xcc);
    } else {
        memory(target_base+0x393a,&entry,1);memory(target_base+0x10f7,&selector,1);
        if (!mre_observation_frontier_mode(mre_signature_observer,entry,selector)) ExitProcess(163);
    }
}
static int mre_observation_point(DWORD address)
{
    unsigned i;if (!mre_analysis_observer) return -1;
    if (!mre_response_observer && address==target_base+0x393a) return 3;
    for (i=0;i<3;++i)
        if (address==target_base+mre_analysis_sites[i] ||
            (mre_analysis_pending[i] && address==target_base+mre_analysis_sites[i]+6)) return i;
    return -1;
}
static void mre_observation_string(const char *phase,const char *kind,DWORD address,DWORD capacity)
{
    char text[2048];DWORD i;
    if (!address || !capacity || capacity>sizeof(text)) ExitProcess(164);
    for (i=0;i<capacity;++i) {
        memory(address+i,text+i,1);
        if (!text[i]) { dependency_save(phase,kind,text,i+1);return; }
    }
    ExitProcess(164); /* Unknown/truncated strings cannot become input evidence. */
}
static void mre_observation_target(const char *phase,int initialized)
{
    DWORD arguments[3],state,job,next,words[11],option,input,previous[64],count=0,i;
    BYTE bytes[1024];char kind[80];
    memory(mre_analysis_ebp+8,arguments,sizeof(arguments));
    dependency_save(phase,"prepare-jobs-arguments.bin",arguments,sizeof(arguments));
    memory(target_base+0x9020,&state,4);
    if (!state || state!=arguments[0]) ExitProcess(165);
    memory(state,words,44);dependency_save(phase,"driver.bin",words,44);
    memory(target_base+0x91b8,bytes,sizeof(bytes));dependency_save(phase,"shared-scratch.bin",bytes,sizeof(bytes));
    if (mre_signature_observer) {
        DWORD include_flag;
        memory(target_base+0x9010,&include_flag,4);
        dependency_save(phase,"use-standard-include.bin",&include_flag,4);
        if (words[9]) mre_observation_string(phase,"include-path.bin",words[9],1024);
    }
    if (initialized) {
        memory(arguments[2],bytes,20);dependency_save(phase,"pending.bin",bytes,20);
    }
    job=arguments[1];
    while (job) {
        if (count>=64) ExitProcess(166);
        for (i=0;i<count;++i) if (previous[i]==job) ExitProcess(166);
        previous[count]=job;memory(job,bytes,12);memcpy(&next,bytes,4);memcpy(&input,bytes+4,4);
        sprintf(kind,"job-%lu-header.bin",count);dependency_save(phase,kind,bytes,12);
        memory(input,bytes,16);sprintf(kind,"job-%lu-input.bin",count);dependency_save(phase,kind,bytes,16);
        memcpy(&input,bytes+4,4);sprintf(kind,"job-%lu-source.bin",count);
        mre_observation_string(phase,kind,input,1024);++count;job=next;
    }
    dependency_save(phase,"job-addresses.bin",previous,count*4);
    option=words[0];count=0;
    while (option) {
        DWORD record[6];
        if (count>=64) ExitProcess(167);
        for (i=0;i<count;++i) if (previous[i]==option) ExitProcess(167);
        previous[count]=option;memory(option,record,24);
        sprintf(kind,"option-%lu.bin",count);dependency_save(phase,kind,record,24);
        for (i=2;i<=4;++i) if (record[i]) {
            sprintf(kind,"option-%lu-field-%lu.bin",count,i);
            mre_observation_string(phase,kind,record[i],1024);
        }
        option=record[0];++count;
    }
    dependency_save(phase,"option-addresses.bin",previous,count*4);
}
static void mre_observation_trap(int point,DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE thread,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD actual,stack[16],words[4],argument;char phase[64];BYTE message[1024];int after;
    if (point==3) {
        if (mre_analysis_calls!=3 || mre_analysis_pending[0] || mre_analysis_pending[1] || mre_analysis_pending[2]) ExitProcess(168);
        strcpy(phase,"mre-signature-entry");
        dependency_state(phase,teb,tid,context,input,out,err);
        mre_observation_target(phase,1);
        memory(context->Esp,stack,sizeof(stack));
        if (stack[0]!=target_base+0x35f2) ExitProcess(168);
        memory(target_base+0x9020,&argument,4);if (stack[1]!=argument) ExitProcess(168);
        fprintf(dependency_events,"analysis-stop kind=option-signature address=%08lx calls=%lu event=%lu\n",address,mre_analysis_calls,event_number);
        fflush(dependency_events);mre_analysis_stopped=1;TerminateProcess(child,0x7004);return;
    }
    after=mre_analysis_pending[point];
    if (!after) {
        if (mre_analysis_calls!=(DWORD)point) ExitProcess(169);
        if (!mre_analysis_ebp)mre_analysis_ebp=context->Ebp;
        if (context->Ebp!=mre_analysis_ebp) ExitProcess(169);
        ++mre_analysis_calls;++mre_calls;mre_analysis_stack[point]=context->Esp;
        memory(context->Esp,mre_analysis_arguments[point],mre_analysis_bytes[point]);
    } else if (context->Esp!=mre_analysis_stack[point]+mre_analysis_bytes[point] || context->Ebp!=mre_analysis_ebp) ExitProcess(170);
    memory(target_base+mre_analysis_slots[point],&actual,4);
    if (!mre_provider_base || actual!=exported(mre_provider_base,mre_analysis_exports[point])) ExitProcess(171);
    sprintf(phase,"mre-%d-%s",point+1,after?"after":"before");
    dependency_state(phase,teb,tid,context,input,out,err);
    mre_observation_target(phase,0);
    words[0]=mre_provider_base;words[1]=actual;words[2]=mre_analysis_ebp;words[3]=mre_analysis_stack[point];
    dependency_save(phase,"identity.bin",words,sizeof(words));
    dependency_save(phase,"arguments.bin",mre_analysis_arguments[point],mre_analysis_bytes[point]);
    if (point==0) {
        memory(mre_analysis_arguments[0][0],words,4);dependency_save(phase,"engine-out.bin",words,4);
        memory(mre_analysis_arguments[0][2],words,4);dependency_save(phase,"error-out.bin",words,4);
        memory(mre_analysis_arguments[0][3],message,sizeof(message));dependency_save(phase,"message.bin",message,sizeof(message));
        mre_observation_string(phase,"pdb-path.bin",mre_analysis_arguments[0][1],1024);
        if (after && !context->Eax) {
            fprintf(dependency_events,"analysis-stop kind=open-failed calls=%lu event=%lu\n",mre_analysis_calls,event_number);
            fflush(dependency_events);mre_analysis_stopped=1;TerminateProcess(child,0x7004);return;
        }
    } else if (point==1) {
        memory(mre_analysis_arguments[1][1],words,4);dependency_save(phase,"driver-out.bin",words,4);
    }
    fprintf(dependency_events,"mre sequence=%d phase=%s call=%08lx return=%08lx function=%08lx esp=%08lx eax=%08lx event=%lu\n",
        point+1,after?"after":"before",target_base+mre_analysis_sites[point],target_base+mre_analysis_sites[point]+6,actual,context->Esp,context->Eax,event_number);
    fflush(dependency_events);
    if (after && mre_response_observer) mre_file_copy(phase,teb,tid,thread,input,out,err,mre_analysis_arguments[0][1]);
    if (!after) {
        patch_byte(address,0xff);patch_byte(address+6,0xcc);mre_analysis_pending[point]=1;
    } else {
        patch_byte(address,mre_analysis_returns[point]);patch_byte(target_base+mre_analysis_sites[point],0xcc);mre_analysis_pending[point]=0;
    }
    context->Eip=address;
}

static void mre_observation_frontier(DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD arguments[3],state,first_option,frame[3];char selectors[3];
    if (!mre_response_observer) return;
    if (mre_analysis_calls!=3 || mre_analysis_pending[0] || mre_analysis_pending[1] || mre_analysis_pending[2]) ExitProcess(188);
    memory(context->Esp,arguments,sizeof(arguments));
    memory(target_base+0x9020,&state,4);
    if (mre_signature_observer) {
        if (address!=target_base+0x10f7 || arguments[0]!=target_base+0x3989) ExitProcess(188);
        memory(state,&first_option,4);
        if (!first_option || arguments[1]!=first_option || !arguments[2]) ExitProcess(188);
        memory(arguments[2],selectors,sizeof(selectors));
        if (memcmp(selectors,"Mm",3)) ExitProcess(188);
        memory(context->Ebp,frame,sizeof(frame));
        if (frame[0]!=mre_analysis_ebp || frame[1]!=target_base+0x35f2 || frame[2]!=state) ExitProcess(188);
        dependency_save("mre-frontier","signature-frame.bin",frame,sizeof(frame));
        dependency_save("mre-frontier","selectors.bin",selectors,sizeof(selectors));
    } else if (address!=target_base+0x393a || arguments[0]!=target_base+0x35f2 || arguments[1]!=state) ExitProcess(188);
    dependency_state("mre-frontier",teb,tid,context,input,out,err);
    mre_observation_target("mre-frontier",1);
    dependency_save("mre-frontier","formal-arguments.bin",arguments+1,mre_signature_observer?8:4);
    fprintf(dependency_events,"response-frontier kind=%s address=%08lx calls=%lu event=%lu\n",
        mre_signature_observer?"option-selector":"option-signature",address,mre_analysis_calls,event_number);fflush(dependency_events);
}
