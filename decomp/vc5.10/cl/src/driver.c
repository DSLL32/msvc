/* Driver initialization helpers. */
#include "cl.h"
#include "mre.h"
#include <recon.h>

int first_dependency_scan = 1;
int first_stage_notice = 1;
static char mre_error[1024];
/* 004097a8..004097b4: shared with the console-control handler. */
int interrupted, pass_running;
CompilerPass *active_pass;

/* 004054ec..0040550d: release the loaded pass and invalidate its cached exports. */
void unload_pass(CompilerPass *pass)
{
    if (pass->module) {
        FreeLibrary(pass->module);
        pass->module = NULL;
        pass->invoke = NULL;
        pass->abort = NULL;
    }
}

/* 0040550d..004057b4: DLL passes share this process's CRT; EXEs are children. */
int execute_pass(int mode, CompilerPass *pass, char *name, char *environment, char **args)
{
    char **argument, *assignment, *out;
    char *child_args[2];
    unsigned bytes;
    int argc, status;
    HMODULE module;
    InvokeCompilerPass invoke;
    AbortCompilerPass abort;

    if (mode) {
        if (mode < 1 || mode > 3)
            fatal(0);
    } else if (environment) {
        bytes = strlen(environment) + 2;
        if (!(pass->flags & 4))
            for (argument = args + 1; *argument; ++argument)
                bytes += strlen(*argument) + 1;
        assignment = allocate(bytes);
        out = append_text(assignment, environment);
        if (!(pass->flags & 4)) {
            for (argument = args + 1; *argument; ++argument) {
                out = append_text(out, *argument);
                *out++ = ' ';
            }
        }
        *out = 0;
        if (_putenv(assignment))
            fatal(2029, name);
        release(assignment);
        if (!(pass->flags & 4)) {
            child_args[0] = args[0];
            child_args[1] = NULL;
            args = child_args;
        }
    }
    _flushall();
    errno = 0;
    interrupted = 0;
    active_pass = pass;
    if (pass->flags & 4) {
        if (!pass->invoke) {
            module = LoadLibraryA(name);
            if (module) {
                invoke = (InvokeCompilerPass)GetProcAddress(module, "_InvokeCompilerPass@12");
                abort = (AbortCompilerPass)GetProcAddress(module, "_AbortCompilerPass@4");
                if (invoke && abort) {
                    pass->module = module;
                    pass->invoke = invoke;
                    pass->abort = abort;
                } else
                    FreeLibrary(module);
            }
            if (!pass->invoke) {
                fatal(2027, name);
                status = 4;
                goto finished;
            }
        }
        for (argc = 0; args[argc]; ++argc)
            ;
        pass_running = 1;
        __try {
            status = pass->invoke(argc, args, (pass->flags >> 3) & 1);
        } __except (!debug_exceptions && GetExceptionCode() != EXCEPTION_BREAKPOINT) {
            status = 4;
        }
        pass_running = 0;
        if (status || (pass->flags & 8))
            unload_pass(pass);
    } else {
        pass_running = 1;
        status = _spawnvp(_P_WAIT, name, (const char *const *)args);
        pass_running = 0;
    }
finished:
    if (interrupted || (status & 0xff00) == 0x8000) {
        driver_exit(4);
        fatal(0);
    }
    if (status < 0 && errno) {
        fatal(2027, name);
        return 4;
    }
    if (status) {
        if (status == 525)
            fatal(2030, name);
        return 2;
    }
    return 0;
}

/* 00403e5d..0040426b: build pass argv and invoke its DLL or executable. */
int final_pass(Driver *state)
{
    char *temporary, *value, *storage, *out, *name, *prefix, *include, *end;
    char **args, **slot, **argument;
    char *(*copy)(char *, const char *);
    CompilerPass *pass;
    Input *input;
    Option *option;
    int count = 0, bytes, status = 0;
    output_produced = 1;
    temporary = allocate(1024);
    value = allocate(1024);
    pass = state->current_pass;
    input = state->current_input;
    name = pass->name;
    prefix = pass->options;
    bytes = quoted_length(name) + 1;
    if (prefix) {
        bytes += quoted_length(prefix) + 1;
        count = 1;
    }
    for (option = state->options; option; option = option->next) {
        if (!option_for_pass(option, pass))
            continue;
        ++count;
        bytes += expanded_length(option->name, state) + 2;
        if (option->value) {
            ++count;
            bytes += expanded_length(option->value, state) + 2;
        }
    }
    if (state->include_path && use_standard_include) {
        count += 2;
        bytes += quoted_length(state->include_path) + 6;
        for (end = (char *)_mbschr((unsigned char *)state->include_path, ';'); end;
             end = (char *)_mbschr((unsigned char *)(end + 1), ';')) {
            count += 2;
            bytes += 6;
        }
    }
    args = allocate(count * sizeof(char *) + 2 * sizeof(char *));
    storage = allocate(bytes);
    copy = (pass->flags & 4) ? strcpy : quote_argument;
    args[0] = storage;
    if (prefix) {
        quote_argument(temporary, prefix);
        out = append_text(storage, temporary) + 1;
        args[1] = out;
        slot = args + 2;
        copy(temporary, name);
        out = append_text(out, temporary);
    } else {
        slot = args + 1;
        quote_argument(temporary, name);
        out = append_text(storage, temporary);
    }
    for (option = state->options; option; option = option->next) {
        if (!option_for_pass(option, pass))
            continue;
        *slot++ = ++out;
        if (!(option->flags & 8))
            *out++ = pass->option_prefix[1];
        out = append_text(out, expand_filename(temporary, option->name, state));
        if (option->value) {
            if (!(option->flags & 4))
                *slot++ = ++out;
            copy(temporary, expand_filename(value, option->value, state));
            out = append_text(out, temporary);
        }
    }
    if (state->include_path && use_standard_include && strchr("1P", pass->option_prefix[0])) {
        include = state->include_path;
        while ((end = (char *)_mbschr((unsigned char *)include, ';')) != NULL) {
            if (end != include && _mbsspn((unsigned char *)include, (unsigned char *)" \t") < (unsigned)(end - include)) {
                *slot++ = ++out;
                out = append_text(out, "-I");
                *slot++ = ++out;
                *end = 0;
                copy(temporary, include);
                *end = ';';
                out = append_text(out, temporary);
            }
            include = end + 1;
        }
        if (_mbsspn((unsigned char *)include, (unsigned char *)" \t") < strlen(include)) {
            *slot++ = ++out;
            out = append_text(out, "-I");
            *slot++ = ++out;
            copy(temporary, include);
            append_text(out, temporary);
        }
    }
    *slot = NULL;
    if (debug_exceptions) {
        write_message(2, "`");
        argument = args;
        if (prefix) {
            write_message(2, "%s ", prefix);
            argument = args + 1;
        }
        write_message(2, "%s", name);
        while (*++argument)
            write_message(2, " %s", *argument);
        write_message(2, "'\n");
    }
    if (!bz_requested) {
        if (timing_requested)
            start_timer();
        if (prefix)
            status = execute_pass(3, pass, prefix, NULL, args);
        else
            status = execute_pass(input->kind == 9 ? 2 : 0, pass, name, pass->environment, args);
        if (timing_requested)
            stop_timer(name);
    }
    release(args);
    release(storage);
    release(value);
    release(temporary);
    return status;
}

/* 00402e6b..00402fd1: locate each compiler pass once, preferring DLLs. */
void configure_pass(Driver *state)
{
    CompilerPass *pass = state->current_pass;
    char *name, *temporary, *directory;
    char extension[260];
    int found;
    if (pass->flags & 2)
        return;
    pass->flags |= 2;
    name = find_file(state->directory, pass->name, &found);
    if (found) {
        _splitpath(name, NULL, NULL, NULL, extension);
        if (!_stricmp(extension, ".dll"))
            pass->flags |= 4;
    } else {
        release(name);
        temporary = allocate(strlen(pass->name) + 5);
        strcpy(temporary, pass->name);
        strcat(temporary, ".dll");
        name = find_file(state->directory, temporary, &found);
        if (found)
            pass->flags |= 4;
        else {
            release(name);
            strcpy(temporary + strlen(temporary) - 4, ".exe");
            name = find_file(state->directory, temporary, &found);
            if (!found) {
                release(name);
                name = duplicate(pass->name);
            }
        }
        release(temporary);
    }
    pass->name = name;
    if (pass->path) {
        directory = program_directory(name);
        pass->path = find_file(directory, pass->path, &found);
        release(directory);
    }
    if (pass->options)
        pass->options = find_file(state->directory, pass->options, &found);
}

/* 00403e07..00403e5d: execute enabled passes until one fails. */
int run_passes(int *passes, Driver *state, int (*callback)(Driver *))
{
    CompilerPass *pass;
    for (; *passes != 5; ++passes) {
        pass = &compiler_passes[*passes];
        if (!(pass->flags & 1))
            continue;
        state->current_pass = pass;
        pass->flags |= 8;
        configure_pass(state);
        error_count = callback(state);
        if (error_count)
            break;
    }
    return error_count;
}

/* 00403bb7..00403df0: run a source group together or one file at a time. */
int run_jobs(Driver *state, Job *jobs)
{
    int failures = 0, enabled = 0, stages = 0, together, result, *sequence, *pass;
    Job *job;
    Input *input, *object;
    char filename[1024];
    sequence = input_kinds[jobs->input->kind].passes;
    for (pass = sequence; *pass != 5; ++pass)
        if (compiler_passes[*pass].flags & 1)
            ++enabled;
    together = allow_pch && cleanup_temporaries && enabled > 1;
    state->current_jobs = together ? jobs : NULL;
    pass = sequence;
    do {
        if (together) {
            if (!(compiler_passes[*pass].flags & 1)) {
                ++pass;
                continue;
            }
            if (stages++ > 0) {
                jobs = advance_jobs(jobs);
                state->current_jobs = jobs;
            }
            state->current_pass = &compiler_passes[*pass];
            state->current_input = jobs->input;
            if (stages == 1) {
                if (first_stage_notice)
                    first_stage_notice = 0;
                else
                    write_message(2, load_string(331));
            } else if (stages == 2 && enabled > 2)
                write_message(2, load_string(332));
            else if (stages == enabled)
                write_message(2, load_string(333));
        }
        for (job = jobs; job; job = job->next) {
            if (together && job->status)
                continue;
            input = job->input;
            state->current_input = input;
            prepare_inputs(state);
            if (!together || stages <= 1)
                write_message(2, "%s\n", file_name(filename, input->name));
            if (together) {
                configure_pass(state);
                if (!job->next)
                    state->current_pass->flags |= 8;
                else
                    state->current_pass->flags &= ~8;
                result = final_pass(state);
            } else
                result = run_passes(sequence, state, final_pass);
            error_count = result;
            if (result) {
                ++failures;
                job->status = 1;
                if (together)
                    cleanup_files(1);
            }
            if (!together || stages == enabled) {
                if (object_name) {
                    object = create_input(object_name, 3);
                    object->next = input->next;
                    input->next = object;
                }
                cleanup_files(1);
            }
        }
        if (together)
            ++pass;
    } while (together && *pass != 5);
    state->current_jobs = NULL;
    return failures;
}

/* 0040355e..00403709: optional minimal-rebuild dependency scan. */
Job *prepare_jobs(Driver *state, Job *jobs, PendingWork *pending)
{
    Mre *engine;
    MreDriver *mre;
    int error;
    unsigned timestamp;
    Job *job, *result;
    Input *input;
    DeferredInput *tail = NULL, *node;
    if (!minimal_rebuild || !first_dependency_scan) {
        first_dependency_scan = 0;
        return jobs;
    }
    if (!MREFOpenByName(&engine, pdb_name, &error, mre_error, reproducible, TRUE)) {
        warning(4028);
        first_dependency_scan = 0;
        return jobs;
    }
    MREQueryMreDrv(engine, &mre);
    MREDrvOneTimeInit(mre);
    pending->outdated = NULL;
    pending->sources = NULL;
    pending->unrecovered = 0;
    pending->recycled = NULL;
    pending->unrecovered_tail = 0;
    state->deferred_inputs = NULL;
    state->deferred_tail = &state->deferred_inputs;
    timestamp = file_timestamp(state);
    for (job = jobs; job; job = job->next) {
        input = job->input;
        state->current_input = input;
        prepare_inputs(state);
        node = &job->deferred;
        node->next = NULL;
        if (!tail)
            pending->sources = node;
        else
            tail->next = node;
        tail = node;
        node->is_cpp = input->kind == 1 || input->kind == 2;
        node->source_name = input->name;
        node->object_name = duplicate(object_name);
        node->timestamp = timestamp;
        job->browser_name = duplicate(browser_name);
    }
    if (first_dependency_scan && MREDrvFFilesOutOfDate(mre, pending))
        result = jobs_from_deferred(pending->outdated);
    else {
        if (first_dependency_scan) {
            warning(4028);
            first_dependency_scan = 0;
        }
        result = jobs;
    }
    if (!MREDrvFRelease(mre) && first_dependency_scan) {
        warning(4028);
        first_dependency_scan = 0;
    }
    if (!MREFClose(engine, first_dependency_scan) && first_dependency_scan) {
        warning(4028);
        first_dependency_scan = 0;
    }
    return result;
}

/* 00403547..0040355e: only the three accessed prefix words are initialized. */
Job *create_job(Input *input)
{
    Job *job = allocate(sizeof(Job));
    job->next = NULL;
    job->status = 0;
    job->input = input;
    return job;
}

/* 004034fd..00403547: build jobs for source inputs, retaining source order. */
Job *source_jobs(Driver *state)
{
    Input *input;
    Job *head = NULL, *tail = NULL, *job;
    for (input = state->inputs; input; input = input->next) {
        if (!input_kinds[input->kind].category) {
            job = create_job(input);
            if (!head)
                head = job;
            else
                tail->next = job;
            tail = job;
        }
    }
    if (head == tail)
        cleanup_temporaries = 0;
    return head;
}

/* 0040332c..004034fd: group source jobs, drain pending work, then link. */
void run_driver(Driver *state)
{
    int status = 0, *passes;
    Job *jobs, *last, *next;
    Input *input, *image;
    DeferredInput *deferred, *following;
    PendingWork pending;
    char filename[1024];

    jobs = prepare_jobs(state, source_jobs(state), &pending);
    do {
        while (jobs) {
            do {
                last = jobs;
                passes = input_kinds[jobs->input->kind].passes;
                jobs->input->sequence = 0;
                next = last->next;
                while (next && input_kinds[next->input->kind].passes == passes &&
                       last->input->sequence < 259) {
                    next->input->sequence = last->input->sequence + 1;
                    last = next;
                    next = next->next;
                }
                last->next = NULL;
                status |= run_jobs(state, jobs);
                jobs = next;
            } while (jobs);
            jobs = next_jobs(state, &pending);
        }
        jobs = next_jobs(state, &pending);
    } while (jobs);

    if (state->deferred_inputs) {
        write_message(2, load_string(330));
        deferred = state->deferred_inputs;
        while (deferred) {
            following = deferred->next;
            input = CONTAINING_RECORD(deferred, Job, deferred)->input;
            write_message(2, "%s\n", file_name(filename, input->name));
            deferred->next = pending.recycled;
            pending.recycled = deferred;
            deferred = following;
        }
    }
    if (status) {
        error_count = status;
        return;
    }
    for (input = state->inputs; input && input->kind != 3; input = input->next)
        ;
    state->current_input = input ? input : state->inputs;
    prepare_inputs(state);
    error_count = run_passes(input_kinds[3].passes, state, link_pass);
    if (error_count)
        return;
    if (image_name) {
        image = create_input(image_name, 9);
        image->next = state->inputs->next;
        state->inputs->next = image;
    }
    for (input = state->inputs; input; input = input->next) {
        if (input_kinds[input->kind].category == 2) {
            state->current_input = input;
            error_count = run_passes(input_kinds[input->kind].passes, state, final_pass);
            if (error_count)
                return;
        }
    }
}

/* 004023bb..004023c4: use the CRT's complete program path. */
char *program_name(char *name)
{
    return _pgmptr;
}

/* 004022e9..00402319 */
void initialize_driver(char *name, Driver *state)
{
    memset(state, 0, sizeof(Driver));
    state->directory = program_directory(name);
    state->include_path = getenv("INCLUDE");
}

/* 00402de5..00402e6b: retain a separator on the compiler directory. */
char *program_directory(char *name)
{
    char drive_and_directory[_MAX_PATH];
    char directory[_MAX_DIR], filename[_MAX_FNAME], extension[_MAX_EXT];
    int length;
    _splitpath(name, drive_and_directory, directory, filename, extension);
    length = strlen(directory);
    if (length > 0 && directory[length - 1] != '\\' &&
        directory[length - 1] != ':' && directory[length - 1] != '/') {
        directory[length] = '\\';
        directory[length + 1] = '\0';
    }
    return duplicate(strcat(drive_and_directory, directory));
}

/* 004054c5..004054ec */
char *duplicate(char *text)
{
    char *copy;
    if (!text)
        return NULL;
    copy = _strdup(text);
    if (!copy)
        fatal(0);
    return copy;
}

/* 00405495..004054b2: all driver allocations share the fatal-error path. */
void *allocate(unsigned bytes)
{
    void *memory = malloc(bytes);
    if (!memory)
        fatal(0);
    return memory;
}

/* 004054b2..004054c5 */
void release(void *memory)
{
    if (memory)
        free(memory);
}
