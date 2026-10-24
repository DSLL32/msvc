/* Next real continuation only: jobs(NULL), Release and Close. No native replay
 * or success value is implied by these original-only observations. */
static DWORD mre_jobs_stack,mre_lifecycle_stack[2],mre_lifecycle_args[2][2];
static int mre_jobs_pending,mre_jobs_returns,mre_lifecycle_pending[2];
static unsigned mre_lifecycle_calls;
static BYTE mre_lifecycle_returns[2];
static DWORD mre_lifecycle_sites[2]={0x36b9,0x36e2},mre_lifecycle_slots[2]={0xa30c,0xa310};
static const char *mre_lifecycle_names[2]={"_MREDrvFRelease@4","_MREFClose@8"};
static int mre_jobs_mask(BYTE signature,BYTE selector,BYTE comparator,BYTE jobs)
{
    return signature==0x55 && selector==0xff && comparator==0x8b && jobs==0x8b;
}
static void mre_jobs_install(void)
{
    unsigned i;BYTE call[6],jobs,deferred;
    memory(target_base+0x3b5d,&jobs,1);memory(target_base+0x3b8e,&deferred,1);
    if (!mre_jobs_observer || jobs!=0x8b || deferred!=0xcc) ExitProcess(200);
    for (i=0;i<2;++i) {
        memory(target_base+mre_lifecycle_sites[i],call,6);
        if (!mre_observation_call(call,target_base,mre_lifecycle_slots[i])) ExitProcess(200);
        memory(target_base+mre_lifecycle_sites[i]+6,mre_lifecycle_returns+i,1);
        if (mre_lifecycle_returns[i]!=0x85) ExitProcess(200);
        patch_byte(target_base+mre_lifecycle_sites[i],0xcc);
    }
}
static int mre_jobs_point(DWORD address)
{
    unsigned i;if (!mre_jobs_observer) return -1;
    if (mre_jobs_pending && address==target_base+0x3697) return 10;
    for (i=0;i<2;++i)
        if (address==target_base+mre_lifecycle_sites[i] ||
            (mre_lifecycle_pending[i] && address==target_base+mre_lifecycle_sites[i]+6)) return 8+i;
    return -1;
}
static void mre_jobs_enter(DWORD teb,DWORD tid,CONTEXT *context,HANDLE input,HANDLE out,HANDLE err)
{
    DWORD stack[2];BYTE after;
    if (!mre_jobs_observer || mre_jobs_pending || mre_jobs_returns || mre_sort_calls!=1 || mre_sort_pending ||
        mre_compare_calls!=1 || context->Ebp!=mre_analysis_ebp || context->Eip!=target_base+0x3b5e) ExitProcess(201);
    memory(context->Esp,stack,8);memory(target_base+0x3697,&after,1);
    if (stack[0]!=target_base+0x3697 || stack[1] || after!=0x59) ExitProcess(201);
    mre_jobs_stack=context->Esp;
    dependency_state("jobs-before",teb,tid,context,input,out,err);mre_observation_target("jobs-before",1);
    dependency_save("jobs-before","arguments.bin",stack+1,4);
    patch_byte(target_base+0x3697,0xcc);mre_jobs_pending=1;
    fprintf(dependency_events,"jobs phase=before entry=%08lx return=%08lx event=%lu\n",target_base+0x3b5d,stack[0],event_number);fflush(dependency_events);
}
static void mre_jobs_trap(int point,DWORD address,DWORD teb,DWORD tid,CONTEXT *context,HANDLE thread,HANDLE input,HANDLE out,HANDLE err)
{
    unsigned i,argc;int after;DWORD actual,driver,engine,commit,identity[4],slot;BYTE lowio[36];char phase[40];
    if (!mre_jobs_observer || context->Ebp!=mre_analysis_ebp) ExitProcess(202);
    if (point==10) {
        if (!mre_jobs_pending || mre_jobs_returns || address!=target_base+0x3697 || context->Esp!=mre_jobs_stack+4 || context->Eax) ExitProcess(202);
        dependency_state("jobs-after",teb,tid,context,input,out,err);mre_observation_target("jobs-after",1);
        fprintf(dependency_events,"jobs phase=after entry=%08lx return=%08lx eax=%08lx event=%lu\n",target_base+0x3b5d,address,context->Eax,event_number);fflush(dependency_events);
        patch_byte(address,0x59);mre_jobs_pending=0;++mre_jobs_returns;context->Eip=address;return;
    }
    i=point-8;if (i>1 || mre_jobs_pending || mre_jobs_returns!=1 || mre_sort_calls!=1) ExitProcess(202);
    after=mre_lifecycle_pending[i];argc=i+1;
    if (!after) {
        if (mre_lifecycle_calls!=i || address!=target_base+mre_lifecycle_sites[i]) ExitProcess(202);
        memory(context->Esp,mre_lifecycle_args[i],argc*4);mre_lifecycle_stack[i]=context->Esp;
        memory(mre_analysis_arguments[1][1],&driver,4);memory(mre_analysis_arguments[0][0],&engine,4);
        memory(target_base+0x9018,&commit,4);
        if ((!i && mre_lifecycle_args[i][0]!=driver) ||
            (i && (mre_lifecycle_args[i][0]!=engine || mre_lifecycle_args[i][1]!=commit))) ExitProcess(202);
        ++mre_calls;
    } else if (mre_lifecycle_calls!=i || address!=target_base+mre_lifecycle_sites[i]+6 || context->Esp!=mre_lifecycle_stack[i]+argc*4) ExitProcess(202);
    memory(target_base+mre_lifecycle_slots[i],&actual,4);
    if (actual!=exported(mre_provider_base,mre_lifecycle_names[i])) ExitProcess(202);
    sprintf(phase,"mre-%u-%s",i+5,after?"after":"before");
    dependency_state(phase,teb,tid,context,input,out,err);mre_observation_target(phase,1);
    dependency_save(phase,"arguments.bin",mre_lifecycle_args[i],argc*4);
    identity[0]=mre_provider_base;identity[1]=actual;identity[2]=mre_analysis_ebp;identity[3]=mre_lifecycle_stack[i];
    dependency_save(phase,"identity.bin",identity,16);
    fprintf(dependency_events,"mre sequence=%u phase=%s call=%08lx return=%08lx function=%08lx esp=%08lx eax=%08lx event=%lu\n",i+5,after?"after":"before",target_base+mre_lifecycle_sites[i],target_base+mre_lifecycle_sites[i]+6,actual,context->Esp,context->Eax,event_number);fflush(dependency_events);
    if (after) {
        memory(exported(crt_base,"__pioinfo"),&slot,4);memory(slot+108,lowio,36);
        if (lowio[4]&1) mre_file_copy(phase,teb,tid,thread,input,out,err,mre_analysis_arguments[0][1]);
        else mre_closed_file_copy(phase,teb,tid,thread,input,out,err,mre_analysis_arguments[0][1]);
        patch_byte(address,mre_lifecycle_returns[i]);patch_byte(target_base+mre_lifecycle_sites[i],0xcc);
        mre_lifecycle_pending[i]=0;++mre_lifecycle_calls;
    } else {patch_byte(address,0xff);patch_byte(target_base+mre_lifecycle_sites[i]+6,0xcc);mre_lifecycle_pending[i]=1;}
    context->Eip=address;
}
