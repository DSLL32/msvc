#ifndef WPP_ANALCALL_H
#define WPP_ANALCALL_H
#include "ptree.h"
#include <recon.h>
typedef struct call_diagnosis {
    unsigned msg_ambiguous,msg_no_match,msg_no_overload;
} CALL_DIAG;
PTREE AnalyseCall(PTREE expr,CALL_DIAG *diagnosis);
#endif
