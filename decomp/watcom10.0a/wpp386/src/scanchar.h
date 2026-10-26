#ifndef WPP_SCANCHAR_H
#define WPP_SCANCHAR_H
#include <recon.h>
#define LCHR_MAX 257
#define C_AL 0x01
#define C_DI 0x02
#define C_HX 0x04
#define C_EX 0x08
#define C_WS 0x40
#define C_DB 0x80
#define C_BC 0
#define C___ 0
#define C_XW (C_WS|C_EX)
extern unsigned char CharSet[LCHR_MAX];
extern unsigned char TokValue[168];
#endif
