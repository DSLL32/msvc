#include "recon-tool.h"
static const ReconExtent main_code[]={{0x1000,0x1100}}, common_code[]={{0x1100,0x1200}}, child_code[]={{0x1200,0x1300}};
static const ReconFunction functions[]={
    {"main",0x1000,main_code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"self_main",0,0,0,0},
    {"common",0x1100,common_code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"self_common",0,0,0,0},
    {"child",0x1200,child_code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"self_child",0,0,0,0}
};
RECON_EXPORT const ReconBinary binary_descriptor={RECON_ABI_VERSION,sizeof(ReconBinary),"native-self","","",0x1000,functions,RECON_COUNT(functions),0,0};
