/* Format-authored input, real resource callbacks and exact old projection. */
#include <stdio.h>
#include <string.h>
#include <recon-tool.h>
#include "../../src/resource.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"wide read %d: %s\n",__LINE__,#x); return 0; } } while(0)
int resource_wide_read_unit( void )
{
    ResNameOrOrdinal *name;
    int file;
    file=ResOpenFileRO("empty-name.bin");CHECK(file>=0);
    name=ResRead32NameOrOrdinal(file);
    CHECK(name!=0 && name->name[0]==0 && QPos(file)==2);
    (*WResRtns.free)(name);QClose(file,"empty-name.bin");
    return 1;
}
static int prepare_input( const unsigned char *bytes, unsigned size, char *path )
{
    int file;
    file=QOpenRW(path);
    if(file>=0){QWrite(file,bytes,size,path);QSeek(file,0,path);}
    return file;
}
static int named_and_ordinal( void )
{
    static const unsigned char bytes[]={0xff,0xff,0x34,0x12,0,0,
      'A',1,'b',2,0xff,0x30,0,0,0x69,0x37};
    ResNameOrOrdinal *ordinal,*empty,*name;
    unsigned char tail[3];int file;
    file=prepare_input(bytes,sizeof(bytes),"names.tmp");CHECK(file>=0);
    ordinal=ResRead32NameOrOrdinal(file);
    CHECK(ordinal!=0 && ordinal->ord.fFlag==0xff && ordinal->ord.wOrdinalID==0x1234 && QPos(file)==4);
    empty=ResRead32NameOrOrdinal(file);CHECK(empty!=0 && empty->name[0]==0 && QPos(file)==6);
    name=ResRead32NameOrOrdinal(file);
    CHECK(name!=0 && memcmp(name->name,"Ab\xff\0",4)==0 && QPos(file)==14);
    memset(tail,0xa5,sizeof(tail));CHECK(QRead(file,tail,2,"names.tmp")==2);
    CHECK(tail[0]==0x69 && tail[1]==0x37 && tail[2]==0xa5);
    CHECK(ordinal->ord.wOrdinalID==0x1234 && empty->name[0]==0);
    (*WResRtns.free)(ordinal);(*WResRtns.free)(empty);(*WResRtns.free)(name);
    QClose(file,"names.tmp");QDelete("names.tmp");return 1;
}
static int exact_unit_projection( void )
{
    static const unsigned char bytes[]={0,0,0x41,0x01,0,0x01,0xff,0xff,0,0,0x37,0x65};
    struct { unsigned long before;int length;unsigned long after; } size;
    char *empty,*text;int file;
    file=prepare_input(bytes,sizeof(bytes),"projection.tmp");CHECK(file>=0);
    size.before=0x12345678;size.after=0x89abcdef;size.length=-1;
    empty=ResRead32String(file,&size.length);CHECK(empty!=0 && empty[0]==0 && size.length==0 && QPos(file)==2);
    text=ResRead32String(file,&size.length);
    /* Full16-bit zero terminates;0100 appends an embedded low-byte zero.
     * ffff is ordinary string data here, unlike the name ordinal sentinel. */
    CHECK(text!=0 && size.length==3 && memcmp(text,"A\0\xff\0",4)==0 && QPos(file)==10);
    CHECK(size.before==0x12345678 && size.after==0x89abcdef && empty[0]==0);
    (*WResRtns.free)(text);(*WResRtns.free)(empty);
    CHECK(ResReadUint16((unsigned short *)&size.length,file)==0 && (unsigned short)size.length==0x6537);
    QClose(file,"projection.tmp");QDelete("projection.tmp");return 1;
}
static int long_chunked_string( void )
{
    struct { unsigned char before;unsigned char bytes[606];unsigned char after; } input;
    struct { unsigned before;int length;unsigned after; } size;
    char *text,*name;unsigned i;int file;
    memset(&input,0xa5,sizeof(input));
    for(i=0;i<300;++i){input.bytes[2*i]='A'+i%26;input.bytes[2*i+1]=1+i%17;}
    input.bytes[600]=input.bytes[601]=0;
    input.bytes[602]=0x35;input.bytes[603]=0x79;
    file=prepare_input(input.bytes,604,"long.tmp");CHECK(file>=0);
    size.before=0x12345678;size.after=0x89abcdef;size.length=-1;
    text=ResRead32String(file,&size.length);CHECK(text!=0 && size.length==300 && text[300]==0 && QPos(file)==602);
    for(i=0;i<300;++i)CHECK(text[i]=='A'+i%26);
    CHECK(size.before==0x12345678 && size.after==0x89abcdef);
    CHECK(QSeek(file,0,"long.tmp")==0);
    name=(char *)ResRead32NameOrOrdinal(file);CHECK(name!=0 && QPos(file)==602);
    for(i=0;i<300;++i)CHECK(name[i]=='A'+i%26 && text[i]=='A'+i%26);
    CHECK(name[300]==0 && input.before==0xa5 && input.after==0xa5 && input.bytes[604]==0xa5 && input.bytes[605]==0xa5);
    for(i=0;i<300;++i)CHECK(input.bytes[2*i]=='A'+i%26 && input.bytes[2*i+1]==1+i%17);
    (*WResRtns.free)(text);(*WResRtns.free)(name);
    QClose(file,"long.tmp");QDelete("long.tmp");return 1;
}
const ReconTest resource_wide_read_tests[]={
 {"numeric-empty-named-forms-cursors-and-tail",1,named_and_ordinal},
 {"full-unit-termination-and-low-byte-projection",1,exact_unit_projection},
 {"owned-long-chunked-string-and-name",1,long_chunked_string}
};
