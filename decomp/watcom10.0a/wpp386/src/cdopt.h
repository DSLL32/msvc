#ifndef WPP_CDOPT_H
#define WPP_CDOPT_H
#include "types.h"
#include <recon.h>
typedef struct cd_descr CD_DESCR;
typedef struct cdopt_iter CDOPT_ITER;
CD_DESCR *CDoptDtorBuild(TYPE type);
CD_DESCR *CDoptDefCtorBuild(TYPE type);
int CDoptErrorOccurred(CD_DESCR *info);
void CDoptNoAccFun(CD_DESCR *info);
CDOPT_ITER *CDoptIterBeg(CD_DESCR *info);
unsigned char CDoptIterNextComp(CDOPT_ITER *iter);
long CDoptIterOffsetExact(CDOPT_ITER *iter);
#endif
