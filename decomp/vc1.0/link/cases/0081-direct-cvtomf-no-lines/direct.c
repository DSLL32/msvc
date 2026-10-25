#include <stdint.h>
#include <stdlib.h>
/* External standalone converter test driver, outside the reconstructed graph. */
#include <errno.h>
#include "../../src/omf.h"
#include <recon.h>
int *cvtomf_test_errno;
int probe_main(int argc,char **argv)
{
    char program[]="cvtomf";
    cvtomf_test_errno=&errno;
    if(argc!=5 || offsetof(IMAGE_AUX_SYMBOL,Sym.FcnAry.Function.PointerToNextFunction)!=12)return 120;
    argv[0]=program;
    /* Same real public contract as convert_omf_object; no converter state seed. */
    if(setjmp(omf_abort))return 125;
    return omf_main(argc,argv);
}

/* Direct test completion follows the original external driver return/CRT exit. */
int run_probe(void)
{
    char *args[] = {"LINK.EXE","-g","-o","converted.obj","legacy.obj",NULL};
    int code=probe_main(5,args);
    return code==0;
}
