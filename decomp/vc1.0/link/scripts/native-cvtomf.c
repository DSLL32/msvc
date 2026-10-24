/* External standalone converter test driver, outside the reconstructed graph. */
#include <errno.h>
#include "../src/omf.h"
#include <recon.h>
int *cvtomf_test_errno;
int __wrap_main(int argc,char **argv)
{
    char program[]="cvtomf";
    cvtomf_test_errno=&errno;
    if(argc!=5 || offsetof(IMAGE_AUX_SYMBOL,Sym.FcnAry.Function.PointerToNextFunction)!=12)return 120;
    argv[0]=program;
    /* Same real public contract as convert_omf_object; no converter state seed. */
    if(setjmp(omf_abort))return 125;
    return omf_main(argc,argv);
}
