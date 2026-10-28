"""Opt-in, bounded read-only inspection at one restored direct CALL.

This is trusted diagnostic Python, not a sandbox or a fixture oracle. Execution
and breakpoint consumption use the existing broker; no instruction is emulated.
"""
import hashlib
import json
from pathlib import Path

from ipc import ProtocolError

MAX_BYTES = 37656
MAX_HITS = 64
MAX_REGIONS = 4096


def require(condition, message):
    if not condition:
        raise ProtocolError('CF inspection: ' + message)


class Memory:
    """One genuine stopped context. Region observations expire on resume."""
    def __init__(self, broker, budget):
        self._broker = broker
        self._budget = budget
        self._regions = []

    @property
    def bytes_used(self):
        return self._budget['bytes']

    def region(self, address):
        require(type(address) is int and 0 < address <= 0xffffffff, 'region address')
        for row in self._regions:
            if row['base'] <= address < row['base'] + row['size']:
                return dict(row)
        require(self._budget['region_queries'] < MAX_REGIONS, 'region-query cap')
        self._budget['region_queries'] += 1
        row = self._broker.region(address)
        require(all(type(row.get(k)) is int for k in
                    ('base', 'allocation_base', 'allocation_protect', 'size',
                     'state', 'protect', 'type')) and
                0 <= row['base'] <= address < row['base'] + row['size'] <= 0x100000000,
                'invalid observed region')
        self._regions.append(dict(row))
        return dict(row)

    def read(self, address, size):
        require(type(address) is int and type(size) is int and
                0 < address <= 0xffffffff and 1 <= size <= 512 and
                address + size <= 0x100000000, 'read extent')
        require(self.bytes_used + size <= MAX_BYTES, 'aggregate read cap')
        row = self.region(address)
        require(address + size <= row['base'] + row['size'] and
                row['state'] == 0x1000 and not row['protect'] & 0x100 and
                row['protect'] & 0xff in (2, 4, 8, 0x20, 0x40, 0x80),
                'read is outside committed readable region')
        # Reserve before RPC, including failed/short reads; never grant a retry
        # more bytes than the declared aggregate budget.
        self._budget['bytes'] += size
        data = self._broker.read(address, size)
        require(type(data) is bytes and len(data) == size, 'short read')
        self._budget['reads'].append({'address': address, 'size': size, 'hex': data.hex()})
        return data


class Inspection:
    def __init__(self, config, registry, layout, snapshot, masked):
        require(set(config) == {'call_rva', 'script', 'sha256'}, 'configuration fields')
        path = Path(config['script'])
        raw = path.read_bytes()
        require(len(raw) <= 65536 and hashlib.sha256(raw).hexdigest() == config['sha256'],
                'inspector source SHA256/size')
        # Compile exactly the verified bytes (no import-loader reread/cache).
        namespace = {'__name__': 'cf_read_only_inspector', '__file__': str(path)}
        exec(compile(raw, str(path), 'exec'), namespace)
        require(callable(namespace.get('select')) and callable(namespace.get('snapshot')),
                'script requires select(stop,memory) and snapshot(stop,memory)')
        self.select = namespace['select']
        self.snapshot = namespace['snapshot']
        self.site = config['call_rva']
        require(type(self.site) is int and 0 <= self.site < len(snapshot) - 5,
                'CALL RVA bounds')
        self.return_rva = self.site + 5
        self.call = snapshot[self.site:self.return_rva]
        require(self.call[0] == 0xe8, 'watch must be an actual direct E8 CALL')
        self.callee = self.return_rva + int.from_bytes(self.call[1:], 'little', signed=True)
        entries = {r['rva'] for r in registry}
        require(self.site not in entries and self.return_rva not in entries,
                'watch/return conflicts with a registered entry')
        targets = [r for r in registry if r['rva'] == self.callee]
        require(len(targets) == 1 and targets[0]['definition'] == 4 and
                targets[0]['implementation'] == 1,
                'CALL target must be one unambiguous implemented named App')
        owners = [r for r in registry if r['definition'] == 4 and r['implementation'] == 1 and
                  any(a <= self.site and self.return_rva + 1 <= b for a, b in r['code'])]
        require(len(owners) == 1, 'CALL and fallthrough need one restored App owner')
        data = list(layout['text_data']) + [span for r in registry for span in r['text_data']]
        require(not any(a < self.return_rva + 1 and self.site < b for a, b in data),
                'watch overlaps text data')
        require(masked[self.site:self.return_rva + 1] == snapshot[self.site:self.return_rva + 1]
                and 0xcc not in (self.call[0], snapshot[self.return_rva]),
                'watch/return bytes must be restored instructions')
        self.return_byte = snapshot[self.return_rva]
        self.budget = {'bytes': 0, 'region_queries': 0, 'reads': [], 'patch_checks': []}
        self.receipt = {'script': str(path), 'sha256': config['sha256'],
                        'call_rva': self.site, 'callee_rva': self.callee,
                        'return_rva': self.return_rva, 'source_call': self.call.hex(),
                        'max_hits': MAX_HITS, 'max_bytes': MAX_BYTES,
                        'observations': [], 'memory': self.budget,
                        'scope': 'raw diagnostic only; no qualification or general ownership proof'}
        self.watch = self.return_trap = None
        self.waiting = None
        self.hits = 0

    def arm(self, broker, base, owner):
        self.base = base
        self.owner = owner
        self.watch = self._trap(broker, base + self.site, self.call[0])

    def _patch_checks(self, operation, address):
        # Existing C patch() reads one byte before and one after the write.
        # Conservatively reserve both even if an operation fails early.
        require(self.budget['bytes'] + 2 <= MAX_BYTES, 'aggregate patch/read cap')
        self.budget['bytes'] += 2
        self.budget['patch_checks'].append({'operation': operation, 'address': address,
                                           'read_bytes_reserved': 2})

    def _trap(self, broker, address, expected):
        self._patch_checks('trap', address)
        return broker.trap(address, expected)

    def _consume(self, broker, index, address):
        self._patch_checks('consume', address)
        broker.consume(index)

    def handle(self, broker, event, trapped):
        rva = event['exception_address'] - self.base
        if rva not in (self.site, self.return_rva):
            return False
        require((event['pid'], event['tid']) == self.owner, 'foreign watch process/thread')
        memory = Memory(broker, self.budget)
        if rva == self.return_rva:
            require(self.return_trap is not None and self.waiting is not None,
                    'unowned fallthrough trap')
            prior = self.waiting
            require((event['pid'], event['tid']) == prior['owner'] and
                    trapped['esp'] == prior['esp'] and trapped['ebp'] == prior['ebp'],
                    'direct CALL return owner/SP/EBP differs (RET0 required)')
            require(memory.read(self.base + self.site, 5) == self.call and
                    memory.read(self.base + self.return_rva, 1) == b'\xcc',
                    'CALL/return bytes changed while executing')
            self._consume(broker, self.return_trap, self.base + self.return_rva)
            after = broker.context()
            expected = dict(trapped, eip=self.base + self.return_rva)
            require(after == expected and memory.read(self.base + self.return_rva, 1) ==
                    bytes([self.return_byte]), 'return consume/readback differs')
            self.receipt['observations'].append({'return_registers': after, 'owner': list(prior['owner'])})
            self.return_trap = self.waiting = None
            self.watch = self._trap(broker, self.base + self.site, self.call[0])
            return True
        require(self.watch is not None and self.waiting is None and self.hits < MAX_HITS,
                'watch ownership/reentrancy/hit cap')
        self.hits += 1
        require(memory.read(self.base + self.site, 5) == b'\xcc' + self.call[1:] and
                memory.read(self.base + self.return_rva, 1) == bytes([self.return_byte]),
                'owned CALL/fallthrough readback differs')
        self._consume(broker, self.watch, self.base + self.site)
        self.watch = None
        actual = broker.context()
        require(actual == dict(trapped, eip=self.base + self.site), 'CALL consume changed raw context')
        require(memory.read(self.base + self.site, 5) == self.call, 'restored CALL readback differs')
        stop = {'stop_rva': self.site, 'callee_rva': self.callee,
                'return_rva': self.return_rva, 'phase': 'before_call',
                'image_base': self.base, 'pid': event['pid'], 'tid': event['tid'],
                'hit': self.hits, 'registers': actual, 'trap_registers': dict(trapped)}
        saved = json.loads(json.dumps(stop))
        selected = self.select(stop, memory)
        require(type(selected) is bool and stop == saved, 'selector result/context mutation')
        self.receipt['observations'].append({'stop': saved, 'selected': selected})
        if selected:
            self.receipt['snapshot'] = self.snapshot(stop, memory)
            require(isinstance(self.receipt['snapshot'], dict), 'snapshot must be a JSON object')
            require(stop == saved, 'snapshot context mutation')
            require(len(json.dumps(self.receipt, allow_nan=False)) <= 1024 * 1024,
                    'diagnostic result exceeds 1 MiB')
            return 'selected'
        require(self.hits < MAX_HITS, 'no matching phase within hit cap')
        self.waiting = {'owner': (event['pid'], event['tid']),
                        'esp': actual['esp'], 'ebp': actual['ebp']}
        self.return_trap = self._trap(broker, self.base + self.return_rva, self.return_byte)
        return True
