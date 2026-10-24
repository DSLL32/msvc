/* Original-only next slice: first comparator return and one fourth MRE call.
 * All later MRE exports remain guarded. No successful native response implied. */
static DWORD mre_compare_stack,mre_compare_arguments[2],mre_sort_stack,mre_sort_arguments[2];
static BYTE mre_compare_return,mre_sort_return;

static int mre_sort_mask(BYTE signature,BYTE selector,BYTE comparator,BYTE jobs)
{
    return signature==0x55 && selector==0xff && comparator==0x8b && jobs==0xcc;
}
static void mre_sort_install(void)
{
    BYTE call[6];
    memory(target_base+0x3686,call,6);
    if (!mre_sort_observer || !mre_observation_call(call,target_base,0xa308)) ExitProcess(190);
    memory(target_base+0x368c,&mre_sort_return,1);
    if (mre_sort_return!=0x85) ExitProcess(190);
    patch_byte(target_base+0x3686,0xcc);
}
static void mre_compare_enter(DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD stack[3],array,count,values[2],identity[8];
    if (!mre_sort_observer || mre_compare_calls || mre_compare_pending ||
        mre_analysis_calls!=3 || mre_lower_calls!=1 || mre_lower_pending) ExitProcess(191);
    memory(context->Esp,stack,12);
    if (context->Eip!=target_base+0x3b4a || stack[0]!=crt_base+0x2551e || !stack[1] || !stack[2]) ExitProcess(191);
    memory(mre_lower_ebp-16,&array,4);memory(mre_lower_ebp-4,&count,4);
    if (!array || count<2 || count>64 || stack[1]<array || stack[2]<array ||
        stack[1]>=array+count*4 || stack[2]>=array+count*4 || (stack[1]-array)%4 || (stack[2]-array)%4) ExitProcess(191);
    memory(stack[1],values,4);memory(stack[2],values+1,4);
    dependency_state("compare-before",teb,tid,context,input,out,err);
    mre_observation_target("compare-before",1);
    mre_observation_string("compare-before","left.bin",values[0],2048);
    mre_observation_string("compare-before","right.bin",values[1],2048);
    identity[0]=context->Esp;identity[1]=stack[1];identity[2]=stack[2];identity[3]=values[0];identity[4]=values[1];
    identity[5]=mre_lower_ebp;identity[6]=crt_base;identity[7]=stack[0];
    dependency_save("compare-before","identity.bin",identity,sizeof(identity));
    mre_compare_stack=context->Esp;mre_compare_arguments[0]=stack[1];mre_compare_arguments[1]=stack[2];
    memory(crt_base+0x2551e,&mre_compare_return,1);
    if (mre_compare_return!=0x83) ExitProcess(191);
    patch_byte(crt_base+0x2551e,0xcc);mre_compare_pending=1;
    fprintf(dependency_events,"compare phase=before entry=%08lx return=%08lx event=%lu\n",target_base+0x3b49,crt_base+0x2551e,event_number);fflush(dependency_events);
}
static void mre_sort_trap(int point,DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE thread,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD actual,driver,work,values[2],identity[4];char phase[40];int after;
    if (!mre_sort_observer) ExitProcess(192);
    if (point==7) {
        if (!mre_compare_pending || mre_compare_calls || address!=crt_base+0x2551e || context->Esp!=mre_compare_stack+4) ExitProcess(192);
        dependency_state("compare-after",teb,tid,context,input,out,err);mre_observation_target("compare-after",1);
        memory(mre_compare_arguments[0],values,4);memory(mre_compare_arguments[1],values+1,4);
        mre_observation_string("compare-after","left.bin",values[0],2048);
        mre_observation_string("compare-after","right.bin",values[1],2048);
        fprintf(dependency_events,"compare phase=after entry=%08lx return=%08lx eax=%08lx event=%lu\n",target_base+0x3b49,address,context->Eax,event_number);fflush(dependency_events);
        patch_byte(address,mre_compare_return);mre_compare_pending=0;++mre_compare_calls;context->Eip=address;return;
    }
    after=mre_sort_pending;
    if (point!=6 || mre_analysis_calls!=3 || mre_compare_calls!=1 || mre_compare_pending || context->Ebp!=mre_analysis_ebp) ExitProcess(192);
    if (!after) {
        if (mre_sort_calls || address!=target_base+0x3686) ExitProcess(192);
        memory(context->Esp,mre_sort_arguments,8);mre_sort_stack=context->Esp;
        memory(mre_analysis_arguments[1][1],&driver,4);memory(mre_analysis_ebp+16,&work,4);
        if (mre_sort_arguments[0]!=driver || mre_sort_arguments[1]!=work || !driver || !work) ExitProcess(192);
        ++mre_calls;
    } else if (address!=target_base+0x368c || context->Esp!=mre_sort_stack+8) ExitProcess(192);
    memory(target_base+0xa308,&actual,4);
    if (actual!=exported(mre_provider_base,"_MREDrvFFilesOutOfDate@8")) ExitProcess(192);
    strcpy(phase,after?"mre-4-after":"mre-4-before");
    dependency_state(phase,teb,tid,context,input,out,err);mre_observation_target(phase,1);
    dependency_save(phase,"arguments.bin",mre_sort_arguments,8);
    memory(mre_sort_arguments[1],values,8); /* Complete20B record already saved by target observer. */
    identity[0]=mre_provider_base;identity[1]=actual;identity[2]=mre_analysis_ebp;identity[3]=mre_sort_stack;
    dependency_save(phase,"identity.bin",identity,sizeof(identity));
    fprintf(dependency_events,"mre sequence=4 phase=%s call=%08lx return=%08lx function=%08lx esp=%08lx eax=%08lx event=%lu\n",after?"after":"before",target_base+0x3686,target_base+0x368c,actual,context->Esp,context->Eax,event_number);fflush(dependency_events);
    if (after) {
        mre_file_copy(phase,teb,tid,thread,input,out,err,mre_analysis_arguments[0][1]);
        patch_byte(address,mre_sort_return);patch_byte(target_base+0x3686,0xcc);mre_sort_pending=0;++mre_sort_calls;
    } else {patch_byte(address,0xff);patch_byte(target_base+0x368c,0xcc);mre_sort_pending=1;}
    context->Eip=address;
}
static void mre_sort_frontier(DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD stack[2],work,outdated;
    if (!mre_sort_observer || mre_analysis_calls!=3 || mre_sort_calls!=1 || mre_sort_pending ||
        mre_compare_calls!=1 || mre_compare_pending || address!=target_base+0x3b5d || context->Ebp!=mre_analysis_ebp) ExitProcess(193);
    memory(context->Esp,stack,8);memory(mre_analysis_ebp+16,&work,4);memory(work,&outdated,4);
    if (stack[0]!=target_base+0x3697 || stack[1]!=outdated) ExitProcess(193);
    dependency_state("mre-frontier",teb,tid,context,input,out,err);mre_observation_target("mre-frontier",1);
    dependency_save("mre-frontier","formal-arguments.bin",stack+1,4);
    fprintf(dependency_events,"response-frontier kind=deferred-jobs address=%08lx calls=4 event=%lu\n",address,event_number);fflush(dependency_events);
}

/* Read only the first unsupported Release input before the existing7002 stop.
 * This is partial analysis, never execution/response of that fifth operation. */
static void mre_sort_unknown(DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD argument,driver,actual;
    if (!mre_sort_observer || address!=target_base+0x36b9 || mre_sort_calls!=1 || mre_sort_pending || mre_compare_calls!=1 || context->Ebp!=mre_analysis_ebp) ExitProcess(194);
    memory(context->Esp,&argument,4);memory(mre_analysis_arguments[1][1],&driver,4);
    memory(target_base+0xa30c,&actual,4);
    if (argument!=driver || actual!=exported(mre_provider_base,"_MREDrvFRelease@4")) ExitProcess(194);
    dependency_state("mre-unknown-entry",teb,tid,context,input,out,err);mre_observation_target("mre-unknown-entry",1);
    dependency_save("mre-unknown-entry","arguments.bin",&argument,4);
    dependency_save("mre-unknown-entry","actual-import.bin",&actual,4);
    fprintf(dependency_events,"partial-stop kind=unsupported-release call=%08lx argument=%08lx function=%08lx event=%lu\n",address,argument,actual,event_number);fflush(dependency_events);
}
