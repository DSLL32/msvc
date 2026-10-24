/* Fixed query/copy/close units over the existing constructor-owned database. */
#include "../../src/pch.h"
#include "../../src/browser.h"
#include <recon.h>
static void query_streams(const char *boundary)
{
    FILE *streams[7];unsigned i,n,count,length;char line[640];
    static char digits[]="0123456789abcdef";
    streams[0]=expression_file;streams[1]=symbol_file;streams[2]=line_file;
    streams[3]=global_file;streams[4]=debug_file;streams[5]=pch_file;streams[6]=browser_output;
    sprintf(line,"pdb-query-streams %s\n",boundary);native_emit(0,line,strlen(line));
    for(i=0;i<7;++i) {
        length=sprintf(line,"buffer %u ",i);
        if(!streams[i])length+=sprintf(line+length,"unopened\n");
        else {
            count=(unsigned)streams[i]->_ptr-(unsigned)streams[i]->_base;
            if(count>256)fail();
            length+=sprintf(line+length,"%u ",count);
            for(n=0;n<count;++n){unsigned byte=(unsigned char)streams[i]->_base[n];line[length++]=digits[byte>>4];line[length++]=digits[byte&15];}
            line[length++]='\n';
        }
        native_emit(0,line,length);
    }
}
static int query_owned(unsigned address,unsigned bytes)
{
    unsigned i;
    for(i=0;i<allocation_count;++i)
        if(address>=allocations[i]&&address-allocations[i]<=allocation_bytes[i]&&
           bytes<=allocation_bytes[i]-(address-allocations[i]))return 1;
    return 0;
}
static void execute_query(void)
{
    unsigned char buffer[256],*record;
    unsigned length,required,index,result,before_stack,after_stack,stage;
    unsigned short before_control,after_control;
    char line[256],label[80];
    if(database->cumulative!=1||database->incremental!=1||database->writable!=1||database->handle<0||
       database->header.first_type<0x1000||database->header.first_type>=database->header.last_type||
       !query_owned((unsigned)database->type_offsets,4*(database->header.last_type-0x1000)))fail();
    index=database->header.first_type;record=database->type_offsets[index-0x1000];
    if(!query_owned((unsigned)record,4))fail();
    required=(unsigned short)(*(unsigned short *)(record+2)+2);
    if(required<3||required>244||!query_owned((unsigned)record,required+2)||
       (unsigned)buffer<(unsigned)unit_stack+16||
       (unsigned)(buffer+256)>(unsigned)unit_stack+65536+16||
       (unsigned)&length<(unsigned)unit_stack+16||
       (unsigned)&length+4>(unsigned)unit_stack+65536+16)fail();
    sprintf(line,"pdb-query-begin unit %u owner %08x first %u last %u record %08x bytes %u buffer %08x length %08x allocation %08x size65536\n",selected_unit,(unsigned)database,index,database->header.last_type,(unsigned)record,required,(unsigned)buffer,(unsigned)&length,(unsigned)unit_stack+16);
    native_emit(0,line,strlen(line));pdb_memory(&observer,"query-record",(unsigned)record,required+2);
    query_streams("before");
    for(stage=10;stage<(selected_unit==7||selected_unit==10?12U:11U);++stage) {
        if(selected_unit==7||selected_unit==10) {
            memset(buffer,0xa5,sizeof(buffer));length=stage==10?required+4:3;
            sprintf(line,"pdb-query-copy capacity %u buffer %08x length %08x\n",length,(unsigned)buffer,(unsigned)&length);native_emit(0,line,strlen(line));
        }
        __asm { mov before_stack,esp }
        __asm { fnstcw before_control }
        switch(selected_unit) {
        case 6: result=(unsigned)query_program_database_record(database,0,(unsigned short)index); break;
        case 7: result=copy_program_database_record(database,0,(unsigned short)index,buffer,&length); break;
        case 8: result=close_program_database_handle(database); break;
        case 9: result=(unsigned)program_database_type_record(database,(unsigned short)index); break;
        case 10: result=copy_program_database_type_record(database,(unsigned short)index,buffer,&length); break;
        case 11: result=program_database_magic(); break;
        case 12: result=program_database_first_type(database); break;
        case 13: result=program_database_last_type(database); break;
        case 14: result=program_database_payload_bytes(database); break;
        default:fail();result=0;
        }
        __asm { mov after_stack,esp }
        __asm { fnstcw after_control }
        if(before_stack!=after_stack||before_control!=after_control)fail();
        sprintf(line,"pdb-native-query-return %u %08x %08x %08x %04x %04x\n",stage,result,before_stack,after_stack,before_control,after_control);native_emit(0,line,strlen(line));
        if(selected_unit==8)errors("query-result");
        query_streams(stage==10?"after-full":"after-short");
        sprintf(label,"query-object-after-%u",stage);pdb_memory(&observer,label,(unsigned)database,116);
        if(selected_unit==7||selected_unit==10) {
            pdb_memory(&observer,stage==10?"query-copy-full":"query-copy-short",(unsigned)buffer,256);
            pdb_memory(&observer,stage==10?"query-length-full":"query-length-short",(unsigned)&length,4);
        }
    }
}
