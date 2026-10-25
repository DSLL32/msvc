/* Finite normal-help startup classification, reviewed from shipped CLIB
 * publics/fixups and source/ABI/body evidence. Older body variants remain
 * explicit in harness/startup-evidence.json; no application acceptance credit.
 * Unknown post-main, exception and reader paths remain masked.
 * All addresses/ranges below are original image RVAs. */
#include <recon-tool.h>
#include <recon.h>

static const ReconFunction functions[] = {
    { .name = "OpenResFile", .entry_rva = 0x348b8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "OpenResFile", .original_symbol = "OpenResFile_" },
    { .name = "FindResources", .entry_rva = 0x34908, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FindResources", .original_symbol = "FindResources_" },
    { .name = "InitResources", .entry_rva = 0x348c8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitResources", .original_symbol = "InitResources_" },
    { .name = "QWrite", .entry_rva = 0x24d80, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "QWrite", .original_symbol = "QWrite_" },

    { .name = "InitMsg", .entry_rva = 0x24850, .definition = RECON_APP,
      .code = (const ReconExtent[]) { {0x24850, 0x248d6} }, .code_count = 1,
      .implementation = RECON_IMPLEMENTED, .native_symbol = "InitMsg", .original_symbol = "InitMsg_" },
    { .name = "LnkFilesInit", .entry_rva = 0x24ba0, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "LnkFilesInit", .original_symbol = "LnkFilesInit_" },
    { .name = "InitNodes", .entry_rva = 0x1c450, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitNodes", .original_symbol = "InitNodes_" },
    { .name = "InitTokBuff", .entry_rva = 0x19c3c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitTokBuff", .original_symbol = "InitTokBuff_" },
    { .name = "InitSpillFile", .entry_rva = 0x25170, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitSpillFile", .original_symbol = "InitSpillFile_" },
    { .name = "InitSym", .entry_rva = 0x23520, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitSym", .original_symbol = "InitSym_" },
    { .name = "NewSection", .entry_rva = 0x1533c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "NewSection", .original_symbol = "NewSection_" },
    { .name = "InitObjIO", .entry_rva = 0x25430, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitObjIO", .original_symbol = "InitObjIO_" },

    { .name = "LnkInit", .entry_rva = 0x245ac, .definition = RECON_APP,
      .code = (const ReconExtent[]) { {0x245ac, 0x24689} }, .code_count = 1,
      .implementation = RECON_IMPLEMENTED, .native_symbol = "LnkInit", .original_symbol = "LnkInit_" },
    { .name = "TrapBreak", .entry_rva = 0x250c4, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "TrapBreak", .original_symbol = "TrapBreak_" },
    { .name = "StartTime", .entry_rva = 0x17980, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "StartTime", .original_symbol = "StartTime_" },
    { .name = "DoCmdFile", .entry_rva = 0x1316c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "DoCmdFile", .original_symbol = "DoCmdFile_" },
    { .name = "CheckErr", .entry_rva = 0x16cd8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CheckErr", .original_symbol = "CheckErr_" },
    { .name = "MapInit", .entry_rva = 0x17a10, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "MapInit", .original_symbol = "MapInit_" },
    { .name = "ProcObjFiles", .entry_rva = 0x21d50, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ProcObjFiles", .original_symbol = "ProcObjFiles_" },
    { .name = "DoDefaultSystem", .entry_rva = 0x2468c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "DoDefaultSystem", .original_symbol = "DoDefaultSystem_" },
    { .name = "FindLibPaths", .entry_rva = 0x246ec, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FindLibPaths", .original_symbol = "FindLibPaths_" },
    { .name = "ResolveUndefined", .entry_rva = 0x220f4, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ResolveUndefined", .original_symbol = "ResolveUndefined_" },
    { .name = "DecideFormat", .entry_rva = 0x13a70, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "DecideFormat", .original_symbol = "DecideFormat_" },
    { .name = "SetFormat", .entry_rva = 0x138c8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "SetFormat", .original_symbol = "SetFormat_" },
    { .name = "ConvertLazyRefs", .entry_rva = 0x23dec, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ConvertLazyRefs", .original_symbol = "ConvertLazyRefs_" },
    { .name = "PrepareSegments", .entry_rva = 0x15a30, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "PrepareSegments", .original_symbol = "PrepareSegments_" },
    { .name = "CheckSymbolPass", .entry_rva = 0x23bc0, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CheckSymbolPass", .original_symbol = "CheckSymbolPass_" },
    { .name = "DefBSSSyms", .entry_rva = 0x17578, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "DefBSSSyms", .original_symbol = "DefBSSSyms_" },
    { .name = "ReportUndefined", .entry_rva = 0x23b8c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ReportUndefined", .original_symbol = "ReportUndefined_" },
    { .name = "PreAddrCalcFormatSpec", .entry_rva = 0x24528, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "PreAddrCalcFormatSpec", .original_symbol = "PreAddrCalcFormatSpec_" },
    { .name = "CheckClassOrder", .entry_rva = 0x1b150, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CheckClassOrder", .original_symbol = "CheckClassOrder_" },
    { .name = "SetStkSize", .entry_rva = 0x17728, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "SetStkSize", .original_symbol = "SetStkSize_" },
    { .name = "AutoGroup", .entry_rva = 0x10010, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "AutoGroup", .original_symbol = "AutoGroup_" },
    { .name = "CalcAddresses", .entry_rva = 0x1b378, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CalcAddresses", .original_symbol = "CalcAddresses_" },
    { .name = "GetBSSSize", .entry_rva = 0x176e4, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "GetBSSSize", .original_symbol = "GetBSSSize_" },
    { .name = "GetStkAddr", .entry_rva = 0x174b4, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "GetStkAddr", .original_symbol = "GetStkAddr_" },
    { .name = "PostAddrCalcFormatSpec", .entry_rva = 0x24580, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "PostAddrCalcFormatSpec", .original_symbol = "PostAddrCalcFormatSpec_" },
    { .name = "ReportLinkMap", .entry_rva = 0x1bd30, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ReportLinkMap", .original_symbol = "ReportLinkMap_" },
    { .name = "InitLoadFile", .entry_rva = 0x17360, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "InitLoadFile", .original_symbol = "InitLoadFile_" },
    { .name = "ObjPass2", .entry_rva = 0x1f180, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "ObjPass2", .original_symbol = "ObjPass2_" },
    { .name = "FiniLoadFile", .entry_rva = 0x17370, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FiniLoadFile", .original_symbol = "FiniLoadFile_" },
    { .name = "EndTime", .entry_rva = 0x184e0, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "EndTime", .original_symbol = "EndTime_" },
    { .name = "LnkMsg", .entry_rva = 0x18fc4, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "LnkMsg", .original_symbol = "LnkMsg_" },
    { .name = "signal", .entry_rva = 0x3421e, .definition = RECON_CRT,
      .original_symbol = "signal_" },

    { .name = "_setjmp", .entry_rva = 0x33b14, .definition = RECON_CRT,
      .code = (const ReconExtent[]) { {0x33b14, 0x33b4d} }, .code_count = 1,
      .original_symbol = "_setjmp_" },
    /* Known nonreturning CRT endpoint; no body restored before Spawn. */
    { .name = "exit", .entry_rva = 0x33f96, .definition = RECON_CRT,
      .original_symbol = "exit_" },
    { .name = "Spawn", .entry_rva = 0x16d3c, .definition = RECON_APP,
      .code = (const ReconExtent[]) { {0x16d3c, 0x16d6f} }, .code_count = 1,
      .implementation = RECON_IMPLEMENTED, .native_symbol = "Spawn", .original_symbol = "Spawn_" },
    { .name = "DoLinkFromCommandLine", .entry_rva = 0x243c8, .definition = RECON_APP,
      .code = (const ReconExtent[]) { {0x243c8, 0x24525} }, .code_count = 1,
      .implementation = RECON_IMPLEMENTED, .native_symbol = "DoLinkFromCommandLine", .original_symbol = "DoLinkFromCommandLine_" },
    { .name = "QClose", .entry_rva = 0x24e10, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "QClose", .original_symbol = "QClose_" },
    { .name = "QDelete", .entry_rva = 0x24ee8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "QDelete", .original_symbol = "QDelete_" },
    { .name = "_LnkFree", .entry_rva = 0x33344, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "_LnkFree", .original_symbol = "_LnkFree_" },
    { .name = "BurnSystemList", .entry_rva = 0x12670, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "BurnSystemList", .original_symbol = "BurnSystemList_" },
    { .name = "FreeList", .entry_rva = 0x1c5c0, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreeList", .original_symbol = "FreeList_" },
    { .name = "CloseSpillFile", .entry_rva = 0x2539c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CloseSpillFile", .original_symbol = "CloseSpillFile_" },
    { .name = "CleanTraces", .entry_rva = 0x1383c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CleanTraces", .original_symbol = "CleanTraces_" },
    { .name = "FreePaths", .entry_rva = 0x18300, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreePaths", .original_symbol = "FreePaths_" },
    { .name = "FreeUndefs", .entry_rva = 0x1c2e8, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreeUndefs", .original_symbol = "FreeUndefs_" },
    { .name = "CleanLoadFile", .entry_rva = 0x13acc, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CleanLoadFile", .original_symbol = "CleanLoadFile_" },
    { .name = "CleanLinkStruct", .entry_rva = 0x1c398, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "CleanLinkStruct", .original_symbol = "CleanLinkStruct_" },
    { .name = "FreeFormatStuff", .entry_rva = 0x1c1d0, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreeFormatStuff", .original_symbol = "FreeFormatStuff_" },
    { .name = "FreeObjInfo", .entry_rva = 0x2545c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreeObjInfo", .original_symbol = "FreeObjInfo_" },
    { .name = "FreeVirtMem", .entry_rva = 0x23564, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FreeVirtMem", .original_symbol = "FreeVirtMem_" },
    { .name = "FiniMsg", .entry_rva = 0x24b0c, .definition = RECON_APP,
      .implementation = RECON_UNIMPLEMENTED, .native_symbol = "FiniMsg", .original_symbol = "FiniMsg_" },
    { .name = "LnkMemFini", .entry_rva = 0x33300, .definition = RECON_APP,
      .code = (const ReconExtent[]) { {0x33300, 0x33301} }, .code_count = 1,
      .implementation = RECON_IMPLEMENTED, .native_symbol = "LnkMemFini", .original_symbol = "LnkMemFini_" },

    {
        .name = "main", .entry_rva = 0x242e0,
        .code = (const ReconExtent[]) { {0x242e0, 0x243c8} }, .code_count = 1,
        .definition = RECON_APP, .implementation = RECON_IMPLEMENTED,
        .native_symbol = "main", .original_symbol = "main_"
    },
    {
        .name = "memset", .entry_rva = 0x33430,
        .code = (const ReconExtent[]) { {0x33430, 0x33448} }, .code_count = 1,
        .definition = RECON_CRT, .original_symbol = "memset_"
    },
    {
        .name = "stackavail", .entry_rva = 0x334cf,
        .code = (const ReconExtent[]) { {0x334cf, 0x334de} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__STOSB", .entry_rva = 0x33ce0,
        .code = (const ReconExtent[]) { {0x33ce0, 0x33d11} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__STOSD", .entry_rva = 0x33d17,
        .code = (const ReconExtent[]) { {0x33d17, 0x33d83} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__SigInit", .entry_rva = 0x342d7,
        .code = (const ReconExtent[]) { {0x342d7, 0x342fd} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NewExceptionHandler", .entry_rva = 0x345f8,
        .code = (const ReconExtent[]) { {0x345f8, 0x34636} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_cstart", .entry_rva = 0x34656,
        .code = (const ReconExtent[]) { {0x34656, 0x3465b} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "malloc", .entry_rva = 0x355bf,
        .code = (const ReconExtent[]) { {0x355bf, 0x35641} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "free", .entry_rva = 0x35641,
        .code = (const ReconExtent[]) { {0x35641, 0x35671} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_nexpand", .entry_rva = 0x35671,
        .code = (const ReconExtent[]) { {0x35671, 0x35788} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "realloc", .entry_rva = 0x35788,
        .code = (const ReconExtent[]) { {0x35788, 0x35807} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__SingleThread", .entry_rva = 0x35813,
        .code = (const ReconExtent[]) { {0x35813, 0x35819} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NTInit", .entry_rva = 0x35823,
        .code = (const ReconExtent[]) { {0x35823, 0x35921} }, .code_count = 1,
        .text_data = (const ReconExtent[]) { {0x33917, 0x33a18} }, .text_data_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NTMainInit", .entry_rva = 0x35921,
        .code = (const ReconExtent[]) { {0x35921, 0x3594b} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "memcpy", .entry_rva = 0x35a7d,
        .code = (const ReconExtent[]) { {0x35a7d, 0x35a9c} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "strlen", .entry_rva = 0x35a9c,
        .code = (const ReconExtent[]) { {0x35a9c, 0x35aaf} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__init_8087", .entry_rva = 0x364a5,
        .code = (const ReconExtent[]) { {0x364a5, 0x364cf} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__chk8087", .entry_rva = 0x364d9,
        .code = (const ReconExtent[]) { {0x364d9, 0x36518} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NTMain", .entry_rva = 0x365b7,
        .code = (const ReconExtent[]) { {0x365b7, 0x365ce} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__Init_Argv", .entry_rva = 0x365ce,
        .code = (const ReconExtent[]) { {0x365ce, 0x36676} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_SplitParms", .entry_rva = 0x36676,
        .code = (const ReconExtent[]) { {0x36676, 0x36752} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NTAddFileHandle", .entry_rva = 0x37017,
        .code = (const ReconExtent[]) { {0x37017, 0x3708d} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__NTInitFileHandles", .entry_rva = 0x3714e,
        .code = (const ReconExtent[]) { {0x3714e, 0x37183} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_init_stk", .entry_rva = 0x37274,
        .code = (const ReconExtent[]) { {0x37274, 0x37275} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__MemAllocator", .entry_rva = 0x37328,
        .code = (const ReconExtent[]) { {0x37328, 0x373d3} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__MemFree", .entry_rva = 0x373d8,
        .code = (const ReconExtent[]) { {0x373d8, 0x374e3}, {0x373d3, 0x373d8} }, .code_count = 2,
        .definition = RECON_CRT
    },
    {
        .name = "__LinkUpNewMHeap", .entry_rva = 0x374e3,
        .code = (const ReconExtent[]) { {0x374e3, 0x3756a} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__ExpandDGROUP", .entry_rva = 0x3756a,
        .code = (const ReconExtent[]) { {0x3756a, 0x375fb} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__nmemneed", .entry_rva = 0x375fb,
        .code = (const ReconExtent[]) { {0x375fb, 0x375fe} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "_msize", .entry_rva = 0x375fe,
        .code = (const ReconExtent[]) { {0x375fe, 0x37607} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "calloc", .entry_rva = 0x37607,
        .code = (const ReconExtent[]) { {0x37607, 0x3761f} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "strdup", .entry_rva = 0x3761f,
        .code = (const ReconExtent[]) { {0x3761f, 0x37648} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__InitRtns", .entry_rva = 0x37648,
        .code = (const ReconExtent[]) { {0x37648, 0x3768b} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__InitFiles", .entry_rva = 0x376ce,
        .code = (const ReconExtent[]) { {0x376ce, 0x37715} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__setenvp", .entry_rva = 0x3776d,
        .code = (const ReconExtent[]) { {0x3776d, 0x37824} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__init_80x87", .entry_rva = 0x37858,
        .code = (const ReconExtent[]) { {0x37858, 0x37885} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__CMain", .entry_rva = 0x38159,
        .code = (const ReconExtent[]) { {0x38159, 0x381a3} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "__CommonInit", .entry_rva = 0x38ffb,
        .code = (const ReconExtent[]) { {0x38ffb, 0x39006} }, .code_count = 1,
        .definition = RECON_CRT
    },
    {
        .name = "import:GetCurrentProcessId", .entry_rva = 0x396da,
        .code = (const ReconExtent[]) { {0x396da, 0x396e0} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetCurrentProcessId"
    },
    {
        .name = "import:LocalAlloc", .entry_rva = 0x396e0,
        .code = (const ReconExtent[]) { {0x396e0, 0x396e6} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "LocalAlloc"
    },
    {
        .name = "import:DeleteFileA", .entry_rva = 0x396e6,
        .code = (const ReconExtent[]) { {0x396e6, 0x396ec} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "DeleteFileA"
    },
    {
        .name = "import:GetStdHandle", .entry_rva = 0x396ec,
        .code = (const ReconExtent[]) { {0x396ec, 0x396f2} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetStdHandle"
    },
    {
        .name = "import:SetStdHandle", .entry_rva = 0x396f2,
        .code = (const ReconExtent[]) { {0x396f2, 0x396f8} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "SetStdHandle"
    },
    {
        .name = "import:GetTimeZoneInformation", .entry_rva = 0x396f8,
        .code = (const ReconExtent[]) { {0x396f8, 0x396fe} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetTimeZoneInformation"
    },
    {
        .name = "import:GetLocalTime", .entry_rva = 0x396fe,
        .code = (const ReconExtent[]) { {0x396fe, 0x39704} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetLocalTime"
    },
    {
        .name = "import:FileTimeToDosDateTime", .entry_rva = 0x39704,
        .code = (const ReconExtent[]) { {0x39704, 0x3970a} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "FileTimeToDosDateTime"
    },
    {
        .name = "import:FileTimeToLocalFileTime", .entry_rva = 0x3970a,
        .code = (const ReconExtent[]) { {0x3970a, 0x39710} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "FileTimeToLocalFileTime"
    },
    {
        .name = "import:GetLastError", .entry_rva = 0x39710,
        .code = (const ReconExtent[]) { {0x39710, 0x39716} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetLastError"
    },
    {
        .name = "import:GetVersion", .entry_rva = 0x39716,
        .code = (const ReconExtent[]) { {0x39716, 0x3971c} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetVersion"
    },
    {
        .name = "import:GetEnvironmentStrings", .entry_rva = 0x3971c,
        .code = (const ReconExtent[]) { {0x3971c, 0x39722} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetEnvironmentStrings"
    },
    {
        .name = "import:GetCurrentThreadId", .entry_rva = 0x39722,
        .code = (const ReconExtent[]) { {0x39722, 0x39728} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetCurrentThreadId"
    },
    {
        .name = "import:ExitProcess", .entry_rva = 0x39728,
        .code = (const ReconExtent[]) { {0x39728, 0x3972e} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "ExitProcess"
    },
    {
        .name = "import:GetCommandLineA", .entry_rva = 0x3972e,
        .code = (const ReconExtent[]) { {0x3972e, 0x39734} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetCommandLineA"
    },
    {
        .name = "import:SetConsoleMode", .entry_rva = 0x39734,
        .code = (const ReconExtent[]) { {0x39734, 0x3973a} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "SetConsoleMode"
    },
    {
        .name = "import:GetConsoleMode", .entry_rva = 0x3973a,
        .code = (const ReconExtent[]) { {0x3973a, 0x39740} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetConsoleMode"
    },
    {
        .name = "import:GetFileType", .entry_rva = 0x39740,
        .code = (const ReconExtent[]) { {0x39740, 0x39746} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetFileType"
    },
    {
        .name = "import:ReadFile", .entry_rva = 0x39746,
        .code = (const ReconExtent[]) { {0x39746, 0x3974c} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "ReadFile"
    },
    {
        .name = "import:WriteFile", .entry_rva = 0x3974c,
        .code = (const ReconExtent[]) { {0x3974c, 0x39752} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "WriteFile"
    },
    {
        .name = "import:SetFilePointer", .entry_rva = 0x39752,
        .code = (const ReconExtent[]) { {0x39752, 0x39758} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "SetFilePointer"
    },
    {
        .name = "import:CloseHandle", .entry_rva = 0x39758,
        .code = (const ReconExtent[]) { {0x39758, 0x3975e} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "CloseHandle"
    },
    {
        .name = "import:CreateFileA", .entry_rva = 0x3975e,
        .code = (const ReconExtent[]) { {0x3975e, 0x39764} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "CreateFileA"
    },
    {
        .name = "import:GetModuleFileNameA", .entry_rva = 0x39764,
        .code = (const ReconExtent[]) { {0x39764, 0x3976a} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetModuleFileNameA"
    },
    {
        .name = "import:GetProcAddress", .entry_rva = 0x3976a,
        .code = (const ReconExtent[]) { {0x3976a, 0x39770} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "GetProcAddress"
    },
    {
        .name = "import:LoadLibraryA", .entry_rva = 0x39770,
        .code = (const ReconExtent[]) { {0x39770, 0x39776} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "LoadLibraryA"
    },
    {
        .name = "import:RtlUnwind", .entry_rva = 0x39776,
        .code = (const ReconExtent[]) { {0x39776, 0x3977c} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "RtlUnwind"
    },
    {
        .name = "import:FindClose", .entry_rva = 0x3977c,
        .code = (const ReconExtent[]) { {0x3977c, 0x39782} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "FindClose"
    },
    {
        .name = "import:FindNextFileA", .entry_rva = 0x39782,
        .code = (const ReconExtent[]) { {0x39782, 0x39788} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "FindNextFileA"
    },
    {
        .name = "import:FindFirstFileA", .entry_rva = 0x39788,
        .code = (const ReconExtent[]) { {0x39788, 0x3978e} }, .code_count = 1,
        .definition = RECON_IMPORT_NAME,
        .import_dll = "KERNEL32.DLL", .import_name = "FindFirstFileA"
    },
};
RECON_EXPORT const ReconBinary binary_descriptor = {
    RECON_ABI_VERSION, sizeof(ReconBinary), "WLINK.EXE",
    "toolchains/watcom10.0a/.patched/WLINK.EXE",
    "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b",
    0x34656, functions, RECON_COUNT(functions), 0, 0
};
