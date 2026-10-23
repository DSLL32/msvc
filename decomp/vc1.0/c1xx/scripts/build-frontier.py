"""Explicit native and original-toolchain builds with separate wall timings.

Reference/reconstruction tool only, outside ordinary fixture execution.
"""
import argparse
from case_paths import CAPTURE_ROOT
import json
from pathlib import Path
import subprocess
import time
from shared_build_inputs import snapshot, materialize, signature, inputs

BINARY=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('label')
    parser.add_argument('--native-only',action='store_true')
    args=parser.parse_args()
    directory=CAPTURE_ROOT
    directory.mkdir(parents=True,exist_ok=True)
    records=[]
    receipt=directory/('queue-'+args.label+'-build.json')
    commands=[('native',['make','-j4','native','native-trace'])]
    if not args.native_only:
        commands.append(('canonical',['make','-j4','build/c1xx.exe','build/runtime.json','graph-audit-tool','build/graph-metadata.json']))
    for label,command in commands:
        setup=time.perf_counter()
        shared=snapshot()
        compiled=materialize(shared)
        command.append('RECON='+str(compiled.relative_to(BINARY)))
        setup_seconds=time.perf_counter()-setup
        started=time.perf_counter()
        path=directory/('queue-'+args.label+'-'+label+'.log')
        with path.open('w') as log:
            result=subprocess.run(command,cwd=BINARY,stdout=log,stderr=subprocess.STDOUT)
        stable=signature(inputs(compiled))[0]==shared['manifest_sha256']
        records.append(dict(phase=label,seconds=time.perf_counter()-started,
                            status=result.returncode,command=command,log=str(path),
                            shared_inputs=shared,shared_input_setup_seconds=setup_seconds,
                            shared_inputs_unchanged_during_build=stable))
        receipt.write_text(json.dumps(records,indent=2)+'\n')
        if not stable:
            print('Immutable compiled inputs changed during build; dependent acceptance stopped.')
            return 1
        if result.returncode:
            print(path.read_text()[-4000:])
            return result.returncode
    print(receipt)
    return 0


if __name__=='__main__':raise SystemExit(main())
