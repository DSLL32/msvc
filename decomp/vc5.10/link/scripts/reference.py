#!/usr/bin/env python3
"""Explicit original-only reference / masked acceptance capture."""
import argparse
import fcntl
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
import struct
from mask import BINARY, ROOT, construct
from binary import PE


def sha(data): return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=['help','hello-win32','hello-dll'],default='help')
    parser.add_argument('--mode', choices=['original', 'masked','observe-new'], required=True)
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json',
                        help='explicit trial config, including candidate in accepted prefix')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('original_transport', BINARY / 'scripts/wine-tool.py')
    transport = importlib.util.module_from_spec(spec); spec.loader.exec_module(transport)
    config_bytes = args.config.read_bytes()
    config = json.loads(config_bytes)
    scratch = transport.SCRATCH / 'vc5.10/link'
    scratch.mkdir(exist_ok=True, parents=True)
    from case_inputs import load,workspace
    manifest=load(args.case,config)
    capture = Path(tempfile.mkdtemp(prefix=(''if args.case=='help'else args.case+'-')+args.mode + '-', dir=scratch))
    work_owner=None
    if args.case=='help':
        work=capture/'work';work.mkdir()
    else:
        work_owner=workspace(args.case,manifest);work=work_owner.__enter__()
        (capture/'case.json').write_text(json.dumps(manifest,indent=2)+'\n')
    mask, ownership = construct(config)
    if args.mode=='observe-new':
        pe=PE(ROOT/config['target']['path'])
        section,=[s for s in pe.sections if s['flags']&0x20]
        mask=struct.pack('<II',section['rva'],section['virtual_size'])+pe.read(pe.base+section['rva'],section['virtual_size'])
    mutable = bytearray(mask)
    mask_rva = int.from_bytes(mask[:4], 'little')
    entries = []
    records = config['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    base = 0x400000
    for record in records:
        if record['name'] not in config['accepted']: continue
        rva = record['start'] - base
        position = 8 + rva - mask_rva
        byte = mutable[position]
        assert byte != 0xcc, 'accepted entry is still masked'
        entries.append('%x %x 0 %s\n' % (rva, byte, record['name']))
        mutable[position] = 0xcc
    from observers import plan
    for rva,name in plan(config['accepted'],args.case):
        position=8+rva-mask_rva
        entries.append('%x %x 0 %s\n'%(rva,mutable[position],name));mutable[position]=0xcc
    if args.mode=='observe-new':
        for rva,name in [(0x24e75,'observe:new_call_12'),(0x24e7a,'observe:new_return_12'),
                         (0x31d85,'observe:new_call_8'),(0x31d8a,'observe:new_return_8'),
                         (0x31df5,'observe:new_call_4'),(0x31dfa,'observe:new_return_4')]:
            position=8+rva-mask_rva
            entries.append('%x %x 0 %s\n'%(rva,mutable[position],name));mutable[position]=0xcc
    if args.mode=='observe-new' and '_is_libpath@4' not in config['accepted']:
        for rva,name in [(0x5c76b,'observe:mbs_call'),(0x5c771,'observe:mbs_return')]:
            position=8+rva-mask_rva
            entries.append('%x %x 0 %s\n'%(rva,mutable[position],name));mutable[position]=0xcc
    mask = bytes(mutable)
    (capture / 'mask.bin').write_bytes(mask)
    (capture / 'ownership.json').write_text(json.dumps(ownership, indent=2) + '\n')
    (capture / 'functions.json').write_bytes(config_bytes)
    (capture / 'runtime.json').write_bytes((BINARY / 'runtime.json').read_bytes())
    state_plan = [
        ('target',0x67000,20,'initializers.bin'),
        ('target',0x7f028,4,'output-module-global.bin'),
        ('target',0x94930,36,'startup-globals.bin'),
        ('target',0x809d0,12,'buffer12.bin'),
        ('target',0x943a8,12,'buffer8.bin'),
        ('target',0x94398,12,'buffer4.bin'),
        ('target',0x94460,20,'timing.bin'),
        ('target',0x7eeb8,4,'diagnostic-output.bin'),
        ('target',0x7f1a0,256,'debug-flags.bin'),
        ('target',0x67538,12,'file-pointers.bin'),
        ('target',0x6754c,1,'file-mapping-enabled.bin'),
        ('target',0x674dc,4,'file-window-limit.bin'),
        ('target',0x674f4,60,'file-statistics.bin'),
        ('target',0x674f0,64,'mapped-file-slots.bin'),
        ('target',0x944b4,4,'allocation-page-size.bin'),
        ('target',0x7ef7c,4,'maximum-physical-memory.bin'),
        ('target',0x7f138,4,'selected-tool-kind.bin'),
        ('target',0x7f060,4,'selected-tool-name.bin'),
        ('target',0x7f0a4,4,'repro-directory.bin'),
        ('target',0x7ef68,4,'repro-response.bin'),
        ('target',0x7eedc,1,'input-prepass.bin'),
        ('target',0x7f108,12,'input-files.bin'),
        ('target',0x7efd8,4,'argument-tail.bin'),
        ('target',0x7f040,12,'option-arguments.bin'),
        ('target',0x672d0,1,'logo-pending.bin'),
        ('target',0x94888,12,'logo-text.bin'),
        ('target',0x7f01c,1,'incremental-enabled.bin'),
        ('target',0x7f14c,4,'member-header-size.bin'),
        ('target',0x7f0f8,4,'member-write-section.bin'),
        ('target',0x7eed0,4,'member-write-headers.bin'),
        ('target',0x94454,1,'incremental-requested.bin'),
        ('target',0x94478,1,'incremental-switch-set.bin'),
        ('target',0x94474,4,'debug-formats.bin'),
        ('target',0x670ac,1,'pdb-enabled.bin'),
        ('target',0x7f0a0,1,'debug-option-present.bin'),
        ('crt',0x3fa88,4,'crt-fmode.bin'),
        ('crt',0x3fa94,4,'crt-commode.bin'),
        ('crt',0x3f928,4,'crt-adjust-fdiv.bin'),
        ('crt',0x3f590,32,'crt-stdout.bin')]
    if '_zero_allocate@4' in config['accepted']:state_plan.append(('targetptr',0x67538,64,'file-slots.bin'))
    if '_append_argument_record@12' in config['accepted']:state_plan.append(('targetptr',0x7f040,24,'option-node.bin'))
    (capture / 'state-plan.txt').write_text(''.join('%s %x %d %s\n'%item for item in state_plan))
    (capture / 'entries.txt').write_text(''.join(entries))
    helper = BINARY / 'build/capture-help.exe'
    command = ['wine', transport.windows(helper), transport.windows(ROOT / config['target']['path']),
               transport.windows(capture), transport.windows(capture / 'mask.bin') if args.mode != 'original' else '-',
               transport.windows(capture / 'entries.txt'), transport.windows(work)]
    if args.case!='help':command.append(subprocess.list2cmdline([transport.windows(ROOT/config['target']['path'])]+manifest['arguments']))
    env = transport.environment()
    env['TEMP'] = env['TMP'] = transport.windows(work)
    lock_path = Path('/root/.ai-tmp/msvc2-original-capture.lock')
    lock_start = time.perf_counter()
    with lock_path.open('a') as capture_lock:
        fcntl.flock(capture_lock, fcntl.LOCK_EX)
        lock_wait = time.perf_counter() - lock_start
        start = time.perf_counter()
        try:
            result = subprocess.run(command, cwd=work, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=45)
            helper_status, helper_out, helper_err = result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired as error:
            helper_status, helper_out, helper_err = 'timeout', error.stdout or b'', error.stderr or b''
        elapsed = time.perf_counter() - start
    (capture / 'helper.stdout').write_bytes(helper_out); (capture / 'helper.stderr').write_bytes(helper_err)
    if work_owner:
        from case_inputs import save_artifacts
        save_artifacts(work,capture,manifest)
        work_owner.__exit__(None,None,None)
    files = {str(p.relative_to(capture)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
             for p in capture.rglob('*')if p.is_file()}
    canonical_crt = ROOT / 'toolchains/5.10/bin/MSVCRT.DLL'
    expected_crt = 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
    assert sha(canonical_crt.read_bytes()) == expected_crt and canonical_crt.stat().st_size == 277776
    loaded_crt = [name for name, value in files.items()
                  if name.startswith('module-') and value['sha256'] == expected_crt]
    loaded_target = files.get('module-000.bin', {}).get('sha256') == config['target']['sha256']
    receipt = dict(kind={'original':'fresh-unmodified-original-help','masked':'fresh-masked-original-help','observe-new':'explicit-unmasked-original-import-observation'}[args.mode],
                   command=command, mode=args.mode,case=args.case,case_manifest=manifest,working_directory=transport.windows(work),original_sha256=config['target']['sha256'],
                   configuration_sha256=sha(config_bytes), mask_sha256=sha(mask),
                   helper_sha256=sha(helper.read_bytes()), helper_status=helper_status,
                   actual_loaded_crt_hfile_copies=loaded_crt,
                   actual_loaded_crt_verified=bool(loaded_crt), elapsed_seconds=elapsed,
                   capture_execution_lock=str(lock_path), capture_lock_wait_seconds=lock_wait,
                   actual_loaded_target_hfile_verified=loaded_target,
                   native_sha256=sha((BINARY / 'build/native/LINK.EXE').read_bytes()),
                   rebuilt_sha256=sha((BINARY / 'build/link.exe').read_bytes()),
                   source_files={str(p.relative_to(BINARY)):sha(p.read_bytes())
                                 for p in sorted(p for p in (BINARY/'src').iterdir() if p.suffix in ('.c','.h','.cpp'))},
                   environment={k: env[k] for k in ['WINEDLLOVERRIDES', 'WINEPATH', 'TEMP', 'TMP']},
                   files=files)
    (capture / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(capture, json.dumps(receipt), flush=True)
    if helper_status != 0 or not loaded_crt or not loaded_target:
        raise SystemExit('original/provider capture failed; preserve evidence and report before changing baseline')


if __name__ == '__main__': main()
