/* Direct original-backed counts include NUL; every output canary is compared. */
#include <stdio.h>
#include <string.h>
#include <recon.h>
extern unsigned int __stdcall copy_basename(const char *,char *);
int direct_case(void)
{
    static const char *paths[]={"","plain.txt","one/two\\three.rc","C:leaf","C:\\dir\\name.bin","trailing/"};
    FILE *out=fopen("results.bin","wb");
    unsigned i,value;unsigned char buffer[96];
    if(!out)return 0;
    for(i=0;i<6;++i) {
        memset(buffer,0xa5,sizeof(buffer));
        value=copy_basename(paths[i],(char *)buffer+8);
        if(fwrite(&value,4,1,out)!=1 || fwrite(buffer,1,sizeof(buffer),out)!=sizeof(buffer))return 0;
    }
    return fclose(out)==0;
}
