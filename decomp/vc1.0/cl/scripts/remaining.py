#!/usr/bin/env python3
"""Paired masked-original replay for the remaining CL driver cases."""
import argparse
import concurrent.futures
import hashlib
import json
import re
from binary import PE, u16, u32
from pathlib import Path
import shutil
import subprocess
import compare

HERE = compare.wt.PROJECT
FIXTURES = HERE / 'cases/remaining'
SCRATCH = Path.home() / '.ai-tmp/vc1.0-cl/remaining'
CATALOG = {c['id']: c for name in ('catalog.json', 'internal.json', 'extra.json') for c in json.loads((FIXTURES/name).read_text())['cases']}


def run(label, exe, case):
    work = SCRATCH / label / case['id']
    prefix = SCRATCH / ('wine-' + label)
    if not (prefix/'system.reg').exists():
        subprocess.run(['cp', '-a', '--reflink=auto', str(compare.wt.PREFIX), str(prefix)], check=True)
    (SCRATCH/label).mkdir(parents=True, exist_ok=True)
    drive = prefix/'dosdevices/q:'
    if not drive.is_symlink(): drive.symlink_to(SCRATCH/label)
    visible_cwd = 'Q:\\' + case['id']
    if work.exists(): shutil.rmtree(work)
    work.mkdir(parents=True)
    for destination, source in case.get('files', {}).items():
        target = work / destination
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(FIXTURES/source, target)
    for destination, content in case.get('text_files', {}).items():
        target = work / destination
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content.encode('latin1'))
    substitutions = dict(bin=compare.wt.windows(compare.wt.TC/'bin'), cwd=visible_cwd,
                         proxy=compare.wt.windows(HERE/'build/pass-proxy.exe'),
                         fail=compare.wt.windows(HERE/'build/fail-pass.exe'),
                         wait=compare.wt.windows(HERE/'build/wait-pass.exe'))
    env = compare.wt.environment()
    env['WINEPREFIX'] = str(prefix)
    server = subprocess.run(['wineserver', '-p'], env=env)
    assert server.returncode in (0, 2), 'Wine server startup failed'
    for key in ('CL', '_CL_', '_MSC_IDE_FLAGS', 'LINK', '_LINK_', 'MSC_CMD_FLAGS'): env.pop(key, None)
    env.update({k: v.format(**substitutions) for k,v in case.get('environment', {}).items()})
    (SCRATCH/case['id']).mkdir(parents=True, exist_ok=True)
    stem = SCRATCH / case['id'] / label
    special = case.get('mode') or ('clock' if case['id'] == 'timing' else None)
    helper = 'paired-run.exe' if special else 'debug-run.exe'
    args = ['wine', compare.wt.windows(HERE/'build'/helper), compare.wt.windows(exe),
            case['arguments'].format(**substitutions)]
    args += [compare.wt.windows(str(stem)+'.'+suffix) for suffix in ('stdout','stderr','events')]
    args += [visible_cwd]
    if special:
        pe = PE(exe)
        names = compare.publics() if label == 'reconstructed' else {r['name']: r['start'] for r in json.loads(Path('functions.json').read_text())['driver']}
        running = compare.publics()['_'+'child_running'] if label == 'reconstructed' else 0x240d4
        coff = compare.publics()['_'+'coff_format'] if label == 'reconstructed' else 0x26018
        callbacks = compare.publics()['_ide_callbacks'] if label == 'reconstructed' else 0x240d0
        config = SCRATCH/case['id']/(label+'.config')
        config.write_text('%x %x %x %x %x %x\n' % (names['_control_handler@4']-pe.base, running-pe.base, names['_link_pass']-pe.base, coff-pe.base, names['_ide_message']-pe.base, callbacks-pe.base) + '%u %u %u %u\n' % tuple(case.get('clock_calendar', [2026, 1, 15, 0])))
        args += [compare.wt.windows(config), special]
    subprocess.run(args, env=env, check=True, timeout=90, stdin=subprocess.DEVNULL,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return Path(str(stem)+'.events').read_text()



def normalized_artifact(path):
    data = bytearray(path.read_bytes())
    ignored = []
    def zero(offset, size, reason):
        ignored.append(dict(offset=offset, size=size, reason=reason))
        data[offset:offset+size] = b'\0' * size
    def coff(base):
        zero(base+4, 4, 'COFF TimeDateStamp')
        for i in range(u16(data, base+2)):
            section = base+20+u16(data, base+16)+40*i
            if data[section:section+8].rstrip(b'\0') == b'.edata':
                zero(base+u32(data, section+20)+4, 4, 'export-directory TimeDateStamp')
    if data[:2] == b'MZ':
        pe = PE(path)
        header = u32(data, 0x3c)
        zero(header+8, 4, 'PE COFF TimeDateStamp')
        export = u32(data, pe.optional+96)
        if export: zero(pe.offset(pe.base+export)+4, 4, 'export-directory TimeDateStamp')
    elif data[:2] == b'\x4c\x01': coff(0)
    elif data[:8] == b'!<arch>\n':
        offset = 8
        while offset < len(data) and data[offset] != 0:
            assert data[offset+58:offset+60] == b'`\n'
            size = int(data[offset+48:offset+58].rstrip(b'\0 '))
            zero(offset+16, 12, 'archive-member decimal modification timestamp')
            if data[offset+60:offset+62] == b'\x4c\x01': coff(offset+60)
            offset += 60+size+(size&1)
    elif path.suffix.lower() == '.map':
        match = re.search(rb'(?m)^ Timestamp is [^\r\n]+', data)
        assert match, 'expected original LINK map timestamp line'
        ignored.append(dict(offset=match.start(), size=match.end()-match.start(), reason='map timestamp line'))
        data[match.start():match.end()] = b' Timestamp is <timestamp>'
    return bytes(data), ignored


def artifacts(case, baseline):
    labels = ['original', 'reconstructed'] + (['baseline'] if baseline else [])
    paths = {}
    for label in labels:
        root = SCRATCH/label/case['id']
        paths[label] = {str(p.relative_to(root)):p for p in root.rglob('*') if p.is_file()}
    names = set(paths['original'])
    assert all(set(paths[label]) == names for label in labels), 'case product sets differ'
    report = {}
    for name in sorted(names):
        reference = None
        report[name] = {}
        for label in labels:
            path = paths[label][name]
            content, ignored = normalized_artifact(path)
            if reference is None: reference = content
            assert content == reference, 'artifact differs outside documented timestamps: '+name+' '+label
            report[name][label] = dict(bytes=path.stat().st_size, sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                       normalized_sha256=hashlib.sha256(content).hexdigest(), ignored_fields=ignored)
    return report


def paired(case, accepted_count=None, baseline=True):
    orig, rec, cfg, oi, ri = compare.prepare(accepted_count)
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        a = pool.submit(run, 'original', HERE/'build/original.exe', case)
        b = pool.submit(run, 'reconstructed', HERE/'build/reconstructed.exe', case)
        ae, be = a.result(), b.result()
    al,bl = compare.logical(ae,orig,oi), compare.logical(be,rec,ri)
    print('%s: original %s; reconstruction %s' % (case['id'],al,bl),flush=True)
    assert al == bl, 'logical trap mismatch\n'+ae+be
    if al.startswith('exit:'):
        assert int(al[5:]) == case['expected_exit'], 'unexpected case exit'
        if baseline:
            baseline_events=run('baseline',compare.wt.TC/'bin/CL.EXE',case)
            assert baseline_events == ae, 'unmodified original exit differs'
        event_requirements = set(case.get('expected_events', []))
        mode = case.get('mode') or ('clock' if case['id'] == 'timing' else None)
        if mode == 'clock': event_requirements.update('clock-call %08x' % i for i in range(1, 5))
        if mode == 'omf-state': event_requirements.add('internal-omf-state 00000000')
        if mode == 'ide-null-buffer': event_requirements.add('internal-ide-null-buffer 00000000')
        for label in ['original','reconstructed'] + (['baseline'] if baseline else []):
            events = (SCRATCH/case['id']/(label+'.events')).read_text().splitlines()
            assert event_requirements <= set(events), 'controlled input was not delivered: '+label
        for stream in ('stdout','stderr'):
            expected=(SCRATCH/case['id']/('original.'+stream)).read_bytes()
            for label in ['reconstructed'] + (['baseline'] if baseline else []):
                actual=(SCRATCH/case['id']/(label+'.'+stream)).read_bytes()
                assert actual == expected, '%s differs: %s' % (stream,label)
        product_report = artifacts(case, baseline)
        validation = dict(id=case['id'], arguments=case['arguments'], exit_status=int(al[5:]),
                          accepted_count=len(cfg['accepted']), target_sha256=cfg['target']['sha256'],
                          reconstruction_sha256=hashlib.sha256((HERE/'build/cl.exe').read_bytes()).hexdigest(),
                          controlled_clock_calendar=case.get('clock_calendar', [2026,1,15,0]) if mode == 'clock' else None,
                          products=product_report, mode=case.get('mode'), internal_state=case.get('state'),
                          events={label:(SCRATCH/case['id']/(label+'.events')).read_text() for label in ['original','reconstructed'] + (['baseline'] if baseline else [])},
                          stdout=(SCRATCH/case['id']/'original.stdout').read_bytes().decode('latin1'),
                          stderr=(SCRATCH/case['id']/'original.stderr').read_bytes().decode('latin1'), streams={stream: hashlib.sha256((SCRATCH/case['id']/('original.'+stream)).read_bytes()).hexdigest() for stream in ('stdout','stderr')})
        (SCRATCH/case['id']/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        print('Exit, raw streams and products match'+(' the unmodified original.' if baseline else '.'),flush=True)
    return al



def record_validation():
    cfg = json.loads(Path('functions.json').read_text())
    reconstruction_hash = hashlib.sha256((HERE/'build/cl.exe').read_bytes()).hexdigest()
    assert all(r['name'] in cfg['accepted'] for r in cfg['driver']), 'unaccepted source function'
    cases = []
    for name, case in CATALOG.items():
        report = json.loads((SCRATCH/name/'validation.json').read_text())
        assert report['id'] == name and report['arguments'] == case['arguments']
        assert report['target_sha256'] == cfg['target']['sha256']
        assert report['reconstruction_sha256'] == reconstruction_hash, 'stale validation: '+name
        assert report['accepted_count'] == len(cfg['accepted']), 'stale accepted list: '+name
        assert report['exit_status'] == case['expected_exit']
        assert set(report['events']) == {'original','reconstructed','baseline'}
        cases.append(report)
    report = dict(target_sha256=cfg['target']['sha256'], reconstruction_sha256=reconstruction_hash,
                  method='Concurrent paired masked-image execution, followed by the unmodified original; exact streams and product bytes except parsed timestamps',
                  accepted_frontiers=len(cfg['accepted']), implemented_source_functions=len(cfg['driver']),
                  native_driver_functions=sum(0x17010 <= r['start'] < 0x1b081 for r in cfg['driver']),
                  historical_original_cases=['help','compile','hello'], cases=cases)
    (FIXTURES/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Recorded %d paired case validations.' % len(cases))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases',nargs='*',choices=CATALOG)
    parser.add_argument('--all', action='store_true', help='run every catalog, internal-state and timezone case')
    parser.add_argument('--record', action='store_true', help='save existing current-build full-suite evidence to cases/remaining/validation.json')
    parser.add_argument('--runtime',action='store_true')
    parser.add_argument('--accept',help='accept this source function after asserting its matching frontier')
    parser.add_argument('--replay',action='store_true')
    args=parser.parse_args()
    if args.record and not args.cases and not args.all:
        record_validation()
        return
    if not args.cases and not args.all: parser.error('provide case IDs or --all')
    for case_id in args.cases or list(CATALOG):
        case=CATALOG[case_id]
        while True:
            cfg=json.loads(Path('functions.json').read_text())
            key='remaining-'+case_id
            if key not in cfg['cases']:
                cfg['cases'][key] = dict(arguments=case['arguments'], catalog_id=case_id, accepted_from=len(cfg['accepted']))
                Path('functions.json').write_text(json.dumps(cfg,indent=2)+'\n')
            record = cfg['cases'][key]
            if args.replay:
                for count in range(record['accepted_from'],record['accepted_count']):
                    assert paired(case,count,False) == cfg['accepted'][count]
                assert paired(case).startswith('exit:'), 'completed case still traps: '+case_id
                break
            site=paired(case)
            if site.startswith('exit:'):
                if 'accepted_count' not in record:
                    cfg['cases'][key]['accepted_count'] = len(cfg['accepted'])
                    Path('functions.json').write_text(json.dumps(cfg,indent=2)+'\n')
                break
            runtime=next((r for r in compare.indexes()[3] if r['name']==site and 'library' in r),None)
            if site == args.accept or (args.runtime and runtime):
                cfg['accepted'].append(site)
                Path('functions.json').write_text(json.dumps(cfg,indent=2)+'\n')
                if site==args.accept:args.accept=None
                continue
            if args.all and not args.runtime and not args.accept:
                raise RuntimeError('completed case still traps: '+case_id+' '+site)
            break

    if args.record: record_validation()

if __name__=='__main__':main()
