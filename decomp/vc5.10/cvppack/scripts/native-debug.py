#!/usr/bin/env python3
"""Linux GDB inspection; no original execution or reference refresh."""
import argparse
import importlib.util
import json
import os
import resource
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
    start = time.perf_counter()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / 'configuration.json').write_bytes(args.config.read_bytes())
    (output / 'data.json').write_bytes((BINARY / 'data.json').read_bytes())
    from provenance import snapshot
    snapshot(output/'compiled-sources.json',args.content_store)
    config = json.loads((output / 'configuration.json').read_text())
    data = json.loads((output / 'data.json').read_text())
    commands = output / 'commands.gdb'
    state_path = output / 'state.json'
    commands.write_text('set pagination off\nset confirm off\nset disable-randomization off\n'
        'handle SIGTRAP stop print nopass\nrun /help >' + str(output / 'stdout') + ' 2>' + str(output / 'stderr') + '\n'
        'python\nimport gdb,json\nf=gdb.newest_frame()\n'
        'state={"function":f.name(),"pc":int(gdb.parse_and_eval("$pc")),"signal":gdb.execute("info program",to_string=True)}\n'
        'record=next((r for r in '+repr(config['functions'])+' if r["native_symbol"]==f.name()),None)\n'
        'parameters={}\n'
        'for item in (record or {}).get("parameters",[]):\n'
        ' v=f.read_var(item["name"])\n'
        ' if item["kind"]=="string": parameters[item["name"]]={"null":int(v)==0,"bytes":(v.string(encoding="latin1").encode("latin1")+b"\\0").hex() if int(v) else None}\n'
        ' elif item["kind"]=="argv": parameters[item["name"]]=[(v[i].string(encoding="latin1").encode("latin1")+b"\\0").hex() for i in range(int(f.read_var("argc")))]\n'
        ' else: parameters[item["name"]]=int(v) & 0xffffffff\n'
        'state["parameters"]=parameters\n'
        'state["globals"]={item["name"]:bytes(gdb.selected_inferior().read_memory(gdb.parse_and_eval("&"+item["name"]),item["size"])).hex() for item in '+repr(data)+'}\n'
        'open(' + repr(str(state_path)) + ',"w").write(json.dumps(state,indent=2))\nend\nquit\n')
    env = os.environ.copy()
    env.pop('RECON_TRACE_FD', None)
    reference = json.loads((args.original/'receipt.json').read_text()) if args.original else None
    env['VC510_CVPACK_ARGV0'] = reference['canonical_argv0'] if reference else 'R:\\toolchains\\5.10\\bin\\CVPACK.EXE'
    env['RECON_PROGRAM_PATH'] = reference['actual_target_windows'] if reference else env['VC510_CVPACK_ARGV0']
    spec=importlib.util.spec_from_file_location('native_resources',BINARY/'scripts/native-resources.py')
    resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
    resource_provider=resources.prepare(output/'resources.rmr')
    env['VC510_CVPACK_RESOURCES']=str(output/'resources.rmr')
    direct_start=time.perf_counter()
    entry_filter=output/'entry-filter'
    entry_filter.write_text(''.join(record['native_symbol']+'\n' for record in config['functions']))
    (output/'trace-seen').write_bytes(b'')
    def no_core():
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    with (output/'entry-trace').open('wb') as trace,(output/'direct-stdout').open('wb') as out,(output/'direct-stderr').open('wb') as err:
        traced=dict(env,RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(entry_filter),RECON_TRACE_SEEN=str(output/'trace-seen'))
        direct=subprocess.run([str(BINARY/'build/native/CVPACK'),'/help'],cwd=output,env=traced,
            stdout=out,stderr=err,stdin=subprocess.DEVNULL,pass_fds=(trace.fileno(),),timeout=10,preexec_fn=no_core)
    (output/'native-status').write_text(str(direct.returncode)+'\n')
    direct_seconds=time.perf_counter()-direct_start
    execute = time.perf_counter()
    if direct.returncode>=0:
        state_path.write_text(json.dumps(dict(terminal='exit',exit_status=direct.returncode),indent=2)+'\n')
        (output/'gdb-stdout').write_bytes(b'')
        (output/'gdb-stderr').write_bytes(b'')
        for name in ('stdout','stderr'):
            (output/name).write_bytes((output/('direct-'+name)).read_bytes())
        elapsed=0.0
    else:
        result = subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(commands), str(BINARY / 'build/native/CVPACK')],
                            cwd=output, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
        elapsed = time.perf_counter() - execute
        (output / 'gdb-stdout').write_bytes(result.stdout)
        (output / 'gdb-stderr').write_bytes(result.stderr)
        assert result.returncode == 0 and state_path.is_file(), result.stderr
        assert direct.returncode == -5, 'partial native debugger capture must actually trap'
    assert (output/'direct-stdout').read_bytes()==(output/'stdout').read_bytes()
    assert (output/'direct-stderr').read_bytes()==(output/'stderr').read_bytes()
    receipt = dict(schema=1, case='help', native_sha256=sha(BINARY / 'build/native/CVPACK'),
                   configuration_sha256=sha(output / 'configuration.json'),
                   rebuilt_sha256=sha(BINARY / 'build/CVPACK.EXE'),
                   data_sha256=sha(output / 'data.json'),
                   original_receipt_sha256=sha(args.original/'receipt.json') if args.original else None,
                   executed_command=[str(BINARY/'build/native/CVPACK'),'/help'],
                   compiled_sources_sha256=sha(output/'compiled-sources.json'),
                   prepared_resources=resource_provider,
                   reused_original_capture=bool(reference and (args.original/'configuration.json').read_bytes()==(output/'configuration.json').read_bytes()
                       and (reference['native_sha256']!=sha(BINARY/'build/native/CVPACK') or reference['rebuilt_sha256']!=sha(BINARY/'build/CVPACK.EXE'))),
                   outputs=[dict(name=name,bytes=(output/name).stat().st_size,sha256=sha(output/name))
                            for name in ('stdout','stderr','gdb-stdout','gdb-stderr','state.json','entry-trace','entry-filter','native-status','direct-stdout','direct-stderr','resources.rmr')],
                   seconds=dict(direct_execution=direct_seconds,gdb_execution=elapsed,total=time.perf_counter()-start))
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(state_path.read_text())


if __name__ == '__main__':
    main()
