/* Analysis-only console/debugger harness. No callback injection: the registered
 * RC handler is entered only after GenerateConsoleCtrlEvent delivers a real console event. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef DBG_CONTROL_BREAK
#define DBG_CONTROL_BREAK 0x40010008UL
#endif
#ifndef DBG_CONTROL_C
#define DBG_CONTROL_C 0x40010005UL
#endif

typedef DWORD (WINAPI *ConsoleProcessList)(DWORD *, DWORD);
static HANDLE proof;
static volatile LONG controller_breaks, controller_interrupts;
static HANDLE controller_event;
struct Breakpoint { DWORD address; BYTE original; int active; };

static void note(char *format, ...)
{
    char text[2048];
    DWORD written;
    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    va_end(args);
    WriteFile(proof, text, strlen(text), &written, 0);
    FlushFileBuffers(proof);
}
static BOOL WINAPI controller_handler(DWORD event)
{
    if (event == CTRL_BREAK_EVENT)
        InterlockedIncrement((LONG *)&controller_breaks);
    if (event == CTRL_C_EVENT) {
        InterlockedIncrement((LONG *)&controller_interrupts);
        SetEvent(controller_event);
    }
    return TRUE;
}
static void require(BOOL condition, char *operation)
{
    if (!condition) {
        note("failure operation=%s error=%lu\n", operation, GetLastError());
        ExitProcess(20);
    }
}
static DWORD read_word(HANDLE process, DWORD address)
{
    DWORD value, done;
    require(ReadProcessMemory(process, (void *)address, &value, 4, &done) && done == 4,
            "ReadProcessMemory DWORD");
    return value;
}
static void restore(HANDLE process, struct Breakpoint *point)
{
    DWORD done;
    if (!point->active)
        return;
    require(WriteProcessMemory(process, (void *)point->address, &point->original, 1, &done) && done == 1,
            "restore breakpoint");
    FlushInstructionCache(process, (void *)point->address, 1);
    point->active = 0;
}
static void install(HANDLE process, struct Breakpoint *point, DWORD address)
{
    BYTE trap = 0xcc;
    DWORD done;
    point->address = address;
    require(ReadProcessMemory(process, (void *)address, &point->original, 1, &done) && done == 1,
            "read breakpoint byte");
    /* An existing INT3 is a logical reconstruction frontier, never bypassed. */
    point->active = point->original != trap;
    if (point->active) {
        require(WriteProcessMemory(process, (void *)address, &trap, 1, &done) && done == 1,
                "install breakpoint");
        FlushInstructionCache(process, (void *)address, 1);
    }
}
static void snapshot(char *directory, char *destination)
{
    WIN32_FIND_DATAA data;
    HANDLE search;
    char pattern[1024], input[1024], output[1024];
    sprintf(pattern, "%s\\*", directory);
    search = FindFirstFileA(pattern, &data);
    require(search != INVALID_HANDLE_VALUE, "enumerate boundary files");
    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            sprintf(input, "%s\\%s", directory, data.cFileName);
            sprintf(output, "%s\\%s", destination, data.cFileName);
            require(CopyFileA(input, output, TRUE), "snapshot boundary file");
            note("before_file name=%s bytes=%lu\n", data.cFileName, data.nFileSizeLow);
        }
    } while (FindNextFileA(search, &data));
    FindClose(search);
}
int main(int argc, char **argv)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DEBUG_EVENT event;
    CONTEXT context;
    struct Breakpoint registration, returned, lexer, handler;
    ConsoleProcessList console_processes;
    HANDLE out, err, events;
    DWORD base = 0, status, address, control, done, code, members[64], count, index;
    DWORD requested_group = 0, requested_event, creation_flags, thread_ids[64], thread_count = 1;
    HANDLE thread_handles[64];
    int loader_break = 0, registered = 0, generated = 0, reached_handler = 0;
    int has_controller, has_target, context_repaired, send_after_continue = 0;
    int waiting_delivery = 0;
    char command[2048], line[256];
    if (argc != 13) return 2;
    requested_event = !strcmp(argv[12], "ctrl-c") ? CTRL_C_EVENT : CTRL_BREAK_EVENT;
    creation_flags = DEBUG_ONLY_THIS_PROCESS;
    if (requested_event == CTRL_BREAK_EVENT) creation_flags |= CREATE_NEW_PROCESS_GROUP;
    memset(&registration, 0, sizeof(registration));
    memset(&returned, 0, sizeof(returned));
    memset(&lexer, 0, sizeof(lexer));
    memset(&handler, 0, sizeof(handler));
    proof = CreateFileA(argv[10], GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    require(proof != INVALID_HANDLE_VALUE, "open proof");
    FreeConsole();
    require(AllocConsole(), "AllocConsole");
    require(SetConsoleCtrlHandler(NULL, FALSE), "enable inherited Ctrl+C handling");
    controller_event = CreateEventA(0, TRUE, FALSE, 0);
    require(controller_event != 0, "controller event");
    require(SetConsoleCtrlHandler(controller_handler, TRUE), "controller handler");
    note("console=real controller_pid=%lu\n", GetCurrentProcessId());
    console_processes = (ConsoleProcessList)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetConsoleProcessList");
    require(console_processes != 0, "GetConsoleProcessList available");
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    out = CreateFileA(argv[3], GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, 0);
    err = CreateFileA(argv[4], GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, 0);
    events = CreateFileA(argv[5], GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    require(out != INVALID_HANDLE_VALUE && err != INVALID_HANDLE_VALUE && events != INVALID_HANDLE_VALUE,
            "open streams");
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = out;
    si.hStdError = err;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    sprintf(command, "\"%s\" %s", argv[1], argv[2]);
    require(CreateProcessA(argv[1], command, 0, 0, TRUE,
                          creation_flags, 0, argv[6], &si, &pi),
            "CreateProcess console child");
    if (requested_event == CTRL_BREAK_EVENT) requested_group = pi.dwProcessId;
    thread_ids[0] = pi.dwThreadId;
    thread_handles[0] = pi.hThread;
    note("target_pid=%lu primary_tid=%lu group=%lu flags=%08lx\n", pi.dwProcessId,
         pi.dwThreadId, requested_group, creation_flags);
    note("group_semantics=%s event=%lu\n", requested_group ? "new-target-pid" : "console-local-broadcast", requested_event);
    for (;;) {
        require(WaitForDebugEvent(&event, 30000), "WaitForDebugEvent");
        status = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            base = (DWORD)event.u.CreateProcessInfo.lpBaseOfImage;
            install(pi.hProcess, &registration, base + strtoul(argv[7], 0, 16));
            install(pi.hProcess, &lexer, base + strtoul(argv[8], 0, 16));
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
        } else if (event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            if (waiting_delivery) {
                require(SuspendThread(pi.hThread) == 0, "resuspend primary during OS callback creation");
                waiting_delivery = 0;
                note("primary_resuspended=1 boundary=OS_callback_thread_creation\n");
            }
            note("target_thread=%lu start=%08lx\n", event.dwThreadId,
                 (DWORD)event.u.CreateThread.lpStartAddress);
            require(thread_count < 64, "thread handle capacity");
            thread_ids[thread_count] = event.dwThreadId;
            thread_handles[thread_count++] = event.u.CreateThread.hThread;
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            code = event.u.Exception.ExceptionRecord.ExceptionCode;
            address = (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code == DBG_CONTROL_BREAK || code == DBG_CONTROL_C) {
                note("control_exception=%08lx tid=%lu disposition=not_handled\n", code, event.dwThreadId);
                status = DBG_EXCEPTION_NOT_HANDLED;
            } else if (code == EXCEPTION_BREAKPOINT && !loader_break) {
                loader_break = 1;
            } else if (code == EXCEPTION_BREAKPOINT &&
                       ((registration.active && address == registration.address) ||
                        (returned.active && address == returned.address) ||
                        (lexer.active && address == lexer.address) || address == handler.address)) {
                HANDLE thread = 0;
                for (index = 0; index != thread_count; ++index)
                    if (thread_ids[index] == event.dwThreadId) thread = thread_handles[index];
                require(thread != 0, "debugger thread handle");
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                require(GetThreadContext(thread, &context), "GetThreadContext");
                context_repaired = 0;
                if (address == registration.address) {
                    require(event.dwThreadId == pi.dwThreadId && read_word(pi.hProcess, context.Esp+8) == 1,
                            "native TRUE registration");
                    control = read_word(pi.hProcess, context.Esp+4);
                    require(control == base + strtoul(argv[9], 0, 16), "native handler identity");
                    install(pi.hProcess, &handler, control);
                    install(pi.hProcess, &returned, read_word(pi.hProcess, context.Esp));
                    restore(pi.hProcess, &registration);
                    note("registration handler_rva=%08lx tid=%lu\n", control-base, event.dwThreadId);
                } else if (address == returned.address) {
                    require(context.Eax == 1, "SetConsoleCtrlHandler succeeded");
                    registered = 1;
                    restore(pi.hProcess, &returned);
                    note("registration_return=TRUE\n");
                } else if (address == lexer.address) {
                    require(registered && event.dwThreadId == pi.dwThreadId, "registered pre-lexer boundary");
                    /* Keep the lexer entry trapped: Wine's primary thread must
                     * run briefly to dispatch SIGINT, but may not enter RC code.
                     * CREATE_THREAD and this repeat trap may arrive either first.
                     */
                    if (generated) {
                        if (waiting_delivery) {
                            require(SuspendThread(pi.hThread) == 0, "suspend primary at repeated lexer barrier");
                            waiting_delivery = 0;
                            note("primary_resuspended=1 boundary=repeated_lexer_barrier\n");
                        } else note("pending_primary_breakpoint=lexer barrier_already_suspended=1\n");
                        context.Eip = address;
                        require(SetThreadContext(thread, &context), "restore repeated lexer barrier EIP");
                        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
                        continue;
                    }
                    context.Eip = address;
                    require(SetThreadContext(thread, &context), "restore lexer breakpoint EIP");
                    context_repaired = 1;
                    require(SuspendThread(pi.hThread) == 0, "suspend primary thread once");
                    note("boundary=before_initialize_lexer primary_suspended=1\n");
                    snapshot(argv[6], argv[11]);
                    count = console_processes(members, 64);
                    require(count > 0 && count <= 64, "console process membership");
                    has_controller = has_target = 0;
                    for (index = 0; index != count; ++index) {
                        note("console_member=%lu\n", members[index]);
                        if (members[index] == GetCurrentProcessId()) has_controller = 1;
                        if (members[index] == pi.dwProcessId) has_target = 1;
                    }
                    require(count == 2 && has_controller && has_target, "isolated two-process real console");
                    send_after_continue = 1;
                } else {
                    require(generated && event.dwThreadId != pi.dwThreadId, "signal-created callback thread");
                    control = read_word(pi.hProcess, context.Esp+4);
                    require(control == requested_event, "native callback received requested event");
                    reached_handler = 1;
                    note("callback pid=%lu tid=%lu argument=%lu handler_rva=%08lx\n",
                         event.dwProcessId, event.dwThreadId, control, address-base);
                    if (!handler.active) {
                        sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                                code, address-base, base, event.u.Exception.dwFirstChance);
                        WriteFile(events, line, strlen(line), &done, 0);
                        TerminateProcess(pi.hProcess, 99);
                        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
                        continue;
                    }
                    restore(pi.hProcess, &handler);
                }
                if (!context_repaired) {
                    context.Eip = address;
                    require(SetThreadContext(thread, &context), "restore breakpoint EIP");
                }
            } else if (code != EXCEPTION_BREAKPOINT && event.u.Exception.dwFirstChance) {
                HANDLE failed_thread = 0;
                DWORD stack[128], frame;
                for (index = 0; index != thread_count; ++index)
                    if (thread_ids[index] == event.dwThreadId) failed_thread = thread_handles[index];
                memset(&context, 0, sizeof(context));
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                if (failed_thread && GetThreadContext(failed_thread, &context) &&
                    ReadProcessMemory(pi.hProcess, (void *)context.Esp, stack, sizeof(stack), &done)) {
                    note("exception_context esp=%08lx ebp=%08lx eax=%08lx\n",context.Esp,context.Ebp,context.Eax);
                    for (index=0; index<128; ++index) note("stack%lu=%08lx\n",index,stack[index]);
                    frame=context.Ebp;
                    for (index=0; index<12 && frame; ++index) {
                        note("frame%lu=%08lx caller=%08lx arg=%08lx\n",index,frame,read_word(pi.hProcess,frame+4),read_word(pi.hProcess,frame+8));
                        frame=read_word(pi.hProcess,frame);
                    }
                }

                note("first_chance_exception=%08lx address=%08lx tid=%lu disposition=not_handled\n",
                     code, address, event.dwThreadId);
                status = DBG_EXCEPTION_NOT_HANDLED;
            } else {
                sprintf(line, "exception %08lx rva %08lx base %08lx first %lu\n",
                        code, address-base, base, event.u.Exception.dwFirstChance);
                WriteFile(events, line, strlen(line), &done, 0);
                TerminateProcess(pi.hProcess, 99);
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            if (generated && requested_event == CTRL_C_EVENT)
                require(WaitForSingleObject(controller_event, 5000) == WAIT_OBJECT_0, "controller received console-local Ctrl+C broadcast");
            note("exit=%lu generated=%d reached_handler=%d controller_breaks=%ld controller_interrupts=%ld\n",
                 event.u.ExitProcess.dwExitCode, generated, reached_handler, controller_breaks, controller_interrupts);
            require(controller_breaks == 0, "break signal did not reach controller");
            require(controller_interrupts == (generated && requested_event == CTRL_C_EVENT ? 1 : 0), "controller broadcast count");
            sprintf(line, "exit %lu\n", event.u.ExitProcess.dwExitCode);
            WriteFile(events, line, strlen(line), &done, 0);
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
            break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, status);
        if (send_after_continue) {
            require(GenerateConsoleCtrlEvent(requested_event, requested_group), "GenerateConsoleCtrlEvent");
            generated = 1;
            send_after_continue = 0;
            note("signal=%s group=%lu api=GenerateConsoleCtrlEvent success=1\n",
                 requested_event == CTRL_C_EVENT ? "CTRL_C_EVENT" : "CTRL_BREAK_EVENT", requested_group);
            if (requested_event == CTRL_C_EVENT) {
                waiting_delivery = 1;
                require(ResumeThread(pi.hThread) == 1, "allow pending native console signal delivery");
                note("primary_resumed=1 reason=deliver_pending_console_signal\n");
            }
        }
    }
    for (index = 1; index != thread_count; ++index) CloseHandle(thread_handles[index]);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(events);
    CloseHandle(out);
    CloseHandle(err);
    CloseHandle(controller_event);
    CloseHandle(proof);
    FreeConsole();
    return 0;
}
