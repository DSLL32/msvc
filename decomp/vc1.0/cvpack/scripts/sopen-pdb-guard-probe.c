/* Original-only narrow PDB guard controller; never reconstruction source. */
#include <io.h>
#include <recon.h>

int guard_probe(int argc, char **argv)
{
    int fd;
    int second_mode = argv[4][0] == 'W' ? 0x8002 : 0x8000;
    int second_share = argv[4][0] == 'H' ? 0x10 : 0x40;
    const char *second_name = argv[3]+(argv[4][0] == 'N');
    fd = _sopen(argv[3], 0x8000, 0x40, 0);
    if (fd == -1) return 91;
    _close(fd);
    return _sopen(second_name, second_mode, second_share, 0);
}
