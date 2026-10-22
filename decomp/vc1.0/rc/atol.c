/* Original 0x0100f180..0x0100f22a. The old CRT classifies input bytes
 * unsigned and accumulates modulo 2^32, without a range-error path. */
#include <stdlib.h>
#include <ctype.h>

long __cdecl atol(const char *text)
{
    const unsigned char *next = (const unsigned char *)text;
    unsigned long value = 0;
    unsigned int character, sign;
    while (isspace(*next))
        ++next;
    character = *next++;
    sign = character;
    if (character == '-' || character == '+')
        character = *next++;
    while (isdigit(character)) {
        value = value * 10 + character - '0';
        character = *next++;
    }
    if (sign == '-')
        value = -value;
    return (long)value;
}

/* Original 0x0100f230..0x0100f23e. A distinct CRT graph vertex. */
int __cdecl atoi(const char *text)
{
    return (int)atol(text);
}
