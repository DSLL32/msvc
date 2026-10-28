#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <dir.h>
#include <recon.h>
extern int use_split(const char *,char *,char *,char *,char *),library_extension(const char *);
typedef struct {const char *path;int flags;const char *part[4];} Vector;
static const Vector cases[]={
 {"",0,{"","","",""}},
 {"vector.c",6,{"","","vector",".c"}},
 {"engine.cpp",6,{"","","engine",".cpp"}},
 {"LIBRARY.LIB",6,{"","","LIBRARY",".LIB"}},
 {"  C:\\dir\\name.tar.lib",30,{"C:","\\dir\\","name.tar",".lib"}},
 {"C:leaf",20,{"C:","","leaf",""}},
 {"C:",16,{"C:","","",""}},
 {"C:\\",24,{"C:","\\","",""}},
 {"dir/",8,{"","dir/","",""}},
 {".",8,{"",".","",""}},
 {"..",8,{"","..","",""}},
 {"a/..",8,{"","a/..","",""}},
 {".lib",2,{"","","",".lib"}},
 {"file.",6,{"","","file","."}},
 {"...",6,{"","","..","."}},
 {"*.lib",7,{"","","*",".lib"}},
 {"dir*/plain",12,{"","dir*/","plain",""}},
 {"Z:dir\\a?.lib",31,{"Z:","dir\\","a?",".lib"}},
 {"/tmp/a.b",14,{"","/tmp/","a",".b"}},
 {"  ",0,{"","","",""}},
 {" file name .lib ",6,{"","","file name ",".lib "}},
 {"C:..",24,{"C:","..","",""}}
};

int main(int argc,char **argv)
{
 unsigned i,j,mask;unsigned char buffers[4][265];char *out[4];char longpath[261];
 unsigned short oldcw,newcw;long double live;char generic[260];long page=sysconf(_SC_PAGESIZE);char *guard;
 if(argc>1){
  if(!strcmp(argv[1],"null"))return use_split(NULL,NULL,NULL,NULL,NULL);
  if(!strcmp(argv[1],"long")){char text[262];memset(text,'a',261);text[261]=0;return use_split(text,NULL,NULL,NULL,NULL);}
  if(!strcmp(argv[1],"nonascii"))return use_split("\200.lib",NULL,NULL,NULL,NULL);
  if(!strcmp(argv[1],"wrap"))return use_split("a.c",NULL,NULL,NULL,(char *)(uintptr_t)0xfffffffe);
  if(!strcmp(argv[1],"overlap")){
   char *shared=mmap(NULL,page,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);pid_t child;int status;
   assert(shared!=MAP_FAILED);memset(shared,0x5a,page);strcpy(shared,"a.lib");child=fork();assert(child>=0);
   if(!child){use_split(shared,shared+1,NULL,NULL,shared+30);_exit(90);}
   assert(waitpid(child,&status,0)==child && WIFEXITED(status)&&WEXITSTATUS(status)==125);
   assert(!strcmp(shared,"a.lib") && shared[30]==0x5a);assert(!munmap(shared,page));return 0;
  }
  if(!strcmp(argv[1],"outputs")){memset(buffers,0x5a,sizeof(buffers));return use_split("C:a.c",(char*)buffers,(char*)buffers,NULL,NULL);}
 }
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)for(mask=0;mask<16;++mask){
  memset(buffers,0xa5,sizeof(buffers));for(j=0;j<4;++j)out[j]=(mask&(1<<j))?(char*)buffers[j]+3:NULL;
  errno=137;recon_doserrno=138;recon_SetLastError(139);
  __asm__ volatile("fnstcw %0;fld1":"=m"(oldcw));
  assert(use_split(cases[i].path,out[0],out[1],out[2],out[3])==cases[i].flags);
  __asm__ volatile("fstpt %0;fnstcw %1":"=m"(live),"=m"(newcw));
  assert(errno==137 && recon_doserrno==138 && recon_GetLastError()==139 && oldcw==newcw && live==1.0L);
  for(j=0;j<4;++j){unsigned k;if(out[j]){assert(!strcmp(out[j],cases[i].part[j]));for(k=0;k<3;++k)assert(buffers[j][k]==0xa5);for(k=3+strlen(out[j])+1;k<265;++k)assert(buffers[j][k]==0xa5);}else for(k=0;k<265;++k)assert(buffers[j][k]==0xa5);}
 }
 memset(longpath,'x',260);longpath[260]=0;memset(buffers,0xa5,sizeof(buffers));
 assert(use_split(longpath,NULL,NULL,(char*)buffers[0],NULL)==FILENAME);assert(strlen((char*)buffers[0])==255 && buffers[0][256]==0xa5);
 longpath[0]='.';assert(use_split(longpath,NULL,NULL,NULL,(char*)buffers[1])==EXTENSION && strlen((char*)buffers[1])==255);
 longpath[0]='/';longpath[259]='/';assert(use_split(longpath,NULL,(char*)buffers[2],NULL,NULL)==DIRECTORY && strlen((char*)buffers[2])==255);
 guard=mmap(NULL,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(guard!=MAP_FAILED);assert(!mprotect(guard+page,page,PROT_NONE));
 assert(use_split("vector.c",NULL,NULL,NULL,guard+page-3)==6 && !strcmp(guard+page-3,".c"));
 assert(use_split("vector",NULL,NULL,NULL,guard+page-1)==FILENAME && !guard[page-1]);assert(!munmap(guard,page*2));
 assert(library_extension("dir\\UPPER.LIB") && !library_extension("engine.cpp") && !library_extension("library"));
 recon_splitpath("  plain.c",NULL,NULL,generic,NULL);assert(!strcmp(generic,"  plain"));
 puts("Borland fnsplit: source vectors/all optional outputs, flags, bounded tails, exact output pages and retained generic parser PASS");return 0;
}
