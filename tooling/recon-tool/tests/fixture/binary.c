#include "recon-tool.h"
static const ReconExtent main_code[]={{0x1000,0x1100}}, add_code[]={{0x1100,0x1110}}, trap_code[]={{0x1200,0x1203}};
static const ReconFunction fns[]={
 {"main",0x1000,main_code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"fixture_main",0,0,0},
 {"add",0x1100,add_code,1,0,0,RECON_APP,RECON_IMPLEMENTED,"fixture_add",0,0,0},
 {"trap",0x1200,trap_code,1,0,0,RECON_APP,RECON_UNIMPLEMENTED,"fixture_trap",0,0,0}
};
RECON_EXPORT const ReconBinary binary_descriptor={RECON_ABI_VERSION,sizeof(ReconBinary),"fixture","", "",0x1000,fns,RECON_COUNT(fns),0,0};
