/* Pull in the same static CRT termination-table members as LINK.EXE.
 * Merely using printf/fopen does not pull these separate objects from LIBC.
 */
extern void _endstdio(void), _rmtmp(void);
void (*const runtime_termination_anchors[])(void) = {_endstdio, _rmtmp};
/* CINITTMP.OBJ owns .CRT$XPX; RMTMP.OBJ alone has no table entry. */
extern int _tmpoff;
int *const temporary_runtime_anchor = &_tmpoff;
