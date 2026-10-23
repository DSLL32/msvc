#ifndef RECON_NATIVE_SYS_STAT_H
#define RECON_NATIVE_SYS_STAT_H
#include_next <sys/stat.h>
/* Legacy types and _stat remapping follow the caller's includes in recon.h. */
#define _S_IFMT 0170000
#define _S_IFDIR 0040000
#define _S_IFCHR 0020000
#define _S_IFIFO 0010000
#define _S_IFREG 0100000
#define _S_IREAD 0000400
#define _S_IWRITE 0000200
#define _S_IEXEC 0000100
#endif
