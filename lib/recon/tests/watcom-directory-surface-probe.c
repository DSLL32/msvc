#include <assert.h>
#include <direct.h>
#include <recon.h>
#ifdef RECON_CRT_PROFILE_WATCOM_100A
typedef char DirectorySize[sizeof(DIR)==289?1:-1];
typedef char AttributeOffset[offsetof(struct dirent,d_attr)==21?1:-1];
typedef char TimeOffset[offsetof(struct dirent,d_time)==22?1:-1];
typedef char DateOffset[offsetof(struct dirent,d_date)==24?1:-1];
typedef char SizeOffset[offsetof(struct dirent,d_size)==26?1:-1];
typedef char NameOffset[offsetof(struct dirent,d_name)==30?1:-1];
typedef char InodeOffset[offsetof(struct dirent,d_ino)==286?1:-1];
typedef char FirstOffset[offsetof(struct dirent,d_first)==288?1:-1];
typedef char NameLength[sizeof(((DIR *)0)->d_name)==256?1:-1];
typedef char Attributes[_A_NORMAL==0 && _A_RDONLY==1 && _A_HIDDEN==2 &&
 _A_SYSTEM==4 && _A_VOLID==8 && _A_SUBDIR==16 && _A_ARCH==32 && NAME_MAX==255?1:-1];
static DIR *(__cdecl *open_directory)(const char *)=opendir;
static struct dirent *(__cdecl *read_directory)(DIR *)=readdir;
static int (__cdecl *close_directory)(DIR *)=closedir;
#else
#ifdef opendir
#error Watcom directory alias outside profile
#endif
static struct ReconWatcomDirent *(__cdecl *open_directory)(const char *)=recon_watcom_opendir;
static struct ReconWatcomDirent *(__cdecl *read_directory)(struct ReconWatcomDirent *)=recon_watcom_readdir;
static int (__cdecl *close_directory)(struct ReconWatcomDirent *)=recon_watcom_closedir;
#endif
int main(int argc,char **argv)
{
    unsigned char owned[289];void *input;int operation;
    assert(argc==3);memset(owned,0xa5,sizeof(owned));
    input=argv[2][0]=='n'?NULL:argv[2][0]=='i'?(void *)1:owned;
    operation=argv[1][0];
    if(operation=='o')open_directory((const char *)input);
    else if(operation=='r')read_directory((struct ReconWatcomDirent *)input);
    else if(operation=='c')close_directory((struct ReconWatcomDirent *)input);
    else assert(0);
    return 42;
}
