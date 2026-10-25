/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Declarative mnemonic data for the encountered AsmInit/hash path.
 * Source names: released fullops.tok/inlnops.tok plus retained commented T_AT
 * in h/asmins.h, commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
 * Names were selected against actual1994 records; later additions are absent.
 * AsmOpTokens is ONLY the read token projection, not dummy encoding rows.
 * AsmChars must be named/shared: index/length substrings alias its storage.
 * All names/token IDs/lengths/indices were checked against the1994 table. */
#include "asminit.h"
#include <recon.h>

char AsmChars[] =
    ".listmacroallpushcontextincludelibpopcontext.continue.fardata?cmpxchg8bexterndef"
    "nearstack.errdifi.erridni.errndef.listall.startupfarstackfnrstordfnrstorwfnstenv"
    "dfnstenvwhighwordlengthoflroffsetreadonlysubtitlewatcom_c.dosseg.errdef.lfcond.l"
    "istif.nocref.nolist.repeat.sfcond.tfcondcasemapcommentcompactfdecstpfincstpflden"
    "vdfldenvwfnsavedfnsavewfortranfrndintfrstordfrstorwfsincosfstenvdfstenvwfucomppf"
    "xtractfyl2xp1lowwordnothingprivatesegmentstdcallsyscalltextequtypedef.alpha.brea"
    "k.const.data?.errnb.errnz.model.radix.stack.until.while.xcref.xlistassumecatstrc"
    "ommonelseifexportfcomppfdivrpficompfidivrfisubrfldl2efldl2tfldlg2fldln2fnclexfnd"
    "isifninitfnstcwfnstswfpatanfprem1fsavedfsavewfscalefsetpmfsubrpglobalifdifidnifn"
    "defignoreinvlpginvokeloopneloopnzmediumemoryopattroptionos_dos_os2pascalpublicpu"
    "shadpushfdrecordsdwordsetnaesetnbesetngesetnlesizeofstructsubttlvarargwbinvd.286"
    "c.286p.386p.486p.586p.8086.8087.code.cref.endw.errb.erre.exit.sallaliasalignbasi"
    "cboundbswapcallfcmpsbcmpsdcmpswcpuidendifenterrorextrnf2xm1faddpfbstpfclexfdisif"
    "divpffreefiaddfimulfinitfistpfldcwfldpifmulpfnenifptanfsqrtfstcwfstswfsubpfwaitf"
    "wordgroupifdefiretdjecxzlabelargeleavelocalodsblodsdlodswloopeloopzmovsbmovsdmov"
    "swmovsxmovzxoutsboutsdoutswpopadpopfdprotopurgepwordqwordmsrdtscrepnerepnzsbytes"
    "casbscasdscaswsetaesetbesetgesetlesetncsetnesetnosetnpsetnsetnzsetpesetposhortsm"
    "allstosbstosdstoswordtbyteunionuse16use32widthwrmsrxlatb.186.287.387.seqaddrarpl"
    "cltscwdechoendpendsequ2evenfabsfbldfchsfcosfenifildflatfld1fldzfnopforcfstpftstf"
    "xamfxchugeifnbinsbinsdinswintojcxzjmpfjnaejnbejngejnlelahflgdtlidtlldtlmswlockma"
    "sknameno87pageparaprocrepzretfretnsahfsetcsetesetosetsetzsgdtshldshrdsidtsldtsms"
    "wtesthistinyusesverrverwxaddaaadaamaasadcandbsrbtcbtrbtscbwcdqclcldclicmcr0cr2cr"
    "3cr4dasdr0dr1dr2dr3dr6dr7dupeaxebpebxedxesphltif1if2ifbirpjaejbejgejlejncjnejnoj"
    "npjnsjnzjpejpolfslgslslssltrnegorgptrclrcrolrsmsarsbbtr3tr4tr5tr6tr7xorbhdhjojsj"
    "z";

struct AsmCodeName AsmOpcode[] = {
    { 0, 4, 1416, NULL }, /* T_186: .186 */
    { 0, 4, 876, NULL }, /* T_286: .286 */
    { 0, 5, 876, NULL }, /* T_286C: .286c */
    { 0, 5, 881, NULL }, /* T_286P: .286p */
    { 0, 4, 1420, NULL }, /* T_287: .287 */
    { 0, 4, 886, NULL }, /* T_386: .386 */
    { 0, 5, 886, NULL }, /* T_386P: .386p */
    { 0, 4, 1424, NULL }, /* T_387: .387 */
    { 0, 4, 891, NULL }, /* T_486: .486 */
    { 0, 5, 891, NULL }, /* T_486P: .486p */
    { 0, 4, 896, NULL }, /* T_586: .586 */
    { 0, 5, 896, NULL }, /* T_586P: .586p */
    { 0, 5, 901, NULL }, /* T_8086: .8086 */
    { 0, 5, 906, NULL }, /* T_8087: .8087 */
    { 0, 6, 469, NULL }, /* T_ALPHA: .alpha */
    { 0, 6, 475, NULL }, /* T_BREAK: .break */
    { 0, 5, 911, NULL }, /* T_CODE: .code */
    { 0, 6, 481, NULL }, /* T_CONST: .const */
    { 0, 9, 44, NULL }, /* T_CONTINUE: .continue */
    { 0, 5, 916, NULL }, /* T_CREF: .cref */
    { 0, 5, 487, NULL }, /* T_DATA: .data */
    { 0, 6, 487, NULL }, /* T_DATA_UN: .data? */
    { 0, 7, 217, NULL }, /* T_DOSSEG: .dosseg */
    { 0, 5, 921, NULL }, /* T_ENDW: .endw */
    { 0, 4, 89, NULL }, /* T_ERR: .err */
    { 0, 5, 926, NULL }, /* T_ERRB: .errb */
    { 0, 7, 224, NULL }, /* T_ERRDEF: .errdef */
    { 0, 7, 89, NULL }, /* T_ERRDIF: .errdif */
    { 0, 8, 89, NULL }, /* T_ERRDIFI: .errdifi */
    { 0, 5, 931, NULL }, /* T_ERRE: .erre */
    { 0, 7, 97, NULL }, /* T_ERRIDN: .erridn */
    { 0, 8, 97, NULL }, /* T_ERRIDNI: .erridni */
    { 0, 6, 493, NULL }, /* T_ERRNB: .errnb */
    { 0, 8, 105, NULL }, /* T_ERRNDEF: .errndef */
    { 0, 6, 499, NULL }, /* T_ERRNZ: .errnz */
    { 0, 5, 936, NULL }, /* T_EXIT: .exit */
    { 0, 8, 53, NULL }, /* T_FARDATA: .fardata */
    { 0, 9, 53, NULL }, /* T_FARDATA_UN: .fardata? */
    { 0, 7, 231, NULL }, /* T_LFCOND: .lfcond */
    { 0, 5, 0, NULL }, /* T_LIST: .list */
    { 0, 8, 113, NULL }, /* T_LISTALL: .listall */
    { 0, 7, 238, NULL }, /* T_LISTIF: .listif */
    { 0, 10, 0, NULL }, /* T_LISTMACRO: .listmacro */
    { 0, 13, 0, NULL }, /* T_LISTMACROALL: .listmacroall */
    { 0, 6, 505, NULL }, /* T_MODEL: .model */
    { 0, 7, 245, NULL }, /* T_NOCREF: .nocref */
    { 0, 7, 252, NULL }, /* T_NOLIST: .nolist */
    { 0, 6, 511, NULL }, /* T_RADIX: .radix */
    { 0, 7, 259, NULL }, /* T_REPEAT: .repeat */
    { 0, 5, 941, NULL }, /* T_SALL: .sall */
    { 0, 4, 1428, NULL }, /* T_SEQ: .seq */
    { 0, 7, 266, NULL }, /* T_SFCOND: .sfcond */
    { 0, 6, 517, NULL }, /* T_STACK: .stack */
    { 0, 8, 121, NULL }, /* T_STARTUP: .startup */
    { 0, 7, 273, NULL }, /* T_TFCOND: .tfcond */
    { 0, 6, 523, NULL }, /* T_UNTIL: .until */
    { 0, 6, 529, NULL }, /* T_WHILE: .while */
    { 0, 6, 535, NULL }, /* T_XCREF: .xcref */
    { 0, 6, 541, NULL }, /* T_XLIST: .xlist */
    { 0, 3, 1708, NULL }, /* T_AAA: aaa */
    { 0, 3, 1709, NULL }, /* T_AAD: aad */
    { 0, 3, 1712, NULL }, /* T_AAM: aam */
    { 0, 3, 1715, NULL }, /* T_AAS: aas */
    { 0, 3, 1468, NULL }, /* T_ABS2: abs */
    { 0, 3, 1718, NULL }, /* T_ADC: adc */
    { 0, 3, 1020, NULL }, /* T_ADD: add */
    { 0, 4, 1432, NULL }, /* T_ADDR: addr */
    { 0, 2, 1575, NULL }, /* T_AH: ah */
    { 0, 2, 10, NULL }, /* T_AL: al */
    { 0, 5, 946, NULL }, /* T_ALIAS: alias */
    { 0, 5, 951, NULL }, /* T_ALIGN: align */
    { 0, 3, 1721, NULL }, /* T_AND: and */
    { 0, 4, 1436, NULL }, /* T_ARPL: arpl */
    { 0, 6, 547, NULL }, /* T_ASSUME: assume */
    { 0, 2, 58, NULL }, /* T_AT: at */
    { 0, 2, 1789, NULL }, /* T_AX: ax */
    { 0, 5, 956, NULL }, /* T_BASIC: basic */
    { 0, 2, 1911, NULL }, /* T_BH: bh */
    { 0, 2, 794, NULL }, /* T_BL: bl */
    { 0, 5, 961, NULL }, /* T_BOUND: bound */
    { 0, 2, 33, NULL }, /* T_BP: bp */
    { 0, 3, 1469, NULL }, /* T_BSF: bsf */
    { 0, 3, 1724, NULL }, /* T_BSR: bsr */
    { 0, 5, 966, NULL }, /* T_BSWAP: bswap */
    { 0, 2, 203, NULL }, /* T_BT: bt */
    { 0, 3, 1727, NULL }, /* T_BTC: btc */
    { 0, 3, 1730, NULL }, /* T_BTR: btr */
    { 0, 3, 1733, NULL }, /* T_BTS: bts */
    { 0, 2, 1795, NULL }, /* T_BX: bx */
    { 0, 4, 1275, NULL }, /* T_BYTE: byte */
    { 0, 1, 7, NULL }, /* T_C: c */
    { 0, 4, 444, NULL }, /* T_CALL: call */
    { 0, 5, 971, NULL }, /* T_CALLF: callf */
    { 0, 7, 280, NULL }, /* T_CASEMAP: casemap */
    { 0, 6, 553, NULL }, /* T_CATSTR: catstr */
    { 0, 3, 1736, NULL }, /* T_CBW: cbw */
    { 0, 3, 1739, NULL }, /* T_CDQ: cdq */
    { 0, 2, 66, NULL }, /* T_CH: ch */
    { 0, 2, 26, NULL }, /* T_CL: cl */
    { 0, 3, 1742, NULL }, /* T_CLC: clc */
    { 0, 3, 1744, NULL }, /* T_CLD: cld */
    { 0, 3, 1747, NULL }, /* T_CLI: cli */
    { 0, 4, 1440, NULL }, /* T_CLTS: clts */
    { 0, 3, 1750, NULL }, /* T_CMC: cmc */
    { 0, 3, 62, NULL }, /* T_CMP: cmp */
    { 0, 4, 976, NULL }, /* T_CMPS: cmps */
    { 0, 5, 976, NULL }, /* T_CMPSB: cmpsb */
    { 0, 5, 981, NULL }, /* T_CMPSD: cmpsd */
    { 0, 5, 986, NULL }, /* T_CMPSW: cmpsw */
    { 0, 7, 62, NULL }, /* T_CMPXCHG: cmpxchg */
    { 0, 9, 62, NULL }, /* T_CMPXCHG8B: cmpxchg8b */
    { 0, 4, 287, NULL }, /* T_COMM: comm */
    { 0, 7, 287, NULL }, /* T_COMMENT: comment */
    { 0, 6, 559, NULL }, /* T_COMMON: common */
    { 0, 7, 294, NULL }, /* T_COMPACT: compact */
    { 0, 5, 991, NULL }, /* T_CPUID: cpuid */
    { 0, 3, 1752, NULL }, /* T_CR0: cr0 */
    { 0, 3, 1755, NULL }, /* T_CR2: cr2 */
    { 0, 3, 1758, NULL }, /* T_CR3: cr3 */
    { 0, 3, 1761, NULL }, /* T_CR4: cr4 */
    { 0, 2, 304, NULL }, /* T_CS: cs */
    { 0, 3, 1444, NULL }, /* T_CWD: cwd */
    { 0, 4, 1444, NULL }, /* T_CWDE: cwde */
    { 0, 2, 1141, NULL }, /* T_CX: cx */
    { 0, 3, 1707, NULL }, /* T_DAA: daa */
    { 0, 3, 1764, NULL }, /* T_DAS: das */
    { 0, 2, 965, NULL }, /* T_DB: db */
    { 0, 2, 1021, NULL }, /* T_DD: dd */
    { 0, 3, 302, NULL }, /* T_DEC: dec */
    { 0, 2, 144, NULL }, /* T_DF: df */
    { 0, 2, 1913, NULL }, /* T_DH: dh */
    { 0, 2, 93, NULL }, /* T_DI: di */
    { 0, 3, 584, NULL }, /* T_DIV: div */
    { 0, 2, 176, NULL }, /* T_DL: dl */
    { 0, 2, 803, NULL }, /* T_DP: dp */
    { 0, 2, 1251, NULL }, /* T_DQ: dq */
    { 0, 3, 1767, NULL }, /* T_DR0: dr0 */
    { 0, 3, 1770, NULL }, /* T_DR1: dr1 */
    { 0, 3, 1773, NULL }, /* T_DR2: dr2 */
    { 0, 3, 1776, NULL }, /* T_DR3: dr3 */
    { 0, 3, 1779, NULL }, /* T_DR6: dr6 */
    { 0, 3, 1782, NULL }, /* T_DR7: dr7 */
    { 0, 2, 815, NULL }, /* T_DS: ds */
    { 0, 2, 1260, NULL }, /* T_DT: dt */
    { 0, 3, 1785, NULL }, /* T_DUP: dup */
    { 0, 2, 817, NULL }, /* T_DW: dw */
    { 0, 5, 817, NULL }, /* T_DWORD: dword */
    { 0, 2, 1798, NULL }, /* T_DX: dx */
    { 0, 3, 1788, NULL }, /* T_EAX: eax */
    { 0, 3, 1791, NULL }, /* T_EBP: ebp */
    { 0, 3, 1794, NULL }, /* T_EBX: ebx */
    { 0, 4, 1447, NULL }, /* T_ECHO: echo */
    { 0, 3, 1140, NULL }, /* T_ECX: ecx */
    { 0, 3, 754, NULL }, /* T_EDI: edi */
    { 0, 3, 1797, NULL }, /* T_EDX: edx */
    { 0, 4, 565, NULL }, /* T_ELSE: else */
    { 0, 6, 565, NULL }, /* T_ELSEIF: elseif */
    { 0, 3, 922, NULL }, /* T_END: end */
    { 0, 5, 996, NULL }, /* T_ENDIF: endif */
    { 0, 4, 1451, NULL }, /* T_ENDP: endp */
    { 0, 4, 1455, NULL }, /* T_ENDS: ends */
    { 0, 5, 1001, NULL }, /* T_ENTER: enter */
    { 0, 2, 459, NULL }, /* T_EQ: eq */
    { 0, 3, 459, NULL }, /* T_EQU: equ */
    { 0, 4, 1459, NULL }, /* T_EQU2: equ2 */
    { 0, 5, 1004, NULL }, /* T_ERROR: error */
    { 0, 2, 433, NULL }, /* T_ES: es */
    { 0, 3, 845, NULL }, /* T_ESI: esi */
    { 0, 3, 1800, NULL }, /* T_ESP: esp */
    { 0, 4, 1463, NULL }, /* T_EVEN: even */
    { 0, 6, 571, NULL }, /* T_EXPORT: export */
    { 0, 6, 71, NULL }, /* T_EXTERN: extern */
    { 0, 9, 71, NULL }, /* T_EXTERNDEF: externdef */
    { 0, 5, 1009, NULL }, /* T_EXTRN: extrn */
    { 0, 5, 1014, NULL }, /* T_F2XM1: f2xm1 */
    { 0, 4, 1467, NULL }, /* T_FABS: fabs */
    { 0, 4, 1019, NULL }, /* T_FADD: fadd */
    { 0, 5, 1019, NULL }, /* T_FADDP: faddp */
    { 0, 3, 54, NULL }, /* T_FAR: far */
    { 0, 8, 129, NULL }, /* T_FARSTACK: farstack */
    { 0, 4, 1471, NULL }, /* T_FBLD: fbld */
    { 0, 5, 1024, NULL }, /* T_FBSTP: fbstp */
    { 0, 4, 1475, NULL }, /* T_FCHS: fchs */
    { 0, 5, 1029, NULL }, /* T_FCLEX: fclex */
    { 0, 4, 577, NULL }, /* T_FCOM: fcom */
    { 0, 5, 577, NULL }, /* T_FCOMP: fcomp */
    { 0, 6, 577, NULL }, /* T_FCOMPP: fcompp */
    { 0, 4, 1479, NULL }, /* T_FCOS: fcos */
    { 0, 7, 301, NULL }, /* T_FDECSTP: fdecstp */
    { 0, 5, 1034, NULL }, /* T_FDISI: fdisi */
    { 0, 4, 583, NULL }, /* T_FDIV: fdiv */
    { 0, 5, 1039, NULL }, /* T_FDIVP: fdivp */
    { 0, 5, 583, NULL }, /* T_FDIVR: fdivr */
    { 0, 6, 583, NULL }, /* T_FDIVRP: fdivrp */
    { 0, 4, 1483, NULL }, /* T_FENI: feni */
    { 0, 5, 1044, NULL }, /* T_FFREE: ffree */
    { 0, 5, 1049, NULL }, /* T_FIADD: fiadd */
    { 0, 5, 589, NULL }, /* T_FICOM: ficom */
    { 0, 6, 589, NULL }, /* T_FICOMP: ficomp */
    { 0, 5, 595, NULL }, /* T_FIDIV: fidiv */
    { 0, 6, 595, NULL }, /* T_FIDIVR: fidivr */
    { 0, 4, 1487, NULL }, /* T_FILD: fild */
    { 0, 5, 1054, NULL }, /* T_FIMUL: fimul */
    { 0, 7, 308, NULL }, /* T_FINCSTP: fincstp */
    { 0, 5, 1059, NULL }, /* T_FINIT: finit */
    { 0, 4, 1064, NULL }, /* T_FIST: fist */
    { 0, 5, 1064, NULL }, /* T_FISTP: fistp */
    { 0, 5, 601, NULL }, /* T_FISUB: fisub */
    { 0, 6, 601, NULL }, /* T_FISUBR: fisubr */
    { 0, 4, 1491, NULL }, /* T_FLAT: flat */
    { 0, 3, 315, NULL }, /* T_FLD: fld */
    { 0, 4, 1495, NULL }, /* T_FLD1: fld1 */
    { 0, 5, 1069, NULL }, /* T_FLDCW: fldcw */
    { 0, 6, 315, NULL }, /* T_FLDENV: fldenv */
    { 0, 7, 315, NULL }, /* T_FLDENVD: fldenvd */
    { 0, 7, 322, NULL }, /* T_FLDENVW: fldenvw */
    { 0, 6, 607, NULL }, /* T_FLDL2E: fldl2e */
    { 0, 6, 613, NULL }, /* T_FLDL2T: fldl2t */
    { 0, 6, 619, NULL }, /* T_FLDLG2: fldlg2 */
    { 0, 6, 625, NULL }, /* T_FLDLN2: fldln2 */
    { 0, 5, 1074, NULL }, /* T_FLDPI: fldpi */
    { 0, 4, 1499, NULL }, /* T_FLDZ: fldz */
    { 0, 4, 1079, NULL }, /* T_FMUL: fmul */
    { 0, 5, 1079, NULL }, /* T_FMULP: fmulp */
    { 0, 6, 631, NULL }, /* T_FNCLEX: fnclex */
    { 0, 6, 637, NULL }, /* T_FNDISI: fndisi */
    { 0, 5, 1084, NULL }, /* T_FNENI: fneni */
    { 0, 6, 643, NULL }, /* T_FNINIT: fninit */
    { 0, 4, 1503, NULL }, /* T_FNOP: fnop */
    { 0, 7, 137, NULL }, /* T_FNRSTOR: fnrstor */
    { 0, 8, 137, NULL }, /* T_FNRSTORD: fnrstord */
    { 0, 8, 145, NULL }, /* T_FNRSTORW: fnrstorw */
    { 0, 6, 329, NULL }, /* T_FNSAVE: fnsave */
    { 0, 7, 329, NULL }, /* T_FNSAVED: fnsaved */
    { 0, 7, 336, NULL }, /* T_FNSAVEW: fnsavew */
    { 0, 6, 649, NULL }, /* T_FNSTCW: fnstcw */
    { 0, 7, 153, NULL }, /* T_FNSTENV: fnstenv */
    { 0, 8, 153, NULL }, /* T_FNSTENVD: fnstenvd */
    { 0, 8, 161, NULL }, /* T_FNSTENVW: fnstenvw */
    { 0, 6, 655, NULL }, /* T_FNSTSW: fnstsw */
    { 0, 3, 343, NULL }, /* T_FOR: for */
    { 0, 4, 1507, NULL }, /* T_FORC: forc */
    { 0, 7, 343, NULL }, /* T_FORTRAN: fortran */
    { 0, 6, 661, NULL }, /* T_FPATAN: fpatan */
    { 0, 5, 667, NULL }, /* T_FPREM: fprem */
    { 0, 6, 667, NULL }, /* T_FPREM1: fprem1 */
    { 0, 5, 1089, NULL }, /* T_FPTAN: fptan */
    { 0, 7, 350, NULL }, /* T_FRNDINT: frndint */
    { 0, 6, 357, NULL }, /* T_FRSTOR: frstor */
    { 0, 7, 357, NULL }, /* T_FRSTORD: frstord */
    { 0, 7, 364, NULL }, /* T_FRSTORW: frstorw */
    { 0, 2, 189, NULL }, /* T_FS: fs */
    { 0, 5, 673, NULL }, /* T_FSAVE: fsave */
    { 0, 6, 673, NULL }, /* T_FSAVED: fsaved */
    { 0, 6, 679, NULL }, /* T_FSAVEW: fsavew */
    { 0, 6, 685, NULL }, /* T_FSCALE: fscale */
    { 0, 6, 691, NULL }, /* T_FSETPM: fsetpm */
    { 0, 4, 371, NULL }, /* T_FSIN: fsin */
    { 0, 7, 371, NULL }, /* T_FSINCOS: fsincos */
    { 0, 5, 1094, NULL }, /* T_FSQRT: fsqrt */
    { 0, 3, 378, NULL }, /* T_FST: fst */
    { 0, 5, 1099, NULL }, /* T_FSTCW: fstcw */
    { 0, 6, 378, NULL }, /* T_FSTENV: fstenv */
    { 0, 7, 378, NULL }, /* T_FSTENVD: fstenvd */
    { 0, 7, 385, NULL }, /* T_FSTENVW: fstenvw */
    { 0, 4, 1511, NULL }, /* T_FSTP: fstp */
    { 0, 5, 1104, NULL }, /* T_FSTSW: fstsw */
    { 0, 4, 697, NULL }, /* T_FSUB: fsub */
    { 0, 5, 1109, NULL }, /* T_FSUBP: fsubp */
    { 0, 5, 697, NULL }, /* T_FSUBR: fsubr */
    { 0, 6, 697, NULL }, /* T_FSUBRP: fsubrp */
    { 0, 4, 1515, NULL }, /* T_FTST: ftst */
    { 0, 5, 392, NULL }, /* T_FUCOM: fucom */
    { 0, 6, 392, NULL }, /* T_FUCOMP: fucomp */
    { 0, 7, 392, NULL }, /* T_FUCOMPP: fucompp */
    { 0, 5, 1114, NULL }, /* T_FWAIT: fwait */
    { 0, 5, 1119, NULL }, /* T_FWORD: fword */
    { 0, 4, 1519, NULL }, /* T_FXAM: fxam */
    { 0, 4, 1523, NULL }, /* T_FXCH: fxch */
    { 0, 7, 399, NULL }, /* T_FXTRACT: fxtract */
    { 0, 5, 406, NULL }, /* T_FYL2X: fyl2x */
    { 0, 7, 406, NULL }, /* T_FYL2XP1: fyl2xp1 */
    { 0, 2, 838, NULL }, /* T_GE: ge */
    { 0, 6, 703, NULL }, /* T_GLOBAL: global */
    { 0, 5, 1124, NULL }, /* T_GROUP: group */
    { 0, 2, 1858, NULL }, /* T_GS: gs */
    { 0, 2, 180, NULL }, /* T_GT: gt */
    { 0, 4, 169, NULL }, /* T_HIGH: high */
    { 0, 8, 169, NULL }, /* T_HIGHWORD: highword */
    { 0, 3, 1803, NULL }, /* T_HLT: hlt */
    { 0, 4, 1526, NULL }, /* T_HUGE: huge */
    { 0, 4, 596, NULL }, /* T_IDIV: idiv */
    { 0, 2, 94, NULL }, /* T_IF: if */
    { 0, 3, 1806, NULL }, /* T_IF1: if1 */
    { 0, 3, 1809, NULL }, /* T_IF2: if2 */
    { 0, 3, 1812, NULL }, /* T_IFB: ifb */
    { 0, 5, 1129, NULL }, /* T_IFDEF: ifdef */
    { 0, 5, 709, NULL }, /* T_IFDIF: ifdif */
    { 0, 6, 709, NULL }, /* T_IFDIFI: ifdifi */
    { 0, 3, 569, NULL }, /* T_IFE: ife */
    { 0, 5, 712, NULL }, /* T_IFIDN: ifidn */
    { 0, 6, 712, NULL }, /* T_IFIDNI: ifidni */
    { 0, 4, 1530, NULL }, /* T_IFNB: ifnb */
    { 0, 6, 717, NULL }, /* T_IFNDEF: ifndef */
    { 0, 6, 723, NULL }, /* T_IGNORE: ignore */
    { 0, 4, 1055, NULL }, /* T_IMUL: imul */
    { 0, 2, 24, NULL }, /* T_IN: in */
    { 0, 3, 24, NULL }, /* T_INC: inc */
    { 0, 7, 24, NULL }, /* T_INCLUDE: include */
    { 0, 10, 24, NULL }, /* T_INCLUDELIB: includelib */
    { 0, 3, 1534, NULL }, /* T_INS2: ins */
    { 0, 4, 1534, NULL }, /* T_INSB: insb */
    { 0, 4, 1538, NULL }, /* T_INSD: insd */
    { 0, 4, 1542, NULL }, /* T_INSW: insw */
    { 0, 3, 354, NULL }, /* T_INT: int */
    { 0, 4, 1546, NULL }, /* T_INTO: into */
    { 0, 4, 872, NULL }, /* T_INVD: invd */
    { 0, 6, 729, NULL }, /* T_INVLPG: invlpg */
    { 0, 6, 735, NULL }, /* T_INVOKE: invoke */
    { 0, 4, 1134, NULL }, /* T_IRET: iret */
    { 0, 5, 1134, NULL }, /* T_IRETD: iretd */
    { 0, 3, 1815, NULL }, /* T_IRP: irp */
    { 0, 2, 1818, NULL }, /* T_JA: ja */
    { 0, 3, 1818, NULL }, /* T_JAE: jae */
    { 0, 2, 1821, NULL }, /* T_JB: jb */
    { 0, 3, 1821, NULL }, /* T_JBE: jbe */
    { 0, 2, 1550, NULL }, /* T_JC: jc */
    { 0, 4, 1550, NULL }, /* T_JCXZ: jcxz */
    { 0, 2, 1139, NULL }, /* T_JE: je */
    { 0, 5, 1139, NULL }, /* T_JECXZ: jecxz */
    { 0, 2, 1824, NULL }, /* T_JG: jg */
    { 0, 3, 1824, NULL }, /* T_JGE: jge */
    { 0, 2, 1827, NULL }, /* T_JL: jl */
    { 0, 3, 1827, NULL }, /* T_JLE: jle */
    { 0, 3, 1554, NULL }, /* T_JMP: jmp */
    { 0, 4, 1554, NULL }, /* T_JMPF: jmpf */
    { 0, 3, 1558, NULL }, /* T_JNA: jna */
    { 0, 4, 1558, NULL }, /* T_JNAE: jnae */
    { 0, 3, 1562, NULL }, /* T_JNB: jnb */
    { 0, 4, 1562, NULL }, /* T_JNBE: jnbe */
    { 0, 3, 1830, NULL }, /* T_JNC: jnc */
    { 0, 3, 1833, NULL }, /* T_JNE: jne */
    { 0, 3, 1566, NULL }, /* T_JNG: jng */
    { 0, 4, 1566, NULL }, /* T_JNGE: jnge */
    { 0, 3, 1570, NULL }, /* T_JNL: jnl */
    { 0, 4, 1570, NULL }, /* T_JNLE: jnle */
    { 0, 3, 1836, NULL }, /* T_JNO: jno */
    { 0, 3, 1839, NULL }, /* T_JNP: jnp */
    { 0, 3, 1842, NULL }, /* T_JNS: jns */
    { 0, 3, 1845, NULL }, /* T_JNZ: jnz */
    { 0, 2, 1915, NULL }, /* T_JO: jo */
    { 0, 2, 1848, NULL }, /* T_JP: jp */
    { 0, 3, 1848, NULL }, /* T_JPE: jpe */
    { 0, 3, 1851, NULL }, /* T_JPO: jpo */
    { 0, 2, 1917, NULL }, /* T_JS: js */
    { 0, 2, 1919, NULL }, /* T_JZ: jz */
    { 0, 5, 1144, NULL }, /* T_LABEL: label */
    { 0, 4, 1574, NULL }, /* T_LAHF: lahf */
    { 0, 3, 1148, NULL }, /* T_LAR: lar */
    { 0, 5, 1148, NULL }, /* T_LARGE: large */
    { 0, 3, 1663, NULL }, /* T_LDS: lds */
    { 0, 2, 177, NULL }, /* T_LE: le */
    { 0, 3, 1153, NULL }, /* T_LEA: lea */
    { 0, 5, 1153, NULL }, /* T_LEAVE: leave */
    { 0, 6, 177, NULL }, /* T_LENGTH: length */
    { 0, 8, 177, NULL }, /* T_LENGTHOF: lengthof */
    { 0, 3, 844, NULL }, /* T_LES: les */
    { 0, 3, 1854, NULL }, /* T_LFS: lfs */
    { 0, 4, 1578, NULL }, /* T_LGDT: lgdt */
    { 0, 3, 1857, NULL }, /* T_LGS: lgs */
    { 0, 4, 1582, NULL }, /* T_LIDT: lidt */
    { 0, 4, 1586, NULL }, /* T_LLDT: lldt */
    { 0, 4, 1590, NULL }, /* T_LMSW: lmsw */
    { 0, 5, 1158, NULL }, /* T_LOCAL: local */
    { 0, 4, 1594, NULL }, /* T_LOCK: lock */
    { 0, 4, 1162, NULL }, /* T_LODS: lods */
    { 0, 5, 1162, NULL }, /* T_LODSB: lodsb */
    { 0, 5, 1167, NULL }, /* T_LODSD: lodsd */
    { 0, 5, 1172, NULL }, /* T_LODSW: lodsw */
    { 0, 4, 741, NULL }, /* T_LOOP: loop */
    { 0, 5, 1177, NULL }, /* T_LOOPE: loope */
    { 0, 6, 741, NULL }, /* T_LOOPNE: loopne */
    { 0, 6, 747, NULL }, /* T_LOOPNZ: loopnz */
    { 0, 5, 1182, NULL }, /* T_LOOPZ: loopz */
    { 0, 3, 413, NULL }, /* T_LOW: low */
    { 0, 7, 413, NULL }, /* T_LOWWORD: lowword */
    { 0, 8, 185, NULL }, /* T_LROFFSET: lroffset */
    { 0, 3, 1860, NULL }, /* T_LSL: lsl */
    { 0, 3, 1862, NULL }, /* T_LSS: lss */
    { 0, 2, 454, NULL }, /* T_LT: lt */
    { 0, 3, 1865, NULL }, /* T_LTR: ltr */
    { 0, 5, 5, NULL }, /* T_MACRO: macro */
    { 0, 4, 1598, NULL }, /* T_MASK: mask */
    { 0, 6, 753, NULL }, /* T_MEDIUM: medium */
    { 0, 6, 758, NULL }, /* T_MEMORY: memory */
    { 0, 3, 506, NULL }, /* T_MOD: mod */
    { 0, 3, 1187, NULL }, /* T_MOV: mov */
    { 0, 4, 1187, NULL }, /* T_MOVS: movs */
    { 0, 5, 1187, NULL }, /* T_MOVSB: movsb */
    { 0, 5, 1192, NULL }, /* T_MOVSD: movsd */
    { 0, 5, 1197, NULL }, /* T_MOVSW: movsw */
    { 0, 5, 1202, NULL }, /* T_MOVSX: movsx */
    { 0, 5, 1207, NULL }, /* T_MOVZX: movzx */
    { 0, 3, 1056, NULL }, /* T_MUL: mul */
    { 0, 4, 1602, NULL }, /* T_NAME: name */
    { 0, 2, 80, NULL }, /* T_NE: ne */
    { 0, 4, 80, NULL }, /* T_NEAR: near */
    { 0, 9, 80, NULL }, /* T_NEARSTACK: nearstack */
    { 0, 3, 1868, NULL }, /* T_NEG: neg */
    { 0, 4, 1606, NULL }, /* T_NO87: no87 */
    { 0, 3, 1504, NULL }, /* T_NOP: nop */
    { 0, 3, 420, NULL }, /* T_NOT: not */
    { 0, 7, 420, NULL }, /* T_NOTHING: nothing */
    { 0, 6, 187, NULL }, /* T_OFFSET: offset */
    { 0, 6, 764, NULL }, /* T_OPATTR: opattr */
    { 0, 6, 770, NULL }, /* T_OPTION: option */
    { 0, 2, 142, NULL }, /* T_OR: or */
    { 0, 3, 1871, NULL }, /* T_ORG: org */
    { 0, 6, 776, NULL }, /* T_OS_DOS: os_dos */
    { 0, 6, 780, NULL }, /* T_OS_OS2: os_os2 */
    { 0, 3, 1212, NULL }, /* T_OUT: out */
    { 0, 4, 1212, NULL }, /* T_OUTS: outs */
    { 0, 5, 1212, NULL }, /* T_OUTSB: outsb */
    { 0, 5, 1217, NULL }, /* T_OUTSD: outsd */
    { 0, 5, 1222, NULL }, /* T_OUTSW: outsw */
    { 0, 4, 1610, NULL }, /* T_PAGE: page */
    { 0, 4, 1614, NULL }, /* T_PARA: para */
    { 0, 6, 786, NULL }, /* T_PASCAL: pascal */
    { 0, 3, 34, NULL }, /* T_POP: pop */
    { 0, 4, 1227, NULL }, /* T_POPA: popa */
    { 0, 5, 1227, NULL }, /* T_POPAD: popad */
    { 0, 10, 34, NULL }, /* T_POPCONTEXT: popcontext */
    { 0, 4, 1232, NULL }, /* T_POPF: popf */
    { 0, 5, 1232, NULL }, /* T_POPFD: popfd */
    { 0, 7, 427, NULL }, /* T_PRIVATE: private */
    { 0, 4, 1618, NULL }, /* T_PROC: proc */
    { 0, 5, 1237, NULL }, /* T_PROTO: proto */
    { 0, 3, 1874, NULL }, /* T_PTR: ptr */
    { 0, 6, 792, NULL }, /* T_PUBLIC: public */
    { 0, 5, 1242, NULL }, /* T_PURGE: purge */
    { 0, 4, 13, NULL }, /* T_PUSH: push */
    { 0, 5, 798, NULL }, /* T_PUSHA: pusha */
    { 0, 6, 798, NULL }, /* T_PUSHAD: pushad */
    { 0, 11, 13, NULL }, /* T_PUSHCONTEXT: pushcontext */
    { 0, 5, 804, NULL }, /* T_PUSHF: pushf */
    { 0, 6, 804, NULL }, /* T_PUSHFD: pushfd */
    { 0, 5, 1247, NULL }, /* T_PWORD: pword */
    { 0, 5, 1252, NULL }, /* T_QWORD: qword */
    { 0, 3, 1876, NULL }, /* T_RCL: rcl */
    { 0, 3, 1879, NULL }, /* T_RCR: rcr */
    { 0, 5, 1255, NULL }, /* T_RDMSR: rdmsr */
    { 0, 5, 1259, NULL }, /* T_RDTSC: rdtsc */
    { 0, 8, 193, NULL }, /* T_READONLY: readonly */
    { 0, 6, 810, NULL }, /* T_RECORD: record */
    { 0, 3, 260, NULL }, /* T_REP: rep */
    { 0, 4, 260, NULL }, /* T_REPE: repe */
    { 0, 5, 1264, NULL }, /* T_REPNE: repne */
    { 0, 5, 1269, NULL }, /* T_REPNZ: repnz */
    { 0, 4, 1622, NULL }, /* T_REPZ: repz */
    { 0, 3, 1135, NULL }, /* T_RET: ret */
    { 0, 4, 1626, NULL }, /* T_RETF: retf */
    { 0, 4, 1630, NULL }, /* T_RETN: retn */
    { 0, 3, 1881, NULL }, /* T_ROL: rol */
    { 0, 3, 1006, NULL }, /* T_ROR: ror */
    { 0, 3, 1884, NULL }, /* T_RSM: rsm */
    { 0, 4, 1634, NULL }, /* T_SAHF: sahf */
    { 0, 3, 942, NULL }, /* T_SAL: sal */
    { 0, 3, 1887, NULL }, /* T_SAR: sar */
    { 0, 3, 1890, NULL }, /* T_SBB: sbb */
    { 0, 5, 1274, NULL }, /* T_SBYTE: sbyte */
    { 0, 4, 1279, NULL }, /* T_SCAS: scas */
    { 0, 5, 1279, NULL }, /* T_SCASB: scasb */
    { 0, 5, 1284, NULL }, /* T_SCASD: scasd */
    { 0, 5, 1289, NULL }, /* T_SCASW: scasw */
    { 0, 6, 816, NULL }, /* T_SDWORD: sdword */
    { 0, 3, 221, NULL }, /* T_SEG2: seg */
    { 0, 7, 434, NULL }, /* T_SEGMENT: segment */
    { 0, 4, 1294, NULL }, /* T_SETA: seta */
    { 0, 5, 1294, NULL }, /* T_SETAE: setae */
    { 0, 4, 1299, NULL }, /* T_SETB: setb */
    { 0, 5, 1299, NULL }, /* T_SETBE: setbe */
    { 0, 4, 1638, NULL }, /* T_SETC: setc */
    { 0, 4, 1642, NULL }, /* T_SETE: sete */
    { 0, 4, 1304, NULL }, /* T_SETG: setg */
    { 0, 5, 1304, NULL }, /* T_SETGE: setge */
    { 0, 4, 1309, NULL }, /* T_SETL: setl */
    { 0, 5, 1309, NULL }, /* T_SETLE: setle */
    { 0, 5, 822, NULL }, /* T_SETNA: setna */
    { 0, 6, 822, NULL }, /* T_SETNAE: setnae */
    { 0, 5, 828, NULL }, /* T_SETNB: setnb */
    { 0, 6, 828, NULL }, /* T_SETNBE: setnbe */
    { 0, 5, 1314, NULL }, /* T_SETNC: setnc */
    { 0, 5, 1319, NULL }, /* T_SETNE: setne */
    { 0, 5, 834, NULL }, /* T_SETNG: setng */
    { 0, 6, 834, NULL }, /* T_SETNGE: setnge */
    { 0, 5, 840, NULL }, /* T_SETNL: setnl */
    { 0, 6, 840, NULL }, /* T_SETNLE: setnle */
    { 0, 5, 1324, NULL }, /* T_SETNO: setno */
    { 0, 5, 1329, NULL }, /* T_SETNP: setnp */
    { 0, 5, 1334, NULL }, /* T_SETNS: setns */
    { 0, 5, 1338, NULL }, /* T_SETNZ: setnz */
    { 0, 4, 1646, NULL }, /* T_SETO: seto */
    { 0, 4, 692, NULL }, /* T_SETP: setp */
    { 0, 5, 1343, NULL }, /* T_SETPE: setpe */
    { 0, 5, 1348, NULL }, /* T_SETPO: setpo */
    { 0, 4, 1650, NULL }, /* T_SETS: sets */
    { 0, 4, 1653, NULL }, /* T_SETZ: setz */
    { 0, 4, 1657, NULL }, /* T_SGDT: sgdt */
    { 0, 3, 1661, NULL }, /* T_SHL: shl */
    { 0, 4, 1661, NULL }, /* T_SHLD: shld */
    { 0, 5, 1353, NULL }, /* T_SHORT: short */
    { 0, 3, 1665, NULL }, /* T_SHR: shr */
    { 0, 4, 1665, NULL }, /* T_SHRD: shrd */
    { 0, 2, 372, NULL }, /* T_SI: si */
    { 0, 4, 1669, NULL }, /* T_SIDT: sidt */
    { 0, 4, 846, NULL }, /* T_SIZE: size */
    { 0, 6, 846, NULL }, /* T_SIZEOF: sizeof */
    { 0, 4, 1673, NULL }, /* T_SLDT: sldt */
    { 0, 5, 1358, NULL }, /* T_SMALL: small */
    { 0, 4, 1677, NULL }, /* T_SMSW: smsw */
    { 0, 2, 1801, NULL }, /* T_SP: sp */
    { 0, 2, 220, NULL }, /* T_SS: ss */
    { 0, 2, 3, NULL }, /* T_ST: st */
    { 0, 3, 651, NULL }, /* T_STC: stc */
    { 0, 3, 441, NULL }, /* T_STD: std */
    { 0, 7, 441, NULL }, /* T_STDCALL: stdcall */
    { 0, 3, 241, NULL }, /* T_STI: sti */
    { 0, 4, 1363, NULL }, /* T_STOS: stos */
    { 0, 5, 1363, NULL }, /* T_STOSB: stosb */
    { 0, 5, 1368, NULL }, /* T_STOSD: stosd */
    { 0, 5, 1373, NULL }, /* T_STOSW: stosw */
    { 0, 3, 556, NULL }, /* T_STR: str */
    { 0, 5, 852, NULL }, /* T_STRUC: struc */
    { 0, 6, 852, NULL }, /* T_STRUCT: struct */
    { 0, 3, 201, NULL }, /* T_SUB: sub */
    { 0, 8, 201, NULL }, /* T_SUBTITLE: subtitle */
    { 0, 6, 858, NULL }, /* T_SUBTTL: subttl */
    { 0, 5, 1376, NULL }, /* T_SWORD: sword */
    { 0, 7, 448, NULL }, /* T_SYSCALL: syscall */
    { 0, 5, 1381, NULL }, /* T_TBYTE: tbyte */
    { 0, 4, 1681, NULL }, /* T_TEST: test */
    { 0, 7, 455, NULL }, /* T_TEXTEQU: textequ */
    { 0, 4, 1684, NULL }, /* T_THIS: this */
    { 0, 4, 1688, NULL }, /* T_TINY: tiny */
    { 0, 5, 204, NULL }, /* T_TITLE: title */
    { 0, 3, 1893, NULL }, /* T_TR3: tr3 */
    { 0, 3, 1896, NULL }, /* T_TR4: tr4 */
    { 0, 3, 1899, NULL }, /* T_TR5: tr5 */
    { 0, 3, 1902, NULL }, /* T_TR6: tr6 */
    { 0, 3, 1905, NULL }, /* T_TR7: tr7 */
    { 0, 7, 462, NULL }, /* T_TYPEDEF: typedef */
    { 0, 5, 1386, NULL }, /* T_UNION: union */
    { 0, 5, 1391, NULL }, /* T_USE16: use16 */
    { 0, 5, 1396, NULL }, /* T_USE32: use32 */
    { 0, 4, 1692, NULL }, /* T_USES: uses */
    { 0, 6, 864, NULL }, /* T_VARARG: vararg */
    { 0, 4, 1696, NULL }, /* T_VERR: verr */
    { 0, 4, 1700, NULL }, /* T_VERW: verw */
    { 0, 4, 1115, NULL }, /* T_WAIT: wait */
    { 0, 8, 209, NULL }, /* T_WATCOM_C: watcom_c */
    { 0, 6, 870, NULL }, /* T_WBINVD: wbinvd */
    { 0, 5, 1401, NULL }, /* T_WIDTH: width */
    { 0, 4, 173, NULL }, /* T_WORD: word */
    { 0, 5, 1406, NULL }, /* T_WRMSR: wrmsr */
    { 0, 4, 1704, NULL }, /* T_XADD: xadd */
    { 0, 4, 65, NULL }, /* T_XCHG: xchg */
    { 0, 4, 1411, NULL }, /* T_XLAT: xlat */
    { 0, 5, 1411, NULL }, /* T_XLATB: xlatb */
    { 0, 3, 1908, NULL }, /* T_XOR: xor */
    { 0, 0, 0, NULL }
};

const unsigned short AsmOpTokens[] = {
    T_186, T_286, T_286C, T_286P, T_287, T_386, T_386P, T_387,
    T_486, T_486P, T_586, T_586P, T_8086, T_8087, T_ALPHA, T_BREAK,
    T_CODE, T_CONST, T_CONTINUE, T_CREF, T_DATA, T_DATA_UN, T_DOSSEG, T_ENDW,
    T_ERR, T_ERRB, T_ERRDEF, T_ERRDIF, T_ERRDIFI, T_ERRE, T_ERRIDN, T_ERRIDNI,
    T_ERRNB, T_ERRNDEF, T_ERRNZ, T_EXIT, T_FARDATA, T_FARDATA_UN, T_LFCOND, T_LIST,
    T_LISTALL, T_LISTIF, T_LISTMACRO, T_LISTMACROALL, T_MODEL, T_NOCREF, T_NOLIST, T_RADIX,
    T_REPEAT, T_SALL, T_SEQ, T_SFCOND, T_STACK, T_STARTUP, T_TFCOND, T_UNTIL,
    T_WHILE, T_XCREF, T_XLIST, T_AAA, T_AAD, T_AAM, T_AAS, T_ABS2,
    T_ADC, T_ADC, T_ADC, T_ADC, T_ADC, T_ADC, T_ADC, T_ADC,
    T_ADC, T_ADD, T_ADD, T_ADD, T_ADD, T_ADD, T_ADD, T_ADD,
    T_ADD, T_ADD, T_ADDR, T_AH, T_AL, T_ALIAS, T_ALIGN, T_AND,
    T_AND, T_AND, T_AND, T_AND, T_AND, T_AND, T_AND, T_AND,
    T_ARPL, T_ARPL, T_ASSUME, T_AT, T_AX, T_BASIC, T_BH, T_BL,
    T_BOUND, T_BP, T_BSF, T_BSF, T_BSR, T_BSR, T_BSWAP, T_BT,
    T_BT, T_BT, T_BT, T_BTC, T_BTC, T_BTC, T_BTC, T_BTR,
    T_BTR, T_BTR, T_BTR, T_BTS, T_BTS, T_BTS, T_BTS, T_BX,
    T_BYTE, T_C, T_CALL, T_CALL, T_CALL, T_CALL, T_CALL, T_CALL,
    T_CALL, T_CALL, T_CALLF, T_CALLF, T_CALLF, T_CALLF, T_CALLF, T_CASEMAP,
    T_CATSTR, T_CBW, T_CDQ, T_CH, T_CL, T_CLC, T_CLD, T_CLI,
    T_CLTS, T_CMC, T_CMP, T_CMP, T_CMP, T_CMP, T_CMP, T_CMP,
    T_CMP, T_CMP, T_CMP, T_CMPS, T_CMPSB, T_CMPSD, T_CMPSW, T_CMPXCHG,
    T_CMPXCHG, T_CMPXCHG8B, T_COMM, T_COMMENT, T_COMMON, T_COMPACT, T_CPUID, T_CR0,
    T_CR2, T_CR3, T_CR4, T_CS, T_CWD, T_CWDE, T_CX, T_DAA,
    T_DAS, T_DB, T_DD, T_DEC, T_DEC, T_DEC, T_DEC, T_DEC,
    T_DF, T_DH, T_DI, T_DIV, T_DIV, T_DIV, T_DIV, T_DIV,
    T_DL, T_DP, T_DQ, T_DR0, T_DR1, T_DR2, T_DR3, T_DR6,
    T_DR7, T_DS, T_DT, T_DUP, T_DW, T_DWORD, T_DX, T_EAX,
    T_EBP, T_EBX, T_ECHO, T_ECX, T_EDI, T_EDX, T_ELSE, T_ELSEIF,
    T_END, T_ENDIF, T_ENDP, T_ENDS, T_ENTER, T_EQ, T_EQU, T_EQU2,
    T_ERROR, T_ES, T_ESI, T_ESP, T_EVEN, T_EXPORT, T_EXTERN, T_EXTERNDEF,
    T_EXTRN, T_F2XM1, T_FABS, T_FADD, T_FADD, T_FADD, T_FADD, T_FADD,
    T_FADDP, T_FAR, T_FARSTACK, T_FBLD, T_FBSTP, T_FCHS, T_FCLEX, T_FCOM,
    T_FCOM, T_FCOM, T_FCOM, T_FCOMP, T_FCOMP, T_FCOMP, T_FCOMP, T_FCOMPP,
    T_FCOS, T_FDECSTP, T_FDISI, T_FDIV, T_FDIV, T_FDIV, T_FDIV, T_FDIV,
    T_FDIVP, T_FDIVR, T_FDIVR, T_FDIVR, T_FDIVR, T_FDIVR, T_FDIVRP, T_FENI,
    T_FFREE, T_FIADD, T_FIADD, T_FICOM, T_FICOM, T_FICOMP, T_FICOMP, T_FIDIV,
    T_FIDIV, T_FIDIVR, T_FIDIVR, T_FILD, T_FILD, T_FILD, T_FIMUL, T_FIMUL,
    T_FINCSTP, T_FINIT, T_FIST, T_FIST, T_FISTP, T_FISTP, T_FISTP, T_FISUB,
    T_FISUB, T_FISUBR, T_FISUBR, T_FLAT, T_FLD, T_FLD, T_FLD, T_FLD,
    T_FLD1, T_FLDCW, T_FLDENV, T_FLDENVD, T_FLDENVW, T_FLDL2E, T_FLDL2T, T_FLDLG2,
    T_FLDLN2, T_FLDPI, T_FLDZ, T_8087, T_FMUL, T_FMUL, T_FMUL, T_FMUL,
    T_FMUL, T_FMULP, T_FNCLEX, T_FNDISI, T_FNENI, T_FNINIT, T_FNOP, T_FNRSTOR,
    T_FNRSTORD, T_FNRSTORW, T_FNSAVE, T_FNSAVED, T_FNSAVEW, T_FNSTCW, T_FNSTENV, T_FNSTENVD,
    T_FNSTENVW, T_FNSTSW, T_FOR, T_FORC, T_FORTRAN, T_FPATAN, T_FPREM, T_FPREM1,
    T_FPTAN, T_FRNDINT, T_FRSTOR, T_FRSTORD, T_FRSTORW, T_FS, T_FSAVE, T_FSAVED,
    T_FSAVEW, T_FSCALE, T_FSETPM, T_FSIN, T_FSINCOS, T_FSQRT, T_FST, T_FST,
    T_FST, T_FSTCW, T_FSTENV, T_FSTENVD, T_FSTENVW, T_FSTP, T_FSTP, T_FSTP,
    T_FSTP, T_FSTSW, T_FSTSW, T_FSUB, T_FSUB, T_FSUB, T_FSUB, T_FSUB,
    T_FSUBP, T_FSUBR, T_FSUBR, T_FSUBR, T_FSUBR, T_FSUBR, T_FSUBRP, T_FTST,
    T_FUCOM, T_FUCOM, T_FUCOMP, T_FUCOMP, T_FUCOMPP, T_FWAIT, T_FWORD, T_FXAM,
    T_FXCH, T_FXCH, T_FXTRACT, T_FYL2X, T_FYL2XP1, T_GE, T_GLOBAL, T_GROUP,
    T_GS, T_GT, T_HIGH, T_HIGHWORD, T_HLT, T_HUGE, T_IDIV, T_IDIV,
    T_IDIV, T_IDIV, T_IF, T_IF1, T_IF2, T_IFB, T_IFDEF, T_IFDIF,
    T_IFDIFI, T_IFE, T_IFIDN, T_IFIDNI, T_IFNB, T_IFNDEF, T_IGNORE, T_IMUL,
    T_IMUL, T_IMUL, T_IMUL, T_IMUL, T_IMUL, T_IMUL, T_IMUL, T_IMUL,
    T_IMUL, T_IN, T_IN, T_INC, T_INC, T_INC, T_INC, T_INC,
    T_INCLUDE, T_INCLUDELIB, T_INS2, T_INSB, T_INSD, T_INSW, T_INT, T_INT,
    T_INTO, T_INVD, T_INVLPG, T_INVOKE, T_IRET, T_IRETD, T_IRP, T_JA,
    T_JA, T_JAE, T_JAE, T_JB, T_JB, T_JBE, T_JBE, T_JC,
    T_JC, T_JCXZ, T_JE, T_JE, T_JECXZ, T_JG, T_JG, T_JGE,
    T_JGE, T_JL, T_JL, T_JLE, T_JLE, T_JMP, T_JMP, T_JMP,
    T_JMP, T_JMP, T_JMP, T_JMP, T_JMP, T_JMPF, T_JMPF, T_JMPF,
    T_JMPF, T_JMPF, T_JNA, T_JNA, T_JNAE, T_JNAE, T_JNB, T_JNB,
    T_JNBE, T_JNBE, T_JNC, T_JNC, T_JNE, T_JNE, T_JNG, T_JNG,
    T_JNGE, T_JNGE, T_JNL, T_JNL, T_JNLE, T_JNLE, T_JNO, T_JNO,
    T_JNP, T_JNP, T_JNS, T_JNS, T_JNZ, T_JNZ, T_JO, T_JO,
    T_JP, T_JP, T_JPE, T_JPE, T_JPO, T_JPO, T_JS, T_JS,
    T_JZ, T_JZ, T_LABEL, T_LAHF, T_LAR, T_LAR, T_LARGE, T_LDS,
    T_LE, T_LEA, T_LEAVE, T_LENGTH, T_LENGTHOF, T_LES, T_LFS, T_LGDT,
    T_LGDT, T_LGS, T_LIDT, T_LIDT, T_LLDT, T_LLDT, T_LMSW, T_LMSW,
    T_LOCAL, T_LOCK, T_LODS, T_LODSB, T_LODSD, T_LODSW, T_LOOP, T_LOOPE,
    T_LOOPNE, T_LOOPNZ, T_LOOPZ, T_LOW, T_LOWWORD, T_LROFFSET, T_LSL, T_LSL,
    T_LSS, T_LT, T_LTR, T_LTR, T_MACRO, T_MASK, T_MEDIUM, T_MEMORY,
    T_MOD, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV,
    T_MOV, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV, T_MOV,
    T_MOV, T_MOV, T_MOV, T_MOVS, T_MOVSB, T_MOVSD, T_MOVSW, T_MOVSX,
    T_MOVSX, T_MOVZX, T_MOVZX, T_MUL, T_MUL, T_MUL, T_MUL, T_NAME,
    T_NE, T_NEAR, T_NEARSTACK, T_NEG, T_NEG, T_NEG, T_NEG, T_NO87,
    T_NOP, T_NOT, T_NOT, T_NOT, T_NOT, T_NOTHING, T_OFFSET, T_OPATTR,
    T_OPTION, T_OR, T_OR, T_OR, T_OR, T_OR, T_OR, T_OR,
    T_OR, T_OR, T_ORG, T_OS_DOS, T_OS_OS2, T_OUT, T_OUT, T_OUTS,
    T_OUTSB, T_OUTSD, T_OUTSW, T_PAGE, T_PARA, T_PASCAL, T_POP, T_POP,
    T_POP, T_POP, T_POPA, T_POPAD, T_POPCONTEXT, T_POPF, T_POPFD, T_PRIVATE,
    T_PROC, T_PROTO, T_PTR, T_PUBLIC, T_PURGE, T_PUSH, T_PUSH, T_PUSH,
    T_PUSH, T_PUSH, T_PUSH, T_PUSHA, T_PUSHAD, T_PUSHCONTEXT, T_PUSHF, T_PUSHFD,
    T_PWORD, T_QWORD, T_RCL, T_RCL, T_RCL, T_RCL, T_RCL, T_RCL,
    T_RCL, T_RCL, T_RCL, T_RCL, T_RCL, T_RCL, T_RCR, T_RCR,
    T_RCR, T_RCR, T_RCR, T_RCR, T_RCR, T_RCR, T_RCR, T_RCR,
    T_RCR, T_RCR, T_RDMSR, T_RDTSC, T_READONLY, T_RECORD, T_REP, T_REPE,
    T_REPNE, T_REPNZ, T_REPZ, T_RET, T_RET, T_RETF, T_RETF, T_RETN,
    T_RETN, T_ROL, T_ROL, T_ROL, T_ROL, T_ROL, T_ROL, T_ROL,
    T_ROL, T_ROL, T_ROL, T_ROL, T_ROL, T_ROR, T_ROR, T_ROR,
    T_ROR, T_ROR, T_ROR, T_ROR, T_ROR, T_ROR, T_ROR, T_ROR,
    T_ROR, T_RSM, T_SAHF, T_SAL, T_SAL, T_SAL, T_SAL, T_SAL,
    T_SAL, T_SAL, T_SAL, T_SAL, T_SAL, T_SAL, T_SAL, T_SAR,
    T_SAR, T_SAR, T_SAR, T_SAR, T_SAR, T_SAR, T_SAR, T_SAR,
    T_SAR, T_SAR, T_SAR, T_SBB, T_SBB, T_SBB, T_SBB, T_SBB,
    T_SBB, T_SBB, T_SBB, T_SBB, T_SBYTE, T_SCAS, T_SCASB, T_SCASD,
    T_SCASW, T_SDWORD, T_SEG2, T_SEGMENT, T_SETA, T_SETAE, T_SETB, T_SETBE,
    T_SETC, T_SETE, T_SETG, T_SETGE, T_SETL, T_SETLE, T_SETNA, T_SETNAE,
    T_SETNB, T_SETNBE, T_SETNC, T_SETNE, T_SETNG, T_SETNGE, T_SETNL, T_SETNLE,
    T_SETNO, T_SETNP, T_SETNS, T_SETNZ, T_SETO, T_SETP, T_SETPE, T_SETPO,
    T_SETS, T_SETZ, T_SGDT, T_SHL, T_SHL, T_SHL, T_SHL, T_SHL,
    T_SHL, T_SHL, T_SHL, T_SHL, T_SHL, T_SHL, T_SHL, T_SHLD,
    T_SHLD, T_SHLD, T_SHLD, T_SHORT, T_SHR, T_SHR, T_SHR, T_SHR,
    T_SHR, T_SHR, T_SHR, T_SHR, T_SHR, T_SHR, T_SHR, T_SHR,
    T_SHRD, T_SHRD, T_SHRD, T_SHRD, T_SI, T_SIDT, T_SIZE, T_SIZEOF,
    T_SLDT, T_SLDT, T_SMALL, T_SMSW, T_SMSW, T_SP, T_SS, T_ST,
    T_STC, T_STD, T_STDCALL, T_STI, T_STOS, T_STOSB, T_STOSD, T_STOSW,
    T_STR, T_STR, T_STRUC, T_STRUCT, T_SUB, T_SUB, T_SUB, T_SUB,
    T_SUB, T_SUB, T_SUB, T_SUB, T_SUB, T_SUBTITLE, T_SUBTTL, T_SWORD,
    T_SYSCALL, T_TBYTE, T_TEST, T_TEST, T_TEST, T_TEST, T_TEST, T_TEST,
    T_TEXTEQU, T_THIS, T_TINY, T_TITLE, T_TR3, T_TR4, T_TR5, T_TR6,
    T_TR7, T_TYPEDEF, T_UNION, T_USE16, T_USE32, T_USES, T_VARARG, T_VERR,
    T_VERR, T_VERW, T_VERW, T_WAIT, T_WATCOM_C, T_WBINVD, T_WIDTH, T_WORD,
    T_WRMSR, T_XADD, T_XADD, T_XCHG, T_XCHG, T_XCHG, T_XCHG, T_XCHG,
    T_XLAT, T_XLATB, T_XOR, T_XOR, T_XOR, T_XOR, T_XOR, T_XOR,
    T_XOR, T_XOR, T_XOR,
};
