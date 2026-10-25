/* Native host adapter, outside the reconstructed application graph. */
#define RECON_SHIM_IMPL
#include "cvpack.h"
#include <recon.h>
#undef main
extern void cvpack_widen_load(const char *,const char *);
int cvpack_native_setup(int debug_dll, const char *module_path)
{
 ReconModuleSpec module; HMODULE target;
 memset(&module,0,sizeof(module));
 module.name="CVPACK.EXE"; module.path=module_path;
 module.sha256="62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6";
 module.flags=RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS;
 target=recon_register_module(&module); if(!target) return 0;
 recon_load_module_resources(target,"resources.rmr");
 if(debug_dll) cvpack_widen_load("widen.rwp","hello.dll");
 return 1;
}
