/* Native layout/alias check only, outside the reconstructed function graph. */
#define RECON_SHIM_IMPL
#include <stddef.h>
#include <stdio.h>
#include "directives.h"
#include <recon.h>
int main(void)
{
    unsigned word;
    rc_token_extra.identifier.hash=0x34;
    rc_token_extra.identifier.length=0x12;
    word=rc_token_extra.string_length;
    rc_token_extra.string_length=0xabcd;
    printf("%u %u %u %u %u %u %u %u %u\n",
           (unsigned)sizeof(rc_token_extra),
           (unsigned)offsetof(RcTokenExtra,identifier.hash),
           (unsigned)offsetof(RcTokenExtra,identifier.length),
           (unsigned)offsetof(RcTokenExtra,string_length),
           (unsigned)sizeof(rc_string_buffer),
           (unsigned)((char *)rc_number_buffer-(char *)rc_string_buffer),
           word,rc_token_extra.identifier.hash,rc_token_extra.identifier.length);
    return 0;
}
