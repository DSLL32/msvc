#!/usr/bin/env python3
"""Capture a Linux GDB frontier without invoking Wine or a PE helper."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import time

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path.home() / ".ai-tmp/msvc2/vc5.10/c1xx"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    args = parser.parse_args()
    if not args.name.replace("-", "").replace("_", "").isalnum():
        raise ValueError("invalid fresh capture name")
    output = SCRATCH / args.name
    output.mkdir(parents=True, exist_ok=False)
    binary = PROJECT / "build/native/c1xx-host"
    candidate_path = json.loads((PROJECT / "functions.json").read_text()).get("candidate_path", "cases/lifecycle/candidate.json")
    inputs = [candidate_path, "functions.json", "runtime.json",
              "graph-metadata.json", "build/c1n32_l.dll", "build/c1xx.map",
              "build/runtime.json", "build/trial.json"]
    snapshot_inputs = {}
    for item in inputs:
        path = PROJECT / item
        destination = "rebuilt-runtime.json" if item == "build/runtime.json" else path.name
        (output / destination).write_bytes(path.read_bytes())
        snapshot_inputs[item] = sha(path)
    native_sha = sha(binary)
    (output / "native-executable.elf").write_bytes(binary.read_bytes())
    provenance = PROJECT / "build/native/provenance.json"
    build = json.loads(provenance.read_text())
    if build["artifact_sha256"] != native_sha:
        raise ValueError("native build provenance belongs to another executable")
    (output / "build-provenance.json").write_bytes(provenance.read_bytes())
    content = output / "source-content"
    content.mkdir()
    pending, copied = list(build["source_content"].values()), set()
    while pending:
        digest = pending.pop()
        if digest in copied:
            continue
        if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            raise ValueError("invalid source content identity")
        copied.add(digest)
        source = PROJECT / "build/source-content" / digest
        if sha(source) != digest:
            raise ValueError("compiled source content changed")
        (content / digest).write_bytes(source.read_bytes())
        try:
            nested = json.loads(source.read_bytes())
        except (UnicodeDecodeError, ValueError):
            continue
        if isinstance(nested, dict) and "source_content" in nested:
            pending.extend(nested["source_content"].values())
    candidate = json.loads((PROJECT / candidate_path).read_text())
    minimal = candidate['case'] == 'minimal'
    completion = False
    if minimal:
        spec = importlib.util.spec_from_file_location('minimal_capture',Path(__file__).with_name('capture-minimal-native.py'))
        adapter = importlib.util.module_from_spec(spec);spec.loader.exec_module(adapter)
        result,elapsed = adapter.capture(PROJECT,output,binary,json.loads((output/'trial.json').read_text()))
    else:
        completion = candidate['trial'] == '_DllMain@12' or (candidate['trial'] is None and '_DllMain@12' in candidate['accepted'])
        if completion:
            environment = os.environ.copy()
            environment['RECON_LIFECYCLE_STATE'] = str(output / 'lifecycle-state.jsonl')
            (output / 'filter.txt').write_text('DllMain\n')
            (output / 'seen.txt').write_text('')
            environment['RECON_TRACE_FILTER'] = str(output / 'filter.txt')
            environment['RECON_TRACE_SEEN'] = str(output / 'seen.txt')
            with (output / 'trace.txt').open('wb') as trace:
                environment['RECON_TRACE_FD'] = str(trace.fileno())
                started = time.perf_counter()
                result = subprocess.run([str(binary)], cwd=PROJECT, env=environment, pass_fds=(trace.fileno(),),
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
                elapsed = time.perf_counter() - started
            (output / 'stdout.bin').write_bytes(result.stdout)
            (output / 'stderr.bin').write_bytes(result.stderr)
            records = [json.loads(line) for line in (output / 'lifecycle-state.jsonl').read_text().splitlines()]
            if result.returncode != 0 or len(records) != 2:
                raise ValueError('native lifecycle did not really complete both returns')
            (output / 'state.json').write_text(json.dumps({'registration':records[0]['registration'],
                'registered_token':records[0]['module'],'checkpoints':records},indent=2)+'\n')
        else:
            provider_break = candidate["trial"] == "__CRT_INIT@12"
            breakpoint = "break *recon_initterm\n" if provider_break else ""
            commands = """set pagination off
        set confirm off
        %s\
        run > %s 2> %s
        bt
        python
        import gdb,json,struct
        frame=gdb.newest_frame()
        if frame.name()=='recon_initterm':
            raw=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$esp')),12))
            stack=struct.unpack('<III',raw)
            args={'begin':stack[1],'end':stack[2]}
        else:
            args={name:int(frame.read_var(name)) for name in ('module','reason','reserved')}
        value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args}
        if frame.name()=='recon_initterm': value['raw_stack']=list(stack)
        value['control_word']=int(gdb.parse_and_eval('$fctrl'))
        value['target_state']={'process_attached':int(gdb.parse_and_eval('process_attached')),
            'raw_dll_entry':int(gdb.parse_and_eval('(unsigned int)raw_dll_entry')),
            'dll_adjust_fdiv':int(gdb.parse_and_eval('dll_adjust_fdiv')),
            'onexit_begin':int(gdb.parse_and_eval('(unsigned int)onexit_begin')),
            'onexit_end':int(gdb.parse_and_eval('(unsigned int)onexit_end')),
            'initializer_begin':int(gdb.parse_and_eval('(unsigned int)crt_initializers')),
            'provider_adjust_fdiv':int(gdb.parse_and_eval('recon_adjust_fdiv')),
            'last_error':int(gdb.parse_and_eval('last_error'))}
        value['target_state']['initializer_words']=[int(gdb.parse_and_eval('(unsigned int)crt_initializers[%%d]'%%n)) for n in range(2)]
        value['target_state']['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*onexit_begin')) if value['target_state']['onexit_begin'] else None
        host=frame
        while host is not None and host.name()!='main': host=host.older()
        if host is None: raise RuntimeError('missing host registration frame')
        spec=host.read_var('spec')
        value['registration']={key:spec[key].string() for key in ('name','path','sha256')}
        value['registration']['flags']=int(spec['flags'])
        value['registered_token']=int(host.read_var('module'))
        open(%r,'w').write(json.dumps(value,indent=2)+'\\n')
        end
        quit
        """ % (breakpoint, str(output / "stdout.bin"), str(output / "stderr.bin"), str(output / "state.json"))
            (output / "commands.gdb").write_text(commands)
            started = time.perf_counter()
            result = subprocess.run(["gdb", "--batch", "-x", str(output / "commands.gdb"),
                                     "--args", str(binary)], cwd=PROJECT,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
            elapsed = time.perf_counter() - started
            (output / "gdb.txt").write_bytes(result.stdout)
            marker = b"Breakpoint 1," if provider_break else b"Program received signal SIGTRAP"
            if result.returncode or marker not in result.stdout:
                raise ValueError("native run has no raw INT3 frontier")
    if sha(binary) != native_sha:
        raise ValueError("native executable changed during capture")
    for item, digest in snapshot_inputs.items():
        if sha(PROJECT / item) != digest:
            raise ValueError("capture configuration/build changed during native run")
    receipt = {"schema": 1, "kind": "native-completion" if completion else "native-gdb-frontier", "exit": result.returncode, "seconds": elapsed,
               "native_sha256": native_sha, "snapshot_inputs": snapshot_inputs,
               "compiled_source_provenance_sha256": sha(output / "build-provenance.json"),
               "artifacts": {p.name: {"sha256": sha(p), "size": p.stat().st_size}
                             for p in sorted(output.iterdir()) if p.is_file()}}
    if candidate.get("prospective_only"):
        receipt["scope"] = "prospective raw-only tail chain; no graph acceptance or function credit"
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(output), "seconds": elapsed}))


if __name__ == "__main__":
    main()
