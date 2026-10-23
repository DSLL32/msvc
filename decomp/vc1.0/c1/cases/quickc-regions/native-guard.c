/* External guard control; never part of the reconstructed application graph. */
#include <string.h>
#include "../../src/initializers.h"
#include <recon.h>
extern int __real_main(int, char **, char **);
int __wrap_main(int argc, char **argv, char **envp)
{
    if (argc == 2 && !strcmp(argv[1], "--unsupported")) {
        emit_initializer_builtin_call(0, 2, 0);
        return 97;
    }
    return __real_main(argc, argv, envp);
}
