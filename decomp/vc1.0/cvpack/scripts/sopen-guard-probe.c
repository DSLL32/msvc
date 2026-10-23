/* Original-only guard validation controller; never reconstruction source. */
#include <io.h>
#include <recon.h>

int guard_probe(int argc, char **argv)
{
    int fd;
    int first_mode = argv[4][0] == 'F' ? 0x4002 : 0x8002;
    int second_mode = argv[4][0] == 'T' ? 0x4002 : 0x8002;
    int second_share = argv[4][0] == 'S' ? 0x10 : 0x40;
    fd = _sopen(argv[3], first_mode, 0x40, 0);
    if (fd == -1) return 91;
    _close(fd);
    return _sopen(argv[3], second_mode, second_share, 0);
}
