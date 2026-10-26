/* Native CLI host only. RC's approved App/CRT-boundary bridge deliberately
 * does not invoke the separately retained Win32 mainCRTStartup. */
#include <string.h>
#include <recon.h>

extern int __cdecl resource_main(int,char **,char **);

int main(int argc,char **argv,char **environment)
{
    ReconNativeEntry *process;
    ReconModuleSpec program;
    int status;

    process=recon_native_entry_initialize(argc,argv,environment);
    program.name=strrchr(process->program_path,'\\');
    program.name=program.name ? program.name+1 : process->program_path;
    program.path=process->program_path;
    program.sha256="eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591";
    program.flags=RECON_MODULE_PRIMARY|RECON_MODULE_NO_TLS;
    if(!recon_register_module(&program))
        recon_unsupported("RC native primary module registration");
    status=resource_main(process->argc,process->argv,process->environment);
    recon_record_terminal_status((uint32_t)status);
    return status;
}
