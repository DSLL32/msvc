#include <recon.h>
typedef char wchar_and_count_width[sizeof(WCHAR)==2 && sizeof(size_t)==4 ? 1 : -1];
int main(int argc,char **argv)
{
    WCHAR a[]={65,65535,0},b[]={65,32768,0};
    int (__cdecl *compare)(const WCHAR *,const WCHAR *,size_t)=wcsncmp;
    if(argc<2)return 42;
    if(!strcmp(argv[1],"null-zero")) return compare(NULL,NULL,0);
    if(!strcmp(argv[1],"null-left")) return compare(NULL,b,13);
    if(!strcmp(argv[1],"null-right")) return compare(a,NULL,13);
    if(!strcmp(argv[1],"zero")) return compare(a,b,0);
    if(!strcmp(argv[1],"equal")) return compare(a,a,3);
    if(!strcmp(argv[1],"wide")) return compare(a,b,3);
    if(!strcmp(argv[1],"maximum")) return compare(a,b,(size_t)-1);
    a[0]=b[0]=0;return compare(a,b,1);
}
