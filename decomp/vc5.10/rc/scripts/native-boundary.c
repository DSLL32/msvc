/* Native-only, in-process response replay. No original or PE helper is run. */
#include "rc.h"
#include "native-fixture.h"
#include <recon.h>

typedef struct CallbackResponse {
    unsigned window, code, bytes;
    int result;
    char *before, *after;
} CallbackResponse;
typedef struct FileResponse {
    char *path, *data;
    unsigned bytes;
} FileResponse;
static char *response, *cursor, *limit, *logical_cwd;
static CallbackResponse callbacks[16];
static FileResponse inputs[64], artifacts[16];
static unsigned callback_count, input_count, artifact_count, used, version;
static unsigned environment_count, argument_count;
static unsigned post_allocation[64], post_offset[64];
static char **expected_argv, **argument_storage, **expected_environment;
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
static char *string(void)
{
    unsigned bytes=word();
    char *value;
    if (!bytes || bytes>16384) reject("string length");
    value=block(bytes);
    if (value[bytes-1] || strlen(value)+1!=bytes) reject("string terminator");
    return value;
}
static char **vector(unsigned *count)
{
    char **items;
    unsigned n;
    *count=word();
    if (*count>4096) reject("vector limit");
    items=(char **)malloc((*count+1)*sizeof(char *));
    if (!items) reject("vector allocation");
    for(n=0;n<*count;++n) items[n]=string();
    items[*count]=NULL;
    return items;
}
static char **copy_vector(char **original, unsigned count)
{
    char **items;
    unsigned n, bytes;
    items=(char **)malloc((count+1)*sizeof(char *));
    if (!items) reject("vector copy allocation");
    for(n=0;n<count;++n){
        bytes=strlen(original[n])+1;
        items[n]=(char *)malloc(bytes);
        if (!items[n]) reject("string copy allocation");
        memcpy(items[n],original[n],bytes);
    }
    items[count]=NULL;
    return items;
}
static void check_vector(char **actual, char **expected, unsigned count)
{
    unsigned n;
    for(n=0;n<count;++n)
        if (!actual[n] || strcmp(actual[n],expected[n])) reject("export vector content");
    if (actual[count]) reject("export vector terminator");
}
static void file_records(FileResponse *records, unsigned *count, unsigned maximum)
{
    unsigned n;
    *count=word();
    if (*count>maximum) reject("file count");
    for(n=0;n<*count;++n){
        records[n].path=string();
        if (!*records[n].path || strchr(records[n].path,':') ||
            records[n].path[0]=='/' || records[n].path[0]=='\\' ||
            strstr(records[n].path,"..")) reject("nonrelative case path");
        records[n].bytes=word();
        records[n].data=block(records[n].bytes);
    }
}
static void check_input(FileResponse *input)
{
    FILE *file=fopen(input->path,"rb");
    char buffer[4096];
    unsigned position=0, bytes;
    if (!file) reject("case input open");
    while(position<input->bytes){
        bytes=input->bytes-position;
        if (bytes>sizeof(buffer)) bytes=sizeof(buffer);
        if (fread(buffer,1,bytes,file)!=bytes ||
            memcmp(buffer,input->data+position,bytes)) reject("case input bytes");
        position+=bytes;
    }
    if (fgetc(file)!=EOF) reject("case input length");
    fclose(file);
}
void rc_fixture_prepare(int argc, char **argv)
{
    const char *path, *case_name;
    FILE *input;
    long bytes;
    unsigned n;
    path=getenv("RC_DEPENDENCY_RESPONSE");
    if (!path) reject("response not configured");
    input=fopen(path,"rb");
    if (!input || fseek(input,0,SEEK_END)) reject("response open");
    bytes=ftell(input);
    if (bytes<16 || bytes>16777216 || fseek(input,0,SEEK_SET)) reject("response length");
    response=(char *)malloc((unsigned)bytes);
    if (!response || fread(response,1,(unsigned)bytes,input)!=(unsigned)bytes) reject("response read");
    fclose(input);cursor=response;limit=response+bytes;
    if (memcmp(block(4),"RCRP",4)) reject("response magic");
    version=word();
    if (version!=1 && version!=2) reject("response version");
    if (version==2){
        case_name=string();
        if (strcmp(case_name,"resources-rich")) reject("unknown response case");
    }
    expected_argv=vector(&argument_count);
    expected_environment=vector(&environment_count);
    rc_fixture_argc=(int)argument_count;
    if (version==1){
        if (argument_count!=2 || strcmp(expected_argv[0],"S:\\vc5.10\\rc\\original\\RC.EXE") ||
            strcmp(expected_argv[1],"/help")) reject("response argv");
    } else {
        if (argument_count!=6 || strcmp(expected_argv[1],"/r") ||
            strcmp(expected_argv[2],"/dCASE_FROM_COMMAND=7") ||
            strcmp(expected_argv[3],"/iinclude") ||
            strcmp(expected_argv[4],"/foresources.res") ||
            strcmp(expected_argv[5],"resources.rc")) reject("response argv");
        logical_cwd=string();
        path=getenv("RC_DEPENDENCY_CWD");
        if (!path || strcmp(path,logical_cwd)) reject("logical dependency cwd");
        file_records(inputs,&input_count,64);
        if (!input_count) reject("missing case inputs");
        for(n=0;n<argument_count;++n){
            post_allocation[n]=word();post_offset[n]=word();
            if (post_allocation[n]>=argument_count ||
                post_offset[n]>strlen(expected_argv[post_allocation[n]])) reject("post argv alias");
        }
    }
    if (argc!=(int)argument_count) reject("unknown host argument count");
    for(n=1;n<argument_count;++n)
        if (strcmp(argv[n],expected_argv[n])) reject("unknown host case");
    callback_count=word();
    if ((version==1 && callback_count!=4) || (version==2 && callback_count)) reject("callback count");
    for(n=0;n<callback_count;++n){
        CallbackResponse *c=callbacks+n;
        c->window=word();c->code=word();c->bytes=word();c->result=(int)word();
        if (c->window || c->code || !c->bytes || c->bytes>16384 || c->result) reject("callback contract");
        c->before=block(c->bytes);c->after=block(c->bytes);
        if (c->before[c->bytes-1] || strlen(c->before)+1!=c->bytes) reject("callback input terminator");
    }
    if (version==2){
        file_records(artifacts,&artifact_count,16);
        if (artifact_count!=1 || strcmp(artifacts[0].path,"resources.res") ||
            artifacts[0].bytes!=3672) reject("rich artifact contract");
    }
    export_result=(int)word();
    if (export_result || cursor!=limit) reject("response trailing bytes/result");
    /* Mutable target vectors do not expose the frozen expected strings. */
    argument_storage=copy_vector(expected_argv,argument_count);
    rc_fixture_argv=(char **)malloc((argument_count+1)*sizeof(char *));
    if (!rc_fixture_argv) reject("argv allocation");
    memcpy(rc_fixture_argv,argument_storage,(argument_count+1)*sizeof(char *));
    rc_fixture_environment=copy_vector(expected_environment,environment_count);
}
int WINAPI RC(HWND window, unsigned flags, DiagnosticCallback diagnostic,
              ResourceCallback resource, int argc, char **argv)
{
    unsigned n;
    const char *log;
    FILE *file, *output;
    if (!response || used++ || window || flags || diagnostic!=rc_diagnostic || resource || argc!=rc_fixture_argc || argv!=rc_fixture_argv)
        reject("export arguments");
    check_vector(argv,expected_argv,argument_count);
    check_vector(rc_fixture_environment,expected_environment,environment_count);
    for(n=0;n<input_count;++n) check_input(inputs+n);
    for(n=0;n<artifact_count;++n){
        output=fopen(artifacts[n].path,"rb");
        if (output) { fclose(output);reject("unexpected existing artifact"); }
    }
    log=getenv("RC_DEPENDENCY_LOG");
    file=log?fopen(log,"wb"):NULL;
    if (log&&!file) reject("log open");
    if (file) fprintf(file,"RC %u %u %u\n",(unsigned)window,flags,(unsigned)argc);
    for(n=0;n<input_count;++n)
        if (file) fprintf(file,"input %s %u\n",inputs[n].path,inputs[n].bytes);
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
    if (version==2){
        for(n=0;n<argument_count;++n){
            argv[n]=argument_storage[post_allocation[n]]+post_offset[n];
            if (file) fprintf(file,"argv-after %u %u %u\n",n,post_allocation[n],post_offset[n]);
        }
    }
    for(n=0;n<artifact_count;++n){
        output=fopen(artifacts[n].path,"wb");
        if (!output || fwrite(artifacts[n].data,1,artifacts[n].bytes,output)!=artifacts[n].bytes ||
            fclose(output)) reject("artifact write");
        if (file) fprintf(file,"artifact %s %u\n",artifacts[n].path,artifacts[n].bytes);
    }
    if (file) { fprintf(file,"return %d\n",export_result);fclose(file); }
    return export_result;
}
BOOL WINAPI Handler(DWORD control)
{
    reject("uncaptured control-handler invocation");
    return FALSE;
}
