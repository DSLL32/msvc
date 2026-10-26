#!/usr/bin/env python3
"""Private relocatable copies for canonical native entries; never a target loader."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run(*args, **kwargs):
    return subprocess.check_output(args, text=True, env={**os.environ, 'LC_ALL': 'C'}, **kwargs)


def identifier(value):
    if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', value):
        raise ValueError('module/asset ID must be a C identifier: ' + value)
    return value


def symbols(path, flag):
    return {line.split()[0] for line in run('nm', '-P', flag, str(path)).splitlines() if line.split()}


def replace_file(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_bytes() != data:
        temporary = path.with_name(path.name + '.new')
        temporary.write_bytes(data)
        os.replace(temporary, path)


def bundle(args):
    output = Path(args.output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='partial-', dir=output.parent) as temporary:
        raw = Path(temporary) / 'raw.o'
        final = Path(temporary) / 'final.o'
        # Allocate COMMON here too so its state belongs to this bundle, rather
        # than escaping into the final executable's ordinary BSS.
        # Resolve the module project's existing alias identities before symbol
        # isolation, so aliases and their targets stay in the same namespace.
        run(args.ld, '-m', 'elf_i386', '-r', '-d', '-o', str(raw),
            *['@' + str(Path(path).resolve()) for path in args.alias_rsp], *args.objects)
        defined = symbols(raw, '--defined-only')
        globals_ = symbols(raw, '--extern-only') & defined
        undefined = symbols(raw, '--undefined-only')
        mapping = {}
        for pair in args.bind:
            old, sep, new = pair.partition('=')
            if not sep or not old or not new or old in mapping or old not in undefined:
                raise ValueError('direct binding must name one undefined symbol: ' + pair)
            mapping[old] = new
        if args.kind == 'app':
            if args.main_symbol in defined:
                if args.main_name in defined:
                    raise ValueError('private App main name already defined')
                mapping[args.main_symbol] = args.main_name
            exports = []
        else:
            identifier(args.id)
            exports = args.export
            if not exports or len(set(exports)) != len(exports):
                raise ValueError('module exports must be nonempty and unique')
            for name in exports:
                if name not in globals_:
                    raise ValueError('required module symbol is not defined: ' + name)
            # Give COMDAT signatures private names too: localization must not
            # permit identically named module contributions to merge at link.
            for name in sorted(globals_):
                mapping[name] = args.id + '_' + name
        command = [args.objcopy]
        for old, new in mapping.items():
            command += ['--redefine-sym', old + '=' + new]
        # Only defined symbols become local: external runtime imports stay global.
        for name in sorted(globals_ - set(exports)) if args.kind == 'module' else []:
            command += ['--localize-symbol', mapping[name]]
        command += [str(raw), str(final)]
        run(*command)
        state = []
        constructors = []
        for line in run('readelf', '-SW', str(final)).splitlines():
            match = re.match(r'\s*\[\s*\d+\]\s+(.+)', line)
            if not match:
                continue
            fields = match[1].split()
            if len(fields) != 10:
                continue
            name, kind, address, offset, size, entsize, flags, link, info, align = fields
            size, align = int(size, 16), int(align)
            if not size or 'A' not in flags:
                continue
            if 'T' in flags:
                raise ValueError('static bundle TLS requires a separate owner contract: ' + name)
            if name in ('.init', '.fini') or name.startswith(('.init_array', '.fini_array', '.preinit_array', '.ctors', '.dtors')):
                constructors.append(name)
            elif 'W' in flags:
                if not re.fullmatch(r'[A-Za-z0-9_.$-]+', name):
                    raise ValueError('unsupported writable section name: ' + name)
                state.append((name, align, kind))
        if args.kind == 'module':
            if constructors and args.constructors != 'premain':
                raise ValueError('module has process constructors/finalizers; declare premain explicitly: ' + ','.join(constructors))
            if constructors and args.reset != 'none':
                raise ValueError('premain constructors cannot be combined with a reload snapshot; select reset=none')
            alignment = max([1] + [v[1] for v in state])
            start, end = ('recon_module_' + args.id + '_state_' + suffix for suffix in ['start', 'end'])
            # Select the bundle before the default .data/.bss wildcard consumes it.
            # One output region deliberately includes initialized bytes and BSS.
            quoted = str(output).replace('\\', '\\\\').replace('"', '\\"')
            def select(kind):
                names = ' '.join(v[0] for v in state if (v[2] == 'NOBITS') == (kind == 'bss'))
                return f'KEEP("{quoted}"({names}))' if names else ''
            script = (f'SECTIONS {{ .recon_{args.id}_data ALIGN({alignment}) : {{\n'
                      f'  {start} = .;\n  {select("data")}\n  . = ALIGN({alignment});\n}}\n'
                      f'.recon_{args.id}_bss (NOLOAD) : {{\n  {select("bss")}\n'
                      f'  . = ALIGN({alignment});\n  {end} = .;\n}} }} INSERT BEFORE .data;\n')
            replace_file(args.script, script.encode())
            Path(args.script).touch()
        replace_file(output, final.read_bytes())
        output.touch()
        replace_file(str(output) + '.json', (json.dumps(dict(
            kind=args.kind, renamed=mapping, localized=sorted(globals_ - set(exports)) if args.kind == 'module' else [],
            state_sections=state, constructors=constructors,
            constructor_policy=args.constructors, reset=args.reset), indent=2) + '\n').encode())


def asset(args):
    identifier(args.id)
    source, output = Path(args.source).resolve(), Path(args.output).resolve()
    original = '_binary_' + re.sub(r'[^A-Za-z0-9]', '_', source.name)
    prefix = 'recon_asset_' + args.id
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='asset-', dir=output.parent) as temporary:
        result = Path(temporary) / 'asset.o'
        command = [args.objcopy, '-I', 'binary', '-O', 'elf32-i386', '-B', 'i386',
                   '--rename-section', '.data=.rodata.recon_asset.' + args.id + ',alloc,load,readonly,data,contents',
                   '--add-section', '.note.GNU-stack=/dev/null',
                   '--set-section-flags', '.note.GNU-stack=noload,readonly']
        for suffix in ['start', 'end', 'size']:
            command += ['--redefine-sym', original + '_' + suffix + '=' + prefix + '_' + suffix]
        run(*command, source.name, str(result), cwd=source.parent)
        replace_file(output, result.read_bytes())
        output.touch()


parser = argparse.ArgumentParser(description=__doc__)
sub = parser.add_subparsers(dest='operation', required=True)
p = sub.add_parser('bundle')
p.add_argument('--kind', choices=['app', 'module'], required=True)
p.add_argument('--output', required=True)
p.add_argument('--id')
p.add_argument('--script')
p.add_argument('--main-name', default='recon_entry_app_main')
p.add_argument('--main-symbol', default='main')
p.add_argument('--export', action='append', default=[])
p.add_argument('--bind', action='append', default=[])
p.add_argument('--alias-rsp', action='append', default=[])
p.add_argument('--constructors', choices=['reject', 'premain'], default='reject')
p.add_argument('--reset', choices=['region', 'none'], default='region')
p.add_argument('--ld', default='ld')
p.add_argument('--objcopy', default='objcopy')
p.add_argument('objects', nargs='+')
p = sub.add_parser('asset')
p.add_argument('--id', required=True)
p.add_argument('--source', required=True)
p.add_argument('--output', required=True)
p.add_argument('--objcopy', default='objcopy')
p = sub.add_parser('config')
p.add_argument('--output', required=True)
p.add_argument('--value', action='append', default=[])
p = sub.add_parser('support')
p.add_argument('objects', nargs='*')
args = parser.parse_args()
try:
    if args.operation == 'bundle':
        bundle(args)
    elif args.operation == 'asset':
        asset(args)
    elif args.operation == 'config':
        replace_file(args.output, (json.dumps(args.value) + '\n').encode())
    else:
        for obj in args.objects:
            if 'main' in symbols(Path(obj), '--defined-only'):
                raise ValueError('canonical support defines generated main; select startup without shim main: ' + obj)
except (ValueError, subprocess.CalledProcessError) as error:
    parser.exit(1, 'entry-build: ' + str(error) + '\n')
