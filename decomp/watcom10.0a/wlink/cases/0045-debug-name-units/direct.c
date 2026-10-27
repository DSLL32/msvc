/* Native-only valid descriptor/debug owners and actual empty callbacks. */
#include <string.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include "../../src/demangle.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) return 0; } while(0)
extern int base_32_num( output_desc *, int * );
extern void ZeroAddrInfo( debug_info * );
extern void EndTime( void );
extern void FreeVirtMem( void );
int debug_name_unit( void )
{
 output_desc data;
 memset(&data,0,sizeof(data));data.suppress_output=2;
 forceSuppression(&data);
 return data.suppress_output==5;
}
static int suppression_and_base32( void )
{
 static const char *numbers[]={"0!","9!","A!","V!","10!","1V!","10A!"};
 static const int values[]={0,9,10,31,32,63,1034};
 output_desc data,expected;
 unsigned i;int value;
 for(i=0;i<6;++i) {
  memset(&data,0x5a,sizeof(data));expected=data;
  data.suppress_output=i;expected.suppress_output=i<=2?i+3:i;
  forceSuppression(&data);CHECK(memcmp(&data,&expected,sizeof(data))==0);
  unforceSuppression(&data);CHECK(data.suppress_output==(i<=2?i:i-3));
 }
 for(i=0;i<RECON_COUNT(numbers);++i) {
  init_descriptor(&data,0,0,numbers[i],0,0,0);
  value=-1;CHECK(base_32_num(&data,&value)==1 && value==values[i]);
  CHECK(*data.input=='!' && data.input==numbers[i]+strlen(numbers[i])-1);
 }
 return 1;
}
static int zero_address_owner( void )
{
 struct { unsigned char before; debug_info info; unsigned char after; } owner;
 debug_info expected;
 static const unsigned char bytes[14]={0,0,0,0,0,0,1,0,0,0,0,0,0,0};
 memset(&owner,0,sizeof(owner));owner.before=0xa5;owner.after=0x5a;
 expected=owner.info;VirtMemInit();ZeroAddrInfo(&owner.info);
 CHECK(owner.info.addr.init!=0 && owner.info.addr.size==14);
 CHECK(owner.info.addr.curr==0 && owner.info.DBIWrite==owner.info.addr.init+14);
 CHECK(memcmp((void *)owner.info.addr.init,bytes,sizeof(bytes))==0);
 expected.addr.init=owner.info.addr.init;expected.addr.size=14;
 expected.DBIWrite=owner.info.addr.init+14;
 CHECK(memcmp(&owner.info,&expected,sizeof(expected))==0);
 CHECK(owner.before==0xa5 && owner.after==0x5a);FreeVirtMem();
 return 1;
}
static int empty_callbacks( void )
{
 section sect,sect_before;group_entry group,group_before;
 unsigned short state;
 memset(&sect,0x69,sizeof(sect));sect_before=sect;
 memset(&group,0x5a,sizeof(group));group_before=group;
 state=LinkState;EndTime();RefGroup(&group);ODBISectCleanup(&sect);
 CHECK(LinkState==state);
 CHECK(memcmp(&sect,&sect_before,sizeof(sect))==0);
 CHECK(memcmp(&group,&group_before,sizeof(group))==0);
 return 1;
}
const ReconTest debug_name_unit_tests[]={
 {"descriptor-suppression-and-small-base32",1,suppression_and_base32},
 {"owned14B-empty-debug-address-record",1,zero_address_owner},
 {"actual-empty-debug-map-callbacks",1,empty_callbacks}
};
