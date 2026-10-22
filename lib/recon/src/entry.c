/* Optional native host entry for programs whose PE CRT startup is separate. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__) && defined(RECON_NATIVE_ENTRY)
#include <unistd.h>
extern char **environ;
#if defined(RECON_NATIVE_ENTRY_TWO_ARGS)
extern int RECON_NATIVE_ENTRY(int,char **);
int main(int argc,char **argv) { return RECON_NATIVE_ENTRY(argc,argv); }
#else
extern int RECON_NATIVE_ENTRY(int,char **,char **);
int main(int argc,char **argv) { return RECON_NATIVE_ENTRY(argc,argv,environ); }
#endif
#endif
