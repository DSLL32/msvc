#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <dir.h>
#include <recon.h>
#define CHECK(x) assert(x)
typedef char size_is_272[sizeof(struct ffblk)==272?1:-1];
typedef char name_at_16[offsetof(struct ffblk,ff_name)==16?1:-1];
extern int use_find(const char *,struct ffblk *,int);
static int descriptors(void){DIR *d=opendir("/proc/self/fd");struct dirent *e;int n=0;CHECK(d);while((e=readdir(d)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,".."))++n;CHECK(!closedir(d));return n;}
int main(int argc,char **argv)
{
 struct {unsigned char prefix[3];struct ffblk ff;unsigned char tail[7];} __attribute__((packed)) owner;
 struct ffblk other;int baseline,n,result;long reserved;unsigned short cw_before,cw_after;long double live;
 if(argc>1){
  if(!strcmp(argv[1],"null"))return use_find("normal.cfg",NULL,0);
  if(!strcmp(argv[1],"attr"))return use_find("normal.cfg",&other,16);
  if(!strcmp(argv[1],"overlap"))return use_find((char *)&other,&other,0);
  if(!strcmp(argv[1],"long")){char text[260];memset(text,'x',260);return use_find(text,&other,0);}
  if(!strcmp(argv[1],"nonascii"))return use_find("\200.cfg",&other,0);
  if(!strcmp(argv[1],"fds")){
   struct rlimit bound={32,32};CHECK(!setrlimit(RLIMIT_NOFILE,&bound));while(open("/dev/null",O_RDONLY)>=0){}
   CHECK(use_find("normal.cfg",&other,0)==-1);CHECK(errno==4 && recon_doserrno==4);return 0;
  }
  if(!strcmp(argv[1],"permission")){
   if(geteuid()==0)CHECK(!seteuid(65534));CHECK(use_find("blocked/item.cfg",&other,0)==-1);CHECK(errno==5 && recon_doserrno==5);return 0;
  }
 }
 memset(&owner,0xa5,sizeof(owner));baseline=descriptors();
 errno=137;recon_doserrno=138;recon_SetLastError(139);
 __asm__ volatile("fnstcw %0; fld1":"=m"(cw_before));
 result=use_find("normal.cfg",&owner.ff,0);
 __asm__ volatile("fstpt %0; fnstcw %1":"=m"(live),"=m"(cw_after));
 CHECK(!result && live==1.0L && cw_before==cw_after);
 CHECK(errno==137 && recon_doserrno==138 && recon_GetLastError()==139);
 CHECK(owner.ff.ff_fsize==7 && owner.ff.ff_attrib==FILE_ATTRIBUTE_NORMAL && !strcmp(owner.ff.ff_name,"normal.cfg"));
 CHECK(owner.ff.ff_fdate==((44<<9)|(1<<5)|2) && owner.ff.ff_ftime==((3<<11)|(4<<5)|3));
 CHECK((unsigned char)owner.ff.ff_name[11]==0xa5); /* strcpy writes through NUL only. */
 for(n=0;n<3;++n)CHECK(owner.prefix[n]==0xa5);for(n=0;n<7;++n)CHECK(owner.tail[n]==0xa5);
 CHECK(descriptors()==baseline+1);reserved=owner.ff.ff_reserved;CHECK(reserved);
 for(n=0;n<100;++n){CHECK(!use_find("normal.cfg",&owner.ff,0));CHECK(owner.ff.ff_reserved==reserved);}
 CHECK(descriptors()==baseline+1);
 CHECK(!use_find("readonly.cfg",&other,0) && other.ff_attrib==FILE_ATTRIBUTE_READONLY && other.ff_reserved!=reserved);
 CHECK(descriptors()==baseline+2);
 memset((char *)&owner.ff+4,0x5a,sizeof(owner.ff)-4);
 CHECK(use_find("missing.cfg",&owner.ff,0)==-1 && errno==2 && recon_doserrno==2 && recon_GetLastError()==2);
 CHECK(owner.ff.ff_reserved==reserved);for(n=4;n<sizeof(owner.ff);++n)CHECK(((unsigned char*)&owner.ff)[n]==0x5a);
 CHECK(descriptors()==baseline+1);
 CHECK(use_find("absent/normal.cfg",&owner.ff,0)==-1 && errno==2 && recon_doserrno==3);
 CHECK(use_find("onlydir",&owner.ff,0)==-1 && errno==2 && recon_doserrno==18);
 CHECK(!use_find("*.cfg",&owner.ff,0) && !(owner.ff.ff_attrib&FILE_ATTRIBUTE_DIRECTORY));
 CHECK(!use_find("normal.cfg",&owner.ff,0));
 puts("Borland findfirst ABI, metadata, unaligned output, reuse, independent searches, attributes, errors and tails PASS");return 0;
}
