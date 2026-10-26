#ifndef WPP_FRONTEND_H
#define WPP_FRONTEND_H
#include <stdio.h>
#include <recon.h>
/* Recovered byte/bit positions of encountered1994 compiler flags. */
struct comp_flags {
    unsigned char reserved0a:1, signed_char:1, stats_printed:1, check_syntax:1, extensions_enabled:1, inline_intrinsics:1, reserved0c:1, extra_stats_wanted:1;
    unsigned char external_defn_found:1, scanning_c_comment:1, dump_prototypes:1, use_base_types:1, quiet_mode:1, reserved1b:2, no_conmsg:1;
    unsigned char zu_switch_used:1, zd_switch_used:1, reserved2a:1, main_has_parms:1, register_conventions:1, reserved2b:1, emit_library_names:1, strings_in_code:1;
    unsigned char mfi_switch_used:1, banner_printed:1, undefine_all_macros:1, cpp_line_wanted:1, rescan_buffer_done:1, cpp_output:1, cpp_output_to_file:1, cpp_output_requested:1;
    unsigned char bw_switch_used:1, bd_switch_used:1, save_restore_segregs:1, zc_switch_used:1, zf_switch_used:1, zm_switch_used:1, zw_switch_used:1, debug_plus_used:1;
    unsigned char debug_tilde_used:1, bm_switch_used:1, emit_names:1, warnings_cause_bad_exit:1, use_unicode:1, ec_switch_used:1, ee_switch_used:1, ep_switch_used:1;
    unsigned char sg_switch_used:1, reserved6a:1, has_winmain:1, has_libmain:1, reserved6b:1, za_switch_used:1, jis_to_unicode:1, emit_dependencies:1;
    unsigned char emit_browser_info:1, ew_switch_used:1, op_switch_used:1, excs_enabled:1, st_switch_used:1, zz_switch_used:1, extended_defines:1, cmd_file:1;
    unsigned char zv_switch_used:1, all_debug_type_names:1, wo_switch_used:1, reserved8a:1, returns_promoted:1, parsing_finished:1, reserved8b:2;
    unsigned char has_main:1, extern_C_defn_found:1, xcva_switch_used:1, target_multi_thread:1, reserved9b:2, xcmb_switch_used:1, fatal_error:1;
    unsigned char reserved10a:1, use_pcheaders:1, watch_for_pcheader:1, check_truncated_fnames:1, reserved10b:1, no_pch_warnings:1, ei_switch_used:1, reserved10c:1;
    unsigned char reserved11;
};
extern struct comp_flags CompFlags;
extern unsigned WngCount, ErrCount;
extern unsigned SrcLineCount, IncLineCount;
extern char *ForceInclude, *WholeFName, *SrcFName;
extern void *Environment;
void ScanInit(void);
void parseCmdLine(char **argv);
void CBanner(void);
void CCusage(void);
void GenCOptions(char **argv);
void MakePgmName(char *filename);
int makeExitStatus(int fatal);
void PTypeCheckInit(void);
void ErrFileOpen(void);
void PpOpen(void);
void OpenPgmFile(void);
void PrtChar(int ch);
int OpenSrcFile(char *filename, int is_lib);
int IoSuppOpenSrc(char *filename, int kind);
void PpParse(void);
void CgFrontModInitInit(void);
int NextToken(void);
void ParseDecls(void);
void RtnGenerate(void);
void ModuleInitFini(void);
void ScopeEndFileScope(void);
void ScopeCreatePCHDebugSym(void);
void CgBackEnd(void);
void CgFrontFini(void);
void CloseFiles(void);
int IoSuppCloseFile(FILE **file);
extern FILE *DefFile;
int SrcFileFClose(FILE *fp);
void IoSuppSetLineBuffering(FILE *fp, unsigned size);
void set_buffering(FILE *fp, unsigned size, int mode);
/* Exact original e1d1 is a shared empty host hook; original name unknown. */
void frontend_host_hook(void);
void resetHandlers(void);
int _dos_switch_char(void);
void PpInit(void);
void PpStartFile(void);
extern FILE *CppFile;
extern unsigned char PreProcChar;
int doCCompile(char **argv);
#endif
