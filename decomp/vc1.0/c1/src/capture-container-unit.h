/* Explicit direct container units over actual parsed -FI ownership. */
#include <recon.h>
static int container_unit=-1;
static DWORD container_vector;
static DWORD container_type;
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
        if(n==sizeof(name)||strcmp(name,index==0?"forced-a.h":index==1?"forced-b.h":"forced-c.h"))return 0;
        sprintf(line,"container-item %s %u address %08lx name %s\n",boundary,index,items[index],name);
        WriteFile(log,line,strlen(line),&written,0);
    }
    return 1;
}
static int container_prepare(HANDLE process,HANDLE log,DWORD base)
{
    DWORD got,items,symbol;
    if(container_unit<0||container_unit>7||!ReadProcessMemory(process,(void *)(base+0x50494),&container_vector,4,&got)||got!=4||
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
    return 1;
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
    return container_vector_state(process,log,container_vector,"after")&&
           (container_unit>1||container_vector_state(process,log,result,"result"));
}
