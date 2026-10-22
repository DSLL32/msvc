"""Explicit native and original-toolchain builds with separate wall timings.

Reference/reconstruction tool only, outside ordinary fixture execution.
"""
import argparse
import json
from pathlib import Path
import subprocess
import time

BINARY=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('label')
    args=parser.parse_args()
    directory=Path.home()/'.ai-tmp/vc1.0-c1xx'
    records=[]
    receipt=directory/('queue-'+args.label+'-build.json')
    for label,command in [('native',['make','-j4','native','native-trace']),
                          ('canonical',['make','-j4','build/c1xx.exe','build/runtime.json'])]:
        started=time.perf_counter()
        path=directory/('queue-'+args.label+'-'+label+'.log')
        with path.open('w') as log:
            result=subprocess.run(command,cwd=BINARY,stdout=log,stderr=subprocess.STDOUT)
        records.append(dict(phase=label,seconds=time.perf_counter()-started,
                            status=result.returncode,command=command,log=str(path)))
        receipt.write_text(json.dumps(records,indent=2)+'\n')
        if result.returncode:
            print(path.read_text()[-4000:])
            return result.returncode
    print(receipt)
    return 0


if __name__=='__main__':raise SystemExit(main())
