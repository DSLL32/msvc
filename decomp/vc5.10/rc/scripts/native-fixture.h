#ifndef RC_NATIVE_FIXTURE_H
#define RC_NATIVE_FIXTURE_H
#include <recon.h>
void rc_fixture_prepare(int, char **);
extern int rc_fixture_argc;
extern char **rc_fixture_argv, **rc_fixture_environment;
#endif
