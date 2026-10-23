#!/usr/bin/env python3
"""Focused acceptance/snapshot and ownership rejections; no execution of Wine."""
import hashlib
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from accept import validate_pair
from audit import ownership
from state import BINARY, load



def update_receipt(directory, **changes):
    path = directory / 'receipt.json'
    receipt = json.loads(path.read_text())
    receipt.update(changes)
    for item in receipt['outputs']:
        raw = (directory / item['name']).read_bytes()
        item.update(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
    path.write_text(json.dumps(receipt))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pair',type=Path,required=True)
    parser.add_argument('--native-name',default='native')
    parser.add_argument('--audit',type=Path)
    parser.add_argument('--config',type=Path,default=BINARY/'functions.json')
    args=parser.parse_args()
    source=args.pair.resolve()
    from binary import PE
    config=json.loads(args.config.read_text());pe=PE(BINARY.parents[2]/config['target']['path'])
    assert Path(args.native_name).name==args.native_name
    baseline=validate_pair(args.config, source / 'original', source / args.native_name,args.audit)
    tests = []
    def reject(name, change, audit_change=None):
        with tempfile.TemporaryDirectory(prefix='vc510-cvpack-negative-', dir=Path.home()/'.ai-tmp') as temp:
            trial = Path(temp)
            shutil.copytree(source, trial / 'pair')
            cfg = trial / 'functions.json'
            cfg.write_bytes(args.config.read_bytes())
            original, native = trial / 'pair/original', trial / 'pair' / args.native_name
            audit_path=None
            if args.audit:
                shutil.copytree(args.audit.resolve().parent,trial/'audit')
                audit_path=trial/'audit'/args.audit.name
            change(cfg, original, native)
            if audit_change:
                selected=audit_path or original/'audit.json'
                audit_change(selected)
                if selected==original/'audit.json':update_receipt(original,audit_sha256=hashlib.sha256(selected.read_bytes()).hexdigest())
            # Avoid an earlier cross-receipt-hash failure hiding the property
            # being tested. Preserve all stronger raw-byte/current-input checks.
            native_receipt=json.loads((native/'receipt.json').read_text())
            native_receipt['original_receipt_sha256']=hashlib.sha256((original/'receipt.json').read_bytes()).hexdigest()
            (native/'receipt.json').write_text(json.dumps(native_receipt))
            try:
                validate_pair(cfg, original, native,audit_path)
            except (AssertionError, KeyError, ValueError):
                tests.append(name)
            else:
                raise AssertionError('mutation was accepted: ' + name)
    reject('stale-original-snapshot', lambda c,o,n: (o/'configuration.json').write_text('{}'))
    reject('stale-native-snapshot', lambda c,o,n: (n/'configuration.json').write_text('{}'))
    reject('matching-stale-snapshots', lambda c,o,n: [(d/'configuration.json').write_text('{}') for d in (o,n)])
    reject('wrong-trial-config', lambda c,o,n: c.write_text(c.read_text()+'\n'))
    reject('changed-raw-original', lambda c,o,n: (o/'events').write_text('frontier 00404620\n'))
    if baseline['terminal']=='trap':
        reject('changed-raw-native', lambda c,o,n: (n/'gdb-stdout').write_text(''))
    else:
        reject('changed-raw-native-output', lambda c,o,n: (n/'stdout').write_text('changed'))
    def missing(c,o,n):
        (o/'events').write_text('')
        (n/'gdb-stdout').write_text('')
        update_receipt(o)
        update_receipt(n)
    reject('both-missing-frontiers-even-with-matching-receipt-hashes', missing)
    def wrong(c,o,n):
        (o/'events').write_text('frontier 00404620\n')
        update_receipt(o)
    reject('wrong-real-original-frontier', wrong)
    reject('wrong-provider', lambda c,o,n: update_receipt(o,actual_loaded_crt={'bytes':277776,'sha256':'0'*64}))
    reject('unknown-candidate', lambda c,o,n: update_receipt(o,candidates=['invented_candidate']))
    reject('stale-native-binary', lambda c,o,n: update_receipt(n,native_sha256='0'*64))
    reject('stale-Win32-binary', lambda c,o,n: update_receipt(n,rebuilt_sha256='0'*64))
    reject('unmasked-not-acceptance', lambda c,o,n: update_receipt(o,unmasked=True))
    reject('wrong-mask', lambda c,o,n: update_receipt(o,masked_image_sha256='0'*64))
    def provider_bytes(c,o,n):
        receipt=json.loads((o/'receipt.json').read_text())
        fake=o/'wrong-loaded-provider.dll'
        raw=bytearray(Path(receipt['actual_loaded_crt']['capture_path']).read_bytes())
        raw[-1]^=1
        fake.write_bytes(raw)
        receipt['actual_loaded_crt']['capture_path']=str(fake)
        # Attacker updates the listed file hash while retaining canonical claim.
        receipt['outputs'].append(dict(name=fake.name,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
        (o/'receipt.json').write_text(json.dumps(receipt))
    reject('wrong-actual-provider-bytes-despite-canonical-claim-and-refreshed-file-hash',provider_bytes)
    def native_state(c,o,n,field,value):
        path=n/'state.json';state=json.loads(path.read_text())
        state[field[0]][field[1]]=value
        path.write_text(json.dumps(state));update_receipt(n)
    if baseline['terminal']=='trap':
        reject('wrong-typed-string',lambda c,o,n:native_state(c,o,n,('parameters','file'),dict(null=False,bytes='62616400')))
        reject('wrong-typed-integer',lambda c,o,n:native_state(c,o,n,('parameters','resources'),0))
        reject('wrong-app-global',lambda c,o,n:native_state(c,o,n,('globals','show_logo'),'00000000'))
        reject('unproved-live-pointer',lambda c,o,n:native_state(c,o,n,('globals','debug_directory'),'01000000'))
    else:
        def status(c,o,n,value):
            (n/'native-status').write_text(value);update_receipt(n)
        reject('missing-actual-native-exit',lambda c,o,n:status(c,o,n,''))
        reject('wrong-actual-native-exit',lambda c,o,n:status(c,o,n,'0\n'))
        def exit_line(c,o,n,value):
            path=o/'events';path.write_text(path.read_text().replace('exit 1\n',value));update_receipt(o)
        reject('missing-real-original-completion',lambda c,o,n:exit_line(c,o,n,''))
        reject('wrong-real-original-completion',lambda c,o,n:exit_line(c,o,n,'exit 0\n'))
    def wrong_audit(c,o,n):
        # Change the immutable historical audit; a refresh cannot detach from it.
        path=o/'audit.json';audit=json.loads(path.read_text());audit['config_sha256']='0'*64
        path.write_text(json.dumps(audit));update_receipt(o,audit_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    reject('historical-audit-detached-from-trial',wrong_audit)
    def prefix_only(path):
        audit=json.loads(path.read_text());audit['candidates']=[];audit['checked_functions']=[];audit['graph']['checked_functions']=0
        path.write_text(json.dumps(audit))
    reject('accepted-prefix-audit-is-not-candidate-audit',lambda c,o,n:None,prefix_only)
    def omitted_entry(c,o,n):
        path=n/'entry-trace';lines=path.read_text().splitlines()
        candidate=json.loads((o/'receipt.json').read_text())['candidates'][0]
        name=next(r['native_symbol'] for r in config['functions'] if r['name']==candidate)
        path.write_text('\n'.join(line for line in lines if line!=name)+'\n');update_receipt(n)
    reject('omitted-native-candidate-entry',omitted_entry)
    def unused_candidate(c,o,n):
        path=o/'events';candidate=json.loads((o/'receipt.json').read_text())['candidates'][0]
        address=next(r['original'] for r in config['functions'] if r['name']==candidate)
        path.write_text(path.read_text().replace('entry %08x\n'%address,''));update_receipt(o)
    reject('candidate-never-entered-original',unused_candidate)
    candidate = json.loads(json.dumps(config))
    candidate['functions'][0].update(status='candidate', extent=[0x404620,0x4048b5])
    ownership(candidate, pe)
    other = dict(candidate['functions'][0],name='overlap',symbol='_overlap',original=0x404630,extent=[0x404630,0x404640])
    candidate['functions'].append(other)
    try:
        ownership(candidate, pe)
    except AssertionError:
        tests.append('overlapping-application-owners')
    else:
        raise AssertionError('overlap was accepted')
    output = BINARY / 'build/rejections.json'
    output.write_text(json.dumps({'schema':1,'rejected':tests,'count':len(tests)},indent=2)+'\n')
    print(len(tests), 'negative checks passed')


if __name__ == '__main__':
    main()
