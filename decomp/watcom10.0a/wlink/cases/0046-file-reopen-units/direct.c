/* Valid file ownership, not resource exhaustion or allocation-failure tests. */
#include <string.h>
#include <stdio.h>
#include <recon-tool.h>
#include "../../src/cmdline.h"
#include <recon.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"reopen assertion %d: %s\n",__LINE__,#x); return 0; } } while(0)
extern int TempFileOpen( char * );
int file_reopen_unit( void )
{
 int handle;
 handle=TempFileOpen("existing.bin");
 CHECK(handle>=0);
 QClose(handle,"existing.bin");
 return 1;
}
static int existing_binary_owner( void )
{
 static const unsigned char expected[]={0,13,10,26,255,127,'B','I','N'};
 struct { unsigned char before,bytes[9],after; } buffer;
 int first,second;
 first=TempFileOpen("existing.bin");second=TempFileOpen("existing.bin");
 CHECK(first>=0 && second>=0 && first!=second);
 memset(&buffer,0xa5,sizeof(buffer));
 CHECK(QRead(first,buffer.bytes,sizeof(expected),"existing.bin")==sizeof(expected));
 CHECK(memcmp(buffer.bytes,expected,sizeof(expected))==0);
 CHECK(buffer.before==0xa5 && buffer.after==0xa5);
 CHECK(QSeek(second,4,"existing.bin")==4);
 CHECK(QRead(second,buffer.bytes,1,"existing.bin")==1 && buffer.bytes[0]==255);
 CHECK(QRead(first,buffer.bytes,1,"existing.bin")==0);
 QClose(first,"existing.bin");QClose(second,"existing.bin");
 return 1;
}
static int overlay_transition( int create )
{
 static const unsigned char initial[]={0,13,10,26,255,127,'B','I','N'};
 static const unsigned char patch[]={0xde,0,0xad};
 unsigned char expected[9],buffer[10];
 outfilelist root,closed,active,target;
 int handle;unsigned size;
 memset(&root,0,sizeof(root));memset(&closed,0,sizeof(closed));
 memset(&active,0,sizeof(active));memset(&target,0,sizeof(target));
 root.next=&closed;root.fname="root-not-opened";root.handle=-1;
 closed.next=&active;closed.handle=-1;closed.fname="closed-not-opened";
 active.next=&target;active.fname="active.tmp";
 target.fname="target.tmp";target.handle=-1;target.file_loc=create?0:3;
 handle=QOpenRW(target.fname);CHECK(handle>=0);
 CHECK(QWrite(handle,(void *)initial,sizeof(initial),target.fname)==sizeof(initial));
 QClose(handle,target.fname);
 active.handle=QOpenRW(active.fname);CHECK(active.handle>=0);OutFiles=&root;
 OpenOvlFile(&target);
 CHECK(active.handle==-1 && target.handle>=0 && root.handle==-1 && closed.handle==-1);
 CHECK(root.next==&closed && closed.next==&active && active.next==&target && target.next==0);
 CHECK(QPos(target.handle)==target.file_loc);
 CHECK(QWrite(target.handle,(void *)patch,sizeof(patch),target.fname)==sizeof(patch));
 CHECK(QPos(target.handle)==target.file_loc+sizeof(patch));
 QSeek(target.handle,0,target.fname);memset(buffer,0xa5,sizeof(buffer));
 size=create?sizeof(patch):sizeof(initial);
 CHECK(QRead(target.handle,buffer,sizeof(buffer),target.fname)==size);
 if(create) memcpy(expected,patch,sizeof(patch));
 else { memcpy(expected,initial,sizeof(initial));memcpy(expected+3,patch,sizeof(patch)); }
 CHECK(memcmp(buffer,expected,size)==0 && buffer[size]==0xa5);
 QClose(target.handle,target.fname);target.handle=-1;OutFiles=0;
 QDelete(active.fname);QDelete(target.fname);
 return 1;
}
static int reopen_overlay( void ) { return overlay_transition(0); }
static int create_overlay( void ) { return overlay_transition(1); }
const ReconTest file_reopen_tests[]={
 {"existing-binary-independent-descriptors",1,existing_binary_owner},
 {"overlay-reopen-position-and-preserved-bytes",1,reopen_overlay},
 {"overlay-zero-position-create-truncates",1,create_overlay}
};
