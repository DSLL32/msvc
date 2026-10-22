/* Command-line option grammar and defaults, reached by parse_options. */
#include "options.h"

CompilerOptions options = {
    0, /* option_nologo */
    0, /* option_FAT */
    "/", /* path_characters */
    0, /* option_pf */
    0, /* option_C */
    0, /* defines */
    0, /* option_E */
    0, /* include_directories */
    0, /* forced_includes */
    0, /* option_EP */
    0, /* option_Fi */
    0, /* source_file */
    0, /* option_J */
    0, /* japanese_identifiers */
    175, /* option_Bu */
    0, /* option_Brepro */
    0, /* option_ide */
    0, /* option_MT */
    0, /* option_MD */
    0, /* option_M */
    0, /* option_WX */
    1, /* option_W */
    -1, /* option_Zp */
    0, /* option_Oi */
    0, /* option_Ol */
    0, /* option_Oq */
    1, /* option_Gn */
    "c1.err", /* error_file */
    "", /* intermediate_prefix */
    0, /* option_xc */
    0, /* option_m */
    0, /* option_V */
    1, /* option_Gs */
    0, /* option_Gc */
    0, /* option_Gr */
    0, /* option_Gd */
    0, /* option_Gz */
    0, /* option_NV */
    0, /* option_Fr */
    0, /* option_FR */
    0, /* option_A */
    0, /* option_Q */
    "msvc.pdb", /* database_file */
    0, /* option_Fo */
    0, /* option_Fs */
    0, /* option_PLMN */
    0, /* option_PLMF */
    0, /* option_GA */
    0, /* option_GD */
    0, /* option_Gw */
    0, /* option_Gq */
    0, /* option_GW */
    0, /* option_GEf */
    0, /* option_Gf */
    0, /* option_G1 */
    0, /* option_G2 */
    0, /* option_G3 */
    0, /* option_G4 */
    0, /* option_qc */
    0, /* option_Oa */
    0, /* option_Ow */
    1, /* option_Os */
    0, /* option_Oc */
    0, /* option_Og */
    0, /* option_Oe */
    0, /* option_Op */
    0, /* option_Oz */
    0, /* option_Or */
    0, /* option_Oo */
    0, /* option_Ov */
    0, /* option_Of */
    0, /* option_Oy */
    0, /* option_Ob */
    1, /* option_NoComdat */
    0, /* pch_use_file */
    0, /* pch_use */
    0, /* pch_create_file */
    0, /* pch_create */
    0, /* option_Yd */
    0, /* option_Fp */
    0, /* pch_auto_file */
    0, /* pch_auto */
    0, /* pch_link_name */
    0, /* unrecognized_argument */
    0, /* z_a */
    0, /* z_E */
    0, /* debug_info */
    0, /* debug_old_format */
    0, /* z_n */
    0, /* z_f */
    0, /* prototypes_only */
    0, /* z_I */
    0, /* z_r */
    1, /* line_records */
};

static ZRule z_rules[] = {
    { 'a', 5, &options.z_a },
    { 'e', 1, &options.z_a },
    { 'E', 1, &options.z_E },
    { 'i', 1, &options.debug_info },
    { '7', 1, &options.debug_old_format },
    { 'n', 1, &options.z_n },
    { 'f', 1, &options.z_f },
    { 'g', 1, &options.prototypes_only },
    { 'p', 1, &options.option_Zp },
    { 'I', 1, &options.z_I },
    { 'r', 1, &options.z_r },
    { 0, 0, 0 }
};

OptionRule option_rules[] = {
    { "-nologo", &options.option_nologo, 1, 0x1, 0 },
    { "-FAT", &options.option_FAT, 1, 0x1, 0 },
    { "-pc#", &options.path_characters, 1, 0x22, 0 },
    { "-pf", &options.option_pf, 1, 0x1, 0 },
    { "-C", &options.option_C, 1, 0x1, 0 },
    { "-D#", &options.defines, 1, 0x26, 0 },
    { "-E", &options.option_E, 1, 0x1, 0 },
    { "-I#", &options.include_directories, 1, 0x26, 0 },
    { "-FI#", &options.forced_includes, 1, 0x26, 0 },
    { "-EP", &options.option_EP, 1, 0x1, 0 },
    { "-Fi#", &options.option_Fi, 1, 0x22, 0 },
    { "-f", &options.source_file, 1, 0x22, 0 },
    { "-J", &options.option_J, 1, 0x1, 0 },
    { "-Jd", &options.japanese_identifiers, 1, 0x1, 0 },
    { "-Bd", &skip_fault_protection, 1, 0x1, 0 },
    { "-Bu#", &options.option_Bu, 1, 0x24, 0 },
    { "-Brepro", &options.option_Brepro, 1, 0x1, 0 },
    { "-ide", &options.option_ide, 1, 0x1, 0 },
    { "-MT", &options.option_MT, 1, 0x1, 0 },
    { "-MD", &options.option_MD, 1, 0x1, 0 },
    { "-M#", &options.option_M, 1, 0x24, 0 },
    { "-WX", &options.option_WX, 1, 0x1, 0 },
    { "-W#", &options.option_W, 1, 0x24, 0 },
    { "-Zp", &options.option_Zp, 1, 0x1, 0 },
    { "-Zp#", &options.option_Zp, 1, 0x24, 0 },
    { "-Z*", z_rules, 1, 0x23, 0 },
    { "-Oi", &options.option_Oi, 1, 0x1, 0 },
    { "-Ol", &options.option_Ol, 1, 0x1, 0 },
    { "-Oq", &options.option_Oq, 1, 0x1, 0 },
    { "-Gn", &options.option_Gn, 1, 0x5, 0 },
    { "-ef#", &options.error_file, 1, 0x22, 0 },
    { "-il#", &options.intermediate_prefix, 1, 0x22, 0 },
    { "-xc", &options.option_xc, 1, 0x1, 0 },
    { "-m", &options.option_m, 1, 0x1, 0 },
    { "-V#", &options.option_V, 1, 0x22, 0 },
    { "-Gs", &options.option_Gs, 1, 0x5, 0 },
    { "-Gc", &options.option_Gc, 1, 0x1, 0 },
    { "-Gr", &options.option_Gr, 1, 0x1, 0 },
    { "-Gd", &options.option_Gd, 1, 0x1, 0 },
    { "-Gz", &options.option_Gz, 1, 0x1, 0 },
    { "-NV#", &options.option_NV, 1, 0x22, 0 },
    { "-Fr#", &options.option_Fr, 1, 0x22, 0 },
    { "-FR#", &options.option_FR, 1, 0x22, 0 },
    { "-A#", &options.option_A, 1, 0x22, 0 },
    { "-Q#", &options.option_Q, 1, 0x22, 0 },
    { "-Fd#", &options.database_file, 1, 0x22, 0 },
    { "-Fo*", &options.option_Fo, 1, 0x22, 0 },
    { "-c#", &options.option_Fo, 1, 0x22, 0 },
    { "-Fx*", &options.option_Fo, 1, 0x22, 0 },
    { "-Fs", &options.option_Fs, 1, 0x1, 0 },
    { "-PLMN", &options.option_PLMN, 1, 0x1, 0 },
    { "-PLMF", &options.option_PLMF, 1, 0x1, 0 },
    { "-GA", &options.option_GA, 1, 0x1, 0 },
    { "-GD", &options.option_GD, 1, 0x1, 0 },
    { "-Gw", &options.option_Gw, 1, 0x1, 0 },
    { "-Gq", &options.option_Gq, 1, 0x1, 0 },
    { "-GW", &options.option_GW, 1, 0x1, 0 },
    { "-GEf", &options.option_GEf, 1, 0x1, 0 },
    { "-Gf", &options.option_Gf, 1, 0x1, 0 },
    { "-G1", &options.option_G1, 1, 0x1, 0 },
    { "-G2", &options.option_G2, 1, 0x1, 0 },
    { "-G3", &options.option_G3, 1, 0x1, 0 },
    { "-G4", &options.option_G4, 1, 0x1, 0 },
    { "-qc", &options.option_qc, 1, 0x1, 0 },
    { "-Oa", &options.option_Oa, 1, 0x1, 0 },
    { "-Ow", &options.option_Ow, 1, 0x1, 0 },
    { "-Os", &options.option_Os, 1, 0x5, 0 },
    { "-Ot", &options.option_Os, 1, 0x1, 0 },
    { "-Oc", &options.option_Oc, 1, 0x1, 0 },
    { "-Og", &options.option_Og, 1, 0x1, 0 },
    { "-Oe", &options.option_Oe, 1, 0x1, 0 },
    { "-Op", &options.option_Op, 1, 0x1, 0 },
    { "-Oz", &options.option_Oz, 1, 0x1, 0 },
    { "-On", &options.option_Oz, 1, 0x5, 0 },
    { "-Or", &options.option_Or, 1, 0x1, 0 },
    { "-Oo", &options.option_Oo, 1, 0x1, 0 },
    { "-Ov", &options.option_Ov, 1, 0x1, 0 },
    { "-Of", &options.option_Of, 1, 0x1, 0 },
    { "-Oy", &options.option_Oy, 1, 0x1, 0 },
    { "-Ob#", &options.option_Ob, 1, 0x24, 0 },
    { "-NoComdat", &options.option_NoComdat, 1, 0x5, 0 },
    { "-Yu*", &options.pch_use_file, 1, 0x22, 0 },
    { "-Yu", &options.pch_use, 1, 0x1, 0 },
    { "-Yc*", &options.pch_create_file, 1, 0x22, 0 },
    { "-Yc", &options.pch_create, 1, 0x1, 0 },
    { "-Yd", &options.option_Yd, 1, 0x1, 0 },
    { "-Fp#", &options.option_Fp, 1, 0x22, 0 },
    { "-YX*", &options.pch_auto_file, 1, 0x22, 0 },
    { "-YX", &options.pch_auto, 1, 0x1, 0 },
    { "-Yl*", &options.pch_link_name, 1, 0x22, 0 },
    { "*", &options.unrecognized_argument, 0, 0x22, 0 },
    { 0, 0, 0, 0, 0 }
};
