#!/usr/bin/env python3
"""Explicit masked/unmasked original capture. Not a fixture dependency."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import time
from state import BINARY, ROOT, load, case, mask, prepare, sha
from runtime import identify

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--unmasked', action='store_true')
    parser.add_argument('--discover-runtime', action='store_true',
                        help='unmask reached strict LIBC/import identities in-process, logging every range')
    parser.add_argument('--inspect',action='append',default=[],help='original VA:length memory to retain at trap')
    parser.add_argument('--candidate',action='append',default=[],help='proposed source function to validate before acceptance')
    args = parser.parse_args()
    start = time.perf_counter()
    config, original = load()
    record = case(args.case)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output/'configuration.json').write_text(json.dumps(config,indent=2)+'\n')
    work = output / 'work'
    prepare(record, work)
    masked = output / 'CVPACK.EXE'
    if args.unmasked:
        masked.write_bytes(original.data)
    else:
        mask(config, original, masked,args.candidate)
    spec = importlib.util.spec_from_file_location('original_tool', BINARY/'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    debugger = BINARY / 'build/debug-run.exe'
    assert debugger.is_file(), 'build explicit reference debugger first'
    argv = ['wine', str(debugger), tool.windows(masked), ' '.join(record['argv']),
            tool.windows(output/'stdout'), tool.windows(output/'stderr'),
            tool.windows(output/'events'), tool.windows(work)]
    runtime=None
    if args.discover_runtime:
        assert not args.unmasked
        runtime=identify(ROOT/config['target']['path'],{'_main':config['target']['application_main']})
        plan=output/'runtime-plan'
        ranges={(r['start'],r['end']) for r in runtime['functions']+runtime['imports']}
        application=[r for r in config['functions'] if r['status']=='accepted' or r['name'] in args.candidate]
        image=bytearray(masked.read_bytes())
        for function in application:
            image[original.offset(function['original'])]=0xcc
            ranges.add(tuple(function['extent']))
        masked.write_bytes(image)
        plan.write_text(''.join('%x %x %x\n'%(a,b,original.offset(a)) for a,b in sorted(ranges)))
        (output/'runtime-identities.json').write_text(json.dumps(runtime,indent=2)+'\n')
        argv.extend([tool.windows(plan),tool.windows(ROOT/config['target']['path'])])
    env=tool.environment()
    env.pop('_MSC_IDE_FLAGS',None)
    env['CVPACK_CAPTURE_ARGV0']=tool.windows(ROOT/config['target']['path'])
    env['CVPACK_CAPTURE_INSPECT']=','.join(args.inspect)
    setup = time.perf_counter() - start
    run = time.perf_counter()
    result = subprocess.run(argv, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=90)
    execution = time.perf_counter() - run
    (output/'host-stdout').write_bytes(result.stdout)
    (output/'host-stderr').write_bytes(result.stderr)
    assert result.returncode == 0, repr(result.stderr)
    receipt = dict(case=args.case, unmasked=args.unmasked, command=argv,
        original_sha256=config['target']['sha256'], executable_sha256=sha(masked),
        debugger_sha256=sha(debugger), configuration_sha256=sha(BINARY/'functions.json'),
        discover_runtime=args.discover_runtime,canonical_argv0=env['CVPACK_CAPTURE_ARGV0'],
        candidates=args.candidate,
        environment={'LIB':env['LIB'],'_MSC_IDE_FLAGS':None},
        accepted=[r['name'] for r in config['functions'] if r['status']=='accepted'],
        seconds=dict(setup=setup,execution=execution,total=time.perf_counter()-start),
        artifacts=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(work.iterdir())])
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print((output/'events').read_text())
    print(output/'receipt.json')

if __name__ == '__main__':
    main()
