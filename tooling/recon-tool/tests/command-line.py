#!/usr/bin/env python3
"""Native-only exact raw input, mutable alias and process lifetime control."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
REPO = Path(os.environ.get('RECON_TOOL_TEST_REPO', ROOT.parents[1]))
TOOL = Path(os.environ.get('RECON_TOOL_TEST_EXE', ROOT / 'target/i686-unknown-linux-gnu/debug/recon-tool'))
scratch = Path.home() / '.ai-tmp'
scratch.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='raw-command-line-', dir=scratch) as directory:
    work = Path(directory)
    source = work / 'case.c'
    case = work / 'case.so'
    source.write_text(r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recon-tool.h"
static char *command_line;
static unsigned calls;
void recon_bind_command_line(char *text, size_t bytes)
{
    if (calls++ || !text || bytes != strlen(text) + 1) abort();
    command_line = text;
}
static int setup(void)
{
    const char *expected = getenv("RAW_EXPECTED");
    return expected ? calls == 1 && command_line && !strcmp(command_line, expected) : !calls;
}
static void check_at_exit(void)
{
    if (command_line[0] != 'X') abort();
    printf("exit:%s\n", command_line);
}
static int invoke(int argc, char **argv)
{
    if (!getenv("RAW_EXPECTED")) return 0;
    if (argc != 2 || strcmp(argv[0], "T:\\observed.exe") || strcmp(argv[1], "?")) return 9;
    printf("raw:%s\n", command_line);
    command_line[0] = 'X';
    return atexit(check_at_exit) ? 8 : 0;
}
RECON_EXPORT const ReconBinary binary_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconBinary),
 .name="raw-command-line-control", .original_path="", .original_sha256=""};
RECON_EXPORT const ReconCase case_descriptor = {
 .abi_version=RECON_ABI_VERSION, .struct_size=sizeof(ReconCase),
 .name="raw-command-line", .description="native-only transport control",
 .execution=RECON_IO, .terminal=RECON_EXIT, .io=invoke, .setup=setup};
''')
    subprocess.run(['clang', '-m32', '-shared', '-fPIC', '-DRECON_NATIVE_TEST',
                    '-I' + str(ROOT / 'include'), str(source),
                    str(REPO / 'lib/recon/src/trace.c'), '-ldl', '-o', str(case)], check=True)
    raw = '"R:\\requested.exe"\t "?"  '
    for name, text in [('exact', raw), ('absent', None)]:
        value = dict(args=['T:\\observed.exe', '?'], stdout='text:', stderr='text:',
                     exit_code=0, reject_unexpected_files=True)
        if text is not None:
            value.update(raw_command_line=text, env={'RAW_EXPECTED': text},
                         stdout='text:raw:' + text + '\nexit:X' + text[1:] + '\n')
        original = work / (name + '.json')
        capture = work / (name + '.bin')
        report = work / (name + '-report.json')
        original.write_text(json.dumps(value))
        subprocess.run([TOOL, 'capture', 'pack', original, capture], check=True)
        result = subprocess.run([TOOL, 'run', case, '--capture', capture, '--report', report],
                                capture_output=True, text=True, timeout=5)
        data = json.loads(report.read_text())[0]
        assert result.returncode == 0 and data['pass'], (result.stdout, result.stderr, data)
        assert not Path(data['workspace']).exists()
        print(f'{name}: PASS {data["elapsed_ms"]:.3f} ms')
print('exact raw text before setup, argv separation, mutable exit alias and old-fixture absence PASS')
