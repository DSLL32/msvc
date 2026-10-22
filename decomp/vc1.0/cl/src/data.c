#include "cl.h"
char *frontend_defaults[] = {
    "-ef%e",
    "-il%t",
    "-f%f",
    "-W1",
    "-Zep8",
    "-G4s",
    "-Otb0",
    "-dos",
    "-Fo%b.obj",
    "-Fe%b.%X",
    "-pc\\:/",
    "-bLNOI",
    "-ML",
    "-coff",
    "-bCdebug:none",
    NULL
};
char *preprocessor_defaults[] = {
    "-D_MSC_VER=800",
    "-D_WIN32",
    NULL
};

/* Original command-line grammar, at 000266e0. */
extern int option_callback_19014(char **, int *);
extern int option_callback_190d0(char **, int *);
extern int option_callback_19198(char **, int *);
extern int option_callback_1920c(char **, int *);
extern int option_callback_19228(char **, int *);
OptionRule option_rules[] = {
    { "?", "D=bh", NULL, NULL, NULL },
    { "batch", NULL, NULL, NULL, NULL },
    { "^bL!*", "L=!*", NULL, NULL, NULL },
    { "^bC!*", "C=!*", NULL, NULL, NULL },
    { "^bS!*", "S=!*", NULL, NULL, NULL },
    { "Bd", "D=bv,1P2=*", NULL, NULL, NULL },
    { "Bk:#", "D=Bk,=bil:*", NULL, NULL, NULL },
    { "bzalign", "2=*", NULL, NULL, NULL },
    { "Bz", "D=bv,=*", NULL, NULL, NULL },
    { "B1!+", "D=bp:1*", NULL, NULL, NULL },
    { "Bx!+", "D=bp:P*", NULL, NULL, NULL },
    { "B2!+", "D=bp:2*", NULL, NULL, NULL },
    { "Bl!+", "D=bp:L*,=bp:C*", NULL, NULL, NULL },
    { "Brepro", "1P2LC=*", NULL, NULL, NULL },
    { "Bp1!+", "D=bx:1*", NULL, NULL, NULL },
    { "Bpx!+", "D=bx:P*", NULL, NULL, NULL },
    { "Bp2!+", "D=bx:2*", NULL, NULL, NULL },
    { "Bpl!+", "D=bx:C*", NULL, NULL, NULL },
    { "Bt", "D=*", NULL, NULL, NULL },
    { "c", "D=bd:LC", NULL, NULL, NULL },
    { "C", "1P=*", NULL, NULL, "=E,=EP,=P" },
    { "^coff", "D=*,D=bc:LC,=bd:L", NULL, "=omf", NULL },
    { "^dos", "2=*", NULL, NULL, NULL },
    { "D!@", NULL, NULL, NULL, option_callback_190d0 },
    { "d1!+", "1P=!*", NULL, NULL, NULL },
    { "d2!+", "2=!*", NULL, NULL, NULL },
    { "E", "1P=E,D=bE,=br,=bd:.,=ba:1,=ba:P", NULL, NULL, NULL },
    { "EP", "1P=E,=*,D=bE,=br,=bd:.,=ba:1,=ba:P", NULL, NULL, NULL },
    { "^ef!+", "1P2=*:*", NULL, NULL, NULL },
    { "FEEDBACK", "D=br,D1P2=ide", NULL, NULL, NULL },
    { "^f!+", "1P2=f:%f", NULL, NULL, NULL },
    { "FA:#", "D=bA:2 a,2=FA!a*", NULL, "=FA:?,=bA:?", NULL },
    { "Fa:#", "2=Fa:(*|%b),D=bA:2 a,-bo:Fa (*|%b)", NULL, "=Fa:?,=bA:?,=bo:?", NULL },
    { "Fc:#", "2=Fa:(*|%b),D=bA:2 asc,-bo:Fc (*|%b)", NULL, "=FA:?,=Fa:?,=bA:?,=bo:?", NULL },
    { "Fl:#", "2=Fa:(*|%b),D=bA:2 ac,-bo:Fl (*|%b)", NULL, "=FA:?,=Fa:?,=bA:?,=bo:?", NULL },
    { "F!*x", "L=STACK\\:!*,C=stack\\:!*", NULL, "=STACK\\::?,=stack\\::?", NULL },
    { "Fd:#", "1P=*!(*|msvc)<pdb", NULL, "=Fd:?", NULL },
    { "Fe:#", "D=Fe:(*|%b)<%X,C=out\\:!(*|%b)<%X", NULL, "=Fe:?,=out\\::?", NULL },
    { "FI!*", "1P=*:*", NULL, NULL, NULL },
    { "Fm:#", "D=bm,=*:*,L=MAP,C=map\\:!%m", NULL, "=Fm:?,=map\\::?", NULL },
    { "Fo:#", "D1P2=*!(*|%b)<obj,D-bo:Fo (*|%b)", NULL, "=*:?,=bo:?", NULL },
    { "Fp:#", "1P=*!(*|msvc)<pch", NULL, "=Fp:?", NULL },
    { "Fr:#", "1P=*:(*|%b)<sbr,D-bo:Fr (*|%b)", NULL, "=Fr:?,=FR:?,=bo:?", NULL },
    { "FR:#", "1P=*:(*|%b)<sbr,D-bo:FR (*|%b)", NULL, "=Fr:?,=FR:?,=bo:?", NULL },
    { "FP:?#", NULL, NULL, NULL, NULL },
    { "Fs:?#", NULL, NULL, NULL, NULL },
    { "G3", "1P-D!_M_IX86=300,1P2=*", NULL, "=G4,=G5,=D!_M_IX86=?", NULL },
    { "G4", "1P-D!_M_IX86=400,1P2=*", NULL, "=G3,=G5,=D!_M_IX86=?", NULL },
    { "G5", "1P-D!_M_IX86=500,=G4,2=*", NULL, "=G3,=G4,=D!_M_IX86=?", NULL },
    { "Gd", "1P=*", "=Gr,=Gz", NULL, NULL },
    { "Ge", "D=*,2=Gs!0", NULL, "=Gs:?", NULL },
    { "Gf", "1P=*", NULL, NULL, NULL },
    { "Gh", "2=*", NULL, NULL, NULL },
    { "Gr", "1P=*", "=Gd,=Gz", NULL, NULL },
    { "Gs:$x[0,10-4294967295,4096]", "1P=*,2=*!(*|4096)", NULL, "=Ge,=Gs:?", NULL },
    { "Gy", "2=*", "=cbstring", NULL, NULL },
    { "Gz", "1P=*", "=Gd,=Gr", NULL, NULL },
    { "GE:?#", NULL, NULL, NULL, NULL },
    { "Gp:?#", NULL, NULL, NULL, NULL },
    { "Gt:?#", NULL, NULL, NULL, NULL },
    { "help", "D=bh", NULL, NULL, NULL },
    { "HELP", "D=bh", NULL, NULL, NULL },
    { "H!*x", "2=*:*", NULL, "=H:?", NULL },
    { "I!+", "1P=*:*", NULL, NULL, NULL },
    { "^il!+", "1P2=*:*", NULL, NULL, NULL },
    { "J", "1P=*,-D!_CHAR_UNSIGNED", NULL, NULL, NULL },
    { "LD", "D=*,C=dll,=implib\\:!%x.lib,1P-D!_MT,2-MT", NULL, "-ML,-MD", NULL },
    { "Bbb2", "2=*", NULL, NULL, NULL },
    { "Bbb1", "2=*", NULL, NULL, NULL },
    { "link:@", NULL, NULL, NULL, option_callback_19014 },
    { "MD", "1P-D!_MT,-D!_DLL,2=*", NULL, "=ML,=MT,=:defaultlib\\:?", NULL },
    { "ML", "2=*", NULL, "=MD,=MT,=D!_MT,=D!_DLL", NULL },
    { "MT", "1P-D!_MT,2=*", NULL, "=MD,=ML,=D!_DLL", NULL },
    { "nl!*x", "2=H:*", NULL, NULL, NULL },
    { "nologo", "L=NOL,C=nologo", NULL, NULL, NULL },
    { "o!+", "D=Fe:(*|%b)<%X,C=out\\:!(*|%b)<%X", NULL, NULL, NULL },
    { "O", "1P=Ot", NULL, "=Os", NULL },
    { "O1", "D=*,1P-Gs,-Gf,-Og,-Os,-Ob!1,2-Gs!4096,-Gy", NULL, "=Od,=Ox,=O2,-Ge,-Gs:?,-Oi,-Ot,-Oy,-Ob:?", NULL },
    { "O2", "D=*,1P-Gs,-Gf,-Og,-Oi,-Ot,-Oy,-Ob!1,2-Gs!4096,-Gy", NULL, "=Od,=O1,-Ge,-Gs:?,-Os,-Oy-,-Ob:?", NULL },
    { "Oa", "1P=*", NULL, "=Oa-", NULL },
    { "Oa-", "D=*", NULL, "=Oa", NULL },
    { "Ob!$d[0-2]", "1P=*!*", NULL, "=Ob:?", NULL },
    { "Od", "D=*,1P-Ob!0,-Ot", NULL, "=Ox,=O1,=O2,=Og,-Gf,-Gy,-Oi,-Os,-Oy,-Ob:?", NULL },
    { "Og", "1P=*", NULL, "=Og-", NULL },
    { "Og-", "D=*", NULL, "=Og", NULL },
    { "Oi", "1P=*", NULL, "=Oi-", NULL },
    { "Oi-", "D=*", NULL, "=Oi", NULL },
    { "Op", "1P=*", NULL, "=Op-", NULL },
    { "Op-", "D=*", NULL, "=Op", NULL },
    { "Os", "1P=*", NULL, "=Ot", NULL },
    { "Ot", "1P=*", NULL, "=Os", NULL },
    { "Ow", "1P=*", NULL, "=Ow-", NULL },
    { "Ow-", "D=*", NULL, "=Ow", NULL },
    { "Ox", "D=*,1P-Gs,-Og,-Oi,-Ot,-Oy,-Ob!1,2-Gs!4096", NULL, "=Od,=O1,-Ge,-Gs:?,-Os,-Oy-,-Ob:?", NULL },
    { "Oy", "1P=*", NULL, "=Oy-", NULL },
    { "Oy-", "D=*", NULL, "=Oy", NULL },
    { "OV:?#", NULL, NULL, NULL, NULL },
    { "P", "D=br,=bd:.,=ba:1,=ba:P,1P=E,=Fi:%b.i", NULL, NULL, NULL },
    { "^pc!+", "1P=*:*", NULL, NULL, NULL },
    { "S", "D=bd:LC,2=Fa:%b.asm", "=Fo:?", NULL, NULL },
    { "Tc!@", NULL, NULL, NULL, option_callback_1920c },
    { "Tp!@", NULL, NULL, NULL, option_callback_19228 },
    { "To!@", NULL, NULL, NULL, option_callback_19198 },
    { "U!*", "D=bU:*", NULL, NULL, NULL },
    { "u", "D=bu", NULL, NULL, NULL },
    { "V!*", "1P=*:*", NULL, NULL, NULL },
    { "vd!$d[0,1]", "P=*!*", "=vd:?", NULL, NULL },
    { "vmb", "P=*", "=vmg", NULL, NULL },
    { "vmg", "P=*", "=vmb", NULL, NULL },
    { "vmm", "P=*", "=vms,=vmv", NULL, NULL },
    { "vms", "P=*", "=vmm,=vmv", NULL, NULL },
    { "vmv", "P=*", "=vmm,=vms", NULL, NULL },
    { "WX", "1P2=*", NULL, NULL, NULL },
    { "W!*d[0-4,1]", "1P2=*:*", NULL, "=W:?", NULL },
    { "w", "1P2=W:0", NULL, "=W:?", NULL },
    { "^xenix", NULL, NULL, "=dos", NULL },
    { "X", "D=*", NULL, NULL, NULL },
    { "Yc:#", "1P=*!*", NULL, "=Yc:?,=YX:?", NULL },
    { "Yd", "1P=*", NULL, NULL, NULL },
    { "Yu:#", "1P=*!*", NULL, "=Yu:?,=YX:?", NULL },
    { "YX:#", "1P=*!*", NULL, "=Yc:?,=Yu:?", NULL },
    { "Yl:#", "1P=*!*", NULL, NULL, NULL },
    { "Za", "1P2=*,1P-Op", "=Ze", "-Op-", NULL },
    { "Ze", "1P=*", "=Za", NULL, NULL },
    { "Zg", "D=bd:.,=ba:1,=ba:P,=br,1P=*", NULL, NULL, NULL },
    { "Zd", "2=*,L=LIN,C=!debugtype\\:cv,=!debug\\:notmapped\\,full", NULL, "=Zi,=Z7,=:debugtype\\:?,=:debug\\:?", NULL },
    { "Z7", "1P=*,2=Zi,L=COD,C=!debugtype\\:cv,=!debug\\:notmapped\\,full", NULL, "=Zi,=Zd,=:debugtype\\:?,=:debug\\:?", NULL },
    { "Zi", "1P2=*,L=COD,C=!debugtype\\:cv,=!debug\\:notmapped\\,full", NULL, "=Z7,=Zd,=:debugtype\\:?,=:debug\\:?", NULL },
    { "Zn", "1P=*", NULL, NULL, "=FR:?,=Fr:?" },
    { "Zl", "2=*", NULL, "=:defaultlib\\:?", NULL },
    { "Zp:$d[1,2,4,16,8]", "1P=*!(*|1)", NULL, "=*:?", NULL },
    { "Zs", "D=bd:.,=ba:1,=ba:P", NULL, NULL, NULL },
    { "c6386", "2=*", NULL, NULL, NULL },
    { "cbstring", "2=*", "=Gy", NULL, NULL },
    { "cvnew", "L=COD,C=!debugtype\\:cvnew,=!debug\\:notmapped\\,full", NULL, NULL, NULL },
    { "debug_thread", "2=*", NULL, NULL, NULL },
    { "force_thread", "1P=thread,2=*", NULL, NULL, NULL },
    { "noxseh", "2=*", NULL, NULL, NULL },
    { "^omf", "D2=*,D=bc:CL,=bd:C", NULL, "=coff", NULL },
    { "thread", "1P=thread,2=force_thread,=debug_thread", NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

/* Driver switch actions, original 00026080. */
char *object_file, *executable_file, *map_file;
int make_dll, coff_format = 1;
extern void driver_switch_173ac(Item *);
extern void driver_switch_17368(Item *);
extern void driver_switch_172e4(Item *);
extern void driver_switch_17274(Item *);
int global_26030 = 0;
extern void driver_switch_171fc(Item *);
extern void driver_switch_1719c(Item *);
extern void driver_switch_1713c(Item *);
int global_26078 = 1;
extern void remove_define(Item *);
extern void driver_switch_170a0(Item *);
int global_26040 = 0;
int global_2602c = 0;
int global_2607c = 0;
int global_26034 = 0;
int global_22400 = 0;
extern void driver_switch_17010(Item *);
int global_2601c = 1;
DriverSwitch driver_switches[] = {
    { "Fo", 3, &object_file },
    { "Fe", 3, &executable_file },
    { "bA", 4, driver_switch_173ac },
    { "ba", 4, driver_switch_17368 },
    { "bc", 4, driver_switch_172e4 },
    { "bd", 4, driver_switch_17274 },
    { "bE", 1, &preprocess },
    { "bh", 1, &help_mode },
    { "bil", 3, &driver_storage.temporary },
    { "bm", 1, &global_26030 },
    { "bo", 4, driver_switch_171fc },
    { "bx", 4, driver_switch_1719c },
    { "bp", 4, driver_switch_1713c },
    { "br", 2, &global_26078 },
    { "bU", 4, remove_define },
    { "bu", 4, driver_switch_170a0 },
    { "bv", 1, &global_26040 },
    { "Bk", 1, &global_2602c },
    { "Bt", 1, &global_2607c },
    { "Bz", 1, &global_26034 },
    { "coff", 1, &coff_format },
    { "Fm", 3, &map_file },
    { "Ge", 5, NULL },
    { "ide", 1, &global_22400 },
    { "LD", 4, driver_switch_17010 },
    { "O1", 5, NULL },
    { "O2", 5, NULL },
    { "Oa-", 5, NULL },
    { "Od", 5, NULL },
    { "Og-", 5, NULL },
    { "Oi-", 5, NULL },
    { "Op-", 5, NULL },
    { "Ow-", 5, NULL },
    { "Ox", 5, NULL },
    { "Oy-", 5, NULL },
    { "omf", 2, &coff_format },
    { "X", 2, &global_2601c },
    { NULL, 0, NULL }
};

/* Compiler and linker pass profiles, original 00027300. */
static PassRecord passes_27210[] = {
    { "c13232.exe", "c1.err", "MSC_CMD_FLAGS=", 1, 0x2d31, NULL },
    { "c23232.exe", "c23.err", "MSC_CMD_FLAGS=", 1, 0x2d32, NULL },
    { NULL, NULL, NULL, 0, 0, NULL }
};
static PassRecord passes_27258[] = {
    { "c1xx3232.exe", "c1.err", "MSC_CMD_FLAGS=", 1, 0x2d50, NULL },
    { "c23232.exe", "c23.err", "MSC_CMD_FLAGS=", 1, 0x2d32, NULL },
    { NULL, NULL, NULL, 0, 0, NULL }
};
static PassRecord passes_272a0[] = {
    { "link386.exe", NULL, NULL, 0, 0x2f4c, NULL },
    { "link.exe", NULL, NULL, 1, 0x2d43, NULL },
    { NULL, NULL, NULL, 0, 0, NULL }
};
static PassRecord passes_272e8[] = { { NULL, NULL, NULL, 0, 0, NULL } };
Profile profiles[] = {
    { "c", 0, passes_27210 },
    { "cpp", 0, passes_27258 },
    { "cxx", 0, passes_27258 },
    { "obj", 1, passes_272a0 },
    { "lib", 1, passes_272a0 },
    { "def", 1, passes_272a0 },
    { "exp", 1, passes_272a0 },
    { NULL, 1, passes_272a0 },
    { NULL, 2, passes_272e8 },
    { NULL, -1, NULL }
};
