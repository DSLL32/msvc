#include "preprocess.h"
#include <recon.h>

/* Tokens used by the resource preprocessor's character table. */
enum RcCharacterToken {
    RC_T_SPACE=0, RC_T_CR=1, RC_T_SLASH=2, RC_T_END_OR_ESCAPE=3,
    RC_T_STAR=4, RC_T_LF=5, RC_T_SINGLE_QUOTE=7, RC_T_DOUBLE_QUOTE=8,
    RC_T_PERIOD=9, RC_T_NOT=10, RC_T_HASH=11, RC_T_PERCENT=12,
    RC_T_EQUAL=13, RC_T_XOR=14, RC_T_OR=15, RC_T_AND=16,
    RC_T_PLUS=17, RC_T_MINUS=18, RC_T_LESS=19, RC_T_GREATER=20,
    RC_T_INVALID=23, RC_T_RIGHT_BRACE=24, RC_T_RIGHT_BRACKET=25,
    RC_T_COLON=26, RC_T_COMMA=27, RC_T_RIGHT_PAREN=28,
    RC_T_DIGIT=29, RC_T_LEFT_BRACE=30, RC_T_LEFT_BRACKET=31,
    RC_T_LEFT_PAREN=32, RC_T_QUESTION=33, RC_T_SEMICOLON=34,
    RC_T_TILDE=35, RC_T_IDENTIFIER=40, RC_T_AT=41, RC_T_BAD_PUNCTUATION=43, RC_T_BOM=44
};

#define X RC_T_INVALID
#define I RC_T_IDENTIFIER
#define D RC_T_DIGIT
#define INVALID8 X,X,X,X,X,X,X,X

/* One identified lexical table, indexed by the actual input character.
 * Setup assigns synthetic tokens to control characters1..4 and enables '$'. */
unsigned short rc_character_tokens[256]={
    /* 00..07 */ RC_T_END_OR_ESCAPE,X,X,X,X,X,X,X,
    /* 08..0f */ X,RC_T_SPACE,RC_T_LF,RC_T_SPACE,RC_T_SPACE,RC_T_CR,X,X,
    /* 10..17 */ INVALID8,
    /* 18..1f */ X,X,RC_T_END_OR_ESCAPE,X,X,X,X,X,
    /* 20..27 */ RC_T_SPACE,RC_T_NOT,RC_T_DOUBLE_QUOTE,RC_T_HASH,
                 RC_T_BAD_PUNCTUATION,RC_T_PERCENT,RC_T_AND,RC_T_SINGLE_QUOTE,
    /* 28..2f */ RC_T_LEFT_PAREN,RC_T_RIGHT_PAREN,RC_T_STAR,RC_T_PLUS,
                 RC_T_COMMA,RC_T_MINUS,RC_T_PERIOD,RC_T_SLASH,
    /* 30..37 */ D,D,D,D,D,D,D,D,
    /* 38..3f */ D,D,RC_T_COLON,RC_T_SEMICOLON,RC_T_LESS,RC_T_EQUAL,
                 RC_T_GREATER,RC_T_QUESTION,
    /* 40..47 */ RC_T_AT,I,I,I,I,I,I,I,
    /* 48..4f */ I,I,I,I,I,I,I,I,
    /* 50..57 */ I,I,I,I,I,I,I,I,
    /* 58..5f */ I,I,I,RC_T_LEFT_BRACKET,RC_T_END_OR_ESCAPE,
                 RC_T_RIGHT_BRACKET,RC_T_XOR,I,
    /* 60..67 */ RC_T_BAD_PUNCTUATION,I,I,I,I,I,I,I,
    /* 68..6f */ I,I,I,I,I,I,I,I,
    /* 70..77 */ I,I,I,I,I,I,I,I,
    /* 78..7f */ I,I,I,RC_T_LEFT_BRACE,RC_T_OR,RC_T_RIGHT_BRACE,RC_T_TILDE,X,
    /* 80..ff: byte-range high characters are not identifier tokens. */
    INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,
    INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8,INVALID8
};
#undef INVALID8
#undef D
#undef I
#undef X

/* The identifier, numeric and comment scanners share these WORD flags. */
enum RcCharacterFlags {
    RC_CF_BINARY=1,RC_CF_OCTAL=2,RC_CF_DECIMAL=4,RC_CF_HEX=8,
    RC_CF_IDENTIFIER=16,RC_CF_RADIX_LETTER=32,
    RC_CF_SPACE=64,RC_CF_COMMENT_BREAK=128
};
#define ID RC_CF_IDENTIFIER
#define HX (ID|RC_CF_HEX)
#define RX (ID|RC_CF_RADIX_LETTER)
#define HR (HX|RC_CF_RADIX_LETTER)
#define BN (ID|RC_CF_BINARY|RC_CF_OCTAL|RC_CF_DECIMAL|RC_CF_HEX)
#define OC (ID|RC_CF_OCTAL|RC_CF_DECIMAL|RC_CF_HEX)
#define DC (ID|RC_CF_DECIMAL|RC_CF_HEX)
#define WS RC_CF_SPACE
#define CB RC_CF_COMMENT_BREAK
/* All256 default words are identified. The extra boundary word remains
 * unavailable: the original class getter can alias the following union keyword
 * there. Its literal/consumer relationship is separate from this table proof. */
unsigned short rc_character_classes[257]={
    /* 00..0f */ CB,0,0,0,0,0,0,0,0,WS,CB,WS,WS,0,0,0,
    /* 10..1f */ 0,0,0,0,0,0,0,0,0,0,CB,0,0,0,0,0,
    /* 20..2f */ WS,0,0,0,0,0,0,0,0,0,CB,0,0,0,0,0,
    /* 30..3f */ BN,BN,OC,OC,OC,OC,OC,OC,DC,DC,0,0,0,0,0,0,
    /* 40..4f */ 0,HX,HR,HX,HR,HX,HX,ID,RX,ID,ID,ID,ID,ID,ID,RX,
    /* 50..5f */ ID,RX,ID,ID,ID,ID,ID,ID,ID,ID,ID,0,0,0,0,ID,
    /* 60..6f */ 0,HX,HR,HX,HR,HX,HX,ID,RX,ID,ID,ID,ID,ID,ID,RX,
    /* 70..7f */ ID,RX,ID,ID,ID,ID,ID,ID,ID,ID,ID,0,0,0,0,0
    /* 80..ff: all identified flags are zero. Index256 is unclaimed storage. */
};
#undef ID
#undef HX
#undef RX
#undef HR
#undef BN
#undef OC
#undef DC
#undef WS
#undef CB

void __stdcall rc_set_character_token(unsigned short character,unsigned short token)
{
    if(character<=256) rc_character_tokens[(unsigned char)character]=token;
}

void __stdcall rc_set_character_class(unsigned short character,unsigned short flags)
{
    if(character<=256) rc_character_classes[character]=flags;
}

unsigned short __stdcall rc_character_token(unsigned short character)
{
    if(character==0xfeff) return RC_T_BOM;
    if(character>256) return RC_T_IDENTIFIER;
    /* This boundary word belongs to the adjacent class table in the image. */
    if(character==256) return rc_character_classes[0];
    return rc_character_tokens[character];
}
