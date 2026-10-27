/* Format-derived byte assertions, real files/callbacks and valid typed owners. */
#include <stdio.h>
#include <string.h>
#include <recon-tool.h>
#include "../../src/resource.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"resource writer %d: %s\n",__LINE__,#x); return 0; } } while(0)
int resource_writer_unit( void )
{
 char output[6];
 return DefaultConversion(3,"AbZ",output)==6 && memcmp(output,"A\0b\0Z\0",6)==0;
}
static int readback( int handle, const unsigned char *expected, unsigned size, char *name )
{
 unsigned char actual[1024];
 CHECK(size<sizeof(actual));memset(actual,0xa5,sizeof(actual));
 CHECK(QPos(handle)==size && QSeek(handle,0,name)==0);
 CHECK(QRead(handle,actual,sizeof(actual),name)==size);
 CHECK(memcmp(actual,expected,size)==0 && actual[size]==0xa5);
 QClose(handle,name);QDelete(name);return 1;
}
static int scalar_and_identifier( void )
{
 unsigned char u8=0x9a;unsigned short u16=0xbcde;unsigned long u32=0x12345678;
 WResID numeric;
 struct { unsigned char before; WResID id; char extra[2]; unsigned char after; } named;
 static const unsigned char expected[]={0x9a,0xde,0xbc,0x78,0x56,0x34,0x12,
  0,0x34,0x12,1,3,'A','b',0xff,3,'A','b',0xff,3,0,'A',0,'b',0,0xff,0};
 int file;
 memset(&named,0xa5,sizeof(named));named.id.IsName=1;named.id.ID.Name.NumChars=3;
 memcpy(named.id.ID.Name.Name,"Ab\xff",3);numeric.IsName=0;numeric.ID.Num=0x1234;
 file=QOpenRW("scalar.tmp");CHECK(file>=0);
 CHECK(ResWriteUint8(&u8,file)==0 && ResWriteUint16(&u16,file)==0 && ResWriteUint32(&u32,file)==0);
 CHECK(WResWriteWResID(&numeric,file)==0 && WResWriteWResID(&named.id,file)==0);
 CHECK(WResWriteWResIDName(&named.id.ID.Name,file)==0);
 CHECK(WResWriteWResIDNameUni(&named.id.ID.Name,1,file)==0);
 CHECK(named.before==0xa5 && named.after==0xa5 && memcmp(named.id.ID.Name.Name,"Ab\xff",3)==0);
 CHECK(u8==0x9a && u16==0xbcde && u32==0x12345678);
 return readback(file,expected,sizeof(expected),"scalar.tmp");
}
static int directory_records( void )
{
 WResTypeInfo type;WResResInfo res;WResLangInfo lang;WResHeader header;WResExtHeader ext;
 struct { WResTypeInfo info; char extra[3]; } named;
 unsigned char expected[64],initial[64];unsigned used;int file;
 static const unsigned char records[]={2,0,0,0x34,0x12,3,0,1,4,'T','y','p','e',
  1,0,0,0x77,0,9,4,1,0x30,0x10,0x44,0x33,0x22,0x11,0x88,0x77,0x66,0x55};
 type.NumResources=2;type.TypeName.IsName=0;type.TypeName.ID.Num=0x1234;
 memset(&named,0,sizeof(named));named.info.NumResources=3;named.info.TypeName.IsName=1;
 named.info.TypeName.ID.Name.NumChars=4;memcpy(named.info.TypeName.ID.Name.Name,"Type",4);
 res.NumResources=1;res.ResName.IsName=0;res.ResName.ID.Num=0x77;
 lang.lang.lang=0x409;lang.lang.sublang=1;lang.MemoryFlags=0x1030;
 lang.Offset=0x11223344;lang.Length=0x55667788;
 file=QOpenRW("records.tmp");CHECK(file>=0);
 CHECK(WResWriteTypeRecord(&type,file)==0 && WResWriteTypeRecord(&named.info,file)==0);
 CHECK(WResWriteResRecord(&res,file)==0 && WResWriteLangRecord(&lang,file)==0);
 CHECK(readback(file,records,sizeof(records),"records.tmp"));
 memset(&header,0,sizeof(header));memset(&ext,0,sizeof(ext));
 header.Magic[0]=0xc3d4c1d7;header.Magic[1]=0xc3d2cdcf;header.DirOffset=0x12345678;
 header.NumResources=2;header.NumTypes=1;header.WResVer=2;ext.TargetOS=1;
 memset(initial,0xa5,sizeof(initial));file=QOpenRW("header.tmp");CHECK(file>=0);
 CHECK(QWrite(file,initial,32,"header.tmp")==32);
 CHECK(WResWriteHeaderRecord(&header,file)==0 && QPos(file)==18);
 CHECK(WResWriteExtHeader(&ext,file)==0 && QPos(file)==28);
 /* The writer must seek to offset zero and overwrite only its18+10 bytes. */
 used=0;memcpy(expected,"\xd7\xc1\xd4\xc3\xcf\xcd\xd2\xc3\x78\x56\x34\x12\x02\x00\x01\x00\x02\x00",18);used+=18;
 expected[used++]=1;memset(expected+used,0,9);used+=9;memset(expected+used,0xa5,4);used+=4;
 CHECK(QSeek(file,32,"header.tmp")==32);
 return readback(file,expected,used,"header.tmp");
}
static int strings_and_conversion( void )
{
 struct { unsigned char before; char bytes[602]; unsigned char after; } output;
 char input[301];unsigned char expected[620];unsigned i,n;int file;
 memset(&output,0xa5,sizeof(output));
 CHECK(DefaultConversion(0,0,output.bytes)==0 && output.bytes[0]==(char)0xa5);
 CHECK(DefaultConversion(301,input,0)==602);
 for(i=0;i<300;++i)input[i]=(char)('A'+i%26);input[300]=0;
 CHECK(DefaultConversion(301,input,output.bytes)==602);
 for(i=0;i<301;++i)CHECK(output.bytes[2*i]==input[i] && output.bytes[2*i+1]==0);
 CHECK(output.before==0xa5 && output.after==0xa5);
 file=QOpenRW("strings.tmp");CHECK(file>=0);
 CHECK(ResWriteString(0,0,file)==0 && ResWriteString("Hi",0,file)==0);
 CHECK(ResWriteString("Hi",1,file)==0 && ResWriteString(input,1,file)==0);
 n=0;expected[n++]=0;memcpy(expected+n,"Hi\0",3);n+=3;memcpy(expected+n,"H\0i\0\0\0",6);n+=6;
 for(i=0;i<301;++i){expected[n++]=input[i];expected[n++]=0;}
 return readback(file,expected,n,"strings.tmp");
}
static int mres_name_and_header( void )
{
 ResNameOrOrdinal ordinal;ResNameOrOrdinal *name=(ResNameOrOrdinal *)"Named";
 MResResourceHeader header;int file;unsigned char expected[64];unsigned n;
 ordinal.ord.fFlag=0xff;ordinal.ord.wOrdinalID=10;
 file=QOpenRW("names.tmp");CHECK(file>=0);
 CHECK(ResWriteNameOrOrdinal(0,0,file)==0);
 CHECK(ResWriteNameOrOrdinal(&ordinal,0,file)==0 && ResWriteNameOrOrdinal(&ordinal,1,file)==0);
 CHECK(ResWriteNameOrOrdinal(name,1,file)==0);
 n=0;memcpy(expected,"\0\xff\x0a\0\xff\xff\x0a\0",8);n=8;
 memcpy(expected+n,"N\0a\0m\0e\0d\0\0\0",12);n+=12;
 CHECK(readback(file,expected,n,"names.tmp"));
 header.Type=&ordinal;header.Name=name;header.MemoryFlags=0x30;header.Size=7;
 file=QOpenRW("mres.tmp");CHECK(file>=0 && MResWriteResourceHeader(&header,file)==0);
 memcpy(expected,"\xff\x0a\0Named\0\x30\0\x07\0\0\0",15);
 return readback(file,expected,15,"mres.tmp");
}
const ReconTest resource_writer_tests[]={
 {"scalar-IDs-and-byte-zero-extension",1,scalar_and_identifier},
 {"typed-directory-records-and-header-cursors",1,directory_records},
 {"short-and-owned-long-conversion-strings",1,strings_and_conversion},
 {"Win16-names-ordinals-and-header",1,mres_name_and_header}
};
