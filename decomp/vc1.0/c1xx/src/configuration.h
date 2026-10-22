#ifndef CONFIGURATION_H
#define CONFIGURATION_H
/* The PCH configuration snapshot copies this complete logical state. Original
   0x9f798..0x9fa94: 63 scalar slots followed by two 256-byte paths. Reserved
   fields retain their original offsets until their consumers are reached. */
typedef struct CompilerConfiguration {
    int z_E; /* original 0x9f798 */
    int extensions_enabled; /* original 0x9f79c */
    int option_Gr; /* original 0x9f7a0 */
    int field0c; /* original 0x9f7a4 */
    int japanese_identifiers; /* original 0x9f7a8 */
    int option_J; /* original 0x9f7ac */
    int option_vmb; /* original 0x9f7b0 */
    int option_vmg; /* original 0x9f7b4 */
    int option_vms; /* original 0x9f7b8 */
    int option_vmm; /* original 0x9f7bc */
    int option_vmv; /* original 0x9f7c0 */
    int option_vd; /* original 0x9f7c4 */
    int option_MD; /* original 0x9f7c8 */
    int option_M; /* original 0x9f7cc */
    int option_MT; /* original 0x9f7d0 */
    int option_Gc; /* original 0x9f7d4 */
    int option_PLMF; /* original 0x9f7d8 */
    int option_PLMN; /* original 0x9f7dc */
    int debug_info; /* original 0x9f7e0 */
    int option_NoComdat; /* original 0x9f7e4 */
    int option_GA; /* original 0x9f7e8 */
    int option_GD; /* original 0x9f7ec */
    int option_GEf; /* original 0x9f7f0 */
    int option_Gq; /* original 0x9f7f4 */
    int option_GW; /* original 0x9f7f8 */
    int option_Gw; /* original 0x9f7fc */
    int option_Yd; /* original 0x9f800 */
    int field6c; /* original 0x9f804 */
    int option_Gn; /* original 0x9f808 */
    int far_call_mode; /* original 0x9f80c */
    int prologue_mode; /* original 0x9f810 */
    int field7c; /* original 0x9f814 */
    int option_EP; /* original 0x9f818 */
    int option_pf; /* original 0x9f81c */
    int unguarded_compilation; /* original 0x9f820 */
    int option_Bu; /* original 0x9f824 */
    int option_Brepro; /* original 0x9f828 */
    int option_ide; /* original 0x9f82c */
    int option_Oi; /* original 0x9f830 */
    int option_Ol; /* original 0x9f834 */
    int option_Oq; /* original 0x9f838 */
    int option_Gs; /* original 0x9f83c */
    int fielda8; /* original 0x9f840 */
    int fieldac; /* original 0x9f844 */
    int option_Gd; /* original 0x9f848 */
    int option_Gz; /* original 0x9f84c */
    int fieldb8; /* original 0x9f850 */
    int option_G1; /* original 0x9f854 */
    int option_G2; /* original 0x9f858 */
    int option_G3; /* original 0x9f85c */
    int option_G4; /* original 0x9f860 */
    int option_qc; /* original 0x9f864 */
    int option_Op; /* original 0x9f868 */
    int option_Ob; /* original 0x9f86c */
    int option_Oy; /* original 0x9f870 */
    int option_Oa; /* original 0x9f874 */
    int option_Ow; /* original 0x9f878 */
    int option_Oz; /* original 0x9f87c */
    int browser_enabled; /* original 0x9f880 */
    int browser_full; /* original 0x9f884 */
    int precompiled_option_flags; /* original 0x9f888 */
    int fieldf4; /* original 0x9f88c */
    int line_records; /* original 0x9f890 */
    char database_path[256];
    char pch_object_name[256];
} CompilerConfiguration;
typedef char ConfigurationSizeMustBe764[(sizeof(CompilerConfiguration) == 764) ? 1 : -1];
#endif
