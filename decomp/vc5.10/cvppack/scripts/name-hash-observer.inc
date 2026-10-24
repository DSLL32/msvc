/* Original-only private-allocation observation before stream initialization. */
static DWORD name_hash_sizes[128], name_hash_addresses[128], name_hash_count;
static DWORD name_hash_pending, name_hash_pending_site, name_hash_started, name_hash_copy_arena;
static void name_hash_observation(FILE *log, HANDLE process, CONTEXT *context, DWORD site)
{
    DWORD size, index, offset, args[7];
    unsigned char *bytes;
    if (site == 0x40e920) {
        name_hash_started = 1;
        if (!read_bytes(process, context->Esp, args, sizeof(args))) { fprintf(log, "namehash-observation-failed\n"); return; }
        fprintf(log, "namehash-entry %08lx", context->Esp);
        for (index = 0; index < 7; ++index) fprintf(log, " %08lx", args[index]);
        fputc('\n', log);
    } else if (site == 0x40e9aa || site == 0x40e9e9 || site == 0x40eb0a || site == 0x40ff9d || site == 0x40ffaa) {
        if (name_hash_pending || name_hash_count >= 128 || main_free_count != 12 || source_family_frees != 2 ||
            !read_bytes(process, context->Esp, &size, 4) || !size || size > 65536) {
            fprintf(log, "namehash-observation-failed\n"); return;
        }
        name_hash_pending = size; name_hash_pending_site = site;
        fprintf(log, "namehash-allocation-call %08lx %lu %lu %lu %08lx %08lx %08lx %08lx %08lx\n",
            site, size, main_free_count, source_family_frees, context->Esp,
            context->Ebx, context->Esi, context->Edi, context->Ebp);
    } else if (site == 0x40e9af || site == 0x40e9ee || site == 0x40eb0f || site == 0x40ffa2 || site == 0x40ffaf) {
        if (!name_hash_pending || site != name_hash_pending_site + 5 || !context->Eax) {
            fprintf(log, "namehash-observation-failed\n"); return;
        }
        name_hash_addresses[name_hash_count] = context->Eax;
        name_hash_sizes[name_hash_count++] = name_hash_pending;
        fprintf(log, "namehash-allocation-return %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
            site, context->Eax, context->Esp, context->Ebx, context->Esi, context->Edi, context->Ebp);
        name_hash_pending = 0;
    } else if (site == 0x40eba0 || site == 0x40eba5) {
        if (!read_bytes(process, context->Esp, args, 8)) { fprintf(log, "namehash-observation-failed\n"); return; }
        fprintf(log, "namehash-arena-init-%s %08lx %08lx %lu %08lx %08lx %08lx %08lx\n", site == 0x40eba0 ? "call" : "return", context->Esp, args[0], args[1], context->Ebx, context->Esi, context->Edi, context->Ebp);
        if (site == 0x40eba5) {
            if (!read_bytes(process, args[0], args, 24)) { fprintf(log, "namehash-observation-failed\n"); return; }
            fprintf(log, "namehash-arena-initialized");
            for (index = 0; index < 6; ++index) fprintf(log, " %08lx", args[index]);
            fputc('\n', log);
        }
    } else if (site == 0x40ebb0) {
        WORD input;
        if (!read_bytes(process, context->Esp, args, 4) || !read_bytes(process, context->Edx, &input, 2)) { fprintf(log, "namehash-observation-failed\n"); return; }
        name_hash_copy_arena = context->Ecx;
        fprintf(log, "namehash-arena-copy-call %08lx %08lx %08lx %lu %04x %08lx %08lx %08lx %08lx\n", context->Esp, context->Ecx, context->Edx, args[0], input, context->Ebx, context->Esi, context->Edi, context->Ebp);
        if (!read_bytes(process, name_hash_copy_arena, args, 24)) { fprintf(log, "namehash-observation-failed\n"); return; }
        fprintf(log, "namehash-arena-before-copy");
        for (index = 0; index < 6; ++index) fprintf(log, " %08lx", args[index]);
        fputc('\n', log);
    } else if (site == 0x40ebb5) {
        fprintf(log, "namehash-arena-copy-return %08lx %08lx %08lx %08lx %08lx %08lx\n", context->Esp, context->Eax, context->Ebx, context->Esi, context->Edi, context->Ebp);
        if (!read_bytes(process, name_hash_copy_arena, args, 24)) { fprintf(log, "namehash-observation-failed\n"); return; }
        fprintf(log, "namehash-arena-after-copy");
        for (index = 0; index < 6; ++index) fprintf(log, " %08lx", args[index]);
        fputc('\n', log);
    } else if (site == 0x410010) {
        if (name_hash_pending || name_hash_count != 5) { fprintf(log, "namehash-observation-failed\n"); return; }
        for (index = 0; index < name_hash_count; ++index) {
            size = index == 4 ? 2 : name_hash_sizes[index]; bytes = malloc(size);
            if (!bytes || !read_bytes(process, name_hash_addresses[index], bytes, size)) {
                free(bytes); fprintf(log, "namehash-observation-failed\n"); return;
            }
            fprintf(log, "namehash-owned-bytes %08lx %lu ", name_hash_addresses[index], size);
            for (offset = 0; offset < size; ++offset) fprintf(log, "%02x", bytes[offset]);
            fputc('\n', log); free(bytes);
        }
    }
}
