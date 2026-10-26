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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* The original402-word inline-assembler vocabulary, selected from released
 * inlnops.tok. Its mkopcode length/lexical substring packing reproduces all
 * original indices and the1208-byte shared character spelling. */
#include "asm.h"
#include <recon.h>
char AsmChars[]=
    "cmpxchg8bfnrstordfnrstorwfnstenvdfnstenvwfdecstpfincstpfldenvdfl"
    "denvwfnsavedfnsavewfrndintfrstordfrstorwfsincosfstenvdfstenvwfuc"
    "omppfxtractfyl2xp1fcomppfdivrpficompfidivrfisubrfldl2efldl2tfldl"
    "g2fldln2fnclexfndisifninitfnstcwfnstswfpatanfprem1fsavedfsavewfs"
    "calefsetpmfsubrpinvlpgloopneloopnzoffsetpushadpushfdsetnaesetnbe"
    "setngesetnlewbinvd.286c.286p.386p.486p.586p.8086.8087boundbswapc"
    "allfcmpsbcmpsdcmpswcpuidwordenterf2xm1faddpfbstpfclexfdisifdivpf"
    "freefiaddfimulfinitfistpfldcwfldpifmulpfnenifptanfsqrtfstcwfstsw"
    "fsubpfwaitfwordiretdjecxzleavelodsblodsdlodswloopeloopzmovsbmovs"
    "dmovswmovsxmovzxoutsboutsdoutswpopadpopfdpwordqwordmsrdtscrepner"
    "epnzscasbscasdscaswsetaesetbesetgesetlesetncsetnesetnosetnpsetns"
    "etnzsetpesetposhortstosbstosdstoswtbytewrmsrxlatb.186.287.387arp"
    "lcltscwdefabsfbldfchsfcosfenifildfld1fldzfnopfstpftstfxamfxchins"
    "binsdinswintojcxzjmpfjnaejnbejngejnlelahflgdtlidtlldtlmswlocknea"
    "rno87reperepzretfretnsahfsetcsetesetosetsetzsgdtshldshrdsidtsldt"
    "smswtestverrverwxaddaaadaamaasadcandbsrbtcbtrbtscbwcdqclcldclicm"
    "cr0cr2cr3cr4dasdr0dr1dr2dr3dr6dr7dupeaxebpebxediedxesiespfarhltj"
    "aejbejgejlejncjnejnojnpjnsjnzjpejpolarlfslgslslssltrnegnotptrclr"
    "crolrorsmsalsarsbbsegstdstistr3tr4tr5tr6tr7xorbhdhjojsjz"
;
struct AsmCodeName AsmOpcode[T_NULL+1]={
    {0,4,753,0}, /* .186 */
    {0,4,338,0}, /* .286 */
    {0,5,338,0}, /* .286c */
    {0,5,343,0}, /* .286p */
    {0,4,757,0}, /* .287 */
    {0,4,348,0}, /* .386 */
    {0,5,348,0}, /* .386p */
    {0,4,761,0}, /* .387 */
    {0,4,353,0}, /* .486 */
    {0,5,353,0}, /* .486p */
    {0,4,358,0}, /* .586 */
    {0,5,358,0}, /* .586p */
    {0,5,363,0}, /* .8086 */
    {0,5,368,0}, /* .8087 */
    {0,3,980,0}, /* aaa */
    {0,3,981,0}, /* aad */
    {0,3,984,0}, /* aam */
    {0,3,987,0}, /* aas */
    {0,3,990,0}, /* adc */
    {0,3,423,0}, /* add */
    {0,2,870,0}, /* ah */
    {0,2,257,0}, /* al */
    {0,3,993,0}, /* and */
    {0,4,765,0}, /* arpl */
    {0,2,1061,0}, /* ax */
    {0,2,1198,0}, /* bh */
    {0,2,546,0}, /* bl */
    {0,5,373,0}, /* bound */
    {0,2,515,0}, /* bp */
    {0,3,779,0}, /* bsf */
    {0,3,996,0}, /* bsr */
    {0,5,378,0}, /* bswap */
    {0,2,999,0}, /* bt */
    {0,3,999,0}, /* btc */
    {0,3,1002,0}, /* btr */
    {0,3,1005,0}, /* bts */
    {0,2,1067,0}, /* bx */
    {0,4,739,0}, /* byte */
    {0,4,383,0}, /* call */
    {0,5,383,0}, /* callf */
    {0,3,1008,0}, /* cbw */
    {0,3,1011,0}, /* cdq */
    {0,2,4,0}, /* ch */
    {0,2,202,0}, /* cl */
    {0,3,1014,0}, /* clc */
    {0,3,1016,0}, /* cld */
    {0,3,1019,0}, /* cli */
    {0,4,769,0}, /* clts */
    {0,3,1022,0}, /* cmc */
    {0,3,0,0}, /* cmp */
    {0,4,388,0}, /* cmps */
    {0,5,388,0}, /* cmpsb */
    {0,5,393,0}, /* cmpsd */
    {0,5,398,0}, /* cmpsw */
    {0,7,0,0}, /* cmpxchg */
    {0,9,0,0}, /* cmpxchg8b */
    {0,5,403,0}, /* cpuid */
    {0,3,1024,0}, /* cr0 */
    {0,3,1027,0}, /* cr2 */
    {0,3,1030,0}, /* cr3 */
    {0,3,1033,0}, /* cr4 */
    {0,2,44,0}, /* cs */
    {0,3,773,0}, /* cwd */
    {0,4,773,0}, /* cwde */
    {0,2,534,0}, /* cx */
    {0,3,979,0}, /* daa */
    {0,3,1036,0}, /* das */
    {0,2,377,0}, /* db */
    {0,2,424,0}, /* dd */
    {0,3,42,0}, /* dec */
    {0,2,16,0}, /* df */
    {0,2,1200,0}, /* dh */
    {0,2,86,0}, /* di */
    {0,3,153,0}, /* div */
    {0,2,178,0}, /* dl */
    {0,2,301,0}, /* dp */
    {0,3,1039,0}, /* dr0 */
    {0,3,1042,0}, /* dr1 */
    {0,3,1045,0}, /* dr2 */
    {0,3,1048,0}, /* dr3 */
    {0,3,1051,0}, /* dr6 */
    {0,3,1054,0}, /* dr7 */
    {0,2,307,0}, /* ds */
    {0,3,1057,0}, /* dup */
    {0,2,407,0}, /* dw */
    {0,5,407,0}, /* dword */
    {0,2,1073,0}, /* dx */
    {0,3,1060,0}, /* eax */
    {0,3,1063,0}, /* ebp */
    {0,3,1066,0}, /* ebx */
    {0,3,533,0}, /* ecx */
    {0,3,1069,0}, /* edi */
    {0,3,1072,0}, /* edx */
    {0,5,412,0}, /* enter */
    {0,2,313,0}, /* es */
    {0,3,1075,0}, /* esi */
    {0,3,1078,0}, /* esp */
    {0,5,417,0}, /* f2xm1 */
    {0,4,777,0}, /* fabs */
    {0,4,422,0}, /* fadd */
    {0,5,422,0}, /* faddp */
    {0,3,1081,0}, /* far */
    {0,4,781,0}, /* fbld */
    {0,5,427,0}, /* fbstp */
    {0,4,785,0}, /* fchs */
    {0,5,432,0}, /* fclex */
    {0,4,146,0}, /* fcom */
    {0,5,146,0}, /* fcomp */
    {0,6,146,0}, /* fcompp */
    {0,4,789,0}, /* fcos */
    {0,7,41,0}, /* fdecstp */
    {0,5,437,0}, /* fdisi */
    {0,4,152,0}, /* fdiv */
    {0,5,442,0}, /* fdivp */
    {0,5,152,0}, /* fdivr */
    {0,6,152,0}, /* fdivrp */
    {0,4,793,0}, /* feni */
    {0,5,447,0}, /* ffree */
    {0,5,452,0}, /* fiadd */
    {0,5,158,0}, /* ficom */
    {0,6,158,0}, /* ficomp */
    {0,5,164,0}, /* fidiv */
    {0,6,164,0}, /* fidivr */
    {0,4,797,0}, /* fild */
    {0,5,457,0}, /* fimul */
    {0,7,48,0}, /* fincstp */
    {0,5,462,0}, /* finit */
    {0,4,467,0}, /* fist */
    {0,5,467,0}, /* fistp */
    {0,5,170,0}, /* fisub */
    {0,6,170,0}, /* fisubr */
    {0,3,55,0}, /* fld */
    {0,4,801,0}, /* fld1 */
    {0,5,472,0}, /* fldcw */
    {0,6,55,0}, /* fldenv */
    {0,7,55,0}, /* fldenvd */
    {0,7,62,0}, /* fldenvw */
    {0,6,176,0}, /* fldl2e */
    {0,6,182,0}, /* fldl2t */
    {0,6,188,0}, /* fldlg2 */
    {0,6,194,0}, /* fldln2 */
    {0,5,477,0}, /* fldpi */
    {0,4,805,0}, /* fldz */
    {0,4,482,0}, /* fmul */
    {0,5,482,0}, /* fmulp */
    {0,6,200,0}, /* fnclex */
    {0,6,206,0}, /* fndisi */
    {0,5,487,0}, /* fneni */
    {0,6,212,0}, /* fninit */
    {0,4,809,0}, /* fnop */
    {0,7,9,0}, /* fnrstor */
    {0,8,9,0}, /* fnrstord */
    {0,8,17,0}, /* fnrstorw */
    {0,6,69,0}, /* fnsave */
    {0,7,69,0}, /* fnsaved */
    {0,7,76,0}, /* fnsavew */
    {0,6,218,0}, /* fnstcw */
    {0,7,25,0}, /* fnstenv */
    {0,8,25,0}, /* fnstenvd */
    {0,8,33,0}, /* fnstenvw */
    {0,6,224,0}, /* fnstsw */
    {0,6,230,0}, /* fpatan */
    {0,5,236,0}, /* fprem */
    {0,6,236,0}, /* fprem1 */
    {0,5,492,0}, /* fptan */
    {0,7,83,0}, /* frndint */
    {0,6,90,0}, /* frstor */
    {0,7,90,0}, /* frstord */
    {0,7,97,0}, /* frstorw */
    {0,2,104,0}, /* fs */
    {0,5,242,0}, /* fsave */
    {0,6,242,0}, /* fsaved */
    {0,6,248,0}, /* fsavew */
    {0,6,254,0}, /* fscale */
    {0,6,260,0}, /* fsetpm */
    {0,4,104,0}, /* fsin */
    {0,7,104,0}, /* fsincos */
    {0,5,497,0}, /* fsqrt */
    {0,3,111,0}, /* fst */
    {0,5,502,0}, /* fstcw */
    {0,6,111,0}, /* fstenv */
    {0,7,111,0}, /* fstenvd */
    {0,7,118,0}, /* fstenvw */
    {0,4,813,0}, /* fstp */
    {0,5,507,0}, /* fstsw */
    {0,4,266,0}, /* fsub */
    {0,5,512,0}, /* fsubp */
    {0,5,266,0}, /* fsubr */
    {0,6,266,0}, /* fsubrp */
    {0,4,817,0}, /* ftst */
    {0,5,125,0}, /* fucom */
    {0,6,125,0}, /* fucomp */
    {0,7,125,0}, /* fucompp */
    {0,5,517,0}, /* fwait */
    {0,5,522,0}, /* fword */
    {0,4,821,0}, /* fxam */
    {0,4,825,0}, /* fxch */
    {0,7,132,0}, /* fxtract */
    {0,5,139,0}, /* fyl2x */
    {0,7,139,0}, /* fyl2xp1 */
    {0,2,1130,0}, /* gs */
    {0,3,1084,0}, /* hlt */
    {0,4,165,0}, /* idiv */
    {0,4,458,0}, /* imul */
    {0,2,49,0}, /* in */
    {0,3,49,0}, /* inc */
    {0,3,829,0}, /* ins */
    {0,4,829,0}, /* insb */
    {0,4,833,0}, /* insd */
    {0,4,837,0}, /* insw */
    {0,3,87,0}, /* int */
    {0,4,841,0}, /* into */
    {0,4,334,0}, /* invd */
    {0,6,272,0}, /* invlpg */
    {0,4,527,0}, /* iret */
    {0,5,527,0}, /* iretd */
    {0,2,1087,0}, /* ja */
    {0,3,1087,0}, /* jae */
    {0,2,1090,0}, /* jb */
    {0,3,1090,0}, /* jbe */
    {0,2,845,0}, /* jc */
    {0,4,845,0}, /* jcxz */
    {0,2,532,0}, /* je */
    {0,5,532,0}, /* jecxz */
    {0,2,1093,0}, /* jg */
    {0,3,1093,0}, /* jge */
    {0,2,1096,0}, /* jl */
    {0,3,1096,0}, /* jle */
    {0,3,849,0}, /* jmp */
    {0,4,849,0}, /* jmpf */
    {0,3,853,0}, /* jna */
    {0,4,853,0}, /* jnae */
    {0,3,857,0}, /* jnb */
    {0,4,857,0}, /* jnbe */
    {0,3,1099,0}, /* jnc */
    {0,3,1102,0}, /* jne */
    {0,3,861,0}, /* jng */
    {0,4,861,0}, /* jnge */
    {0,3,865,0}, /* jnl */
    {0,4,865,0}, /* jnle */
    {0,3,1105,0}, /* jno */
    {0,3,1108,0}, /* jnp */
    {0,3,1111,0}, /* jns */
    {0,3,1114,0}, /* jnz */
    {0,2,1202,0}, /* jo */
    {0,2,1117,0}, /* jp */
    {0,3,1117,0}, /* jpe */
    {0,3,1120,0}, /* jpo */
    {0,2,1204,0}, /* js */
    {0,2,1206,0}, /* jz */
    {0,4,869,0}, /* lahf */
    {0,3,1123,0}, /* lar */
    {0,3,946,0}, /* lds */
    {0,3,537,0}, /* lea */
    {0,5,537,0}, /* leave */
    {0,3,677,0}, /* les */
    {0,3,1126,0}, /* lfs */
    {0,4,873,0}, /* lgdt */
    {0,3,1129,0}, /* lgs */
    {0,4,877,0}, /* lidt */
    {0,4,881,0}, /* lldt */
    {0,4,885,0}, /* lmsw */
    {0,4,889,0}, /* lock */
    {0,4,542,0}, /* lods */
    {0,5,542,0}, /* lodsb */
    {0,5,547,0}, /* lodsd */
    {0,5,552,0}, /* lodsw */
    {0,4,278,0}, /* loop */
    {0,5,557,0}, /* loope */
    {0,6,278,0}, /* loopne */
    {0,6,284,0}, /* loopnz */
    {0,5,562,0}, /* loopz */
    {0,3,1132,0}, /* lsl */
    {0,3,1134,0}, /* lss */
    {0,3,1137,0}, /* ltr */
    {0,3,567,0}, /* mov */
    {0,4,567,0}, /* movs */
    {0,5,567,0}, /* movsb */
    {0,5,572,0}, /* movsd */
    {0,5,577,0}, /* movsw */
    {0,5,582,0}, /* movsx */
    {0,5,587,0}, /* movzx */
    {0,3,459,0}, /* mul */
    {0,4,893,0}, /* near */
    {0,3,1140,0}, /* neg */
    {0,4,897,0}, /* no87 */
    {0,3,810,0}, /* nop */
    {0,3,1143,0}, /* not */
    {0,6,290,0}, /* offset */
    {0,2,14,0}, /* or */
    {0,3,592,0}, /* out */
    {0,4,592,0}, /* outs */
    {0,5,592,0}, /* outsb */
    {0,5,597,0}, /* outsd */
    {0,5,602,0}, /* outsw */
    {0,3,607,0}, /* pop */
    {0,4,607,0}, /* popa */
    {0,5,607,0}, /* popad */
    {0,4,612,0}, /* popf */
    {0,5,612,0}, /* popfd */
    {0,3,1146,0}, /* ptr */
    {0,4,296,0}, /* push */
    {0,5,296,0}, /* pusha */
    {0,6,296,0}, /* pushad */
    {0,5,302,0}, /* pushf */
    {0,6,302,0}, /* pushfd */
    {0,5,617,0}, /* pword */
    {0,5,622,0}, /* qword */
    {0,3,1148,0}, /* rcl */
    {0,3,1151,0}, /* rcr */
    {0,5,625,0}, /* rdmsr */
    {0,5,629,0}, /* rdtsc */
    {0,3,634,0}, /* rep */
    {0,4,901,0}, /* repe */
    {0,5,634,0}, /* repne */
    {0,5,639,0}, /* repnz */
    {0,4,905,0}, /* repz */
    {0,3,528,0}, /* ret */
    {0,4,909,0}, /* retf */
    {0,4,913,0}, /* retn */
    {0,3,1153,0}, /* rol */
    {0,3,1156,0}, /* ror */
    {0,3,1158,0}, /* rsm */
    {0,4,917,0}, /* sahf */
    {0,3,1161,0}, /* sal */
    {0,3,1164,0}, /* sar */
    {0,3,1167,0}, /* sbb */
    {0,4,644,0}, /* scas */
    {0,5,644,0}, /* scasb */
    {0,5,649,0}, /* scasd */
    {0,5,654,0}, /* scasw */
    {0,3,1170,0}, /* seg */
    {0,4,659,0}, /* seta */
    {0,5,659,0}, /* setae */
    {0,4,664,0}, /* setb */
    {0,5,664,0}, /* setbe */
    {0,4,921,0}, /* setc */
    {0,4,925,0}, /* sete */
    {0,4,669,0}, /* setg */
    {0,5,669,0}, /* setge */
    {0,4,674,0}, /* setl */
    {0,5,674,0}, /* setle */
    {0,5,308,0}, /* setna */
    {0,6,308,0}, /* setnae */
    {0,5,314,0}, /* setnb */
    {0,6,314,0}, /* setnbe */
    {0,5,679,0}, /* setnc */
    {0,5,684,0}, /* setne */
    {0,5,320,0}, /* setng */
    {0,6,320,0}, /* setnge */
    {0,5,326,0}, /* setnl */
    {0,6,326,0}, /* setnle */
    {0,5,689,0}, /* setno */
    {0,5,694,0}, /* setnp */
    {0,5,699,0}, /* setns */
    {0,5,703,0}, /* setnz */
    {0,4,929,0}, /* seto */
    {0,4,261,0}, /* setp */
    {0,5,708,0}, /* setpe */
    {0,5,713,0}, /* setpo */
    {0,4,933,0}, /* sets */
    {0,4,936,0}, /* setz */
    {0,4,940,0}, /* sgdt */
    {0,3,944,0}, /* shl */
    {0,4,944,0}, /* shld */
    {0,5,718,0}, /* short */
    {0,3,948,0}, /* shr */
    {0,4,948,0}, /* shrd */
    {0,2,105,0}, /* si */
    {0,4,952,0}, /* sidt */
    {0,4,956,0}, /* sldt */
    {0,4,960,0}, /* smsw */
    {0,2,1079,0}, /* sp */
    {0,2,1135,0}, /* ss */
    {0,2,12,0}, /* st */
    {0,3,220,0}, /* stc */
    {0,3,1173,0}, /* std */
    {0,3,1176,0}, /* sti */
    {0,4,723,0}, /* stos */
    {0,5,723,0}, /* stosb */
    {0,5,728,0}, /* stosd */
    {0,5,733,0}, /* stosw */
    {0,3,1179,0}, /* str */
    {0,3,172,0}, /* sub */
    {0,5,738,0}, /* tbyte */
    {0,4,964,0}, /* test */
    {0,3,1180,0}, /* tr3 */
    {0,3,1183,0}, /* tr4 */
    {0,3,1186,0}, /* tr5 */
    {0,3,1189,0}, /* tr6 */
    {0,3,1192,0}, /* tr7 */
    {0,4,968,0}, /* verr */
    {0,4,972,0}, /* verw */
    {0,4,518,0}, /* wait */
    {0,6,332,0}, /* wbinvd */
    {0,4,408,0}, /* word */
    {0,5,743,0}, /* wrmsr */
    {0,4,976,0}, /* xadd */
    {0,4,3,0}, /* xchg */
    {0,4,748,0}, /* xlat */
    {0,5,748,0}, /* xlatb */
    {0,3,1195,0}, /* xor */
    {0,0,0,0}
};
