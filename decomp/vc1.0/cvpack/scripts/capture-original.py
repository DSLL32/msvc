#!/usr/bin/env python3
"""Explicit masked/unmasked original capture. Not a fixture dependency."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import time
from state import BINARY, ROOT, load, case, mask, prepare, sha
from binary import u32,COFF
import struct
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
    parser.add_argument('--watch-spawn',action='store_true',help='explicitly capture spawnlp inputs and return/errno')
    parser.add_argument('--inspect-context',action='store_true',help='retain encountered application globals')
    parser.add_argument('--stop-at',help='explicit identified CRT entry at which to inspect state')
    parser.add_argument('--guard-probe',choices=['good-twice','first-text','second-text','second-share'],help='explicit original-only guard controller, never application acceptance')
    args = parser.parse_args()
    start = time.perf_counter()
    config, original = load()
    record = case(args.case)
    if args.guard_probe:
        assert args.case=='hello-debug-dll' and not args.unmasked and args.discover_runtime
        record=dict(record,argv=[*record['argv'],{'good-twice':'G','first-text':'F','second-text':'T','second-share':'S'}[args.guard_probe]])
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    snapshot=output/'configuration.json'
    snapshot.write_bytes((BINARY/'functions.json').read_bytes())
    assert json.loads(snapshot.read_text())==config,'configuration changed during capture setup'
    from seams import snapshot_policy,check_policy_snapshot
    policy_hash=snapshot_policy(output)
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
    if args.discover_runtime or args.watch_spawn:
        assert not (args.discover_runtime and args.unmasked)
        runtime=identify(ROOT/config['target']['path'],{'_main':config['target']['application_main']})
        plan=output/'runtime-plan'
        ranges={(r['start'],r['end']) for r in runtime['functions']+runtime['imports']} if args.discover_runtime else set()
        application=[r for r in config['functions'] if r['status']=='accepted' or r['name'] in args.candidate] if not args.unmasked else []
        image=bytearray(masked.read_bytes())
        for function in application:
            image[original.offset(function['original'])]=0xcc
            ranges.add(tuple(function['extent']))
        if args.watch_spawn:
            spawn=next(r for r in runtime['functions'] if r['name']=='__spawnlp')
            image[original.offset(spawn['start'])]=0xcc
            ranges.add((spawn['start'],spawn['end']))
        if args.stop_at:
            stops=[r for r in runtime['functions'] if r['name']==args.stop_at]
            assert stops,'unknown explicit CRT boundary'
            for stop in stops:
                ranges.discard((stop['start'],stop['end']))
                image[original.offset(stop['start'])]=0xcc
        masked.write_bytes(image)
        plan.write_text(''.join('%x %x %x\n'%(a,b,original.offset(a)) for a,b in sorted(ranges)))
        (output/'runtime-identities.json').write_text(json.dumps(runtime,indent=2)+'\n')
        argv.extend([tool.windows(plan),tool.windows(ROOT/config['target']['path'])])
    if args.guard_probe:
        probe=COFF((BINARY/'build/sopen-guard-probe.obj').read_bytes())
        sections=[s for s in probe.sections if s['flags']&0x20]
        assert len(sections)==1
        section=sections[0]
        body=bytearray(section['data'])
        main_record=next(r for r in config['functions']if r['name']=='cvpack_main')
        assert len(body)<=main_record['extent'][1]-main_record['extent'][0]
        bindings={'__sopen':0x2dcb0,'__close':0x2db90}
        for offset,symbol,kind in section['relocs']:
            assert kind==20 and symbol['name']in bindings,(kind,symbol)
            addend=struct.unpack_from('<i',body,offset)[0]
            struct.pack_into('<i',body,offset,bindings[symbol['name']]+addend-main_record['original']-offset-4)
        image=bytearray(masked.read_bytes())
        bias=original.offset(main_record['original'])
        image[bias:bias+len(body)]=body
        masked.write_bytes(image)
    from reads import validate as validate_reads
    read_sites=validate_reads(config,original)if args.case=='hello-debug-dll'else []
    if read_sites:
        image=bytearray(masked.read_bytes())
        for site in read_sites:
            image[original.offset(site['call'])]=0xcc
            image[original.offset(site['returned'])]=0xcc
        masked.write_bytes(image)
    env=tool.environment()
    from seams import approved
    seam_records=approved(config,original,json.loads((BINARY/'runtime.json').read_text()),args.case)
    for seam in seam_records:
        assert not args.unmasked,'conditional support is masked-capture only'
        env['CVPACK_CAPTURE_SOPEN_GUARD']='%x:%x:%x'%(seam['start'],seam['end'],original.offset(seam['start']))
        env['CVPACK_CAPTURE_SOPEN_NAME']=seam['guard']['name']
        env['CVPACK_CAPTURE_SOPEN_INPUT']=tool.windows(ROOT/seam['input']['path'])
    env['CVPACK_CAPTURE_READS']=','.join('%x:%x'%(r['call'],r['returned'])for r in read_sites)
    env.pop('_MSC_IDE_FLAGS',None)
    env['CVPACK_CAPTURE_ARGV0']=tool.windows(ROOT/config['target']['path'])
    env['CVPACK_CAPTURE_INSPECT']=','.join(args.inspect)
    walks=json.loads((BINARY/'walks.json').read_text()) if (BINARY/'walks.json').exists() else []
    env['CVPACK_CAPTURE_WALKS']=','.join('%x:%d:%d:%d:%x:%d:%d:%d'%(r['original'],r['size'],r['next'],r['limit'],r.get('count_original',0),r.get('count_size',4),r.get('count_limit',r['limit']),int(r.get('zero_allocation',False))) for r in walks)
    env['CVPACK_CAPTURE_OUTPUTS']=','.join('%x:%d:%d'%(r['original'],p['slot'],json.loads((BINARY/p['evidence']).read_text())['size'])for r in config['functions']for p in r.get('parameters',[])if p['kind']=='stack-output')
    env['CVPACK_CAPTURE_STRINGS']=','.join('%x:%d'%(r['original'],offset)for r in walks for offset in r.get('string_fields',[]))
    env['CVPACK_CAPTURE_ARRAYS']=','.join('%x:%d:%d:%d:%d:%d:%x:%x:%d'%(r['original'],a['offset'],a.get('count_offset',0xffffffff),a.get('count_shift',0),a['max_count'],a['when']['offset'],a['when']['mask'],a['when']['value'],a.get('element_size',4))for r in walks for a in r.get('arrays',[]))
    if args.inspect_context:
        context=json.loads((BINARY/'data.json').read_text())
        requests=[*args.inspect,*['%x:%d'%(r['original'],r['size']) for r in context]]
        if seam_records:requests.append('39614:4')
        for item in context:
            if item.get('kind')=='string-pointer-array':
                for index in range(item['size']//4):
                    address=u32(original.read(item['original']+4*index,4),0)
                    requests.append('%x:%d'%(address,len(original.string(address))+1))
        env['CVPACK_CAPTURE_INSPECT']=','.join(requests)
    if args.watch_spawn:env['CVPACK_CAPTURE_WATCH_SPAWN']='%x'%spawn['start']
    setup = time.perf_counter() - start
    run = time.perf_counter()
    result = subprocess.run(argv, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=90)
    execution = time.perf_counter() - run
    (output/'host-stdout').write_bytes(result.stdout)
    (output/'host-stderr').write_bytes(result.stderr)
    assert result.returncode in ((0,17)if args.guard_probe else (0,)), repr(result.stderr)
    assert sha(snapshot)==sha(BINARY/'functions.json'),'configuration changed during original run'
    check_policy_snapshot(output,policy_hash)
    receipt = dict(case=args.case, unmasked=args.unmasked, command=argv,
        original_sha256=config['target']['sha256'], executable_sha256=sha(masked),
        debugger_sha256=sha(debugger), configuration_sha256=sha(snapshot),
        discover_runtime=args.discover_runtime,canonical_argv0=env['CVPACK_CAPTURE_ARGV0'],
        candidates=args.candidate,
        guard_probe=args.guard_probe,
        helper_status=result.returncode,
        conditional_runtime=seam_records,
        conditional_support_sha256=policy_hash,
        stop_at=args.stop_at,
        environment={'LIB':env['LIB'],'PATH':'','_MSC_IDE_FLAGS':None},
        accepted=[r['name'] for r in config['functions'] if r['status']=='accepted'],
        seconds=dict(setup=setup,execution=execution,total=time.perf_counter()-start),
        artifacts=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(work.iterdir())])
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    lines=(output/'events').read_text().splitlines()
    print(next((line for line in lines if line.startswith('exception ')),lines[-1]))
    print('Restored entry ranges:',sum(line.startswith('runtime ')for line in lines))
    print(output/'receipt.json')

if __name__ == '__main__':
    main()
