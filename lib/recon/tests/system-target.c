#include <assert.h>
#include <recon.h>
int main(int argc,char **argv)
{
    int result; errno=0;
    result=system(argc>1?argv[1]:NULL);
    assert(result==0 && errno==0);
    return 0;
}
