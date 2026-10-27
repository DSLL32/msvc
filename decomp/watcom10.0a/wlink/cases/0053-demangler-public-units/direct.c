/* Public contracts from actual1994 bodies and initial source/test grammar. */
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/demangle.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"demangler unit %d: %s\n",__LINE__,#x); return 0; } } while(0)
int demangler_public_unit( void )
{
 char output[4];memset(output,0xa5,sizeof(output));
 return __mangle_operator("+",0,output)==3 && memcmp(output,"$oj",3)==0 && output[3]==(char)0xa5;
}
static int operator_names( void )
{
 static const char * const groups[3][21]={
  {">>","<<","!","[]","->","*","++","--","-","+","&","->*","/","%",",","()","~","^","|","&&","||"},
  {"==","!=","<","<=",">",">="},
  {"=","*=","+=","-=","/=","%=",">>=","<<=","&=","|=","^="}
 };
 static const unsigned counts[]={21,6,11};static const char prefixes[]={'o','r','a'};
 struct { unsigned char before;char bytes[3];unsigned char after; } owner;
 unsigned g,i;char expected[3];
 for(g=0;g<3;++g)for(i=0;i<counts[g];++i){
  memset(&owner,0xa5,sizeof(owner));expected[0]='$';expected[1]=prefixes[g];expected[2]='a'+i;
  CHECK(__mangle_operator(groups[g][i],0,owner.bytes)==3);
  CHECK(memcmp(owner.bytes,expected,3)==0 && owner.before==0xa5 && owner.after==0xa5);
 }
 memset(&owner,0xa5,sizeof(owner));CHECK(__mangle_operator("??",0,owner.bytes)==0);
 CHECK(owner.before==0xa5 && owner.bytes[0]==(char)0xa5 && owner.bytes[2]==(char)0xa5 && owner.after==0xa5);
 CHECK(__mangle_operator("==tail",2,owner.bytes)==3 && memcmp(owner.bytes,"$ra",3)==0);
 CHECK(__mangle_operator(">>",1,owner.bytes)==3 && memcmp(owner.bytes,"$re",3)==0);
 return 1;
}
static int basename_output( void )
{
 static const char * const names[]={"plain","W?foo$n(i)i","W?a$ni","W?$CT:S$n()_","W?$DT:S$n()_"};
 static const char * const expected[]={"plain","foo","a","S","~S"};
 struct { unsigned char before;char text[64];unsigned char after; } owner;
 unsigned i,n;
 for(i=0;i<RECON_COUNT(names);++i){
  memset(&owner,0xa5,sizeof(owner));n=__demangled_basename(names[i],0,owner.text,sizeof(owner.text));
  CHECK(n==strlen(expected[i]) && strcmp(owner.text,expected[i])==0);
  CHECK(owner.before==0xa5 && owner.after==0xa5);

 }
 /* The old NULL-output path cannot zap a space: foo counts its trailing
  * emitted blank even though the actual output-buffer result is three. */
 CHECK(__demangled_basename("plain",0,0,0)==5);
 CHECK(__demangled_basename("W?foo$n(i)i",0,0,0)==4);
 memset(&owner,0xa5,sizeof(owner));CHECK(__demangled_basename("plainTail",5,owner.text,64)==5 && strcmp(owner.text,"plain")==0);
 memset(&owner,0xa5,sizeof(owner));CHECK(__demangled_basename("W?foo$n(i)i",0,owner.text,2)==1 && memcmp(owner.text,"f\0",2)==0);
 CHECK(owner.before==0xa5 && owner.text[2]==(char)0xa5 && owner.after==0xa5);
 return 1;
}
typedef struct event_log event_log;
typedef struct cookie_owner { event_log *log;unsigned identity;struct cookie_owner *next; } cookie_owner;
struct event_log { dm_pts events[64];int values[64];const char *strings[64];void **slot;unsigned count,first,second;int bad; };
static void record_event( void **slot, dm_pts event, int value, const char *text )
{
 cookie_owner *owner=*slot;event_log *log=owner->log;unsigned n=log->count++;
 if(n>=RECON_COUNT(log->events)){log->bad=1;return;}
 if(n==0)log->slot=slot;else if(log->slot!=slot)log->bad=1;
 if(owner->identity==0){++log->first;*slot=owner->next;}else ++log->second;
 log->events[n]=event;log->values[n]=value;log->strings[n]=text;
}
static int callback_parser( void )
{
 static const dm_pts expected[]={DM_MANGLED_NAME,DM_SCOPED_NAME,DM_NAME,DM_SET_INDEX,DM_IDENTIFIER,DM_EMIT_SPACE,
  DM_RESET_INDEX,DM_ZAP_SPACE,DM_TYPE_ENCODING,DM_MODIFIER_LIST,DM_SET_INDEX,DM_CHAR_ENCODING,
  DM_UNMODIFIED_TYPE,DM_BASIC_TYPE,DM_SET_INDEX,DM_CHAR_ENCODING};
 static const char plain[]="plainTail",mangled[]="W?a$ni";
 event_log log;cookie_owner first,second;unsigned i;
 memset(&log,0,sizeof(log));first.log=second.log=&log;first.identity=0;second.identity=1;first.next=&second;second.next=&second;
 __parse_mangled_name(plain,5,&first,record_event);
 CHECK(!log.bad && log.count==1 && log.first==1 && log.second==0);
 CHECK(log.events[0]==DM_COPY_STRING && log.values[0]==5 && log.strings[0]==plain);
 memset(&log,0,sizeof(log));__parse_mangled_name(mangled,0,&first,record_event);
 CHECK(!log.bad && log.count==RECON_COUNT(expected) && log.first==1 && log.second==RECON_COUNT(expected)-1);
 for(i=0;i<RECON_COUNT(expected);++i)CHECK(log.events[i]==expected[i]);
 CHECK(log.values[4]==1 && log.strings[4]==mangled+2 && log.strings[4][0]=='a');
 CHECK(log.values[11]=='N'-'A' && log.values[15]=='I'-'A');
 for(i=0;i<RECON_COUNT(expected);++i)if(i!=4 && i!=11 && i!=15)CHECK(log.values[i]==0 && log.strings[i]==0);
 return 1;
}
const ReconTest demangler_public_tests[]={
 {"all-operator-tables-length-and-canaries",1,operator_names},
 {"basename-constructors-sizing-and-truncation",1,basename_output},
 {"public-event-order-and-live-cookie-alias",1,callback_parser}
};
