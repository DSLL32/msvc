/* Fixed debugger-only units at the real missing-include fatal entry. */
#ifndef CAPTURE_RESOURCE_UNIT_H
#define CAPTURE_RESOURCE_UNIT_H
static int resource_unit=-1,resource_terminal_seen;
static DWORD resource_nodes[5],resource_words[5][13],resource_terminal_site;
static unsigned char resource_terminal_byte;
static int resource_read(HANDLE process,DWORD address,void *value,DWORD bytes)
{
    DWORD got;
    return ReadProcessMemory(process,(void *)address,value,bytes,&got)&&got==bytes;
}
static int resource_string(HANDLE process,HANDLE log,const char *label,DWORD address)
{
    char text[260],line[340];DWORD written;unsigned i;
    for(i=0;i<sizeof(text);++i) {
        if(!resource_read(process,address+i,text+i,1))return 0;
        if(!text[i])break;
    }
    if(i==sizeof(text))return 0;
    sprintf(line,"resource-string %s %08lx %s\n",label,address,text);
    return WriteFile(log,line,strlen(line),&written,0)&&written==strlen(line);
}
static int resource_snapshot(HANDLE process,HANDLE log,DWORD base,const char *when)
{
    DWORD node,words[13],error,dos,written;short depth;unsigned i;char line[360],label[32];
    if(!resource_read(process,base+0x5a7c8,&node,4)||
       !resource_read(process,base+0x5a7d4,&depth,2)||depth!=4||
       !resource_read(process,base+0x639cc,&error,4)||
       !resource_read(process,base+0x639d0,&dos,4))return 0;
    sprintf(line,"resource-state %s depth %d errno %lu dos %lu\n",when,depth,error,dos);
    WriteFile(log,line,strlen(line),&written,0);
    if(!strcmp(when,"before")&&error!=2)return 0;
    for(i=0;i<5;++i) {
        if(!node||!resource_read(process,node,words,sizeof(words))||words[10])return 0;
        if(!strcmp(when,"before")) {
            unsigned j;
            if((int)words[1]<0||words[0]!=(i<3?0x1000:i==3?0x2000:0x3000))return 0;
            for(j=0;j<i;++j)if(resource_nodes[j]==node||resource_words[j][1]==words[1])return 0;
            resource_nodes[i]=node;memcpy(resource_words[i],words,sizeof(words));
        } else if(resource_nodes[i]!=node)return 0;
        sprintf(line,"resource-node %s %u %08lx",when,i,node);WriteFile(log,line,strlen(line),&written,0);
        {
            unsigned j;for(j=0;j<13;++j){sprintf(line," %08lx",words[j]);WriteFile(log,line,strlen(line),&written,0);}
        }
        WriteFile(log,"\n",1,&written,0);
        sprintf(label,"%s-%u",when,i);if(!resource_string(process,log,label,words[3]))return 0;
        node=words[12];
    }
    return !node;
}
static int resource_prepare(HANDLE process,HANDLE log,DWORD base,CONTEXT *state)
{
    DWORD frame[4],line_number;DWORD written;char line[160];
    if(!resource_read(process,state->Esp,frame,sizeof(frame))||frame[1]!=83||
       !resource_snapshot(process,log,base,"before")||
       !resource_string(process,log,"kind",frame[2])||!resource_string(process,log,"missing",frame[3])||
       !resource_read(process,base+0x4b238,&line_number,4))return 0;
    sprintf(line,"resource-context unit %d marker %08lx esp %08lx line %lu\n",resource_unit,direct_marker,state->Esp,line_number);
    WriteFile(log,line,strlen(line),&written,0);
    if(resource_unit==2) {
        direct_arguments[0]=frame[1];direct_arguments[1]=frame[2];direct_arguments[2]=frame[3];
        direct_arguments[3]=resource_words[0][3];direct_arguments[4]=line_number;
    }
    if(resource_unit>=2) {
        resource_terminal_site=base+(resource_unit==2?0x3aad0:0x43530);
        if(!resource_read(process,resource_terminal_site,&resource_terminal_byte,1)||
           resource_terminal_byte==0xcc||!patch_debug_byte(process,resource_terminal_site,0xcc))return 0;
    }
    return 1;
}
static int resource_terminal_event(HANDLE process,HANDLE thread,HANDLE log,DWORD code,DWORD site,DWORD base,char *buffers)
{
    CONTEXT state;DWORD frame[7],written;unsigned count,i;char line[160];
    if(resource_unit<2||!resource_terminal_site||code!=EXCEPTION_BREAKPOINT||site!=resource_terminal_site)return 0;
    if(resource_terminal_seen++)return -1;
    memset(&state,0,sizeof(state));state.ContextFlags=CONTEXT_FULL|CONTEXT_FLOATING_POINT;
    count=resource_unit==2?7:2;
    if(!GetThreadContext(thread,&state)||!resource_read(process,state.Esp,frame,4*count))return -1;
    sprintf(line,"resource-terminal unit %d site %08lx esp %08lx",resource_unit,site,state.Esp);
    WriteFile(log,line,strlen(line),&written,0);
    for(i=0;i<count;++i){sprintf(line," %08lx",frame[i]);WriteFile(log,line,strlen(line),&written,0);}
    WriteFile(log,"\n",1,&written,0);
    if(resource_unit==2) {
        if(frame[1]!=83||frame[2]!=direct_arguments[1]||frame[3]!=direct_arguments[2]||
           frame[5]!=direct_arguments[3]||frame[6]!=direct_arguments[4]||
           !resource_string(process,log,"reason",frame[4]))return -1;
    } else if(frame[1]!=0x8000)return -1;
    if(resource_unit==3) { WriteFile(log,"resource-streams exit\n",22,&written,0); if(buffers)dump_stream_buffers(process,log,base,buffers); }
    if(!patch_debug_byte(process,site,resource_terminal_byte))return -1;
    resource_terminal_site=0;state.Eip=site;
    return SetThreadContext(thread,&state)?1:-1;
}
#endif
