#define _GNU_SOURCE
#define RECON_SHIM_IMPL
#include <assert.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <recon.h>
int main(int argc,char **argv,char **environment)
{
 struct {char name[10];unsigned char tail[6];} label,filesystem;
 struct {DWORD v;unsigned char tail[4];} serial,maximum,flags;
 struct statfs64 info;uint32_t identity[2];long expected;unsigned short oldcw,newcw;long double live;DWORD error;
 const char *family=NULL;int fd;
 if(argc>1){
  if(!strcmp(argv[1],"unknown")){assert(!chdir("/proc"));return recon_GetVolumeInformationA(NULL,NULL,0,NULL,NULL,NULL,NULL,0);}
  if(!strcmp(argv[1],"next-null"))return recon_borland_400_findnext(NULL);
  if(!strcmp(argv[1],"next-invalid"))return recon_borland_400_findnext((struct ffblk*)1);
  if(!strcmp(argv[1],"export-unknown")){recon_GetProcAddress((HMODULE)1,"UnqualifiedSystemExport");return 99;}
  if(!strcmp(argv[1],"bad-output"))return recon_GetVolumeInformationA(NULL,NULL,0,(DWORD*)1,NULL,NULL,NULL,0);
  if(!strcmp(argv[1],"overlap"))return recon_GetVolumeInformationA(NULL,NULL,0,&serial.v,&serial.v,NULL,NULL,0);
  if(!strcmp(argv[1],"unc"))return recon_GetVolumeInformationA("\\\\server\\share\\",NULL,0,NULL,NULL,NULL,NULL,0);
  if(!strcmp(argv[1],"native"))assert(recon_native_entry_initialize(argc,argv,environment));
 if(argc==1 || strcmp(argv[1],"tmpfs"))assert(recon_GetVolumeInformationA("Z:\\",NULL,0,NULL,NULL,NULL,NULL,0));
 }
 fd=open(".",O_RDONLY|O_DIRECTORY);assert(fd>=0);assert(!fstatfs64(fd,&info));expected=fpathconf(fd,_PC_NAME_MAX);assert(expected>0);assert(!close(fd));
 if(info.f_type==EXT4_SUPER_MAGIC)family="ext";else if(info.f_type==TMPFS_MAGIC)family="tmpfs";else if(info.f_type==OVERLAYFS_SUPER_MAGIC)family="overlay";else if(info.f_type==0x2fc12fc1UL)family="zfs";assert(family);
 memcpy(identity,&info.f_fsid,sizeof(identity));
 memset(&label,0xa5,sizeof(label));memset(&filesystem,0xa5,sizeof(filesystem));memset(&serial,0xa5,sizeof(serial));memset(&maximum,0xa5,sizeof(maximum));memset(&flags,0xa5,sizeof(flags));
 errno=137;recon_doserrno=138;recon_SetLastError(139);
 __asm__ volatile("fnstcw %0;fld1":"=m"(oldcw));
 assert(recon_GetVolumeInformationA(NULL,label.name,10,&serial.v,&maximum.v,&flags.v,filesystem.name,10));
 __asm__ volatile("fstpt %0;fnstcw %1":"=m"(live),"=m"(newcw));
 assert(errno==137 && recon_doserrno==138 && recon_GetLastError()==139 && oldcw==newcw && live==1.0L);
 assert(!label.name[0] && !strcmp(filesystem.name,family) && serial.v==(identity[0]^identity[1]) && maximum.v==(DWORD)expected && flags.v==FILE_CASE_PRESERVED_NAMES);
 {unsigned i;for(i=1;i<sizeof(label);++i)assert(((unsigned char*)&label)[i]==0xa5);for(i=strlen(family)+1;i<sizeof(filesystem);++i)assert(((unsigned char*)&filesystem)[i]==0xa5);for(i=0;i<4;++i)assert(serial.tail[i]==0xa5 && maximum.tail[i]==0xa5 && flags.tail[i]==0xa5);}
 assert(recon_GetVolumeInformationA(NULL,NULL,UINT32_MAX,NULL,NULL,NULL,NULL,UINT32_MAX));
 memset(&serial,0x5a,sizeof(serial));memset(&filesystem,0x5a,sizeof(filesystem));
 assert(!recon_GetVolumeInformationA(NULL,NULL,0,&serial.v,NULL,NULL,filesystem.name,1));assert(recon_GetLastError()==234 && serial.v==0x5a5a5a5a && filesystem.name[0]==0x5a);
 assert(!recon_GetVolumeInformationA(NULL,label.name,0,NULL,NULL,NULL,NULL,0) && recon_GetLastError()==234);
 assert(!recon_GetVolumeInformationA("Z:",NULL,0,NULL,NULL,NULL,NULL,0) && recon_GetLastError()==123);
 assert(!recon_GetVolumeInformationA("Z:\\folder\\",NULL,0,NULL,NULL,NULL,NULL,0) && recon_GetLastError()==144);
 if(argc==1){assert(!recon_GetVolumeInformationA("Q:\\",NULL,0,NULL,NULL,NULL,NULL,0) && recon_GetLastError()==3);}
 if(argc==1 || strcmp(argv[1],"tmpfs"))assert(recon_GetVolumeInformationA("Z:\\",NULL,0,NULL,NULL,NULL,NULL,0));
 errno=147;recon_doserrno=148;assert(!recon_GetProcAddress(recon_GetModuleHandleA("kernel32.dll"),"Borland32"));assert(recon_GetLastError()==127 && errno==147 && recon_doserrno==148);
 assert(!recon_GetProcAddress((HMODULE)1,"IsTNT") && recon_GetLastError()==127);
 assert(recon_GetProcAddress((HMODULE)1,"CreateFileMappingA")== (FARPROC)recon_CreateFileMappingA);
 puts("Volume metadata/native entry/optional buffers/tails/errors/FP and genuine export metadata PASS");return 0;
}
