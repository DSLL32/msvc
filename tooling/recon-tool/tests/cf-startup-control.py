#!/usr/bin/env python3
"""One explicitly reviewed authored startup-op workload; no Watcom App code."""
import argparse
from collections import Counter
import fcntl
import hashlib
import json
import os
from pathlib import Path
import resource
import struct
import subprocess
import sys
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'watcom'))
from arena import Session
from broker import Broker
from catalog import Catalog
from cf_identity import Image
from gate import Span
from ipc import Client


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise AssertionError(message)


def main(plan_path):
    plan = json.loads(plan_path.read_text())
    for path, digest in plan['bindings'].items():
        require(sha(path) == digest, 'frozen input changed: ' + path)
    work = Path(plan['work'])
    work.mkdir()
    (work / 'cwd').mkdir()
    receipt = {'scope': 'one authored startup-op control; original Watcom App instructions0',
               'plan_sha256': sha(plan_path), 'events': [], 'passed': False}
    started = time.monotonic()
    self_cpu = time.process_time()
    usage_before = resource.getrusage(resource.RUSAGE_CHILDREN)
    child = None
    broker = None
    channel = None
    def win(path):
        path = Path(path).resolve()
        for base, drive in ((Path(plan['repo']), 'R:'), (Path.home() / '.ai-tmp/msvc2', 'S:')):
            if path.is_relative_to(base):
                return drive + '\\' + str(path.relative_to(base)).replace('/', '\\')
        raise AssertionError('control path outside fixed shared drives')
    try:
        layout = json.loads(Path(plan['layout']).read_text())
        fields = layout['header']
        image = Image.parse(Path(plan['payload']).read_bytes(), layout['target_sha256'], fields)
        registry = json.loads(Path(plan['registry']).read_text())
        catalog = Catalog(json.loads(Path(plan['catalog']).read_text()), registry,
                          plan['repo'], work / 'graph', authored=True)
        setup_registry = [{'name': '_probe_ready', 'rva': 0x1013, 'code': [[0x1013, 0x101e]],
                           'text_data': [], 'definition': 4, 'implementation': 1}]
        setup = Catalog(json.loads(Path(plan['setup_catalog']).read_text()), setup_registry,
                        plan['repo'], work / 'setup-graph', authored=True)
        env = os.environ.copy()
        env.update(plan['environment_updates'])
        for name in plan['environment_remove']:
            env.pop(name, None)
        with open('/tmp/recon/.capture.lock', 'a+b') as lease:
            fcntl.flock(lease, fcntl.LOCK_EX)
            with Client(work / 'channel', seconds=15, heartbeat_seconds=.01) as channel:
                argv = [plan['wine'], plan['helper'], win(channel.path), win(plan['loader']), win(plan['payload']),
                        win(work / 'cwd'), 'NUL', win(work / 'stdout'), win(work / 'stderr'),
                        win(work / 'events'), '101e', '55', '12000', '1000']
                receipt['command'] = argv
                child = subprocess.Popen(argv, cwd=work, env=env, stdin=subprocess.DEVNULL,
                                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                broker = Broker(channel)
                event, entry = broker.entry(0x101e, 0x55)
                require(event['base'] == 0x400000, 'authored loader rebase')
                index, ready_event, ready = broker.authored_setup(0x1013, 0x55, 0x2030)
                size, marker, actual_entry, base = struct.unpack('<4I', broker.read(event['base'] + 0x3010, 16))
                require(size == image.allocation_size and marker == 0 and actual_entry == base + image.entry_offset,
                        'actual authored allocation differs')
                snapshot = b''.join(broker.read(base + offset, min(512, image.loaded_size - offset))
                                    for offset in range(0, image.loaded_size, 512))
                functions = catalog.functions(image, base, snapshot)
                for descriptor, authorization in setup.descriptors:
                    functions += descriptor.fixed_pe(plan['loader'], event['base'], broker.read, authorization,
                                                      {event['base'] + 0x1013: 0x55})
                spans = [Span(base + row['start'], row['end'] - row['start'], name,
                              write=name in {'probe_word', 'probe_ds', 'probe_ss', 'probe_zero'})
                         for name, row in layout['data'].items()]
                zero = layout['data']['probe_zero']
                session = Session(broker, event, ready, image, base, snapshot, functions, spans,
                                  support_data_spans=[Span(event['base'] + 0x3018, 4, 'authored entry cell')],
                                  max_steps=100, seconds=10, startup=True,
                                  zero_span=Span(base + zero['start'], zero['end'] - zero['start'], 'authored zero buffer', write=True))
                tick = time.monotonic()
                current = session.consume_owned_entry(index, ready)
                result = session.run(current)
                receipt['guarded_runtime_ms'] = (time.monotonic() - tick) * 1000
                require(result['frontier']['context']['eip'] == base + layout['functions']['_probe_unknown']['start'],
                        'wrong actual unknown frontier')
                for name, expected in [('probe_word', 0x1234), ('probe_ds', ready['ds']), ('probe_ss', ready['ss'])]:
                    address = base + layout['data'][name]['start']
                    actual = struct.unpack('<H', broker.read(address, 2))[0]
                    require(actual == expected, 'actual selector/word store differs: ' + name)
                    receipt.setdefault('actual_fields', {})[name] = actual
                require(broker.read(base + zero['start'], 31) == bytes(31), 'REP did not zero exact 31-byte buffer')
                require(broker.read(base + layout['data']['probe_marker']['start'], 4) == bytes(4), 'unknown body executed')
                counts = Counter(step['before']['eip'] - base for step in session.records)
                repeated = {address: count for address, count in counts.items()
                            if image.relocated(base)[address:address + 2] in (b'\xf3\xab', b'\xf3\xaa')}
                require(sorted(repeated.values()) == [1, 3, 7], 'actual REP iteration/zero-count events differ')
                receipt.update(result=result, actual_rep_iterations=repeated, cf_steps=session.cf_instruction_count,
                               total_steps=session.gate.steps, graph_production=catalog.production + setup.production)
                broker.stop()
                out, err = child.communicate(timeout=6)
                require(child.returncode == 0 and not out and not err, 'unexpected broker terminal/streams')
                require((work / 'stdout').read_bytes() == b'' and (work / 'stderr').read_bytes() == b'', 'authored output differs')
                require(b'owned-teardown exit-event=1 signaled=1' in (work / 'events').read_bytes(), 'owned teardown missing')
                receipt['passed'] = True
    except BaseException as error:
        receipt['failure'] = repr(error)
        raise
    finally:
        if child is not None:
            forced = False
            try:
                out, err = child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                out, err = child.communicate(timeout=3)
                forced = True
            (work / 'launcher.stdout').write_bytes(out)
            (work / 'launcher.stderr').write_bytes(err)
            receipt['broker_status'] = child.returncode
            receipt['forced_outer_kill'] = forced
        if broker is not None:
            receipt['events'] = broker.events
        if channel is not None:
            receipt['rpc'] = channel.records
            receipt['poll_waits'] = channel.wait_counts
        receipt['elapsed_ms'] = (time.monotonic() - started) * 1000
        receipt['frontend_cpu_ms'] = (time.process_time() - self_cpu) * 1000
        usage = resource.getrusage(resource.RUSAGE_CHILDREN)
        receipt['waited_children_cpu_ms'] = ((usage.ru_utime + usage.ru_stime) -
                                           (usage_before.ru_utime + usage_before.ru_stime)) * 1000
        receipt['cpu_scope'] = 'frontend and waited children; shared wineserver excluded'
        (work / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS one authored startup-op control; original Watcom App instructions0')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, required=True)
    main(parser.parse_args().plan.resolve())
