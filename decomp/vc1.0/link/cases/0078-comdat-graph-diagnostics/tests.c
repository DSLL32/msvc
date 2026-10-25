#include <stddef.h>
extern int probe_main(int,char **);
static int occupied(void){char *argv[]={"LINK.EXE","payload.bin","--occupied",NULL};return probe_main(3,argv)==122;}
int test_occupied_mapping(void){return occupied();}
