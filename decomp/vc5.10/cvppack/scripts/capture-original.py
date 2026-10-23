#!/usr/bin/env python3
"""Explicit original-only capture; native fixtures never import this module."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
from state import BINARY, ROOT, load, mask, sha

CRT_SHA = 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--unmasked', action='store_true')
    parser.add_argument('--candidate', action='append', default=[])
    parser.add_argument('--resource-probe', action='store_true')
    parser.add_argument('--case', choices=['help','hello-debug-dll'], default='help')
    args = parser.parse_args()
    start = time.perf_counter()
    config, pe = load()
    from caseio import definition,prepare,artifacts
    case = definition(args.case)
    assert not args.resource_probe or args.unmasked, 'resource probe requires explicit whole-original mode'
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / 'configuration.json').write_bytes((BINARY / 'functions.json').read_bytes())
    (output / 'data.json').write_bytes((BINARY / 'data.json').read_bytes())
    (output / 'runtime.json').write_bytes((BINARY / 'runtime.json').read_bytes())
    (output / 'case-definition.json').write_text(json.dumps(case,indent=2)+'\n')
    from provenance import snapshot
    snapshot(output/'compiled-sources.json')
    from audit import run as audit_candidate
    audit_candidate(output / 'configuration.json', args.candidate, output / 'audit.json')
    token = sha(output / 'configuration.json')[:12] + '-' + str(time.time_ns())
    work = Path.home() / '.ai-tmp/msvc2/vc5.10/cvppack/captures' / token
    work.mkdir(parents=True, exist_ok=False)
    prepare(case,work)
    binary = work / 'CVPACK.EXE'
    image = bytearray(pe.data) if args.unmasked else mask(config, pe, args.candidate)
    plan = []
    for record in config['functions']:
        if not args.unmasked and (record['status'] == 'accepted' or record['name'] in args.candidate):
            start_va, end_va = record['extent']
            plan.append((start_va, end_va, pe.offset(start_va)))
    for record in config['support_unmasked']:
        if not args.unmasked:
            plan.append((record['start'], record['end'], pe.offset(record['start'])))
    for start_va, _, _ in plan:
        image[pe.offset(start_va)] = 0xcc
    binary.write_bytes(image)
    (output / 'runtime-plan').write_text(''.join('%x %x %x\n' % row for row in sorted(plan)))
    for name in ('MSVCRT.DLL', 'MSPDB50.DLL'):
        shutil.copyfile(ROOT / 'toolchains/5.10/bin' / name, work / name)
    assert sha(work / 'MSVCRT.DLL') == CRT_SHA
    spec = importlib.util.spec_from_file_location('original_tool', BINARY / 'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    debugger = BINARY / 'build/debug-run.exe'
    local_debugger = work / 'debug-run.exe'
    shutil.copyfile(debugger, local_debugger)
    loaded_provider = work / 'loaded-msvcrt.dll'
    command = ['wine', tool.windows(local_debugger), tool.windows(binary),
               tool.windows(ROOT / config['target']['path']) + ' ' + ' '.join(case['argv']),
               tool.windows(output / 'stdout'), tool.windows(output / 'stderr'),
               tool.windows(output / 'events'), tool.windows(work),
               tool.windows(output / 'runtime-plan'), tool.windows(ROOT / config['target']['path']),
               tool.windows(loaded_provider), str(int(args.resource_probe))]
    setup = time.perf_counter() - start
    execute = time.perf_counter()
    env = tool.environment()
    if case['name']=='hello-debug-dll':env['VC510_CVPACK_PDB_PROVIDER']=tool.windows(work/'loaded-mspdb50.dll')
    data = json.loads((output / 'data.json').read_text())
    env['VC510_CVPACK_DATA'] = ','.join('%x:%d' % (item['original'],item['size']) for item in data)
    env['VC510_CVPACK_GRAPHS'] = ','.join('%x:%d:%d:%x:%d:%d:%d' % (item['original'],item['shape']['size'],item['shape']['link'],
        next((row['original'] for row in data if row['name']==item['shape'].get('count')),0),item['shape'].get('string',0xffffffff),
        item['shape'].get('count_literal',0),item['shape'].get('handle',0xffffffff)) for item in data if 'shape' in item)
    if any(r['name']=='open_mapped_file' and (r['status']=='accepted' or r['name'] in args.candidate) for r in config['functions']):
        env['VC510_CVPACK_PROBES']='4021bd:file-open-call,4021c3:file-open-return'
    if any(r['name']=='create_mapping' and (r['status']=='accepted' or r['name'] in args.candidate) for r in config['functions']):
        env['VC510_CVPACK_PROBES']+=',4024c7:mapping-create-call,4024cd:mapping-create-return,402504:view-map-call,40250a:view-map-return,402514:mapping-close-call,402516:mapping-close-return'
    if any(r['name']=='read_cached_file' and (r['status']=='accepted' or r['name'] in args.candidate) for r in config['functions']):
        env['VC510_CVPACK_PROBES']+=',401984:file-read-entry,4019a3:file-read-mapped-return,4019ac:file-read-stream-return'
    if any(r['name']=='read_codeview_directory' and (r['status']=='accepted' or r['name'] in args.candidate) for r in config['functions']):
        env['VC510_CVPACK_PROBES']+=',40584b:directory-allocation-entry,405851:directory-allocation-result'
    if any(r['name']=='load_debug_subsection' and (r['status']=='accepted' or r['name'] in args.candidate) for r in config['functions']):
        env['VC510_CVPACK_PROBES']+=',40591a:subsection-allocation-entry,405920:subsection-allocation-result'
    env['VC510_CVPACK_BLOBS']=','.join('%x:%d:%d:%d:%x:%d:%d'%(item['original'],item['blob'].get('size',0),
        item['blob'].get('pointers',[dict(offset=0xffffffff)])[0]['offset'],
        item['blob'].get('pointers',[dict(size=0)])[0]['size'],
        next((row['original'] for row in data if row['name']==item['blob'].get('count')),0),item['blob'].get('element_size',0),
        next((row['size'] for row in data if row['name']==item['blob'].get('count')),4)) for item in data if 'blob' in item)
    env['VC510_CVPACK_ARGUMENTS'] = ','.join('%x:%d:%s' % (record['original'],item['slot'],
        'S' if item['kind']=='string' else 'P' if item['kind']=='integer-pointer' else 'A') for record in config['functions']
        for item in record.get('parameters',[]) if item['kind'] in ('string','argv','integer-pointer'))
    result = subprocess.run(command, env=env, stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60, check=False)
    elapsed = time.perf_counter() - execute
    (output / 'host-stdout').write_bytes(result.stdout)
    (output / 'host-stderr').write_bytes(result.stderr)
    assert result.returncode == 0, ('original debugger failure', result.returncode, result.stderr)
    assert loaded_provider.is_file() and sha(loaded_provider) == CRT_SHA, 'actual loaded CRT is not canonical shipped provider'
    assert (BINARY / 'functions.json').read_bytes() == (output / 'configuration.json').read_bytes(), 'configuration changed during capture'
    artifact_records = artifacts(case,work,output)
    for path in sorted(output.glob('events-view-*.bin')):
        artifact_records.append(dict(name=path.name,bytes=path.stat().st_size,sha256=sha(path)))
    receipt = dict(schema=1, case=args.case, unmasked=args.unmasked, candidates=args.candidate,
        case_definition_sha256=sha(output/'case-definition.json'),
        debugger_source_sha256=sha(BINARY/'scripts/debug-run.c'),
        command=command, original_sha256=config['target']['sha256'],
        masked_image_sha256=sha(binary), debugger_sha256=sha(debugger),
        rebuilt_sha256=sha(BINARY / 'build/CVPACK.EXE'),
        configuration_sha256=sha(output / 'configuration.json'), native_sha256=sha(BINARY / 'build/native/CVPACK'),
        plan_sha256=sha(output / 'runtime-plan'),
        actual_loaded_crt=dict(module='MSVCRT.dll', bytes=loaded_provider.stat().st_size,
                              sha256=sha(loaded_provider), capture_path=str(loaded_provider),
                              source='LOAD_DLL_DEBUG_EVENT.hFile exact file copy'),
        actual_target_capture=str(binary),
        actual_target_windows=tool.windows(binary),
        adjacent_inputs=[dict(name=name, sha256=sha(work / name)) for name in ('MSVCRT.DLL', 'MSPDB50.DLL')],
        wine_prefix=str(tool.PREFIX), wine_dll_overrides=tool.environment()['WINEDLLOVERRIDES'],
        canonical_argv0=tool.windows(ROOT / config['target']['path']),
        environment={'MSC_CMD_FLAGS': None, '_MSC_IDE_FLAGS': None},
        data_sha256=sha(output / 'data.json'), runtime_sha256=sha(output / 'runtime.json'),
        audit_sha256=sha(output / 'audit.json'), graph_config_sha256=sha(output / 'graph-config.json'),
        graph_runtime_sha256=sha(output / 'graph-runtime.json'),
        compiled_sources_sha256=sha(output/'compiled-sources.json'),
        outputs=[dict(name=name, bytes=(output / name).stat().st_size, sha256=sha(output / name))
                 for name in ('stdout', 'stderr', 'events', 'audit.json', 'graph-config.json', 'graph-runtime.json','case-definition.json')]+artifact_records,
        seconds=dict(setup=setup, execution=elapsed, total=time.perf_counter()-start))
    if case['name']=='hello-debug-dll':
        actual=work/'loaded-mspdb50.dll'
        assert sha(actual)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
        receipt['actual_loaded_mspdb']=dict(path=str(actual),bytes=actual.stat().st_size,sha256=sha(actual),source='LOAD_DLL_DEBUG_EVENT.hFile exact file copy')
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    lines = (output / 'events').read_text().splitlines()
    print(next((line for line in lines if line.startswith('frontier ')), lines[-1]))


if __name__ == '__main__':
    main()
