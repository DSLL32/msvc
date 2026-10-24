#ifndef RECON_NATIVE_LOCALE_H
#define RECON_NATIVE_LOCALE_H
#include_next <locale.h>
#if !defined(RECON_SHIM_IMPL) && !defined(RECON_CRT_PROFILE_WATCOM_100A)
/* Microsoft LOCALE.H category values. Selection remains unsupported. */
#undef LC_ALL
#undef LC_COLLATE
#undef LC_CTYPE
#undef LC_MONETARY
#undef LC_NUMERIC
#undef LC_TIME
#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_MIN LC_ALL
#define LC_MAX LC_TIME
#endif
#endif
