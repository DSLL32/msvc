"""Shared ordinary-case historical replay; adapters retain their own oracles.

All paths passed to this API are host paths except application, command_line and
cwd (Windows strings). prepare_side writes one masked image and restore plan;
run_pair starts the two independent walkers concurrently and checks receipts.
"""
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import signal
import threading
import time

HERE = Path(__file__).resolve().parent
MAGIC = 0x32505246


def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def freeze(paths):
    return {str(Path(p).resolve()): sha256(p) for p in sorted(set(map(str, paths)))}


def verify_frozen(hashes):
    for path, digest in hashes.items():
        if sha256(path) != digest:
            raise RuntimeError('frozen replay input changed: ' + path)


def build_helper():
    subprocess.run(['make', '-C', str(HERE), 'build/walker.exe'], check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return HERE / 'build/walker.exe'


def _blob(output, value):
    if isinstance(value, str):
        if '\0' in value:
            raise ValueError('embedded NUL in plan string')
        value = value.encode('cp1252')
    output.write(struct.pack('<I', len(value)))
    output.write(value)


def _record(pe, records, name, restore=True):
    found = [r for r in records if r['name'] == name]
    if not found:
        raise ValueError('missing indexed logical function: ' + name)
    entries = sorted({r['start'] - pe.base for r in found})
    chunks = {}
    if restore:
        for r in found:
            for start, end in [(r['start'], r['end']), *r.get('shared', [])]:
                if not start < end:
                    raise ValueError('invalid restore extent: ' + name)
                data = pe.read(start, end - start)
                if len(data) != end - start:
                    raise ValueError('unbacked restore extent: ' + name)
                chunks[(start - pe.base, end - start)] = data
        for entry in entries:
            if not any(rva <= entry < rva + size for rva, size in chunks):
                raise ValueError('entry absent from restore ranges: ' + name)
    return dict(name=name, entries=entries, chunks=chunks)


def _write_record(output, record):
    _blob(output, record['name'])
    output.write(struct.pack('<II', len(record['entries']), len(record['chunks'])))
    for entry in record['entries']:
        output.write(struct.pack('<I', entry))
    for (rva, size), data in sorted(record['chunks'].items()):
        output.write(struct.pack('<II', rva, size))
        output.write(data)


@dataclass
class Side:
    directory: Path
    image: Path
    plan: Path
    receipt: Path
    stdout: Path
    stderr: Path
    wine_env: dict
    expected: list
    first: int
    terminal: str
    timeout: float
    frozen: dict


def prepare_side(*, directory, pe, records, accepted, first, last, terminal,
                 cwd, environment, wine_env, mask_image, image_path=None,
                 application=None, command_line=None, stdout=None, stderr=None,
                 frozen_paths=(), timeout=90):
    """Prepare a side using adapter's audited mask_image(pe,records,names,path).

    terminal is 'exit:N' or the indexed logical name of an unaccepted entry.
    environment is an exact Windows environment dict, or None to inherit the
    Wine launch environment. command_line includes the intended argv[0].
    image_path/application allow adapter-owned canonical executable namespaces.
    first/last are absolute indices into accepted; receipts use absolute indices.
    """
    if not 0 <= first <= last <= len(accepted):
        raise ValueError('invalid acceptance segment')
    if len(set(accepted[:last])) != last:
        raise ValueError('duplicate accepted logical names')
    directory = Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    image = Path(image_path).resolve() if image_path else directory / 'masked.exe'
    image.parent.mkdir(parents=True, exist_ok=True)
    stdout = Path(stdout).resolve() if stdout else directory / 'stdout'
    stderr = Path(stderr).resolve() if stderr else directory / 'stderr'
    stdout.parent.mkdir(parents=True, exist_ok=True)
    stderr.parent.mkdir(parents=True, exist_ok=True)
    frozen = freeze(frozen_paths)
    mask_image(pe, records, accepted[:first], image)
    steps = [_record(pe, records, name) for name in accepted[first:last]]
    if terminal.startswith('exit:'):
        terminal_kind, exit_code = 0, int(terminal.split(':', 1)[1])
        terminal_record = dict(name=terminal, entries=[], chunks={})
    else:
        if terminal in accepted[:last]:
            raise ValueError('terminal must remain masked: ' + terminal)
        terminal_kind, exit_code = 1, 0
        terminal_record = _record(pe, records, terminal, restore=False)
    # PE SizeOfImage, independent of the adapters' PE-reader field names.
    pe_offset = struct.unpack_from('<I', pe.data, 60)[0]
    image_size = struct.unpack_from('<I', pe.data, pe_offset + 24 + 56)[0]
    if not image_size:
        raise ValueError('missing PE image size')
    application = application or windows(image)
    command_line = command_line or ('"' + application + '"')
    if environment is None:
        environment_bytes = b''
    else:
        items = sorted(environment.items(), key=lambda item: item[0].upper())
        if any('\0' in str(k) + str(v) for k, v in items):
            raise ValueError('embedded NUL in environment')
        environment_bytes = ('\0'.join(str(k) + '=' + str(v) for k, v in items)
                             + ('\0\0' if items else '\0\0')).encode('cp1252')
    plan = directory / 'restore.plan'
    with plan.open('wb') as output:
        output.write(struct.pack('<7I', MAGIC, pe.base, image_size,
                                 int(timeout * 1000), len(steps), terminal_kind,
                                 exit_code))
        for value in (application, command_line, str(cwd), windows(stdout),
                      windows(stderr), environment_bytes):
            _blob(output, value)
        _write_record(output, terminal_record)
        for record in steps:
            _write_record(output, record)
    side = Side(directory, image, plan, directory / 'events.jsonl', stdout,
                stderr, dict(wine_env), list(accepted[first:last]), first,
                terminal, timeout, frozen)
    side.frozen.update(freeze([image, plan]))
    (directory / 'plan.json').write_text(json.dumps(dict(
        first=first, last=last, expected=side.expected, terminal=terminal,
        application=application, command_line=command_line, cwd=str(cwd),
        preferred_base=pe.base, frozen=side.frozen), indent=2) + '\n')
    symbols = [dict(name=r['name'], start=r['start']-pe.base, end=r['end']-pe.base)
               for r in records if r['start'] < r['end']]
    (directory/'symbols.json').write_text(json.dumps(symbols, indent=2)+'\n')
    side.frozen.update(freeze([directory/'symbols.json']))
    return side


def _stop_process_group(process):
    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait(timeout=3)


def run_side(side, walker=None, _abort=None):
    walker = Path(walker) if walker else build_helper()
    hashes = dict(side.frozen)
    hashes.update(freeze([walker, HERE/'src/walker.c', Path(__file__)]))
    verify_frozen(hashes)
    started = time.monotonic()
    command = ['wine', windows(walker), windows(side.plan), windows(side.receipt)]
    # Wine services can inherit these handles. Waiting for PIPE EOF can hang
    # after the walker has exited; wait for the process, retaining file logs.
    host_out = (side.directory/'walker.stdout').open('wb')
    host_err = (side.directory/'walker.stderr').open('wb')
    process = subprocess.Popen(command, env=side.wine_env,
                               stdin=subprocess.DEVNULL,
                               stdout=host_out, stderr=host_err,
                               start_new_session=True)
    try:
        while True:
            if _abort is not None and _abort.is_set():
                raise RuntimeError('paired side failed; evidence: ' + str(side.directory))
            if time.monotonic()-started > side.timeout+15:
                raise RuntimeError('walker host timeout; evidence: ' + str(side.directory))
            try:
                process.wait(timeout=0.25)
                break
            except subprocess.TimeoutExpired:
                continue
    except BaseException as error:
        _stop_process_group(process)
        (side.directory/'host-failure.txt').write_text(str(error))
        raise
    finally:
        host_out.close()
        host_err.close()
    if not side.receipt.exists():
        raise RuntimeError('walker omitted receipt; evidence: ' + str(side.directory))
    events = [json.loads(line) for line in side.receipt.read_text().splitlines()]
    for event in events:
        if 'index' in event:
            event['local_index'] = event['index']
            event['index'] += side.first
        if event['event']=='failure' and 'address' in event:
            rva=event['address']-event['base']
            symbols=json.loads((side.directory/'symbols.json').read_text())
            found=[r for r in symbols if r['start']<=rva<r['end']]
            if found:
                symbol=min(found,key=lambda r:r['end']-r['start'])
                offset=rva-symbol['start']
                event['actual']=symbol['name']+('+'+format(offset,'x') if offset else '')
            else:
                event['actual']='unknown:'+format(event['address'],'08x')
    (side.directory/'events-normalized.json').write_text(json.dumps(events,indent=2)+'\n')
    verify_frozen(hashes)
    actual = [e['name'] for e in events if e['event'] == 'frontier']
    passed = events and events[-1]['event'] == 'pass'
    if process.returncode or actual != side.expected or not passed:
        raise RuntimeError('fast replay divergence; evidence: ' + str(side.directory))
    terminal = next(e for e in events if e['event'] == 'terminal')
    if terminal['name'] != side.terminal:
        raise RuntimeError('wrong terminal; evidence: ' + str(side.directory))
    receipt = dict(method='continuous', first=side.first,
                   last=side.first+len(actual), expected=side.expected,
                   terminal=side.terminal, seconds=time.monotonic()-started,
                   events=events, frozen=hashes, directory=str(side.directory))
    (side.directory/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt


def run_pair(left, right, *, receipt_path=None):
    if (left.first, left.expected, left.terminal) != (right.first, right.expected, right.terminal):
        raise ValueError('side plans describe different historical segments')
    walker = build_helper()
    started = time.monotonic()
    abort = threading.Event()
    with ThreadPoolExecutor(2) as pool:
        futures = [pool.submit(run_side, side, walker, abort) for side in (left, right)]
        try:
            for future in as_completed(futures):
                future.result()
        except BaseException:
            abort.set()
            raise
        receipts = [future.result() for future in futures]
    report = dict(method='continuous', first=left.first,
                  last=left.first+len(left.expected), frontiers=len(left.expected),
                  terminal=left.terminal, seconds=time.monotonic()-started,
                  sides=receipts)
    if receipt_path:
        Path(receipt_path).write_text(json.dumps(report, indent=2)+'\n')
    return report
