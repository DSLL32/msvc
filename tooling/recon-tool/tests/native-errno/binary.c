#include "recon-tool.h"
static const ReconExtent code[]={{0x1000,0x1100}};
static const ReconFunction functions[]={
    {"errno_app",0x1000,code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"errno_app",0,0,0,0}
};
RECON_EXPORT const ReconBinary binary_descriptor={RECON_ABI_VERSION,sizeof(ReconBinary),"native-errno","","",0x1000,functions,RECON_COUNT(functions),0,0};
