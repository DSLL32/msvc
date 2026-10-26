#ifndef WPP_CONVDIAG_H
#define WPP_CONVDIAG_H
#include "ptree.h"
#include <recon.h>
/* Initial cgfront.h CNV_DIAG, unchanged five32-bit message roles in1994. */
typedef struct conversion_diagnosis {
    int msg_impossible,msg_ambiguous,msg_virt_der,msg_private,msg_protected;
} CNV_DIAG;
unsigned ConversionDiagnose(unsigned result,PTREE expr,CNV_DIAG *diagnosis);
unsigned CastImplicit(PTREE *expr,TYPE type,unsigned control);
#endif
