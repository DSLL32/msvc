#include <fcntl.h>
#include <errno.h>
#include <io.h>
#include <recon.h>
typedef char BinaryFlag[O_BINARY==0x8000?1:-1];
typedef char AppendFlag[O_APPEND==0x0800?1:-1];
typedef char SharingFlag[O_DENYREAD==0x30 && O_DENYNONE==0x40?1:-1];
int use_open(const char *path){return open(path,O_RDONLY|O_BINARY,-1);}
int two_argument_open(const char *path){return open(path,O_RDONLY|O_BINARY);}
int other_open(const char *path,int flags){return open(path,flags,-1);}
int target_emfile_matches(void){return errno==EMFILE;}
