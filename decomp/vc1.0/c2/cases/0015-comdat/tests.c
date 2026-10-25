#include <sys/wait.h>
#include <unistd.h>
#include <recon-tool.h>
#include "omf-output.h"
#include <recon.h>
extern void __fastcall omf_write_index(OmfRecord *,int);
extern void __fastcall omf_write_communal_size(OmfRecord *,int);
extern unsigned __attribute__((regcall)) omf_alignment(unsigned,unsigned) __asm__("omf_alignment");
static int index_bytes(void)
{
    static const int input[]={-1,0,127,128,32767,65535};
    static const unsigned char expected[][2]={{255,0},{0,0},{127,0},{128,128},{255,255},{255,255}};
    unsigned i,j; OmfRecord record;
    for(i=0;i<RECON_COUNT(input);++i){
        unsigned n=input[i]<128?1:2;
        memset(&record,0xa5,sizeof record);record.count=~0U;record.cursor=record.bytes+8;
        omf_write_index(&record,input[i]);
        if(record.count!=n-1||record.cursor!=record.bytes+8+n)return 0;
        for(j=0;j<sizeof record.bytes;++j)
            if(record.bytes[j]!=(j>=8&&j<8+n?expected[i][j-8]:0xa5))return 0;
    }
    return 1;
}
static int communal_bytes(void)
{
    static const struct {int size;unsigned n;unsigned char bytes[4];} rows[]={
        {-1,1,{255}}, {127,1,{127}}, {128,3,{0x81,128,0}},
        {65535,3,{0x81,255,255}}, {65536,4,{0x84,0,0,1}},
        {0x12345678,4,{0x84,0x78,0x56,0x34}}
    };
    unsigned i,j;OmfRecord record;
    for(i=0;i<RECON_COUNT(rows);++i){
        memset(&record,0xa5,sizeof record);record.count=2;record.cursor=record.bytes+8;
        omf_write_communal_size(&record,rows[i].size);
        if(record.count!=2+rows[i].n||record.cursor!=record.bytes+8+rows[i].n)return 0;
        for(j=0;j<sizeof record.bytes;++j)
            if(record.bytes[j]!=(j>=8&&j<8+rows[i].n?rows[i].bytes[j-8]:0xa5))return 0;
    }
    return 1;
}
static int alignment_owner(void)
{
    unsigned i;static const unsigned align[]={1,2,4,16},expect[]={1,2,5,3};
    if(sizeof(OmfRecordStorage)!=4020||offsetof(OmfRecordStorage,records)!=4)return 0;
    for(i=0;i<4;++i){
        unsigned pointer=(unsigned)&omf_records[i];
        if(pointer%8!=(4+1004*i)%8||pointer!=(unsigned)&omf_records[0]+1004*i)return 0;
        if(omf_alignment(pointer,8)!=pointer||omf_alignment(pointer,align[i])!=expect[i])return 0;
    }
    return 1;
}
static int unsupported_alignment(void)
{
    int status,pid=fork();
    if(pid<0)return 0;
    if(!pid){omf_alignment(0x12345678,3);_exit(99);}
    return waitpid(pid,&status,0)==pid&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGTRAP;
}

const ReconTest c2_tests[] = {
    { "OMF signed/truncated indices and owned tails", 1, index_bytes },
    { "OMF communal size thresholds", 1, communal_bytes },
    { "real adjacent pool modulo and hidden EAX transport", 1, alignment_owner },
    { "unproved alignment remains exact native trap", 1, unsupported_alignment },
};
