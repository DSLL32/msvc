#include <stddef.h>
#include "link-driver.h"
#include <recon.h>
typedef char ContextSize[(sizeof(LinkContext)==664)?1:-1];
typedef char LibrarySize[(sizeof(OutputLibrary)==68)?1:-1];
typedef char MacInitializer[(offsetof(LinkContext,mac_initializer_name)==0x19c)?1:-1];
typedef char MacTerminator[(offsetof(LinkContext,mac_terminator_name)==0x1a0)?1:-1];
typedef char CommandLineLibrary[(offsetof(LinkContext,command_line_library)==0x1cc)?1:-1];
typedef char IndexBuffers[(offsetof(OutputLibrary,index_data)==0x1c)?1:-1];
typedef char LibraryFlags[(offsetof(OutputLibrary,flags)==0x30)?1:-1];
typedef char NextLibrary[(offsetof(OutputLibrary,next)==0x3c)?1:-1];
