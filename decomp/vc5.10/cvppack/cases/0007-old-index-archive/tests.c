/* Focused native checks of this case's typed dependency boundary. These do not
 * replace the ordinary original/native completion pair. */
#define RECON_SHIM_IMPL
#include <sys/wait.h>
#include <unistd.h>
#include <recon-tool.h>
#include "pack.h"
#include "native-widen-abi.h"
#include <recon.h>
extern int cvpack_widen_factory(WidenTi **,unsigned,int);

static int first_record(unsigned mode)
{
    static const unsigned char converted[12] = {
        0x0a,0x00,0x01,0x10,0x02,0x10,0x00,0x00,0x01,0x00,0xf2,0xf1
    };
    ModuleInfo info;
    WidenTi *object;
    unsigned char input[8], saved[8], *output;
    unsigned capacity=65512,i;
    FILE *file=fopen("legacy.dll","rb");
    if(!file || fseek(file,3939,SEEK_SET) || fread(input,1,8,file)!=8) return 90;
    fclose(file);memcpy(saved,input,8);
    memset(&info,0,sizeof(info));info.module=2;current_module_info=&info;
    output=malloc(capacity);if(!output)return 91;memset(output,0xa5,capacity);
    if(!cvpack_widen_factory(&object,256,0))return 92;
    if(mode==1)input[7]^=1;
    if(mode==2)--capacity;
    if(mode==3)object->vtable->destroy(object);
    if(mode==4)object->vtable->query(object,(WidenOutput *)output,input,8,0);
    if(!object->vtable->record(object,output,input,&capacity))return 93;
    if(capacity!=65512 || memcmp(input,saved,8) || memcmp(output,converted,12))return 94;
    for(i=12;i<65512;++i)if(output[i]!=0xa5)return 95;
    return 0;
}
static int check_record(unsigned mode,int expected)
{
    int status,pid=fork();
    if(pid<0)return 0;
    if(!pid)_exit(first_record(mode));
    return waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==expected;
}
static int record_prefix(void){return check_record(0,0);}
static int changed_input(void){return check_record(1,125);}
static int changed_capacity(void){return check_record(2,125);}
static int early_release(void){return check_record(3,125);}
static int wrong_method(void){return check_record(4,125);}
const ReconTest cvpack_c7_tests[5] = {
 {"record-prefix-own-tail",1,record_prefix},
 {"record-changed-input",1,changed_input},
 {"record-changed-capacity",1,changed_capacity},
 {"record-early-release",1,early_release},
 {"record-wrong-method",1,wrong_method}
};
