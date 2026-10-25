"""Ordinary CF debugger: format identity, code masking and first-entry traps.

The loaded original executes its own instructions, CRT and far gateway. This
module does not decode instructions or predict register/memory effects.
"""
import hashlib
import json
import resource
from pathlib import Path
import struct
import subprocess
import time

from broker import Broker
from cf_identity import Image
from ipc import Client, ProtocolError
import frontend as shared


def require(condition, message):
    if not condition:
        raise ProtocolError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mask_image(snapshot, registry, layout, entry):
    """Preserve original relocated bytes outside explicit CODE mask spans."""
    ranges = layout['code']
    require(isinstance(ranges, list) and 0 < len(ranges) <= 64, 'bounded CODE layout required')
    end = 0
    masked = bytearray(snapshot)
    for start, limit in ranges:
        require(type(start) is int and type(limit) is int and end <= start < limit <= len(snapshot),
                'invalid CODE mask ranges')
        masked[start:limit] = b'\xcc' * (limit - start)
        end = limit

    def in_code(start, limit):
        return any(low <= start < limit <= high for low, high in ranges)

    require(in_code(entry, entry + 1), 'CF entry is outside CODE layout')
    apps = {}
    restored = []
    data = list(layout['text_data'])
    for row in registry:
        rva = row['rva']
        require(type(rva) is int and 0 <= rva < len(snapshot), 'registry entry outside initialized image')
        if row['definition'] == 4:
            require(rva not in apps and in_code(rva, rva + 1), 'duplicate/outside CODE App entry')
            apps[rva] = row
        restore = row['definition'] in (1, 2, 3) or (row['definition'] == 4 and row['implementation'] == 1)
        if not restore:
            continue
        for start, limit in row['code']:
            require(in_code(start, limit), 'restored code is outside CODE layout: ' + row['name'])
            require(not any(start <= other['rva'] < limit and other['implementation'] == 0
                            for other in registry if other['definition'] == 4),
                    'restored code overlaps a pending App entry')
            masked[start:limit] = snapshot[start:limit]
            restored.append({'name': row['name'], 'start': start, 'end': limit})
        data.extend(row['text_data'])
    require(len(data) <= 65536, 'text data range limit')
    for start, limit in data:
        require(type(start) is int and type(limit) is int and 0 <= start < limit <= len(snapshot),
                'text data outside initialized image')
        require(not any(start <= rva < limit for rva in apps), 'text data overlaps App entry')
        require(not any(start < row['end'] and row['start'] < limit for row in restored),
                'text data overlaps restored function code')
        masked[start:limit] = snapshot[start:limit]
    return bytes(masked), apps, restored


def execute(request):
    profile = request['profile']
    meta = profile['target']
    root = Path(request['root'])
    repo = Path(request['repo'])
    target = Path(request['target'])
    shared.ROOT = repo
    shared.SCRATCH = Path.home() / '.ai-tmp/msvc2'
    shared.STUB = Path(request['loader'])
    shared.STUB_SHA = profile['loader']['sha256']
    layout = profile['code_layout']
    witness = layout['evidence']
    relative = Path(witness['path'])
    require(not relative.is_absolute() and relative.parts and all(part not in ('.', '..') for part in relative.parts),
            'layout evidence must be repository relative')
    evidence = (repo / relative).resolve()
    require(evidence.is_relative_to(repo.resolve()) and sha(evidence.read_bytes()) == witness['sha256'],
            'CODE layout evidence differs')
    h = meta['header']
    header = (0x4643, h['image_file_offset'], h['initialized_size'], h['relocation_offset'],
              h['allocation_size'], h['entry_offset'])
    image = Image.parse(target.read_bytes(), meta['sha256'], header)
    require(image.header_offset == h['header_file_offset'] and image.groups == meta['relocation_groups'] and
            len(image.fixups) == meta['relocation_count'] and sha(image.initialized) == meta['initialized_sha256'] and
            sha(image.initialized[image.relocation_offset:]) == meta['relocation_sha256'], 'CF metadata differs')
    require(sha(shared.STUB.read_bytes()) == shared.STUB_SHA, 'loader changed')
    # Validate all mask extents before spawning. The second projection uses the
    # actual relocated snapshot; no original instructions have run at that point.
    mask_image(image.initialized, request['registry'], layout, image.entry_offset)
    start = time.monotonic()
    cpu = time.process_time()
    child_cpu = resource.getrusage(resource.RUSAGE_CHILDREN)
    receipt = {'backend': 'watcom-cf', 'mode': 'debugger', 'oracle_written': False,
               'instruction_semantics': 'original execution under Wine', 'sources': request['sources'],
               'profile_sha256': request['profile_sha256'], 'first_entry_order': [], 'floating_save': None}
    child = broker = channel = None
    forced = False
    out = err = b''
    try:
        with Client(root / 'cf-channel', seconds=request['seconds'], heartbeat_seconds=.01) as channel:
            argv = [request['wine'], request['helper'], shared.win(channel.path), shared.win(shared.STUB),
                    request['arguments'], shared.win(Path(request['cwd'])), request['stdin'],
                    shared.win(root / 'original-stdout'), shared.win(root / 'original-stderr'),
                    shared.win(root / 'cf-events'), '10010', 'e9', str(request['timeout_ms']),
                    '1000', shared.win(Path(request['environment']))]
            shared.validate_launch_command(argv)
            receipt['command'] = argv
            child = subprocess.Popen(argv, cwd=request['cwd'], stdin=subprocess.DEVNULL,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            receipt['launcher_pid'] = child.pid
            broker = Broker(channel)
            event, context = broker.entry(0x10010, 0xe9)
            base = event['base']
            receipt['loader_entry'] = {'event': event, 'context': context}
            receipt['loaded_loader'] = shared.loader_bytes(broker, base)
            _, stop, context = broker.loader_prefix()
            receipt['launch'] = {'event': stop, 'context': context}
            slots = struct.unpack('<4I', broker.read(base + 0x30000, 16))
            entry, allocation, repeated, break_flag = slots
            require(allocation == repeated and 0 < allocation and allocation + image.allocation_size < 0x100000000 and
                    entry == allocation + image.entry_offset and context['esi'] == entry and
                    context['ebx'] == allocation and context['edx'] == base + 0x10788 and
                    context['edi'] == context['esp'] + 0x104, 'actual loader allocation/launch differs')
            params = struct.unpack('<5I', broker.read(context['edi'], 20))
            require(params[3] == base + 0x3000c and params[4] == 0, 'loader parameter block differs')
            name = shared.terminated(broker, params[0])
            tail = shared.terminated(broker, params[1])
            environment = shared.terminated(broker, params[2], double=True)
            require(name.decode('ascii').lower() == shared.win(target).lower() + '\0', 'payload name differs')
            require(tail == request['arguments'].encode('ascii') + b'\0', 'payload command tail differs')
            receipt['logical_payload_argv0'] = name[:-1].decode('ascii')
            receipt['target_environment'] = shared.verify_overlay(environment, request['target_env'])
            snapshot = shared.read_all(broker, allocation, image.loaded_size)
            receipt['cf_identity'] = image.compare(snapshot, allocation)
            masked, apps, restored = mask_image(snapshot, request['registry'], layout, image.entry_offset)
            broker.run_setup(allocation, image.allocation_size, entry)
            # Only changed contiguous runs are written; outside CODE and explicit
            # restored code/data remain byte-identical to the real loaded image.
            changed = []
            cursor = 0
            while cursor < len(masked):
                if masked[cursor] == snapshot[cursor]:
                    cursor += 1
                    continue
                low = cursor
                while cursor < len(masked) and masked[cursor] != snapshot[cursor]:
                    cursor += 1
                changed.append([low, cursor])
                for offset in range(low, cursor, 4088):
                    broker.write(allocation + offset, masked[offset:min(offset + 4088, cursor)])
            actual = shared.read_all(broker, allocation, image.loaded_size)
            require(actual == masked, 'actual complete masked initialized image differs')
            receipt['mask'] = {'code': layout['code'], 'text_data': layout['text_data'],
                               'evidence': witness, 'changes': changed, 'restored': restored,
                               'loaded_sha256': sha(snapshot), 'masked_sha256': sha(actual)}
            traps = {}
            seen = set(request['first_entry_seen'])
            order = []
            receipt['first_entry_order'] = order
            for rva, row in apps.items():
                if row['implementation'] == 1 and rva not in seen:
                    require(masked[rva] != 0xcc, 'implemented App entry remains masked')
                    traps[rva] = broker.trap(allocation + rva, masked[rva])
            runtime = time.monotonic()
            for _ in range(65536):
                stop, context = broker.run()
                receipt['last_stop'] = {'event': stop, 'context': context}
                if stop['code'] == 5:
                    receipt['terminal'] = 'exit:%u' % stop['exit_code']
                    break
                require(stop['exception'] == 0x80000003 and stop['first_chance'] and
                        context['eip'] == stop['exception_address'] + 1,
                        'unexpected original debugger exception')
                address = stop['exception_address']
                require(allocation <= address < allocation + image.loaded_size,
                        'external original INT3 at %08x' % address)
                rva = address - allocation
                require(rva in apps, 'masked INT3 is not a known App entry: %x' % rva)
                row = apps[rva]
                if rva not in seen:
                    seen.add(rva)
                    order.append(rva)
                if row['implementation'] == 0:
                    require(broker.read(address, 1) == b'\xcc', 'pending App trap changed')
                    receipt['terminal'] = 'frontier:%x' % rva
                    stack = stack_error = None
                    try:
                        stack = 'hex:' + broker.read(context['esp'], 64).hex()
                    except (ProtocolError, OSError) as error:
                        stack_error = str(error)
                    receipt['frontier_snapshot'] = {
                        'image_base': allocation, 'rva': rva, 'registers': context, 'floating_save': None,
                        'stack_address': context['esp'], 'stack_requested': 64, 'stack': stack,
                        'stack_error': stack_error}
                    break
                require(rva in traps, 'unowned implemented App INT3')
                broker.consume(traps.pop(rva))
            else:
                raise ProtocolError('App entry event budget exhausted')
            receipt['first_entry_order'] = order
            receipt['timing'] = {'setup_ms': (runtime - start) * 1000,
                                 'execution_wall_ms': (time.monotonic() - runtime) * 1000}
            broker.stop()
            out, err = child.communicate(timeout=6)
            require(child.returncode == 0 and not out and not err, 'broker failed/unexpected launcher streams')
            log = (root / 'cf-events').read_bytes()
            require(b'owned-teardown exit-event=1 signaled=1' in log, 'owned teardown missing')
            receipt.update(passed=True, oracle_written=True)
    except BaseException as error:
        receipt['error'] = repr(error)
        raise
    finally:
        if child is not None:
            try:
                out, err = child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                out, err = child.communicate(timeout=3)
                forced = True
            receipt['broker_status'] = child.returncode
            receipt['forced_outer_stop'] = forced
            (root / 'cf-launcher-stdout').write_bytes(out)
            (root / 'wine-diagnostics').write_bytes(err)
        receipt['elapsed_ms'] = (time.monotonic() - start) * 1000
        receipt['frontend_cpu_ms'] = (time.process_time() - cpu) * 1000
        usage = resource.getrusage(resource.RUSAGE_CHILDREN)
        receipt['waited_child_cpu_ms'] = ((usage.ru_utime + usage.ru_stime) -
            (child_cpu.ru_utime + child_cpu.ru_stime)) * 1000
        receipt['cpu_scope'] = 'frontend and waited children; shared wineserver excluded'
        if broker is not None:
            receipt['events'] = broker.events
        if channel is not None:
            receipt['rpc_count'] = len(channel.records)
            receipt['poll_waits'] = channel.wait_counts
        if (root / 'cf-events').exists():
            log = (root / 'cf-events').read_bytes()
            receipt['event_log_sha256'] = sha(log)
            receipt['event_log'] = log[:65536].decode(errors='replace')
        (root / 'cf-result').write_text(json.dumps(receipt))
    return receipt
