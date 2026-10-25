#include <recon-tool.h>
#include <recon-fixture.h>
#include <stdio.h>
#include <stdlib.h>
extern int cvpack_native_setup_resources(int,const char *,const char *);
extern int cvpack_main(int,char **,char **);
extern void cvpack_pdb_load(const char *);
/* Derived from0008 PPB SHA3ccc6001426549a7ef26fae78d5c4ed0218bfebf10766d240b6c9213a2c10a8f.
 * Only App-DLL binding[8,4616) changes; suffix SHAa369a622621819e37f9b3f39b5fb0de622ae22ff2d0ec95f8e711db417c66e11.
 * The dependency consumes the identical PDB/API requests, not the App DLL.
 * Response provenance and strict request/owner/stage checks remain unchanged. */
static int setup(void)
{
 const char *assets=recon_case_assets();char path[4096];int n;
 if(!assets)return 0;
 n=snprintf(path,sizeof(path),"%s/resources/resources.rmr",assets);
 if(n<0 || n>=sizeof(path) || !cvpack_native_setup_resources(0,"R:\\toolchains\\5.10\\bin\\CVPACK.EXE",path))return 0;
 n=snprintf(path,sizeof(path),"%s/dependencies/archive-pdb-derived.ppb",assets);
 if(n<0 || n>=sizeof(path))return 0;
 cvpack_pdb_load(path);return 1;
}
static int invoke(int argc,char **argv){return cvpack_main(argc,argv,(char **)0);}
static const uint32_t order[]={0x4620,0x7680,0x48c0,0x10a0,0x1f10,0x1010,0x1190,0x4a30,0x1310,0x1380,0x1470,0x1400,0x20a0,0x1070,0x2140,0x2480,0xaff0,0xfb90,0xff60,0xfb60,0x5d00,0x1910,0x2270,0x1980,0x22f0,0x1a80,0x5690,0x57b0,0x5760,0x5a10,0x5910,0x59a0,0x51e0,0x4c10,0x4b50,0x5130,0x5250,0xaa00,0x25d0,0x25f0,0x25a0,0x2ea0,0xd1f0,0xcf50,0xb1d0,0x4ee0,0xc100,0xc720,0xbca0,0xb930,0xb900,0xb9f0,0x7610,0xc760,0x5070,0x53c0,0x3080,0x30a0,0xa380,0xa730,0xdef0,0xa790,0xdf30,0xad00,0xaec0,0xaf80,0x3c20,0x3a90,0xccb0,0xcda0,0xce70,0xb210,0xe1a0,0xb490,0xff80,0x2c20,0x4360,0xcc10,0xe640,0xe820,0x8900,0xc080,0xb890,0xb3c0,0xac70,0xabc0,0xacb0,0xd430,0xdd60,0xf4a0,0xf400,0x5470,0xae00,0x5230,0xb140,0xdf60,0xf100,0xf0d0,0x69e0,0x63f0,0x100e0,0x68a0,0xd600,0xf960,0xe920,0x10010,0xede0,0xedc0,0x100c0,0x100d0,0xd9a0,0xf5f0,0x6460,0xd130,0x10240,0x1cc0,0x2360,0x11b0,0x1680,0x23d0,0x1000};
RECON_EXPORT const ReconCase case_descriptor={
 .abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="library-archive",.description="Genuine Zi static-library members, counted library directory and case-bound compiler PDB",
 .first_entry_order=order,.first_entry_count=sizeof(order)/sizeof(order[0]),
 .execution=RECON_IO,.terminal=RECON_EXIT,.setup=setup,.io=invoke
};
