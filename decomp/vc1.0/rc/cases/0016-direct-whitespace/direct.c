/* Direct helper oracle: all256 low-byte inputs, including signed0xff. */
#include <stdio.h>
#include <recon.h>
extern int __stdcall is_space_byte(signed char);
int direct_case(void)
{
    FILE *out=fopen("results.bin","wb");
    unsigned i,value;
    if(!out)return 0;
    for(i=0;i<256;++i) {
        value=is_space_byte((signed char)i);
        if(fwrite(&value,4,1,out)!=1)return 0;
    }
    return fclose(out)==0;
}
