/* OSI PGMPARMS+12 points to the loader-owned zero-initialized BreakFlag.
 * WPP cstart66522..6652a copies it into CFb8d4a. The period NT CRT lacks
 * this OSI cell; retain its real byte/pointer ownership in both builds.
 * Native startup binds the observed initial0/no-delivered-event domain.
 * Console/signal delivery is not implemented or claimed here. */
#include "osi-globals.h"
#include <recon.h>
static char BreakFlag;
char *_BreakFlagPtr=&BreakFlag;
