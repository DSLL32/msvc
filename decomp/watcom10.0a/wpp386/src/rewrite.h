#ifndef WPP_REWRITE_H
#define WPP_REWRITE_H
#include "cgio.h"
#include <recon.h>
typedef struct rewrite_package REWRITE;
void RewriteFree(REWRITE *rewrite);
REWRITE *RewriteRecordInit(CGIO_LOCATION *location);
REWRITE *RewritePackageClassTemplate(REWRITE *rewrite,CGIO_LOCATION *location);
void RewriteRecordToken(REWRITE *rewrite,CGIO_LOCATION *location);
void RewriteToken(void);
REWRITE *RewriteRewind(REWRITE *rewrite);
void RewriteClose(REWRITE *rewrite);
#endif
