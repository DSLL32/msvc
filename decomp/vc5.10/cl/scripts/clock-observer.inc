/* Original-only observation of the two reached driver _ftime calls. */
static DWORD clock_sites[2] = { 0x5402, 0x5431 };
static DWORD clock_buffers[2] = { 0x9798, 0x9688 };
static int clock_installed[2],clock_pending[2];
static BYTE clock_return_bytes[2];
static DWORD clock_stack[2],clock_calls;
static FILE *clock_events;

static int clock_call_bytes(const BYTE *call,DWORD base)
{
    DWORD slot;
    memcpy(&slot,call+2,4);
    return call[0]==0xff && call[1]==0x15 && slot==base+0xa2a4;
}
static int clock_observation_mode(int masked,int link_family,int analysis)
{
    return masked==0 && link_family==1 && analysis==1;
}
static void clock_install(int masked)
{
    char path[MAX_PATH],line[80];FILE *file;unsigned i;BYTE call[6];
    output_path(path,"clock-observer.txt");file=fopen(path,"rb");if (!file) return;
    if (!fgets(line,sizeof(line),file) || strcmp(line,"CL_CLOCK_OBSERVER_V1\n") ||
        fgetc(file)!=EOF || !clock_observation_mode(masked,dependency_link_response,dependency_clock_analysis)) ExitProcess(140);
    fclose(file);output_path(path,"observer\\clock-events.txt");clock_events=fopen(path,"wb");
    if (!clock_events) ExitProcess(141);
    for (i=0;i<2;++i) {
        memory(target_base+clock_sites[i],call,sizeof(call));
        if (call[0]==0xcc) continue;
        if (!clock_call_bytes(call,target_base)) ExitProcess(142);
        memory(target_base+clock_sites[i]+6,clock_return_bytes+i,1);
        clock_installed[i]=1;patch_byte(target_base+clock_sites[i],0xcc);
    }
}
static int clock_point(DWORD address)
{
    unsigned i;
    if (!clock_events) return -1;
    for (i=0;i<2;++i)
        if (clock_installed[i] && (address==target_base+clock_sites[i] ||
            (clock_pending[i] && address==target_base+clock_sites[i]+6))) return i;
    return -1;
}
static void clock_trap(int point,DWORD address,DWORD teb,DWORD tid,CONTEXT *context)
{
    DWORD argument,actual,stack[16],words[9],pointer,i;
    char phase[64],kind[64],text[1024];BYTE buffer[12];
    if (!clock_pending[point]) {
        if (clock_calls!=(DWORD)point) ExitProcess(143);
        ++clock_calls;clock_stack[point]=context->Esp;
    } else if (context->Esp!=clock_stack[point]) ExitProcess(144);
    memory(context->Esp,&argument,4);memory(target_base+0xa2a4,&actual,4);
    if (argument!=target_base+clock_buffers[point] || actual!=exported(crt_base,"_ftime")) ExitProcess(145);
    sprintf(phase,"clock-%d-%s",point+1,clock_pending[point]?"after":"before");
    dependency_save(phase,"context.bin",context,sizeof(*context));
    memory(context->Esp,stack,sizeof(stack));dependency_save(phase,"stack.bin",stack,sizeof(stack));
    error_state_to(teb,tid,phase,clock_events);fflush(clock_events);
    memory(argument,buffer,sizeof(buffer));dependency_save(phase,"timeb.bin",buffer,sizeof(buffer));
    memory(target_base+clock_buffers[0],buffer,sizeof(buffer));dependency_save(phase,"started.bin",buffer,sizeof(buffer));
    memory(target_base+clock_buffers[1],buffer,sizeof(buffer));dependency_save(phase,"stopped.bin",buffer,sizeof(buffer));
    words[0]=exported(crt_base,"_timezone");memory(words[0],words+1,4);
    words[2]=exported(crt_base,"_daylight");memory(words[2],words+3,4);
    words[4]=exported(crt_base,"_tzname");memory(words[4],words+5,8);
    words[7]=exported(crt_base,"_environ");memory(words[7],words+8,4);
    dependency_save(phase,"tz-words.bin",words,sizeof(words));
    for (i=0;i<2;++i) {
        pointer=words[5+i];if (!pointer) ExitProcess(146);
        remote_string(pointer,text,sizeof(text));sprintf(kind,"tz-name-%lu.bin",i);
        dependency_save(phase,kind,text,strlen(text)+1);
    }
    sprintf(kind,"observer\\%s-environment.bin",phase);raw_vector(kind,words[8],-1);
    fprintf(clock_events,"ftime sequence=%d phase=%s call=%08lx return=%08lx function=%08lx argument=%08lx esp=%08lx event=%lu\n",
        point+1,clock_pending[point]?"after":"before",target_base+clock_sites[point],target_base+clock_sites[point]+6,actual,argument,context->Esp,event_number);
    fflush(clock_events);
    if (!clock_pending[point]) {
        clock_pending[point]=1;patch_byte(address,0xff);patch_byte(address+6,0xcc);
    } else {
        clock_pending[point]=0;patch_byte(address,clock_return_bytes[point]);
        patch_byte(target_base+clock_sites[point],0xcc);
    }
    context->Eip=address;
}
