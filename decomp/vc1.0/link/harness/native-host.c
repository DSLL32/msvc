/* Fixture-only typed bridges; ordinary executable startup lives in entry.c. */
#include "../src/editor-image.h"
#include "../src/import-library.h"
#include "../src/omf.h"
#include "../src/resources.h"
#include <recon.h>
extern int link_native_runtime_setup(const char *);
int link_native_host_setup(void)
{
    return link_native_runtime_setup("R:\\toolchains\\1.0-nt\\bin\\LINK.EXE");
}

/* glibc exports an obsolete create_module syscall with this same name. Case
 * DSOs must call through this uninstrumented host bridge; the library's existing
 * -Bsymbolic-functions binds its exact typed call to LINK's original constructor.
 * No application identity, arguments or state are replaced by this adapter. */
Module *link_native_create_module(const char *display_name,const char *path,
    unsigned long member_offset,unsigned short machine,unsigned long symbol_offset,
    unsigned long symbol_count,unsigned short optional_size,unsigned short characteristics,
    unsigned short sections,Library *library,unsigned char *inserted)
{
    return create_module(display_name,path,member_offset,machine,symbol_offset,
        symbol_count,optional_size,characteristics,sections,library,inserted);
}

/* The legacy input pass uses an unwritten seek-distance/Size scalar and flags.
 * Only the defined third-formal-zero output pass is supported natively. This
 * bridge runs before the genuine application entry; it neither seeds locals
 * nor claims that the original defective input pass has a defined result. */
int link_native_serialize_legacy_resources(int input,int output,unsigned long size)
{
    if(size)recon_unsupported("LINK legacy serializer uninitialized input seek distance");
    return serialize_legacy_resources(input,output,size);
}
