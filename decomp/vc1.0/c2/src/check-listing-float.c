/* Isolated listing formatter; report through host printf, not target FILEs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listing.h"
#include <recon.h>
#undef printf
#undef sscanf
static unsigned char output[1000];
void __fastcall write_listing_float(unsigned,InitializerValue *,int);
int main(int argc,char **argv)
{
    InitializerValue value;
    unsigned i,byte,used;
    FILE *stream=stdout;
    if(argc!=5 || strlen(argv[2])!=sizeof(value)*2)return 2;
    for(i=0;i<sizeof(value);++i){if(sscanf(argv[2]+2*i,"%2x",&byte)!=1)return 3;((unsigned char*)&value)[i]=byte;}
    memset(output,0xcc,sizeof(output));
    stream->_ptr=stream->_base=(char *)output;
    stream->_cnt=stream->_bufsiz=sizeof(output);
    stream->_flag=0x102;
    listing_column=strtoul(argv[4],NULL,10);
    write_listing_float(strtoul(argv[1],NULL,10),&value,atoi(argv[3]));
    used=(unsigned)(stream->_ptr-stream->_base);
    if(used>sizeof(output))abort();
    for(i=used;i<sizeof(output);++i)if(output[i]!=0xcc)abort();
    stream->_flag=0;stream->_ptr=stream->_base=NULL;
    printf("%u\n",listing_column);
    for(i=0;i<used;++i)printf("%02x",(unsigned char)output[i]);
    printf("\n");return 0;
}
