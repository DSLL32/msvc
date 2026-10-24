/* Uninstrumented external dependency boundary; unknown operations fail closed. */
#include "../src/mre.h"
#include <recon.h>

extern BOOL mre_open(Mre **,const char *,int *,char *,BOOL,BOOL);
extern DWORD mre_query(Mre *,MreDriver **);
extern void mre_init(MreDriver *);
unsigned mre_call_count;
static void unavailable(void)
{
    ++mre_call_count;
    recon_unsupported("unrecorded MSPDB50 minimal-rebuild operation");
}
BOOL WINAPI MREFOpenByName(Mre **owner,const char *path,int *error,char *message,BOOL reproducible,BOOL write)
{
    return mre_open(owner,path,error,message,reproducible,write);
}
DWORD WINAPI MREQueryMreDrv(Mre *owner,MreDriver **driver)
{
    return mre_query(owner,driver);
}
void WINAPI MREDrvOneTimeInit(MreDriver *driver)
{
    mre_init(driver);
}
BOOL WINAPI MREDrvFFilesOutOfDate(MreDriver *driver,void *work)
{
    (void)driver;(void)work;unavailable();return FALSE;
}
BOOL WINAPI MREDrvFRelease(MreDriver *driver)
{
    (void)driver;unavailable();return FALSE;
}
BOOL WINAPI MREFClose(Mre *owner,BOOL commit)
{
    (void)owner;(void)commit;unavailable();return FALSE;
}
DWORD WINAPI MREDrvFSuccessfulCompile(MreDriver *driver,BOOL success,const char *source,const char *object)
{
    (void)driver;(void)success;(void)source;(void)object;unavailable();return 0;
}
BOOL WINAPI MREDrvFUpdateTargetFile(MreDriver *driver,const char *target,BOOL browser)
{
    (void)driver;(void)target;(void)browser;unavailable();return FALSE;
}
