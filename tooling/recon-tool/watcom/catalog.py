"""Explicit capture-time graph facts and separately bound runtime admission.

The graph producer owns decoding and normalization. This adapter rechecks its
current inputs once before launch; local RET facts alone never authorize an ABI.
"""
import hashlib
import json
import subprocess
from pathlib import Path

from arena import Descriptors, canonical, require, sha


def bound(spec):
    require(isinstance(spec, dict) and set(spec) == {'path', 'sha256'}, 'invalid file binding')
    path = Path(spec['path']).resolve(strict=True)
    require(path.is_file() and sha(path.read_bytes()) == spec['sha256'], 'bound input changed: ' + str(path))
    return path


def record_ranges(record):
    return sorted([[record['start'], record['end']]] + record.get('code_chunks', []))


class SupportDescriptors(Descriptors):
    def __init__(self, report, project, expected_inputs):
        export = report.get('decoded_support_instructions', {})
        require(export.get('schema') == 1 and export.get('inputs') == expected_inputs,
                'original-only support facts have different inputs')
        self.images = export['inputs']['images']
        self.rows = export['functions']
        self.project = Path(project).resolve()

    def selected(self, authorization):
        result = []
        require(authorization, 'empty support admission')
        for name, policy in authorization.items():
            fact = self.rows.get(name, {})
            row = fact.get('original', {})
            require(fact.get('classification') == 'CRT' and policy.get('role') == 'proved-support',
                    'original-only facts are restricted to explicitly proved support')
            evidence = fact.get('evidence', {})
            relative = Path(evidence.get('path', ''))
            require(not relative.is_absolute() and '..' not in relative.parts and relative.parts,
                    'support witness must be project-relative')
            witness = (self.project / relative).resolve(strict=True)
            require(witness.is_relative_to(self.project) and sha(witness.read_bytes()) == evidence['sha256'],
                    'support witness changed or escaped project')
            require(row.get('checked_ret_cleanup_set') is None, 'original-only support fabricated checked ABI')
            record = row.get('record', {})
            require(not record.get('pending_extent', True) and
                    row.get('record_sha256') == policy.get('record_sha256') == sha(canonical(record)),
                    'support record binding changed')
            require(row.get('effective_roots') == [record.get('start')], 'support has alternate roots')
            abi = policy.get('abi', {})
            cleanup = policy.get('cleanup')
            require(abi.get('evidence') == evidence and abi.get('kind') in {'near-ret', 'entry-root'},
                    'support needs separately reviewed witness-bound ABI')
            local = row.get('local_ret_cleanup_set')
            require((abi['kind'] == 'near-ret' and type(cleanup) is int and
                     0 <= cleanup <= 65535 and local == [cleanup]) or
                    (abi['kind'] == 'entry-root' and cleanup is None and local == []),
                    'support ABI disagrees with local return facts')
            require(row.get('instructions'), 'support has no instructions')
            result.append((name, policy, row))
        return result

    def cf(self, image, base, snapshot, authorization):
        for _, policy, row in self.selected(authorization):
            if policy['cleanup'] is None:
                require(row['record']['start'] == image.entry_offset,
                        'unknown return ABI is restricted to the initial CF entry root')
        return super().cf(image, base, snapshot, authorization)


class Catalog:
    def __init__(self, spec, registry, repo, work, *, authored=False):
        require(set(spec) == {'schema', 'producer', 'reports', 'data', 'startup', 'max_steps'},
                'invalid CF catalog fields')
        require(spec['schema'] == 1 and type(spec['startup']) is bool and
                type(spec['max_steps']) is int and 1 <= spec['max_steps'] <= 32768,
                'invalid CF catalog limits')
        self.spec = spec
        self.registry = {row['name']: row for row in registry}
        require(len(self.registry) == len(registry), 'duplicate registry name')
        Path(work).mkdir(parents=True, exist_ok=True)
        producer = bound(spec['producer'])
        graph_repo = Path(__file__).resolve().parents[3] if authored else Path(repo)
        self.descriptors = []
        self.production = []
        for index, report in enumerate(spec['reports']):
            required = {'kind', 'project', 'config', 'runtime', 'rebuilt_runtime', 'metadata',
                        'rebuilt', 'map', 'map_format', 'expected_inputs', 'authorization', 'check_ret'}
            require(set(report) == required and report['kind'] in {'paired', 'original-support'} and
                    report['map_format'] in {'msvc', 'watcom10'} and type(report['check_ret']) is bool,
                    'invalid finite graph request')
            paths = {name: bound(report[name]) for name in
                     ('config', 'runtime', 'rebuilt_runtime', 'metadata', 'rebuilt', 'map')}
            project = Path(report['project']).resolve(strict=True)
            require(project.is_dir(), 'graph project is not a directory')
            config = json.loads(paths['config'].read_text())
            rows = {row['name']: row for row in config['function_registry']}
            analysis_base = config['target']['base']
            require(type(analysis_base) is int and 0 <= analysis_base <= 0xffffffff, 'invalid graph analysis base')
            auth = report['authorization']
            require(isinstance(auth, dict) and auth, 'empty explicit catalog authorization')
            for name, policy in auth.items():
                registered = self.registry.get(name)
                require(registered is not None, 'catalog owner absent from binary_descriptor: ' + name)
                projected = rows.get(name, {})
                require(projected.get('rva') == registered['rva'] and
                        sorted(projected.get('code', [])) == sorted(registered['code']) and
                        sorted(projected.get('text_data', [])) == sorted(registered['text_data']) and
                        projected.get('definition') == registered['definition'] and
                        projected.get('implementation') == registered['implementation'],
                        'catalog projection differs from current registry: ' + name)
                role = policy.get('role')
                require((role == 'proved-support' and registered['definition'] == 3) or
                        (role == 'accepted-prefix' and registered['definition'] == 4 and registered['implementation'] == 1) or
                        (role == 'candidate' and registered['definition'] == 4 and registered['implementation'] == 1) or
                        (role == 'authored-control' and authored and registered['definition'] == 4),
                        'catalog admission disagrees with registry role')
            output = Path(work) / ('cf-graph-%d.json' % index)
            argv = [str(producer), '--profile', 'cl', '--discovery', 'cfg', '--repo', str(graph_repo),
                    '--project', str(project), '--config', str(paths['config']), '--runtime', str(paths['runtime']),
                    '--rebuilt-runtime', str(paths['rebuilt_runtime']), '--metadata', str(paths['metadata']),
                    '--rebuilt', str(paths['rebuilt']), '--map', str(paths['map']),
                    '--map-format', report['map_format'], '--check-ownership', '--check-extents']
            if report['check_ret']:
                argv.append('--check-ret')
            if report['kind'] == 'paired':
                argv.append('--export-instructions')
            else:
                for name in sorted(auth):
                    argv += ['--export-original-support-instructions', name]
            argv += ['--report', str(output), '-q']
            result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
            log = {'command': argv, 'status': result.returncode,
                   'stdout': result.stdout.decode(errors='replace'), 'stderr': result.stderr.decode(errors='replace')}
            self.production.append(log)
            require(result.returncode == 0, 'capture-time graph audit failed: ' + log['stderr'])
            for name, path in paths.items():
                require(sha(path.read_bytes()) == report[name]['sha256'], 'input changed during graph audit')
            require(sha(producer.read_bytes()) == spec['producer']['sha256'], 'producer changed during graph audit')
            raw = output.read_bytes()
            require(len(raw) <= 16 * 1024 * 1024, 'graph report too large')
            facts = json.loads(raw)
            if report['kind'] == 'original-support':
                descriptor = SupportDescriptors(facts, project, report['expected_inputs'])
            else:
                descriptor = Descriptors(output, sha(raw), producer, spec['producer']['sha256'], report['expected_inputs'])
            for name, _, row in descriptor.selected(auth):
                require(row['record']['start'] == analysis_base + self.registry[name]['rva'] and
                        record_ranges(row['record']) == sorted([[analysis_base+a, analysis_base+b] for a,b in self.registry[name]['code']]),
                        'decoded owner differs from current registry extents')
            self.descriptors.append((descriptor, auth))

    def functions(self, image, base, snapshot):
        result = []
        for descriptor, authorization in self.descriptors:
            functions, _ = descriptor.cf(image, base, snapshot, authorization)
            result.extend(functions)
        return result
