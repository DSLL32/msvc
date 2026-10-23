#include "directives.h"
#include <recon.h>

/* Encountered descriptor rows: operators and constants used by the lexer.
 * U denotes unavailable storage, not a valid empty descriptor. Variable
 * native consumers hold before reading U; other keyword rows remain unknown. */
#define U {NULL,0}
RcTokenInfo rc_token_info[256]={
    U, /* token0 unavailable */
    U, /* token1 unavailable */
    U, /* token2 unavailable */
    {L"string",3}, /* token3 */
    {L"float constant",94}, /* token4 */
    {L"double constant",94}, /* token5 */
    {L"long double constant",94}, /* token6 */
    {L"int constant",94}, /* token7 */
    {L"long constant",94}, /* token8 */
    {L"unsigned int constant",94}, /* token9 */
    {L"unsigned long constant",94}, /* token10 */
    U, /* token11 unavailable */
    U, /* token12 unavailable */
    U, /* token13 unavailable */
    U, /* token14 unavailable */
    U, /* token15 unavailable */
    U, /* token16 unavailable */
    U, /* token17 unavailable */
    U, /* token18 unavailable */
    U, /* token19 unavailable */
    U, /* token20 unavailable */
    U, /* token21 unavailable */
    U, /* token22 unavailable */
    U, /* token23 unavailable */
    U, /* token24 unavailable */
    U, /* token25 unavailable */
    U, /* token26 unavailable */
    U, /* token27 unavailable */
    U, /* token28 unavailable */
    U, /* token29 unavailable */
    U, /* token30 unavailable */
    U, /* token31 unavailable */
    U, /* token32 unavailable */
    U, /* token33 unavailable */
    U, /* token34 unavailable */
    U, /* token35 unavailable */
    U, /* token36 unavailable */
    U, /* token37 unavailable */
    U, /* token38 unavailable */
    U, /* token39 unavailable */
    U, /* token40 unavailable */
    U, /* token41 unavailable */
    U, /* token42 unavailable */
    U, /* token43 unavailable */
    {L"&",44}, /* token44 */
    {L"&&",45}, /* token45 */
    {L"&=",89}, /* token46 */
    {L"=",47}, /* token47 */
    U, /* token48 unavailable */
    U, /* token49 unavailable */
    U, /* token50 unavailable */
    {L"--",109}, /* token51 */
    {L"/",97}, /* token52 */
    {L"/=",89}, /* token53 */
    {L"==",103}, /* token54 */
    {L"!",55}, /* token55 */
    U, /* token56 unavailable */
    {L">",119}, /* token57 */
    {L">=",119}, /* token58 */
    {L"++",109}, /* token59 */
    U, /* token60 unavailable */
    {L"<<=",89}, /* token61 */
    {L"<<",124}, /* token62 */
    {L"<",119}, /* token63 */
    {L"<=",119}, /* token64 */
    {L"-",65}, /* token65 */
    {L"-=",89}, /* token66 */
    {L"%",97}, /* token67 */
    {L"%=",89}, /* token68 */
    {L"*",69}, /* token69 */
    {L"*=",89}, /* token70 */
    {L"!=",103}, /* token71 */
    {L"|",72}, /* token72 */
    {L"|=",89}, /* token73 */
    {L"||",74}, /* token74 */
    {L".",126}, /* token75 */
    {L"+",76}, /* token76 */
    {L"+=",89}, /* token77 */
    {L"->",126}, /* token78 */
    U, /* token79 unavailable */
    U, /* token80 unavailable */
    U, /* token81 unavailable */
    {L">>=",89}, /* token82 */
    {L">>",124}, /* token83 */
    U, /* token84 unavailable */
    U, /* token85 unavailable */
    {L"^",86}, /* token86 */
    {L"^=",89}, /* token87 */
    U, /* token88 unavailable */
    U, /* token89 unavailable */
    U, /* token90 unavailable */
    U, /* token91 unavailable */
    U, /* token92 unavailable */
    U, /* token93 unavailable */
    U, /* token94 unavailable */
    U, /* token95 unavailable */
    U, /* token96 unavailable */
    U, /* token97 unavailable */
    U, /* token98 unavailable */
    {L"...",99}, /* token99 */
    U, /* token100 unavailable */
    U, /* token101 unavailable */
    {L"end of file",0}, /* token102 */
};
