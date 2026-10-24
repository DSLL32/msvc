/* Read-only capture of the writer's new output-buffer allocation generation. */
static DWORD buffer_attempts, buffer_pending, buffer_esp;

static void output_buffer_allocation(FILE *log, HANDLE process, CONTEXT *context, DWORD entry)
{
    DWORD arguments[4];
    if (entry == 0x4100f3) {
        if (buffer_pending || buffer_attempts >= 8 || main_free_count != 12 ||
            source_family_frees != 2 || !read_bytes(process, context->Esp, arguments, sizeof(arguments)) ||
            arguments[2] != 0x406b84) goto failed;
        buffer_pending = 1; buffer_esp = context->Esp;
        fprintf(log, "output-buffer-allocation-call %lu %08lx %lu %08lx %lu %08lx\n",
            buffer_attempts, context->Esp, arguments[0], context->Esi, arguments[3], arguments[2]);
        sort_environment(log, process, context, "buffer-call");
    } else {
        if (!buffer_pending || context->Esp != buffer_esp) goto failed;
        fprintf(log, "output-buffer-allocation-return %lu %08lx %08lx %08lx\n",
            buffer_attempts, context->Esp, context->Eax, context->Esi);
        sort_environment(log, process, context, "buffer-return");
        buffer_pending = 0; ++buffer_attempts;
    }
    return;
failed:
    fprintf(log, "output-buffer-observation-failed\n");
}

static void output_buffer_state(FILE *log, HANDLE process, CONTEXT *context)
{
    DWORD fields[4], i;
    unsigned char bytes[8];
    if (buffer_pending || !buffer_attempts || !read_bytes(process, 0x412d34, fields, sizeof(fields)) ||
        fields[3] != 8 || !fields[1] || fields[0] < fields[3] ||
        !read_bytes(process, fields[1], bytes, sizeof(bytes))) {
        fprintf(log, "output-buffer-observation-failed\n"); return;
    }
    fprintf(log, "output-buffer-state %08lx %lu %lu %lu ", fields[1], fields[0], fields[3], fields[2]);
    for (i = 0; i < sizeof(bytes); ++i) fprintf(log, "%02x", bytes[i]);
    fputc('\n', log);
    sort_environment(log, process, context, "buffer-frontier");
}
