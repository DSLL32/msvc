/* External PDB observation protocol, outside the application function graph. */
#ifndef PDB_EXTERNAL_H
#define PDB_EXTERNAL_H
#include "../../src/debug-info.h"
#include <string.h>
#include <recon.h>
typedef int (*PdbRead)(void *,unsigned,void *,unsigned);
typedef void (*PdbEmit)(void *,const char *,unsigned);
typedef struct PdbObserver {
    void *context;
    PdbRead read;
    PdbEmit emit;
    int failed;
} PdbObserver;
static void pdb_memory(PdbObserver *o,const char *name,unsigned address,unsigned bytes)
{
    unsigned char raw[256];
    char line[640];
    static char digits[]="0123456789abcdef";
    unsigned i,length;
    if(bytes>sizeof(raw)||!o->read(o->context,address,raw,bytes)) {o->failed=1;return;}
    length=sprintf(line,"pdb-memory %s %08x %u ",name,address,bytes);
    for(i=0;i<bytes;++i) {line[length++]=digits[raw[i]>>4];line[length++]=digits[raw[i]&15];}
    line[length++]='\n';o->emit(o->context,line,length);
}
static void pdb_list(PdbObserver *o,const char *boundary,unsigned address,int follow)
{
    PdbBufferList list;
    PdbBuffer node;
    char label[100];
    unsigned count=0,pointer;
    sprintf(label,"list-%s",boundary);pdb_memory(o,label,address,sizeof(list));
    if(!follow)return;
    if(!o->read(o->context,address,&list,sizeof(list))) {o->failed=1;return;}
    for(pointer=(unsigned)list.head;pointer && count<32;pointer=(unsigned)node.next) {
        sprintf(label,"node-%s-%u",boundary,count++);pdb_memory(o,label,pointer,sizeof(node));
        if(!o->read(o->context,pointer,&node,sizeof(node))) {o->failed=1;return;}
    }
    if(pointer)o->failed=1;
}
static void pdb_database(PdbObserver *o,const char *boundary,unsigned address,int loaded)
{
    ProgramDatabase db;
    char label[100];
    unsigned offset,count;
    sprintf(label,"object-%s",boundary);pdb_memory(o,label,address,sizeof(db));
    if(!o->read(o->context,address,&db,sizeof(db))) {o->failed=1;return;}
    sprintf(label,"records-%s",boundary);pdb_list(o,label,address+72,1);
    sprintf(label,"blocks-%s",boundary);pdb_list(o,label,address+84,1);
    if(!loaded)return;
    if(db.header.last_type<db.header.first_type||db.header.last_type-db.header.first_type>64||db.header.record_bytes>4096) {o->failed=1;return;}
    count=(db.header.last_type-db.header.first_type)*4;
    if(db.type_offsets && count) {
        sprintf(label,"offsets-%s",boundary);pdb_memory(o,label,(unsigned)db.type_offsets,count);
    }
    if(db.type_index)for(offset=0;offset<16384;offset+=256) {
        sprintf(label,"index-%s-%u",boundary,offset);pdb_memory(o,label,(unsigned)db.type_index+offset,256);
    }
}
#endif
