"""Explicit masked-original C1XX capture, never called by native fixtures.

Produces new scratch evidence only. It does not update cached references and
never executes the reconstructed Win32 binary.
"""
import argparse
import hashlib
import fcntl
import re
import shutil
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import paths, CAPTURE_ROOT, canonical_parts, SCRATCH
from binary import PE
from win32_indexes import imports, mask, logical
from argument_words import observation as argument_observation

BINARY = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--accepted-count', type=int)
    parser.add_argument('--entry-plan', action='store_true', help='capture and assert the accepted application first-entry sequence')
    parser.add_argument('--entry-owner', help='capture only this accepted owner entry; labelled separately from full case projection')
    parser.add_argument('--candidate', help='explicit proposed function to unmask for fresh acceptance evidence')
    parser.add_argument('--pending-accepted', type=Path, help='explicit proposed prefix additions for a runtime boundary chain')
    parser.add_argument('--argument-words', type=int, choices=(4, 6), default=4,
        help='explicit raw stack-word count for a proved current ABI; default remains four')
    parser.add_argument('--ownership-register', choices=('esi', 'ebx', 'edi', 'eax', 'ebp', 'esp', 'eax,esp', 'edx', 'edi,esi', 'esi,ebx,ebp', 'esi,eax', 'edi,esi,eax'),
        help='explicit caller-owned allocation anchor; not an extra callee ABI input')
    parser.add_argument('--observe-state', action='append', default=[], metavar='NAME=VA',
        help='explicit diagnostic-only extra original DWORD observation; never a fixture oracle')
    parser.add_argument('--observe-short', action='append', default=[], metavar='NAME=VA',
        help='explicit exact two-byte diagnostic read; zero-extension encodes only that WORD')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('c1xx_original_tool', BINARY / 'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    configuration = (BINARY / 'functions.json').read_bytes()
    config = json.loads(configuration)
    observations=[]
    for observation in args.observe_state:
        name,address=observation.split('=',1)
        observations.append(dict(name=name,original=int(address,0),bytes=4))
    for observation in args.observe_short:
        name,address=observation.split('=',1)
        observations.append(dict(name=name,original=int(address,0),bytes=2))
    count = len(config['accepted']) if args.accepted_count is None else args.accepted_count
    assert 0 <= count <= len(config['accepted'])
    image = PE(tool.TC / 'bin/C1XX3232.EXE')
    assert hashlib.sha256(image.data).hexdigest() == config['target']['sha256']
    drivers = {r['start'] for r in config['driver']}
    records = [r for r in json.loads((BINARY / 'runtime.json').read_text()) if r['start'] not in drivers]
    records += imports(image) + config['driver']
    accepted = config['accepted'][:count]
    assert not args.entry_owner or (args.entry_plan and args.entry_owner in accepted), 'owner entry must already be accepted'
    pending = json.loads(args.pending_accepted.read_text()) if args.pending_accepted else []
    assert not pending or count == len(config['accepted'])
    assert all(name not in accepted for name in pending) and len(set(pending)) == len(pending)
    assert all(any(r['name'] == name for r in records) for name in pending)
    accepted += pending
    count = len(accepted)
    if args.candidate:
        assert args.candidate not in accepted, 'candidate already accepted'
        assert any(r['name'] == args.candidate for r in records), 'unknown candidate'
        accepted = accepted + [args.candidate]
    CAPTURE_ROOT.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='original-capture-', dir=CAPTURE_ROOT))
    (directory / 'configuration.json').write_bytes(configuration)
    _,fixture,_,_=paths(config,args.case)
    phase = json.loads((fixture / 'manifest.json').read_text())['phases']['c1']
    canonical = phase['canonical_working_directory']
    work = SCRATCH.joinpath(*canonical_parts(canonical))
    lock = (CAPTURE_ROOT/'capture.lock').open('a')
    fcntl.flock(lock, fcntl.LOCK_EX)
    work.mkdir(parents=True, exist_ok=True)
    for existing in work.iterdir():
        assert existing.is_file() and not existing.is_symlink(), str(existing)
        existing.unlink()
    for item in phase['inputs']:
        data = (fixture / item['path']).read_bytes()
        assert len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256']
        (work / Path(item['path']).name).write_bytes(data)
    executable = directory / 'masked-original.exe'
    mask(image, records, accepted, executable)
    plan_names=[]
    if args.entry_plan:
        sources={}
        for line in (BINARY/'cases/canonical-map.txt').read_text().splitlines():
            match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+f\s+(\S+)',line)
            if match and ':' not in match[2]:sources[match[1]]=Path(match[2]).stem
        first=config['cases'][args.case].get('accepted_from',0)
        plan_names=[name for name in accepted[first:] if name in sources and sources[name]!='runtime']
        if args.entry_owner:
            assert args.entry_owner in sources and sources[args.entry_owner]!='runtime'
            plan_names=[args.entry_owner]
        by_name={r['name']:r for r in records}
        data=bytearray(executable.read_bytes())
        rows=[]
        for name in plan_names:
            record=by_name[name]; offset=image.offset(record['start'])
            rows.append('%x %02x %s\n'%(record['start']-image.base,image.data[offset],name))
            data[offset]=0xcc
        executable.write_bytes(data)
        (directory/'entries.plan').write_text(''.join(rows))
    flags = phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC / 'bin'), include=tool.windows(tool.TC / 'include'))
    command = ['wine', tool.windows(BINARY / 'build/debug-run.exe'), tool.windows(executable), '',
               tool.windows(directory / 'stdout.bin'), tool.windows(directory / 'stderr.bin'),
               tool.windows(directory / 'events.txt'), canonical, 'MSC_CMD_FLAGS=' + flags,
               phase['command_line_template'].format(bin=tool.windows(tool.TC / 'bin')),
               ','.join('%x' % (s['original'] - image.base) for s in config['cases'][args.case].get('buffered_streams', config['buffered_streams'])),
               ','.join('%x:%s' % (s['original'] - image.base, 'w' if s.get('bytes')==2 else ('p' if s.get('pointer') else 'v')) for s in config['checkpoint_state']+observations)]
    if args.entry_plan: command.append(tool.windows(directory/'entries.plan'))
    started = time.perf_counter()
    environment = tool.environment()
    environment['C1XX_CAPTURE_ARGUMENT_WORDS'] = str(args.argument_words)
    environment.pop('C1XX_CAPTURE_OWNERSHIP_REGISTER', None)
    if args.ownership_register:
        environment['C1XX_CAPTURE_OWNERSHIP_REGISTER'] = args.ownership_register
    subprocess.run(command, env=environment, check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    capture_seconds=time.perf_counter()-started
    shutil.copytree(work,directory/'case')
    fcntl.flock(lock,fcntl.LOCK_UN); lock.close()
    receipt = dict(case=args.case,scope='explicit original-only reference diagnostic; no cache update',
        accepted_count=count, terminal=logical((directory / 'events.txt').read_text(), image, records),
        candidate=args.candidate, pending_accepted=pending, configuration_sha256=hashlib.sha256(configuration).hexdigest(),
        capture_seconds=capture_seconds, original_sha256=config['target']['sha256'],
        outputs={p.name: dict(bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in work.iterdir() if p.is_file()})
    receipt['canonical_working_directory']=canonical
    events = (directory/'events.txt').read_text()
    if re.search(r'^argument ', events, re.M):
        receipt['argument_observation'] = argument_observation(events, args.argument_words)
    receipt['observer_sha256'] = hashlib.sha256((BINARY/'build/debug-run.exe').read_bytes()).hexdigest()
    if args.ownership_register:
        receipt['ownership_register_observations'] = []
        for register in args.ownership_register.split(','):
            anchors = re.findall(r'^ownership-anchor '+register+r' ([0-9a-f]{8})$', events, re.M)
            assert len(anchors) == 1, 'missing/unreadable requested ownership register'
            receipt['ownership_register_observations'].append(dict(register=register, value=int(anchors[0],16), read_succeeded=True))
    if observations:
        states=re.findall(r'^state \d+ ([0-9a-f]+)$',(directory/'events.txt').read_text(),re.M)
        assert len(states)==len(config['checkpoint_state'])+len(observations)
        receipt['diagnostic_observations']=[dict(field,value=int(value,16)) for field,value in zip(observations,states[len(config['checkpoint_state']):])]
        for index,field in enumerate(receipt['diagnostic_observations']):
            field.update(read_bytes=field['bytes'],read_succeeded=True)
            if field['bytes']==2:
                pattern=r'^observation %d address %08x requested 2 actual 2 success 1$' % (len(config['checkpoint_state'])+index,field['original'])
                assert re.search(pattern,events,re.M), 'missing/short/unreadable exact WORD read'
                field['encoding']='zero-extended unsigned WORD; adjacent bytes not read'
    if args.entry_plan:
        observed=re.findall(r'^entry (\S+) rva [0-9a-f]+ byte [0-9a-f]+$',(directory/'events.txt').read_text(),re.M)
        assert observed==plan_names,(observed,plan_names)
        projection = dict(accepted_first=first,accepted_last=len(accepted),
            accepted_prefix_sha256=hashlib.sha256(json.dumps(accepted,separators=(',',':')).encode()).hexdigest(),
            expected=plan_names,observed=observed,
            events_sha256=hashlib.sha256((directory/'events.txt').read_bytes()).hexdigest(),
            plan_sha256=hashlib.sha256((directory/'entries.plan').read_bytes()).hexdigest())
        if args.entry_owner:
            projection['scope']='single explicitly selected accepted ownership caller entry; not a full case trace'
            receipt['ownership_entry_projection']=projection
        else: receipt['first_entry_projection']=projection
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(directory / 'receipt.json')


if __name__ == '__main__':
    main()
