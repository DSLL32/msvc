/* Process-wide CRT state used by the recovered startup/termination functions.
 * These are semantic declarations of encountered runtime objects, not an image
 * of the original data section. Initializer boundaries remain SDK CRT sections. */
#include "crt-runtime.h"
unsigned int _osver, _winver, _winmajor, _winminor;
unsigned int _umaskval;
unsigned long _doserrno;
int errno, __argc;
char **__argv, **_environ, *_pgmptr;
char _exitflag;
CRTCallback _FPinit;
CRTCallback *__onexitbegin, *__onexitend;
