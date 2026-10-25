#include <recon-tool.h>
extern int c2_scope_owned_record(void);
const ReconTest c2_tests[]={{"actual SEH owned48B allocation at parent query",1,c2_scope_owned_record}};
