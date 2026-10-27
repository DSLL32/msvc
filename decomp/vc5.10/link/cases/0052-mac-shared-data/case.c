#include <recon-tool.h>
#include <recon-fixture.h>
#include <recon-process.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "../../harness/startup.h"
#include <recon.h>
/* The native canonical R: self identity and the original broker's private
 * staged T: identity denote the same-program role, not equal pathname bytes. */
#define CHILD_DENY() do { dprintf(2,"LINK SELF policy rejected at line %d\n",__LINE__); return 0; } while(0)
static int librarian_child(const ReconProcessRequest *request,ReconProcessResponse *response)
{
    static const char program[]="R:\\toolchains\\5.10\\bin\\LINK.EXE";
    static const char quoted_program[]="\"R:\\toolchains\\5.10\\bin\\LINK.EXE\"";
    static const char response_prefix[]="@T:\\vc5.10\\link\\0052-mac-shared-data\\";
    static const char expected[]="/def:\n/name:\"shared.dll\"\n/out:\"shared.lib\"\n/export:Shared,DATA\n/export:Counter,DATA\n/export:Message,DATA\n/export:Reserve,DATA\n/nodefaultlib\n\"shared.obj\"\n/machine:MPPC\n/nologo\n";
    uint32_t i,j=0,insertions=0;
    const char *name;
    char bytes[sizeof(expected)];
    size_t count;
    FILE *file;
    int okay;
    if(request->struct_size!=sizeof(*request) || request->protocol!=RECON_PROCESS_RSP1 ||
       request->index || request->mode || request->argc!=3 ||
       strcmp(request->file,program) || strcmp(request->argv[0],quoted_program) ||
       strcmp(request->argv[1],"/lib") || strcmp(request->cwd,request->workspace_cwd) ||
       !request->target_os_environment_count ||
       request->environment_count!=request->target_os_environment_count+1)CHILD_DENY();
    for(i=0;i<request->target_os_environment_count;++i)
        if(!strncmp(request->target_os_environment[i],"LINK_REPRO_NAME=",16))CHILD_DENY();
    for(i=0;i<request->environment_count;++i) {
        if(!strcmp(request->environment[i],"LINK_REPRO_NAME=\\deflib.rsp"))++insertions;
        else {
            if(j>=request->target_os_environment_count ||
               strcmp(request->environment[i],request->target_os_environment[j++]))CHILD_DENY();
        }
    }
    if(insertions!=1 || j!=request->target_os_environment_count ||
       strncmp(request->argv[2],response_prefix,sizeof(response_prefix)-1))CHILD_DENY();
    name=request->argv[2]+sizeof(response_prefix)-1;
    if(strlen(name)!=11 || strncmp(name,"lah",3) || strcmp(name+7,".tmp"))CHILD_DENY();
    for(i=3;i<7;++i)if(!isxdigit((unsigned char)name[i]))CHILD_DENY();
    file=fopen(name,"rb");
    if(!file)CHILD_DENY();
    count=fread(bytes,1,sizeof(bytes),file);
    okay=count==sizeof(expected)-1 && !ferror(file) &&
        !memcmp(bytes,expected,sizeof(expected)-1);
    if(fclose(file))okay=0;
    if(!okay)CHILD_DENY();
    response->action=RECON_PROCESS_SELF;
    return 1;
}
#undef CHILD_DENY
RECON_EXPORT const ReconProcessPolicy case_process_policy={
    RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,librarian_child
};
static const uint32_t order[]={
 0x24e60,0x24e70,0x24ea0,0x31d70,0x31d80,0x31db0,0x31de0,0x31df0,0x31e20,0x44e60,
 0x44e70,0x44ea0,0xde90,0x15230,0xb560,0x152f0,0x4fe70,0xb660,0x4f560,0x48cb0,
 0x44b10,0x5caa0,0x5cb20,0x5c780,0x5c750,0x5cca0,0x5cc00,0x5c650,0x4fed0,0x44f20,
 0x4fd40,0x337d0,0x62220,0x30c70,0x30c30,0x4f5e0,0x4f640,0x4fe50,0x45130,0xd0c0,
 0x4fe30,0x16ae0,0x61b80,0x48630,0x42f50,0xe3b0,0x4fd80,0xe360,0x4f5b0,0x62eb0,
 0x3e170,0xef90,0xf130,0x5e820,0xfbd0,0xfc00,0xfd90,0xe620,0xe700,0xe670,
 0xfc40,0xfc70,0x4b860,0x61840,0xb7f0,0xb870,0xb960,0xb8f0,0xc7e0,0xc880,
 0xccf0,0xedd0,0xf650,0xfaf0,0xfb20,0x49a50,0x498d0,0x5cd40,0xcab0,0x4e620,
 0xc080,0xc030,0xca10,0x4e5c0,0x5d890,0x497e0,0x10530,0x103f0,0x5cec0,0x49630,
 0x1f9b0,0x55660,0x62620,0x30c20,0x4e370,0x4e2e0,0x4fce0,0x558a0,0xe850,0xe5e0,
 0x5cd00,0x5d8c0,0x610c0,0xea00,0xf7a0,0xfb60,0xfb90,0x31e50,0x5fc90,0x608e0,
 0x60f30,0xcfc0,0x60a10,0x5f430,0xe420,0x61010,0x61d80,0x61dd0,0x30e20,0x61f50,
 0x61d00,0x5ea70,0x61680,0x5eef0,0x5e910,0x616d0,0x5e400,0x622e0,0x5f220,0x41610,
 0x60bb0,0x61000,0xbd10,0xbe20,0x49af0,0x49b90,0x16d60,0x4b590,0x49d10,0x1f010,
 0x4b250,0x1f910,0x4b2a0,0x5ce00,0x4bd30,0xcb40,0x4f4e0,0x4f450,0x4fea0,0xcbd0,
 0x3ce20,0x5c980,0x5c8c0,0x5c2c0,0x3a5d0,0x10580,0x1e6d0,0x3d5c0,0x1e1d0,0x188a0,
 0x1fb80,0x30a20,0x303d0,0x306f0,0x61ee0,0x30430,0x62dc0,0x62540,0x623c0,0x30c10,
 0x31220,0x31130,0x622c0,0x31160,0x31200,0x62580,0x62800,0x62680,0x5dba0,0x62030,
 0x62050,0x4f530,0x3b8e0,0x18fa0,0x1e040,0x19180,0x302b0,0x30020,0x2ffa0,0x2ff30,
 0x1dfa0,0x3d710,0x19220,0x5d860,0x5d830,0x3b990,0x62440,0x62480,0x3ba90,0x19660,
 0x1b230,0x4f490,0x1aba0,0x1adf0,0x1af00,0x611e0,0x61340,0x50e90,0x1b080,0x613a0,
 0x60bc0,0x1dd40,0x1dd60,0x1d610,0x1dec0,0x3bbf0,0xbea0,0x10730,0x106a0,0x44eb0,
 0x31e30,0x31dc0,0x24eb0,0x50e60,0x62070,0x1f950,0x514f0,0x4b7d0,0x4b3b0,0x33d80,
 0x34170,0x40560,0x404e0,0x51940,0x55950,0x61f20,0x517b0,0x51790,0x51890,0x40930,
 0x40b50,0x3e520,0x519f0,0x51b70,
};
static int setup(void)
{
 const char *assets=recon_case_assets();
 char resources[4096];
 int length;
 if(!assets)return 0;
 length=snprintf(resources,sizeof(resources),"%s/resources/messages-repro.rmr",assets);
 return length>=0 && length<sizeof(resources) && recon_link_setup_resources(resources);
}
RECON_EXPORT const ReconCase case_descriptor={
 RECON_ABI_VERSION,sizeof(ReconCase),"mac-shared-data",
 "Link transparent data-only MPPC COFF sections and four shared-data exports through the real Mac backend; no target execution or dependency replay.",
 order,sizeof(order)/sizeof(*order),NULL,0,RECON_IO,RECON_FRONTIER,0x51b70,
 setup,recon_link_invoke,NULL,NULL,0
};
