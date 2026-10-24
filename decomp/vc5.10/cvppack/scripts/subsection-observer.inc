/* Read-only actual calls of the integer-only directory subsection copier. */
static DWORD subsection_sequence, subsection_inflight, subsection_index;
static DWORD subsection_esp, subsection_return_pc;

static void subsection_observation(FILE *log, HANDLE process, CONTEXT *context, DWORD entry)
{
    DWORD stack[2], fields[3], root, used, padding, i;
    unsigned char *payload;
    if (!read_bytes(process, 0x4153b8, &root, 4) || !root ||
        !read_bytes(process, 0x412d40, &used, 4)) goto failed;
    if (entry == 0x4068a4) {
        if (subsection_inflight || subsection_sequence >= 58 ||
            !read_bytes(process, context->Esp, stack, sizeof(stack)) || stack[0] != 0x406c17 ||
            stack[1] != subsection_sequence || context->Eax != stack[1] ||
            !read_bytes(process, root + stack[1] * 12, fields, sizeof(fields)) || fields[2] > 65512) goto failed;
        for (i = 0; i < main_free_count; ++i) if (main_free_addresses[i] && fields[2] &&
            fields[1] < main_free_addresses[i] + main_free_sizes[i] &&
            main_free_addresses[i] < fields[1] + fields[2]) goto failed;
        for (i = 0; i < source_family_frees; ++i) if (fields[2] &&
            fields[1] < source_family_addresses[i] + source_family_sizes[i] &&
            source_family_addresses[i] < fields[1] + fields[2]) goto failed;
        payload = malloc(fields[2] ? fields[2] : 1);
        if (!payload || (fields[2] && !read_bytes(process, fields[1], payload, fields[2]))) { free(payload); goto failed; }
        subsection_inflight = 1; subsection_index = stack[1];
        subsection_esp = context->Esp; subsection_return_pc = stack[0];
        fprintf(log, "subsection-call %lu %08lx %08lx %lu %lu ", subsection_sequence,
            context->Esp, stack[0], stack[1], used);
        for (i = 0; i < sizeof(fields); ++i) fprintf(log, "%02x", ((unsigned char *)fields)[i]);
        fputc('\n', log);
        fprintf(log, "subsection-input %lu %08lx ", subsection_sequence, fields[1]);
        for (i = 0; i < fields[2]; ++i) fprintf(log, "%02x", payload[i]);
        fputc('\n', log); free(payload);
        fprintf(log, "subsection-registers-before %lu %08lx %08lx %08lx %08lx\n", subsection_sequence,
            context->Ebx, context->Esi, context->Edi, context->Ebp);
    } else {
        if (!subsection_inflight || context->Esp != subsection_esp ||
            !read_bytes(process, context->Esp, stack, 4) || stack[0] != subsection_return_pc ||
            !read_bytes(process, root + subsection_index * 12, fields, sizeof(fields)) ||
            !read_bytes(process, 0x415348, &padding, 4)) goto failed;
        fprintf(log, "subsection-return %lu %08lx %08lx %lu %lu ", subsection_sequence,
            context->Esp, stack[0], used, padding);
        for (i = 0; i < sizeof(fields); ++i) fprintf(log, "%02x", ((unsigned char *)fields)[i]);
        fputc('\n', log);
        fprintf(log, "subsection-registers-after %lu %08lx %08lx %08lx %08lx\n", subsection_sequence,
            context->Ebx, context->Esi, context->Edi, context->Ebp);
        subsection_inflight = 0; ++subsection_sequence;
    }
    return;
failed:
    fprintf(log, "subsection-observation-failed\n");
}
