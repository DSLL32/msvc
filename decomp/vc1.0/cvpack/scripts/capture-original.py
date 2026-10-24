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
    parser.add_argument('--guard-probe',choices=['good-twice','first-text','second-text','second-share','pdb-good-twice','pdb-second-write','pdb-second-share','pdb-second-name'],help='explicit original-only guard controller, never application acceptance')
    parser.add_argument('--entry-progress-only',action='store_true',help='explicit filtered-entry progression; no broad application heap equality claim')
    parser.add_argument('--reference-completion',action='store_true',help='stop at actual third module3 reference constructor caller return')
    parser.add_argument("--scalar-completion",action="store_true",help="reviewed98 first actual callback return")
    args = parser.parse_args()
    if args.scalar_completion or args.reference_completion:
        parser.error("Historical97/98 completion capture is disabled by explicit user instruction")
    assert not args.scalar_completion or(args.entry_progress_only and args.candidate==["begin_symbol_block"]),"only98 scalar slice"
    assert not(args.entry_progress_only and args.reference_completion), 'capture modes are separate'
    assert not((args.entry_progress_only or args.reference_completion)and(args.unmasked or args.guard_probe or args.stop_at)), 'bounded capture modes require normal masked candidate'
    assert not(args.entry_progress_only or args.reference_completion)or(args.case=='hello-debug-dll'and len(args.candidate)==1), 'only reviewed function97 boundary'
    assert not args.reference_completion or args.candidate==['create_symbol_reference'],'reference completion remains exact97 only'
    start = time.perf_counter()
    config, original = load()
    record = case(args.case)
    if args.guard_probe:
        assert args.case=='hello-debug-dll' and not args.unmasked and args.discover_runtime
        markers={'good-twice':'G','first-text':'F','second-text':'T','second-share':'S','pdb-good-twice':'P','pdb-second-write':'W','pdb-second-share':'H','pdb-second-name':'N'}
        probe_argv=list(record['argv'])
        if args.guard_probe.startswith('pdb-'):probe_argv[2]=json.loads((BINARY/'cases/abi/pdb-input.json').read_text())['inputs'][0]['windows']
        record=dict(record,argv=[*probe_argv,markers[args.guard_probe]])
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
        probe=COFF((BINARY/('build/sopen-pdb-guard-probe.obj'if args.guard_probe.startswith('pdb-')else'build/sopen-guard-probe.obj')).read_bytes())
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
    read_sites=validate_reads(config,original)if args.case=='hello-debug-dll'and not args.entry_progress_only else []
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
        if seam['phase']==3:
            env['CVPACK_CAPTURE_SOPEN_REOPEN_INPUT']=tool.windows(ROOT/seam['input']['path'])
        elif seam['guard']['mode']==0x8002:
            env['CVPACK_CAPTURE_SOPEN_NAME']=seam['guard']['name']
            env['CVPACK_CAPTURE_SOPEN_INPUT']=tool.windows(ROOT/seam['input']['path'])
        else:
            env['CVPACK_CAPTURE_SOPEN_PDB_NAME']=seam['guard']['name']
            env['CVPACK_CAPTURE_SOPEN_PDB_INPUT']=tool.windows(ROOT/seam['input']['path'])
    from freebuf import validate as validate_freebuf
    freebuf_policy=validate_freebuf(config,original)
    if freebuf_policy and args.case==freebuf_policy['case']:env['CVPACK_CAPTURE_FREEBUF']='1'
    from pdb_read import validate as validate_pdb_read
    if validate_pdb_read(config,original)and args.case=='hello-debug-dll':env['CVPACK_CAPTURE_PDB_HEADER']='1'
    env['CVPACK_CAPTURE_READS']=','.join('%x:%x'%(r['call'],r['returned'])for r in read_sites)
    env.pop('_MSC_IDE_FLAGS',None)
    env['CVPACK_CAPTURE_ARGV0']=tool.windows(ROOT/config['target']['path'])
    env['CVPACK_CAPTURE_INSPECT']=','.join(args.inspect)
    walks=json.loads((BINARY/'walks.json').read_text()) if (BINARY/'walks.json').exists() else []
    if args.case=='hello-debug-dll'and any(r['name']=='append_buffer'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_BUFFER']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='get_type_record'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_RESOLVER']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='publish_recursive_type'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_RECURSIVE_ARENA']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='publish_packed_type'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_PACKED_ARENA']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='publish_packed_type'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_SYMBOL_BLOCKS']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='create_symbol_reference'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_SYMBOL_REFERENCES']='1'
    if args.case=='hello-debug-dll'and any(r['name']=='address_in_module'and r['status']!='stub'for r in config['functions']):env['CVPACK_CAPTURE_MODULE_ADDRESS']='1'
    from buffer_storage import validate as validate_buffer_storage
    if args.case=='hello-debug-dll':
        validate_buffer_storage(config,original)
        from recursive_arena import validate as validate_recursive_arena
        validate_recursive_arena(config,original)
        from packed_arena import validate as validate_packed_arena
        validate_packed_arena(config,original)
        from symbol_blocks import validate as validate_symbol_blocks
        validate_symbol_blocks(config,original)
        from module_address_returns import validate as validate_module_address
        validate_module_address(config,original)
    env['CVPACK_CAPTURE_WALKS']=','.join('%x:%d:%d:%d:%x:%d:%d:%d'%(r['original'],r['size'],r['next'],r['limit'],r.get('count_original',0),r.get('count_size',4),r.get('count_limit',r['limit']),int(r.get('zero_allocation',False))) for r in walks)
    env['CVPACK_CAPTURE_OUTPUTS']=','.join('%x:%d:%d:%d'%(r['original'],(256 if p.get('register')=='edx'else p['slot']),(0 if p['kind']=='counted-string'else 12 if p['kind']in('directory-key','directory-output')else 8 if p['kind']=='symbol-sort-key'else 40 if p['kind']=='file-slot'else 0xffffffff if p['kind']=='mapped-image'else json.loads((BINARY/p['evidence']).read_text())['size']),int(p['kind']in('stack-input','transient-pointer','counted-string','directory-key','symbol-sort-key','file-slot','mapped-image')))for r in config['functions']for p in r.get('parameters',[])if p['kind']in ('stack-output','stack-input','allocation-output','transient-pointer','counted-string','directory-key','directory-output','symbol-sort-key','file-slot','mapped-image'))
    env['CVPACK_CAPTURE_ARENAS']=','.join('%x:%x:%d'%(r['original'],r['arena']['available_original'],r['arena']['capacity'])for r in walks if r.get('arena'))
    env['CVPACK_CAPTURE_TRANSIENTS']=','.join('%x:%d:%d'%(r['original'],p['slot'],offset)for r in config['functions']for p in r.get('parameters',[])if p['kind']=='transient-pointer'for offset in json.loads((BINARY/p['evidence']).read_text())['child_fields'])
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
    output_spec=env['CVPACK_CAPTURE_OUTPUTS']
    if args.entry_progress_only:
        for key in ('BUFFER','RESOLVER','RECURSIVE_ARENA','PACKED_ARENA','SYMBOL_BLOCKS','SYMBOL_REFERENCES','MODULE_ADDRESS','PDB_HEADER','WALKS','OUTPUTS','ARENAS','TRANSIENTS','STRINGS','ARRAYS'):
            env.pop('CVPACK_CAPTURE_'+key,None)
        env['CVPACK_CAPTURE_PROGRESS_ONLY']='1'
        # Only newly consumed typed owners; no historical prefix heap replay.
        selected=[r for r in walks if r['name']in record['current'].get('observed_walks',[])]
        if selected:
            env['CVPACK_CAPTURE_WALKS']=','.join('%x:%d:%d:%d:%x:%d:%d:%d'%(r['original'],r['size'],r['next'],r['limit'],r.get('count_original',0),r.get('count_size',4),r.get('count_limit',r['limit']),int(r.get('zero_allocation',False)))for r in selected)
            env['CVPACK_CAPTURE_STRINGS']=','.join('%x:%d'%(r['original'],offset)for r in selected for offset in r.get('string_fields',[]))
        env['CVPACK_CAPTURE_OUTPUTS']=output_spec
        buffers=[(r,p)for r in config['functions']for p in r.get('parameters',[])if p['kind']in('stack-input','stack-output')and json.loads((BINARY/p['evidence']).read_text()).get('growing_buffer')]
        if buffers:env['CVPACK_CAPTURE_TRANSIENTS']=','.join('%x:%d:20'%(r['original'],p['slot'])for r,p in buffers)
    if args.candidate!=['create_symbol_reference'] and args.entry_progress_only:env['CVPACK_CAPTURE_SCALAR_PROGRESS']='1'
    if args.scalar_completion:env['CVPACK_CAPTURE_SCALAR_COMPLETION']='1'
    if args.reference_completion:env['CVPACK_CAPTURE_REFERENCE_COMPLETION']='1'
    from inputs import original as original_inputs
    ancillary_inputs=original_inputs(record)
    setup = time.perf_counter() - start
    run = time.perf_counter()
    result = subprocess.run(argv, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, stdin=subprocess.DEVNULL, timeout=90)
    execution = time.perf_counter() - run
    (output/'host-stdout').write_bytes(result.stdout)
    (output/'host-stderr').write_bytes(result.stderr)
    assert result.returncode in ((0,17)if args.guard_probe else (0,)), 'original capture helper status %d: %r'%(result.returncode,result.stderr)
    assert sha(snapshot)==sha(BINARY/'functions.json'),'configuration changed during original run'
    check_policy_snapshot(output,policy_hash)
    assert original_inputs(record)==ancillary_inputs,'original ancillary inputs changed'
    receipt = dict(ancillary_inputs=ancillary_inputs,case=args.case, unmasked=args.unmasked, command=argv,
        original_sha256=config['target']['sha256'], executable_sha256=sha(masked),
        debugger_sha256=sha(debugger), configuration_sha256=sha(snapshot),
        discover_runtime=args.discover_runtime,canonical_argv0=env['CVPACK_CAPTURE_ARGV0'],
        candidates=args.candidate,
        guard_probe=args.guard_probe,
        entry_progress_only=args.entry_progress_only,reference_completion=args.reference_completion,scalar_completion=args.scalar_completion,
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
