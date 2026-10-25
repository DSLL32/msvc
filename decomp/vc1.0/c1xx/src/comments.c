/* Source comments, separate from macro replacement and string scanning. */
#include "comments.h"
#include "source-input.h"
#include "characters.h"
#include "initialization.h"
#include <recon.h>

/* Original 0x25c30..0x25c52, stdcall4. Match one recovered source
   character, leaving a different character for the next scanner consumer. */
int __stdcall match_source_character(unsigned char character)
{
    if (next_source_character() == character) return 1;
    --source_text_cursor;
    return 0;
}

/* Original 0x25e50..0x25f14, cdecl. Scan until the matching star/
   delimiter, recovering window boundaries and escaped delimiters separately.
   Newlines still advance source positions and /E output inside the comment. */
void skip_block_comment(void)
{
    unsigned char character;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        if (character == '*') {
following_star:
            character = (unsigned char)*source_text_cursor++;
            if (character == '/') return;
            if (character == '*') goto following_star;
            if (!character) {
                recover_special_character();
                goto following_star;
            }
            if (character == '\\') {
                if (recover_special_character() & 0x10) continue;
                goto following_star;
            }
            if (character_class[character] == CC_LEAD_BYTE) ++source_text_cursor;
        }
        if (character == '\n') {
            ++source_line_number;
            if (options.option_E) {
                if (preprocessor_stream) fputc('\n', preprocessor_stream);
                else write_work_character('\n', compiler_stack);
            }
        } else if (!character) recover_special_character();
    }
}

/* Original 0x25f20..0x25f8e, cdecl. The comment-scanning bit protects
   source-boundary recovery; /C retains block comments outside directive text. */
int skip_source_comment(void)
{
    if (match_source_character('*')) {
        token_allocation_flags |= 0x1000;
        if (!options.option_C || (token_allocation_flags & 0x10000))
            skip_block_comment();
        else copy_block_comment();
        token_allocation_flags &= ~0x1000U;
        return 1;
    }
    if (match_source_character('/')) {
        token_allocation_flags |= 0x1000;
        skip_line_comment();
        token_allocation_flags &= ~0x1000U;
        return 1;
    }
    return 0;
}

/* Original0x24740..0x247a8, cdecl. Discard comment text while
   retaining source-window/line-splice handling and complete DBCS pairs. */
void discard_line_comment(void)
{
    unsigned char character;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        switch (character_class[character]) {
        case CC_ESCAPE_OR_END:
            if (recover_special_character() == 1 && character == '\\')
                compiler_warning(3,10);
            break;
        case CC_LEAD_BYTE:
            ++source_text_cursor;
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        }
    }
}

/* Original 0x25d50..0x25e4e, cdecl. Preserve /C block-comment text,
   excluding CR bytes, while retaining source-window and trigraph recovery. */
void copy_block_comment(void)
{
    unsigned char character;
    fwrite("/*", 2, 1, preprocessor_stream);
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        switch (character_class[character]) {
        case CC_CARRIAGE_RETURN:
            continue;
        case CC_ESCAPE_OR_END:
            if (source_text_cursor[-1] == '?') {
                if (!(recover_trigraph() & 0x40)) continue;
            } else if (source_text_cursor[-1] == '\\') {
                putc('\\', stdout);
                consume_line_escape();
                continue;
            } else {
                recover_special_character();
                continue;
            }
            break;
        case CC_STAR:
            if (match_source_character('/')) {
                fwrite("*/", 2, 1, preprocessor_stream);
                return;
            }
            break;
        case CC_NEWLINE:
            ++source_line_number;
            break;
        }
        fputc(character, preprocessor_stream);
    }
}

/* Original 0x25c60..0x25c7c, cdecl. Both branches are original tail
   transfers. Directive scanning suppresses /C comment retention. */
void skip_line_comment(void)
{
    if (options.option_C && !(token_allocation_flags & 0x10000))
        copy_line_comment();
    else
        discard_line_comment();
}

/* Original 0x25c80..0x25d48, cdecl. The /C line-comment tail copies
   source bytes but leaves the terminating newline for the normal scanner. */
void copy_line_comment(void)
{
    unsigned char character;
    fwrite("//", 2, 1, preprocessor_stream);
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        switch (character_class[character]) {
        case CC_CARRIAGE_RETURN:
            continue;
        case CC_ESCAPE_OR_END:
            if (source_text_cursor[-1] == '?') {
                if (!(recover_trigraph() & 0x40)) continue;
            } else if (source_text_cursor[-1] == '\\') {
                putc('\\', stdout);
                consume_line_escape();
                continue;
            } else {
                recover_special_character();
                continue;
            }
            break;
        case CC_NEWLINE:
            --source_text_cursor;
            return;
        }
        fputc(character, preprocessor_stream);
    }
}
