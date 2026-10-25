/* Native host integration fixture; it deliberately uses the actual process shim. */
#define RECON_SHIM_IMPL
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "recon-fixture.h"
#include <recon.h>

int self_state=7;
int self_common(int value) { return value+1; }
int self_child(void)
{
    char cwd[4096],bytes[8],path[4096];
    FILE *file;
    const char *mode=getenv("SELF_CASE"),*assets=recon_case_assets();
    if(self_state!=8 || self_common(4)!=5 || !getcwd(cwd,sizeof(cwd)) ||
       strcmp(cwd,recon_case_workspace_cwd()) || !getenv("SELF_MARKER") ||
       strcmp(getenv("SELF_MARKER"),"inherited") ||
       !recon_case_logical_cwd() || strcmp(recon_case_logical_cwd(),"S:\\native-self") || !assets)return 30;
    snprintf(path,sizeof(path),"%s/marker",assets);
    file=fopen(path,"rb");if(!file || fgetc(file)!='A' || fgetc(file)!=EOF)return 31;
    fclose(file);
    if(read(0,bytes,8)!=8 || memcmp(bytes,"payload\n",8))return 32;
    write(1,"child\n",6);write(2,"child diagnostic\n",17);
    if(mode && !strcmp(mode,"trap"))__asm__ volatile("int3");
    if(mode && !strcmp(mode,"raised-trap"))raise(SIGTRAP);
    if(mode && !strcmp(mode,"missing-receipt"))_exit(0);
    if(mode && !strcmp(mode,"hang"))for(;;)pause();
    if(mode && !strcmp(mode,"dword"))return 0x12340007;
    return mode && !strcmp(mode,"seven")?7:0;
}
int self_main(int argc,char **argv)
{
    const char *args[]={"T:\\self.exe","child","space arg",NULL};
    const char *captured[]={"captured.exe","x",NULL};
    const char *mode=getenv("SELF_CASE");
    char bytes[5];int result,expected=mode && (!strcmp(mode,"seven") || !strcmp(mode,"dword"))?7:0;
    if(argc==3 && !strcmp(argv[1],"child")) {
        if(strcmp(argv[0],args[0]) || strcmp(argv[2],"space arg"))return 33;
        return self_child();
    }
    if(self_state!=8 || self_common(2)!=3)return 34;
    self_state=99;
    write(1,"parent\n",7);
    result=recon_spawnve(0,args[0],args,NULL);
    if(mode && (!strcmp(mode,"trap") || !strcmp(mode,"raised-trap")))return result==-1?0:35;
    if(mode && !strcmp(mode,"hang"))return 36;
    if(result!=expected || self_state!=99 || read(0,bytes,5)!=5 || memcmp(bytes,"tail\n",5))return 37;
    write(1,result==7?"parent-status:7\n":"parent-status:0\n",16);
    if(mode && !strcmp(mode,"captured")) {
        if(recon_spawnve(0,captured[0],captured,NULL)!=0)return 38;
    }
    return 0;
}
