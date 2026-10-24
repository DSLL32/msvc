/* Linked type database provider cleanup. */
#include "pdb.h"
#include <recon.h>

int type_server_close(TypeServer *server)
{
    TypeServerBlock *block, *next;
    int okay;
#ifdef RECON_NATIVE_TEST
    /* The original reads descriptor before its later NULL test. */
    if (!server) __asm { int 3 }
#endif
    if (server->descriptor >= 0) {
        okay = _close(server->descriptor) >= 0;
        server->descriptor = -1;
    } else okay = 1;
    if (server) {
        if (server->descriptor != -1 && server->descriptor >= 0) {
            _close(server->descriptor);
            server->descriptor = -1;
        }
        if (server->type_offsets) free(server->type_offsets);
        if (server->type_hash) free(server->type_hash);
        for (block = server->index_storage.first; block; block = next) {
            next = block->next;
            free(block);
        }
        for (block = server->type_storage.first; block; block = next) {
            next = block->next;
            free(block);
        }
        free(server);
    }
    return okay;
}
