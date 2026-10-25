/* Shared native case adapter; original main keeps its historical signature. */
#include "entry.h"
extern int main(int, char **, char **);
int c1xx_case_invoke(int argc, char **argv)
{
    return main(argc, argv, (char **)0);
}
