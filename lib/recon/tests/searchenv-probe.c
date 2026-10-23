/* Explicit original-only single-directory _searchenv state and tail probe. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <recon.h>
int main(int argc,char **argv,char **envp)
{
    DWORD errors[2]={0,0x12345678},after,dos;int values[2]={0,111},saved;
    unsigned i,j;char output[260],name[64],directory[260];FILE *file;
    char filename[]="kernel32.lib",variable[]="LIB";
    recon_crt_bind_arguments(argc,argv,envp);
    if(argc==3){_searchenv(argv[1],argv[2],output);printf("%s\n",output);return 0;}
    if(argc==2){
        if(!strcmp(argv[1],"overlap")){_searchenv(filename,variable,filename);return 42;}
        if(!strcmp(argv[1],"locale"))recon_mb_codepage=1252;
        _searchenv(!strcmp(argv[1],"name")?"../kernel32.lib":"kernel32.lib","LIB",output);return 42;
    }
    strcpy(directory,getenv(variable));
    printf("LIB=%s\n",getenv("LIB"));
    for(i=0;i<2;++i)for(j=0;j<2;++j){
        memset(output,0xa5,sizeof(output));errno=values[j];_doserrno=77;SetLastError(errors[i]);
        _searchenv(filename,variable,output);after=GetLastError();saved=errno;dos=_doserrno;
        if(strcmp(filename,"kernel32.lib") || strcmp(variable,"LIB") || strcmp(directory,getenv(variable)))return 124;
        printf("errno=%d->%d dos=77->%lu error=%lu->%lu output=%s\n",values[j],saved,dos,errors[i],after,output);
        sprintf(name,"output-%u-%u.bin",i,j);file=fopen(name,"wb");if(!file)return 125;
        if(fwrite(output,1,sizeof(output),file)!=sizeof(output))return 125;
        fclose(file);
    }
    return 0;
}
