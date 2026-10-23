#!/usr/bin/env python3
"""Check body/path guards without running GDB, Wine or a target executable."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile
import time

import collector_cache as cache


def rejected(name,call):
    try:call()
    except ValueError:return
    raise ValueError('collector guard accepted '+name)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('uncached',type=Path);parser.add_argument('cached',type=Path);args=parser.parse_args();started=time.perf_counter();passed=[]
    manifest=json.loads((args.cached/'collector-cache.json').read_text());body=(args.cached/'collector-body.py').read_bytes()
    if hashlib.sha256(body).hexdigest()!=manifest['body_sha256']:raise ValueError('cached body identity differs')
    for key,paths in manifest['checkpoints'].items():
        expected='python\nrecon_collect_checkpoint(%r,%r,%r,%r)\nend\n'%(key,manifest['body_sha256'],*paths)
        if (args.cached/key).read_text()!=expected:raise ValueError('frozen thin wrapper differs')
    original=(args.uncached/'collect.gdb').read_text();normalized,_,_=cache.normalize(original)
    if normalized.encode()!=body:raise ValueError('cached collector differs from actual historical observation body')
    with tempfile.TemporaryDirectory(prefix='collector-cache-guards-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
        root=Path(temporary);base=original.replace(str(args.uncached),str(root));first=root/'collect.gdb';second=root/'collect-second.gdb'
        valid_second=base.replace(repr(str(root/'state.json')),repr(str(root/'second.json'))).replace(repr(str(root/'native-argv.bin')),repr(str(root/'second-argv.bin')))
        for case in ('different-body','duplicate-path','outside-path'):
            first.write_text(base);second.write_text(valid_second)
            if case=='different-body':second.write_text(valid_second[:-4]+'# changed collector\nend\n')
            elif case=='duplicate-path':second.write_text(valid_second.replace(str(root/'second.json'),str(root/'state.json')))
            else:second.write_text(valid_second.replace(str(root/'second.json'),str(root.parent/'outside.json')))
            rejected(case,lambda:cache.prepare(root,first));passed.append(case)
        first.write_text(base);second.write_text(valid_second);startup=cache.prepare(root,first);namespace={'recon_collect_timings':[]};exec(startup[len('python\n'):-len('end\n')],namespace)
        registry=namespace['recon_checkpoint_paths'];key=next(iter(registry));state,argv=registry[key];digest=namespace['recon_body_sha'];call=namespace['recon_collect_checkpoint']
        for case,arguments in [('wrong-body-hash',(key,'0'*64,state,argv)),('unknown-checkpoint',('unknown',digest,state,argv)),('wrong-state-path',(key,digest,state+'.bad',argv)),('wrong-argv-path',(key,digest,state,argv+'.bad'))]:rejected(case,lambda a=arguments:call(*a));passed.append(case)
        path=root/'collector-body.py';path.write_bytes(path.read_bytes()+b'\n');rejected('body-changed-after-prepare',lambda:exec(startup[len('python\n'):-len('end\n')],{'recon_collect_timings':[]}));passed.append('body-changed-after-prepare')
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed,'actual_historical_body_identical':True,'thin_wrapper_count':len(manifest['checkpoints'])},indent=2))


if __name__=='__main__':main()
