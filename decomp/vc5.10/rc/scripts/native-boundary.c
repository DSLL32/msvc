/* Native-only, in-process response replay. No original or PE helper is run. */
#include "rc.h"
#include "native-fixture.h"
#include <recon.h>

typedef struct CallbackResponse {
    unsigned window, code, bytes;
    int result;
    char *before, *after;
} CallbackResponse;
static char *response, *cursor, *limit;
static CallbackResponse callbacks[16];
static unsigned callback_count, used;
static int export_result;
int rc_fixture_argc;
char **rc_fixture_argv, **rc_fixture_environment;

static void reject(const char *reason)
{
    fprintf(stderr, "RC dependency replay: %s\n", reason);
    exit(125);
}
static unsigned word(void)
{
    unsigned value;
    if (limit-cursor<4) reject("truncated response");
    value=(unsigned char)cursor[0] | ((unsigned char)cursor[1]<<8) |
          ((unsigned char)cursor[2]<<16) | ((unsigned char)cursor[3]<<24);
    cursor+=4;
    return value;
}
static char *block(unsigned bytes)
{
    char *start=cursor;
    if (bytes>(unsigned)(limit-cursor)) reject("truncated payload");
    cursor+=bytes;
    return start;
}
static char **vector(unsigned *count)
{
    char **items;
    unsigned n,bytes;
    *count=word();
    if (*count>4096) reject("vector limit");
    items=(char **)malloc((*count+1)*sizeof(char *));
    if (!items) reject("vector allocation");
    for(n=0;n<*count;++n){
        bytes=word();
        if (!bytes || bytes>16384) reject("string length");
        items[n]=block(bytes);
        if (items[n][bytes-1] || strlen(items[n])+1!=bytes) reject("string terminator");
    }
    items[*count]=NULL;
    return items;
}
void rc_fixture_prepare(int argc, char **argv)
{
    const char *path;
    FILE *input;
    long bytes;
    unsigned n,count,env_count;
    path=getenv("RC_DEPENDENCY_RESPONSE");
    if (!path) reject("response not configured");
    if (argc!=2 || strcmp(argv[1],"/help")) reject("unknown host case");
    input=fopen(path,"rb");
    if (!input || fseek(input,0,SEEK_END)) reject("response open");
    bytes=ftell(input);
    if (bytes<16 || bytes>1048576 || fseek(input,0,SEEK_SET)) reject("response length");
    response=(char *)malloc((unsigned)bytes);
    if (!response || fread(response,1,(unsigned)bytes,input)!=(unsigned)bytes) reject("response read");
    fclose(input);cursor=response;limit=response+bytes;
    if (memcmp(block(8),"RCRP\1\0\0\0",8)) reject("response version");
    rc_fixture_argv=vector(&count);rc_fixture_argc=(int)count;
    rc_fixture_environment=vector(&env_count);
    if (count!=2 || strcmp(rc_fixture_argv[0],"S:\\vc5.10\\rc\\original\\RC.EXE") ||
        strcmp(rc_fixture_argv[1],"/help")) reject("response argv");
    callback_count=word();
    if (callback_count!=4) reject("callback count");
    for(n=0;n<callback_count;++n){
        CallbackResponse *c=callbacks+n;
        c->window=word();c->code=word();c->bytes=word();c->result=(int)word();
        if (c->window || c->code || !c->bytes || c->bytes>16384 || c->result) reject("callback contract");
        c->before=block(c->bytes);c->after=block(c->bytes);
        if (c->before[c->bytes-1] || strlen(c->before)+1!=c->bytes) reject("callback input terminator");
    }
    export_result=(int)word();
    if (export_result || cursor!=limit) reject("response trailing bytes/result");
    /* All response bytes and input vectors are validated before any callback. */
}
int WINAPI RC(HWND window, unsigned flags, DiagnosticCallback diagnostic,
              ResourceCallback resource, int argc, char **argv)
{
    unsigned n;
    const char *log;
    FILE *file;
    if (!response || used++ || window || flags || diagnostic!=rc_diagnostic || resource || argc!=rc_fixture_argc || argv!=rc_fixture_argv)
        reject("export arguments");
    for(n=0;n<(unsigned)argc;++n)
        if (!argv[n] || strcmp(argv[n],n ? "/help" : "S:\\vc5.10\\rc\\original\\RC.EXE")) reject("export argv");
    if (argv[argc]) reject("unterminated export argv");
    log=getenv("RC_DEPENDENCY_LOG");
    file=log?fopen(log,"wb"):NULL;
    if (log&&!file) reject("log open");
    if (file) fprintf(file,"RC %u %u %u\n",(unsigned)window,flags,(unsigned)argc);
    for(n=0;n<callback_count;++n){
        CallbackResponse *c=callbacks+n;
        char *message=(char *)malloc(c->bytes);
        int result;
        if (!message) reject("callback allocation");
        memcpy(message,c->before,c->bytes);
        result=diagnostic((HWND)c->window,c->code,message);
        if (result!=c->result || memcmp(message,c->after,c->bytes)) reject("callback result/mutable buffer");
        if (file) fprintf(file,"callback %u %u %u %d\n",n,c->window,c->code,result);
        free(message);
    }
    if (file) { fprintf(file,"return %d\n",export_result);fclose(file); }
    return export_result;
}
BOOL WINAPI Handler(DWORD control)
{
    reject("uncaptured control-handler invocation");
    return FALSE;
}
