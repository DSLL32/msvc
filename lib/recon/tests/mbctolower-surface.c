#include <mbstring.h>
#include <recon.h>
typedef char UnsignedInt32[(sizeof(unsigned int)==4)?1:-1];
int main(int argc,char **argv)
{
    unsigned int (__cdecl *convert)(unsigned int)=_mbctolower;
    if(argc>1) return (int)convert((unsigned int)strtoul(argv[1],NULL,0));
    return 0;
}
