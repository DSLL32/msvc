#ifndef WPP_DIRECTORY_BOUNDARY_H
#define WPP_DIRECTORY_BOUNDARY_H
/* Native binding only for the target's old CRT callback variant. Applied
 * solely to iosupp.c; never forced into shared shims or host CRT sources. */
typedef struct wpp_directory WppDirectory;
WppDirectory *wpp_opendir(const char *path);
int wpp_closedir(WppDirectory *directory);
#if defined(RECON_NATIVE_TEST) && !defined(WPP_DIRECTORY_IMPL)
#define DIR WppDirectory
#define opendir wpp_opendir
#define closedir wpp_closedir
#endif
#include <recon.h>
#endif
