/* Controlled pass replacement/loader for exercising CL's process handling. */
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <recon.h>
int main(int argc, char **argv)
{
    FILE *file;
    int i;
    if (strstr(argv[0], "fail-pass")) return 7;
    if (strstr(argv[0], "wait-pass")) {
        file = fopen("child-ready", "w");
        if (!file) return 2;
        fclose(file);
        for (i = 0; i < 300; ++i) {
            file = fopen("release-pass", "r");
            if (file) { fclose(file); break; }
            Sleep(100);
        }
        return 0;
    }
    if (argc < 2) return 2;
    return spawnv(0, argv[1], (const char * const *)(argv + 1));
}
