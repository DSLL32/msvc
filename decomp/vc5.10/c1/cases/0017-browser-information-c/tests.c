#include <string.h>
#include "compiler-state.h"
#include "browser.h"
#include <recon-tool.h>
#include <recon.h>

static int browser_scalar_records(void)
{
    static const struct {
        BYTE kind;
        unsigned symbol, line;
        BYTE bytes[4];
        unsigned length;
    } rows[] = {
        { 4, 0x7fff, 0, {4,0xff,0x7f}, 3 },
        { 4, 0x8000, 0, {0x44,0,0x80,0}, 4 },
        { 5, 0xffabcdef, 0, {0x45,0xef,0xcd,0xab}, 4 },
        { 11, 1, 0, {11,1,0}, 3 },
        { 2, 0, 0x1234, {2,0x34,0x12}, 3 },
        { 21, 0, 0xabcd, {21,0xcd,0xab}, 3 }
    };
    unsigned i, j;
    for (i = 0; i < RECON_COUNT(rows); ++i) {
        CompilerBrowserRecord record, before;
        FILE stream, stream_before;
        BYTE output[16];
        memset(&record, 0x97, sizeof(record));
        memset(&stream, 0x69, sizeof(stream));
        memset(output, 0xa5, sizeof(output));
        record.kind = rows[i].kind;
        record.symbol_id = rows[i].symbol;
        record.line = rows[i].line;
        before = record;
        stream._ptr = (char *)output + 3;
        stream._cnt = 10;
        stream_before = stream;
        c1_data.browser_output_stream = &stream;
        emit_browser_record(&record);
        before.kind = rows[i].bytes[0];
        if (memcmp(&record, &before, sizeof(record)) ||
            memcmp(output + 3, rows[i].bytes, rows[i].length) ||
            stream._ptr != (char *)output + 3 + rows[i].length ||
            stream._cnt != 10 - (int)rows[i].length) return 0;
        stream_before._ptr = stream._ptr;
        stream_before._cnt = stream._cnt;
        if (memcmp(&stream, &stream_before, sizeof(stream))) return 0;
        for (j = 0; j < sizeof(output); ++j)
            if ((j < 3 || j >= 3 + rows[i].length) && output[j] != 0xa5)
                return 0;
    }
    return 1;
}
const ReconTest browser_tests[1] = {
    { "browser scalar encodings retain stack-record padding and owned output neighbors", 1, browser_scalar_records }
};
