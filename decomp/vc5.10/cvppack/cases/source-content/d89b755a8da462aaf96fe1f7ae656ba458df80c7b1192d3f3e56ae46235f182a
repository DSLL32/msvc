/* Original-only debugger transport for live public nodes before page padding. */
static void public_padding_before(FILE *log, HANDLE process, CONTEXT *context)
{
    DWORD args[3], table[4096], bucket, node, next, count = 0, remaining, cursor;
    WORD length;
    unsigned char bytes[1042];
    if (!read_bytes(process, context->Esp, args, sizeof(args)) || args[0] != 0x40d62f || args[2] != 4 ||
        !read_bytes(process, args[1], table, sizeof(table)) ||
        !read_bytes(process, 0x412d04, &remaining, 4) || !read_bytes(process, 0x412d08, &cursor, 4)) {
        fprintf(log, "public-padding-observation-failed\n"); return;
    }
    fprintf(log, "public-padding-entry %08lx %08lx %08lx %lu %lu %08lx %08lx %08lx %08lx %08lx\n",
        context->Esp, args[0], args[1], args[2], remaining, cursor,
        context->Ebx, context->Esi, context->Edi, context->Ebp);
    fprintf(log, "public-padding-buckets ");
    for (bucket = 0; bucket < sizeof(table); ++bucket) fprintf(log, "%02x", ((unsigned char *)table)[bucket]);
    fputc('\n', log);
    for (bucket = 0; bucket < 4096; ++bucket) {
        node = table[bucket];
        while (node) {
            if (++count > 512 || !read_bytes(process, node + 16, &length, 2) || length > 1024 ||
                !read_bytes(process, node, bytes, (DWORD)length + 18)) {
                fprintf(log, "public-padding-observation-failed\n"); return;
            }
            memcpy(&next, bytes, 4);
            fprintf(log, "public-padding-node %lu %08lx ", bucket, node);
            for (length = 0; length < (DWORD)(*(WORD *)(bytes + 16)) + 18; ++length) fprintf(log, "%02x", bytes[length]);
            fputc('\n', log); node = next;
        }
    }
}
