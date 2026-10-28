#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/resource.h>
#include <recon.h>
extern int use_open(const char *),two_argument_open(const char *),other_open(const char *,int);
static const unsigned char input[]={'a','\r','\n',0,0x1a,0xff,'z'};
static int descriptors(void){int n=0;DIR *d=opendir("/proc/self/fd");struct dirent *e;assert(d);while((e=readdir(d)))++n;assert(!closedir(d));return n;}
int main(int argc,char **argv)
{
 int fd,second,n,held[4],before;unsigned char bytes[20];unsigned short first_cw,last_cw;long double live;
 if(argc>1){
  if(!strcmp(argv[1],"permission")){
   if(geteuid()==0)assert(!seteuid(65534));assert(use_open("blocked/file.bin")==-1);assert(errno==5 && recon_doserrno==5);return 0;
  }
  if(!strcmp(argv[1],"osfull")){
   struct rlimit limit={32,32};assert(!setrlimit(RLIMIT_NOFILE,&limit));while(open("/dev/null",O_RDONLY)>=0){}
   assert(use_open("input.bin")==-1 && errno==4 && recon_doserrno==4);return 0;
  }
  if(!strcmp(argv[1],"crtfull")){
   int fds[256],count=0;before=descriptors();
   while(count<256 && (fd=use_open("input.bin"))>=0)fds[count++]=fd;
   assert(count==recon_nhandle-3 && fd==-1 && errno==4 && recon_doserrno==4);
   assert(descriptors()==before+count);while(count)assert(!recon_close(fds[--count]));assert(descriptors()==before);return 0;
  }
  if(!strcmp(argv[1],"append"))return other_open("input.bin",0x8800);
  if(!strcmp(argv[1],"share"))return other_open("input.bin",0x8040);
  if(!strcmp(argv[1],"create"))return other_open("new.bin",0x8100);
  if(!strcmp(argv[1],"text"))return other_open("input.bin",0x4000);
  if(!strcmp(argv[1],"null"))return use_open(NULL);
 }
 before=descriptors();for(n=0;n<4;++n){held[n]=open("/dev/null",O_RDONLY);assert(held[n]>=0);}
 errno=137;recon_doserrno=138;recon_SetLastError(139);
 __asm__ volatile("fnstcw %0;fld1":"=m"(first_cw));fd=use_open("input.bin");
 __asm__ volatile("fstpt %0;fnstcw %1":"=m"(live),"=m"(last_cw));
 assert(fd>=3 && errno==137 && recon_doserrno==138 && recon_GetLastError()==139 && live==1.0L && first_cw==last_cw);
 assert(recon_get_osfhandle(fd)!=(long)fd+1);assert(!(fcntl(recon_get_osfhandle(fd)-1,F_GETFD)&FD_CLOEXEC));
 assert((recon_osfile[fd]&0x81)==1);assert(recon_read(fd,bytes,sizeof(bytes))==sizeof(input) && !memcmp(bytes,input,sizeof(input)));
 second=two_argument_open("input.bin");assert(second>=3 && second!=fd);assert(!recon_lseek(second,0,1));
 assert(recon_lseek(fd,2,0)==2 && recon_read(fd,bytes,2)==2 && bytes[0]=='\n' && bytes[1]==0);
 assert(recon_lseek(second,0,1)==0);assert(recon_lseek(second,0,2)==sizeof(input));
 assert(!recon_close(fd));assert(!recon_close(second));
 for(n=0;n<100;++n){fd=two_argument_open("input.bin");assert(fd>=3);assert(!recon_close(fd));}
 for(n=0;n<4;++n)assert(!close(held[n]));assert(descriptors()==before);
 assert(use_open("missing.bin")==-1 && errno==2 && recon_doserrno==2);
 assert(use_open("absent/input.bin")==-1 && errno==2 && recon_doserrno==3);
 assert(use_open("directory")==-1 && errno==5 && recon_doserrno==5);
 fd=use_open("readonly.bin");assert(fd>=3);assert(!recon_close(fd));
 puts("Borland readonly binary open: real descriptors, no vararg fetch, bytes/cursors/lifetimes/inheritance/errors PASS");return 0;
}
