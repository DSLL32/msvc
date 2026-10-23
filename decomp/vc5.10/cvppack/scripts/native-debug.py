#!/usr/bin/env python3
"""Linux GDB inspection; no original execution or reference refresh."""
import argparse
import importlib.util
import json
import os
import resource
import shlex
from pathlib import Path
import subprocess
import time
from state import BINARY, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--original', type=Path)
    parser.add_argument('--config',type=Path,default=BINARY/'functions.json')
    parser.add_argument('--content-store',type=Path)
    args = parser.parse_args()
    reference = json.loads((args.original/'receipt.json').read_text()) if args.original else None
    from caseio import definition,prepare,artifacts
    case = json.loads((args.original/'case-definition.json').read_text()) if reference and 'case_definition_sha256' in reference else definition('help')
    start = time.perf_counter()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    prepare(case,output)
    (output/'case-definition.json').write_text(json.dumps(case,indent=2)+'\n')
    (output / 'configuration.json').write_bytes(args.config.read_bytes())
    (output / 'data.json').write_bytes((BINARY / 'data.json').read_bytes())
    from provenance import snapshot
    snapshot(output/'compiled-sources.json',args.content_store)
    config = json.loads((output / 'configuration.json').read_text())
    data = json.loads((output / 'data.json').read_text())
    commands = output / 'commands.gdb'
    state_path = output / 'state.json'
    commands.write_text('set pagination off\nset confirm off\nset disable-randomization off\n'
        'handle SIGTRAP stop print nopass\n'
        'python\nimport importlib.util\n'
        'spec=importlib.util.spec_from_file_location("frozen_gdb_state",'+repr(str(output/'gdb-state.py'))+')\n'
        'helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)\nhelper.install()\nend\n'
        'run ' + ' '.join(shlex.quote(value) for value in case['argv']) + ' >' + str(output / 'stdout') + ' 2>' + str(output / 'stderr') + '\npython\n'
        'helper.capture('+repr(str(output/'configuration.json'))+','+repr(str(output/'data.json'))+','+repr(str(state_path))+')\nend\nquit\n')
    (output/'gdb-state.py').write_bytes((BINARY/'scripts/gdb-state.py').read_bytes())
    env = os.environ.copy()
    env.pop('VC510_CVPACK_WIDEN',None)
    env.pop('RECON_TRACE_FD', None)
    env['VC510_CVPACK_ARGV0'] = reference['canonical_argv0'] if reference else 'R:\\toolchains\\5.10\\bin\\CVPACK.EXE'
    env['RECON_PROGRAM_PATH'] = reference['actual_target_windows'] if reference else env['VC510_CVPACK_ARGV0']
    spec=importlib.util.spec_from_file_location('native_resources',BINARY/'scripts/native-resources.py')
    resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
    resource_provider=resources.prepare(output/'resources.rmr')
    env['VC510_CVPACK_RESOURCES']=str(output/'resources.rmr')
    prepared_widen=None
    extra_outputs=[]
    if case['name']=='hello-debug-dll':
        spec=importlib.util.spec_from_file_location('native_widen',BINARY/'scripts/native-widen.py')
        widen=importlib.util.module_from_spec(spec);spec.loader.exec_module(widen)
        prepared_widen=widen.prepare(output/'widen.rwp')
        env['VC510_CVPACK_WIDEN']=str(output/'widen.rwp')
        extra_outputs=['widen.rwp']
    direct_start=time.perf_counter()
    entry_filter=output/'entry-filter'
    entry_filter.write_text(''.join(record['native_symbol']+'\n' for record in config['functions']))
    (output/'trace-seen').write_bytes(b'')
    def no_core():
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    with (output/'entry-trace').open('wb') as trace,(output/'direct-stdout').open('wb') as out,(output/'direct-stderr').open('wb') as err:
        traced=dict(env,RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(entry_filter),RECON_TRACE_SEEN=str(output/'trace-seen'))
        direct=subprocess.run([str(BINARY/'build/native/CVPACK'),*case['argv']],cwd=output,env=traced,
            stdout=out,stderr=err,stdin=subprocess.DEVNULL,pass_fds=(trace.fileno(),),timeout=10,preexec_fn=no_core)
    (output/'native-status').write_text(str(direct.returncode)+'\n')
    direct_seconds=time.perf_counter()-direct_start
    direct_artifacts={name:(output/name).read_bytes() for name in case['artifacts']}
    execute = time.perf_counter()
    if direct.returncode>=0:
        state_path.write_text(json.dumps(dict(terminal='exit',exit_status=direct.returncode),indent=2)+'\n')
        (output/'gdb-stdout').write_bytes(b'')
        (output/'gdb-stderr').write_bytes(b'')
        for name in ('stdout','stderr'):
            (output/name).write_bytes((output/('direct-'+name)).read_bytes())
        elapsed=0.0
    else:
        prepare(case,output)
        result = subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(commands), str(BINARY / 'build/native/CVPACK')],
                            cwd=output, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
        elapsed = time.perf_counter() - execute
        (output / 'gdb-stdout').write_bytes(result.stdout)
        (output / 'gdb-stderr').write_bytes(result.stderr)
        assert result.returncode == 0 and state_path.is_file(), result.stderr
        assert direct.returncode == -5, 'partial native debugger capture must actually trap'
    assert (output/'direct-stdout').read_bytes()==(output/'stdout').read_bytes()
    assert (output/'direct-stderr').read_bytes()==(output/'stderr').read_bytes()
    assert all((output/name).read_bytes()==value for name,value in direct_artifacts.items()), 'direct/GDB artifact mismatch'
    # Keep execution inputs and artifacts separate from the captured raw files.
    artifact_records=[]
    for name in case['artifacts']:
        path=output/('artifact-'+name);path.write_bytes((output/name).read_bytes())
        artifact_records.append(dict(name=path.name,bytes=path.stat().st_size,sha256=sha(path)))
    receipt = dict(schema=1, case=case['name'], native_sha256=sha(BINARY / 'build/native/CVPACK'),
                   case_definition_sha256=sha(output/'case-definition.json'),
                   configuration_sha256=sha(output / 'configuration.json'),
                   rebuilt_sha256=sha(BINARY / 'build/CVPACK.EXE'),
                   data_sha256=sha(output / 'data.json'),
                   original_receipt_sha256=sha(args.original/'receipt.json') if args.original else None,
                   executed_command=[str(BINARY/'build/native/CVPACK'),*case['argv']],
                   compiled_sources_sha256=sha(output/'compiled-sources.json'),
                   prepared_resources=resource_provider,
                   prepared_widen=prepared_widen,
                   reused_original_capture=bool(reference and (args.original/'configuration.json').read_bytes()==(output/'configuration.json').read_bytes()
                       and (reference['native_sha256']!=sha(BINARY/'build/native/CVPACK') or reference['rebuilt_sha256']!=sha(BINARY/'build/CVPACK.EXE'))),
                   outputs=[dict(name=name,bytes=(output/name).stat().st_size,sha256=sha(output/name))
                            for name in ['stdout','stderr','gdb-stdout','gdb-stderr','state.json','entry-trace','entry-filter','native-status','direct-stdout','direct-stderr','resources.rmr','case-definition.json','gdb-state.py']+extra_outputs+[path.name for path in sorted(output.glob('native-view-*.bin'))]]+artifact_records,
                   seconds=dict(direct_execution=direct_seconds,gdb_execution=elapsed,total=time.perf_counter()-start))
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(state_path.read_text())


if __name__ == '__main__':
    main()
