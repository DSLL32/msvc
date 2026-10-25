#include <stdint.h>
#include <stdlib.h>
/* External test caller/report writer, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../src/symbols.h"
#include <recon.h>
extern void print_external_table(SymbolTable *);
extern SymbolTable *copy_defined_names(SymbolTable *);
SymbolTable *symbol_test_tables[4];
unsigned long symbol_test_phase;
unsigned char symbol_test_inputs[4096],symbol_test_slots[256];
int *symbol_test_errno;
static int write_all(int fd,const void *bytes,unsigned count)
{
    const char *p=bytes;int n;
    while(count){n=write(fd,p,count);if(n<=0)return 0;p+=n;count-=n;}
    return 1;
}
int probe_main(int argc,char **argv)
{
    External *external;SymbolTable *table;
    unsigned i,j,header[4],fields[3],count;
    unsigned char extra;
    char short_name[9],*name;
    int fd,okay=1;
    if(argc!=2)return 120;
    symbol_test_errno=&errno;fd=open(argv[1],O_RDONLY);
    if(fd<0||read(fd,symbol_test_inputs,4096)!=4096||read(fd,&extra,1)!=0||close(fd))return 121;
    memset(symbol_test_slots,0xa5,sizeof(symbol_test_slots));
    symbol_test_phase=0;initialize_symbols((SymbolTable **)(symbol_test_slots+32));
    symbol_test_tables[0]=*(SymbolTable **)(symbol_test_slots+32);
    for(i=0;i<32;++i){
        symbol_test_phase=1;name=(char *)symbol_test_inputs+i*128+32;
        external=lookup_external(symbol_test_tables[0],strlen(name)<=8,name,symbol_test_slots+80,symbol_test_slots+96);
        if(!external)return 122;
        if(i%3){symbol_test_phase=2;set_external_defined(external,1,symbol_test_tables[0]);}
    }
    symbol_test_phase=3;print_external_table(symbol_test_tables[0]);
    symbol_test_phase=4;symbol_test_tables[1]=copy_defined_names(symbol_test_tables[0]);
    symbol_test_phase=5;print_external_table(symbol_test_tables[1]);
    symbol_test_phase=6;initialize_symbols((SymbolTable **)(symbol_test_slots+48));
    symbol_test_tables[2]=*(SymbolTable **)(symbol_test_slots+48);
    symbol_test_phase=7;print_external_table(symbol_test_tables[2]);
    symbol_test_phase=8;symbol_test_tables[3]=copy_defined_names(symbol_test_tables[2]);
    symbol_test_phase=9;print_external_table(symbol_test_tables[3]);
    fd=open("tables.bin",O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)return 123;
    header[0]=0x314d5953;header[1]=1;header[2]=4;header[3]=0;okay=write_all(fd,header,sizeof(header));
    for(i=0;i<4 && okay;++i){
        table=symbol_test_tables[i];symbol_test_phase=10+i*3;begin_external_enumeration(table);
        count=table->hash->element_count;okay=write_all(fd,&count,4);
        symbol_test_phase=11+i*3;
        while((external=next_external(table))!=NULL){
            if(!count--){okay=0;break;}
            if(external->symbol.N.Name.Short){memcpy(short_name,&external->symbol,8);short_name[8]=0;name=short_name;}
            else name=table->strings.data+external->symbol.N.Name.Long;
            fields[0]=external->flags;fields[1]=external->symbol.N.Name.Short!=0;fields[2]=strlen(name)+1;
            if(!write_all(fd,fields,sizeof(fields))||!write_all(fd,name,fields[2])){okay=0;break;}
        }
        if(count)okay=0;symbol_test_phase=12+i*3;end_external_enumeration(table);
    }
    if(close(fd)||!okay)return 124;
    symbol_test_phase=22;if(recon_fflush(&recon_iob[1]))return 125;
    for(j=0;j<256;++j){
        if((j>=32&&j<36)||(j>=48&&j<52)||j==80)continue;
        if(symbol_test_slots[j]!=0xa5)return 126;
    }
    return 0;
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","names.bin",NULL};
    int code=probe_main(2,args);
    return code==0;
}
