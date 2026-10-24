/* Fixed external PDB query units. No parser/global state is synthesized. */
#include <recon.h>
static char *pdb_query_stream_locations;
static DWORD pdb_query_record,pdb_query_bytes,pdb_query_buffer,pdb_query_length;
static unsigned short pdb_query_index;
static int pdb_query_owned(DWORD address,DWORD bytes)
{
    unsigned i;
    for(i=0;i<pdb_allocation_count;++i)
        if(address>=pdb_allocations[i]&&address-pdb_allocations[i]<=pdb_allocation_bytes[i]&&
           bytes<=pdb_allocation_bytes[i]-(address-pdb_allocations[i]))return 1;
    return 0;
}
static int pdb_query_call(HANDLE process,HANDLE thread,HANDLE log,DWORD base)
{
    static DWORD targets[]={0x7970,0x79d0,0x7b40,0x7de0,0x7e40,0x7fb0,0x7fe0,0x7ff0,0x8000};
    DWORD a=0,b=0,c=0,d=0,self=0,written;
    unsigned args=0,pop=0,index=pdb_unit-6;
    unsigned char buffer[256];unsigned length,i;char line[160];
    if(index>=9)return -1;
    if(pdb_unit==7||pdb_unit==10) {
        length=pdb_stage==11?3:pdb_query_bytes+4;
        for(i=0;i<sizeof(buffer);++i)buffer[i]=0xa5;
        if(!WriteProcessMemory(process,(void *)pdb_query_buffer,buffer,sizeof(buffer),&written)||written!=sizeof(buffer)||
           !WriteProcessMemory(process,(void *)pdb_query_length,&length,4,&written)||written!=4)return -1;
        sprintf(line,"pdb-query-copy capacity %u buffer %08lx length %08lx\n",length,pdb_query_buffer,pdb_query_length);WriteFile(log,line,strlen(line),&written,0);
    }
    if(pdb_unit==6){self=pdb_self;args=1;pop=4;a=pdb_query_index;}
    else if(pdb_unit==7){self=pdb_self;args=3;pop=12;a=pdb_query_index;b=pdb_query_buffer;c=pdb_query_length;}
    else if(pdb_unit==8)self=pdb_self;
    else if(pdb_unit==9){args=2;a=pdb_self;b=pdb_query_index;}
    else if(pdb_unit==10){args=4;a=pdb_self;b=pdb_query_index;c=pdb_query_buffer;d=pdb_query_length;}
    else if(pdb_unit>=12){args=1;a=pdb_self;}
    return pdb_launch(process,thread,log,base+targets[index],self,args,a,b,c,pop,d);
}
static int pdb_query_begin(HANDLE process,HANDLE thread,HANDLE log,DWORD base)
{
    ProgramDatabase object;DWORD got,written;unsigned short length;char line[256];
    if(pdb_stage!=5||pdb_unit<6||pdb_unit>14||
       !ReadProcessMemory(process,(void *)pdb_self,&object,sizeof(object),&got)||got!=sizeof(object)||
       object.cumulative!=1||object.incremental!=1||object.writable!=1||object.handle<0||
       object.header.first_type<0x1000||object.header.first_type>=object.header.last_type||
       !pdb_query_owned((DWORD)object.type_offsets,4*(object.header.last_type-0x1000)))return -1;
    pdb_query_index=object.header.first_type;
    if(!ReadProcessMemory(process,object.type_offsets+(pdb_query_index-0x1000),&pdb_query_record,4,&got)||got!=4||
       !pdb_query_owned(pdb_query_record,4)||
       !ReadProcessMemory(process,(void *)(pdb_query_record+2),&length,2,&got)||got!=2)return -1;
    pdb_query_bytes=(unsigned short)(length+2);
    if(pdb_query_bytes<3||pdb_query_bytes>244||!pdb_query_owned(pdb_query_record,pdb_query_bytes+2))return -1;
    pdb_query_buffer=pdb_external_base+8192;pdb_query_length=pdb_query_buffer+256;
    if(pdb_query_length+4>pdb_external_base+65536)return -1;
    sprintf(line,"pdb-query-begin unit %d owner %08lx first %u last %u record %08lx bytes %lu buffer %08lx length %08lx allocation %08lx size65536\n",pdb_unit,pdb_self,pdb_query_index,object.header.last_type,pdb_query_record,pdb_query_bytes,pdb_query_buffer,pdb_query_length,pdb_external_base);
    WriteFile(log,line,strlen(line),&written,0);
    pdb_memory(&pdb_observer,"query-record",pdb_query_record,pdb_query_bytes+2);
    if(pdb_observer.failed)return -1;
    WriteFile(log,"pdb-query-streams before\n",25,&written,0);
    if(pdb_query_stream_locations)dump_stream_buffers(process,log,base,pdb_query_stream_locations);
    pdb_stage=10;return pdb_query_call(process,thread,log,base);
}
static int pdb_query_return(HANDLE process,HANDLE thread,HANDLE log,DWORD base,CONTEXT *state)
{
    DWORD written;char line[160],label[80];
    sprintf(line,"pdb-query-value stage %d eax %08lx edx %08lx\n",pdb_stage,state->Eax,state->Edx);WriteFile(log,line,strlen(line),&written,0);
    sprintf(line,"pdb-query-streams %s\n",pdb_stage==10?"after-full":"after-short");WriteFile(log,line,strlen(line),&written,0);
    if(pdb_query_stream_locations)dump_stream_buffers(process,log,base,pdb_query_stream_locations);
    sprintf(label,"query-object-after-%d",pdb_stage);pdb_memory(&pdb_observer,label,pdb_self,116);
    if(pdb_unit==7||pdb_unit==10) {
        pdb_memory(&pdb_observer,pdb_stage==10?"query-copy-full":"query-copy-short",pdb_query_buffer,256);
        pdb_memory(&pdb_observer,pdb_stage==10?"query-length-full":"query-length-short",pdb_query_length,4);
        if(pdb_stage==10){pdb_stage=11;return pdb_query_call(process,thread,log,base);}
    }
    if(pdb_unit==8)pdb_unit_errors(process,log,base,"query-result");
    if(pdb_observer.failed||pdb_heap_error)return -1;
    TerminateProcess(process,99);return 1;
}
