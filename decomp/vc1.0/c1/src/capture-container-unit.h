/* Explicit direct container units over actual parsed -FI ownership. */
#include <recon.h>
static int container_unit=-1;
static DWORD container_vector;
static DWORD container_type;
static int container_stage;
static DWORD container_owned[2],container_initial_flags;
static int container_type_state(HANDLE process,HANDLE log,DWORD base,DWORD type,const char *boundary)
{
    DWORD fields[2],shape[2],got,written,depth,index;
    char line[256];
    if(!type||!ReadProcessMemory(process,(void *)type,fields,8,&got)||got!=8||
       !ReadProcessMemory(process,(void *)(base+0x4ff18),&depth,4,&got)||got!=4)return 0;
    sprintf(line,"container-type %s address %08lx flags %04x attributes %04x related %08lx depth %lu\n",boundary,type,(unsigned short)fields[0],(unsigned short)(fields[0]>>16),fields[1],depth);
    WriteFile(log,line,strlen(line),&written,0);
    for(index=0;fields[1];++index) {
        if(index>=3||!ReadProcessMemory(process,(void *)fields[1],shape,8,&got)||got!=8)return 0;
        sprintf(line,"container-shape %s %lu address %08lx flags %04x attributes %04x related %08lx\n",boundary,index,fields[1],(unsigned short)shape[0],(unsigned short)(shape[0]>>16),shape[1]);
        WriteFile(log,line,strlen(line),&written,0);fields[1]=shape[1];
    }
    return 1;
}
static int container_vector_state(HANDLE process,HANDLE log,DWORD vector,const char *boundary)
{
    DWORD fields[3],items[3],got,written;
    char line[512],name[128];
    unsigned index,n;
    short count,capacity;
    if(!vector||!ReadProcessMemory(process,(void *)vector,fields,12,&got)||got!=12)return 0;
    count=(short)fields[0];capacity=(short)(fields[0]>>16);
    if(count!=3||capacity<count||capacity>256||!fields[2]||
       !ReadProcessMemory(process,(void *)fields[2],items,12,&got)||got!=12)return 0;
    sprintf(line,"container-vector %s address %08lx count %d capacity %d arena %ld items %08lx\n",boundary,vector,count,capacity,fields[1],fields[2]);
    WriteFile(log,line,strlen(line),&written,0);
    for(index=0;index<3;++index) {
        for(n=0;n<sizeof(name);++n) {
            if(!ReadProcessMemory(process,(void *)(items[index]+n),name+n,1,&got)||got!=1)return 0;
            if(!name[n])break;
        }
        if(n==sizeof(name)||strcmp(name,index==0?"forced-a.h":index==1&&!(container_unit==8&&!strcmp(boundary,"after"))?"forced-b.h":"forced-c.h"))return 0;
        sprintf(line,"container-item %s %u address %08lx name %s\n",boundary,index,items[index],name);
        WriteFile(log,line,strlen(line),&written,0);
    }
    return 1;
}
static int container_prepare(HANDLE process,HANDLE log,DWORD base)
{
    DWORD got,items,symbol;
    if(container_unit<0||container_unit>16||!ReadProcessMemory(process,(void *)(base+0x50494),&container_vector,4,&got)||got!=4||
       !container_vector_state(process,log,container_vector,"before"))return 0;
    direct_arguments[0]=container_vector;
    if(container_unit==2) {
        if(!ReadProcessMemory(process,(void *)(container_vector+8),&items,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(items+8),direct_arguments+1,4,&got)||got!=4)return 0;
    }
    if(container_unit==4)direct_arguments[1]=container_vector;
    if(container_unit==6) {
        if(!ReadProcessMemory(process,(void *)(container_vector+8),&items,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)items,direct_arguments,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(items+8),direct_arguments+1,4,&got)||got!=4)return 0;
    }
    if(container_unit==7) {
        symbol=parsed_global_symbol(process,base,"container_result");
        if(!symbol||!ReadProcessMemory(process,(void *)(symbol+12),&container_type,4,&got)||got!=4||
           !container_type_state(process,log,base,container_type,"before"))return 0;
        direct_arguments[0]=2;direct_arguments[1]=container_type;
    }
    if(container_unit>=8&&container_unit<=13)container_stage=1;
    if(container_unit==14)direct_arguments[0]=1;
    return 1;
}
/* The v2 seam has only these fixed accepted constructor stages. It never
   steps an instruction, changes parser globals, or resumes the stopped parent. */
static int container_owned_state(HANDLE process,HANDLE log,const char *boundary)
{
    DWORD got,written,fields[2],index,count;
    char line[256];unsigned char bytes[5];short position;
    if(container_unit==8||container_unit==9) {
        if(!ReadProcessMemory(process,(void *)container_owned[0],fields,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(container_owned[0]+4),&position,2,&got)||got!=2)return 0;
        sprintf(line,"container-iterator %s address %08lx vector %08lx index %d\n",boundary,container_owned[0],fields[0],position);
        WriteFile(log,line,strlen(line),&written,0);
    } else if(container_unit>=10&&container_unit<=12) {
        if(container_unit==12&&!strcmp(boundary,"after")) {
            sprintf(line,"container-released address %08lx\n",container_owned[0]);WriteFile(log,line,strlen(line),&written,0);return 1;
        }
        for(index=0;index<(container_unit==11?2U:1U);++index) {
            if(!ReadProcessMemory(process,(void *)container_owned[index],&count,4,&got)||got!=4||count!=31||
               !ReadProcessMemory(process,(void *)(container_owned[index]+4),bytes,5,&got)||got!=5)return 0;
            sprintf(line,"container-bitset %s %lu address %08lx count %lu bytes %02x%02x%02x%02x%02x\n",boundary,index,container_owned[index],count,bytes[0],bytes[1],bytes[2],bytes[3],bytes[4]);
            WriteFile(log,line,strlen(line),&written,0);
        }
    } else if(container_unit==13) {
        if(!ReadProcessMemory(process,(void *)container_owned[0],fields,4,&got)||got!=4)return 0;
        sprintf(line,"container-flags %s address %08lx value %08lx initial %08lx\n",boundary,container_owned[0],fields[0],container_initial_flags);WriteFile(log,line,strlen(line),&written,0);
    }
    return 1;
}
static int container_call(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *current,int constructor)
{
    DWORD words[8],written,target,argument;unsigned index,count;char line[256];
    for(index=0;index<8;++index)words[index]=0xa5a5a5a5;
    words[0]=direct_marker;
    if(constructor) {
        argument=container_unit==13?4:container_unit<10?8:31;
        target=base+(container_unit==13||container_unit<10?0x34ba0:0x223b0);
        count=1;words[1]=argument;
        sprintf(line,"container-constructor-start stage %d target %08lx argument %lu stack %08lx\n",container_stage,target,argument,direct_stack);
    } else {
        target=base+direct_target_rva;count=direct_count;
        for(index=0;index<count;++index)words[index+1]=direct_arguments[index];
        sprintf(line,"direct-start target %08lx marker %08lx stack %08lx count %u args %08lx %08lx\n",target,direct_marker,direct_stack,count,direct_arguments[0],direct_arguments[1]);
    }
    WriteFile(log,line,strlen(line),&written,0);
    if(!WriteProcessMemory(process,(void *)direct_stack,words,sizeof(words),&written)||written!=sizeof(words))return -1;
    current->Eip=target;current->Esp=direct_stack;
    if(!SetThreadContext(thread,current))return -1;
    direct_active=1;return 1;
}
static int container_constructor_return(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *current,char *buffers)
{
    DWORD got,written,canary[4],fields[2],items,symbol,index;
    unsigned char bytes[5],values[4];char line[256];
    if(current->Esp!=direct_stack+8||!current->Eax||current->Ebx!=direct_saved.Ebx||current->Esi!=direct_saved.Esi||
       current->Edi!=direct_saved.Edi||current->Ebp!=direct_saved.Ebp||current->FloatSave.ControlWord!=direct_saved.FloatSave.ControlWord||
       !ReadProcessMemory(process,(void *)(direct_stack+8),canary,16,&got)||got!=16)return -1;
    for(index=0;index<4;++index)if(canary[index]!=0xa5a5a5a5)return -1;
    container_owned[container_stage-1]=current->Eax;
    sprintf(line,"container-constructor-return stage %d address %08lx esp %08lx expected %08lx nonvolatile 1 fctrl %08lx before %08lx canary a5x16\n",container_stage,current->Eax,current->Esp,direct_stack+8,current->FloatSave.ControlWord,direct_saved.FloatSave.ControlWord);
    WriteFile(log,line,strlen(line),&written,0);
    if(container_unit<10) {
        if(!ReadProcessMemory(process,(void *)current->Eax,fields,8,&got)||got!=8||fields[0]||fields[1])return -1;
        fields[0]=container_vector;fields[1]=1;
        if(!WriteProcessMemory(process,(void *)current->Eax,fields,6,&written)||written!=6||
           !ReadProcessMemory(process,(void *)(container_vector+8),&items,4,&got)||got!=4||
           !ReadProcessMemory(process,(void *)(items+8),direct_arguments+1,4,&got)||got!=4)return -1;
        direct_arguments[0]=current->Eax;
    } else if(container_unit<=12) {
        if(!ReadProcessMemory(process,(void *)current->Eax,fields,4,&got)||got!=4||fields[0]!=31||
           !ReadProcessMemory(process,(void *)(current->Eax+4),bytes,5,&got)||got!=5||
           bytes[0]||bytes[1]||bytes[2]||bytes[3]||bytes[4]!=255)return -1;
        values[0]=container_stage==1?0x5a:0x0f;values[1]=container_stage==1?0xa5:0xf0;
        values[2]=container_stage==1?0x0f:0x33;values[3]=container_stage==1?0x40:0x20;
        if(!WriteProcessMemory(process,(void *)(current->Eax+4),values,4,&written)||written!=4)return -1;
        if(container_unit==11&&container_stage==1){container_stage=2;return container_call(process,thread,log,base,current,1);}
        direct_arguments[0]=container_owned[0];if(container_unit==11)direct_arguments[1]=container_owned[1];
    } else {
        symbol=parsed_global_symbol(process,base,"container_result");
        if(!symbol||!ReadProcessMemory(process,(void *)(symbol+20),&container_initial_flags,4,&got)||got!=4||
           !WriteProcessMemory(process,(void *)current->Eax,&container_initial_flags,4,&written)||written!=4)return -1;
        direct_arguments[0]=current->Eax;
    }
    container_stage=0;
    if(!container_owned_state(process,log,"before"))return -1;
    if(buffers)dump_stream_buffers(process,log,base,buffers);
    return container_call(process,thread,log,base,current,0);
}
static int container_return(HANDLE process,HANDLE log,DWORD base,DWORD result)
{
    DWORD got,written,index;char text[512],line[600];
    if(container_unit==6) {
        for(index=0;index<sizeof(text);++index) {
            if(!ReadProcessMemory(process,(void *)(result+index),text+index,1,&got)||got!=1)return 0;
            if(!text[index])break;
        }
        if(index==sizeof(text))return 0;
        sprintf(line,"container-name result address %08lx text %s\n",result,text);WriteFile(log,line,strlen(line),&written,0);
    }
    if(container_unit==7&&(!container_type_state(process,log,base,container_type,"after")||
       !container_type_state(process,log,base,result,"result")))return 0;
    if(container_unit>=8&&container_unit<=13&&!container_owned_state(process,log,"after"))return 0;
    if(container_unit==10) {
        DWORD count;unsigned char bytes[5];
        if(!ReadProcessMemory(process,(void *)result,&count,4,&got)||got!=4||count!=31||
           !ReadProcessMemory(process,(void *)(result+4),bytes,5,&got)||got!=5)return 0;
        sprintf(line,"container-bitset result 0 address %08lx count %lu bytes %02x%02x%02x%02x%02x\n",result,count,bytes[0],bytes[1],bytes[2],bytes[3],bytes[4]);WriteFile(log,line,strlen(line),&written,0);
    }
    if(container_unit==14) {
        DWORD fields[2];
        if(!ReadProcessMemory(process,(void *)result,fields,8,&got)||got!=8)return 0;
        sprintf(line,"container-empty result address %08lx count %d capacity %d arena %ld\n",result,(short)fields[0],(short)(fields[0]>>16),fields[1]);WriteFile(log,line,strlen(line),&written,0);
    }
    return container_vector_state(process,log,container_vector,"after")&&
           (container_unit>1||container_vector_state(process,log,result,"result"));
}
