#!/usr/bin/env python3
"""Conservative, memoized dependencies for literal C includes, without Wine.

Scan both branches of conditionals. Resolve Windows header names without case
sensitivity, rejecting ambiguous host directory entries. Macro-computed includes
and generated files not present yet belong in ORIGINAL_EXTRA_DEPS or local Make
prerequisites.
"""
import argparse
from pathlib import Path
import re


def escape(path):
    return str(path).replace('$', '$$').replace('#', r'\#').replace(' ', r'\ ')


class IncludeResolver:
    def __init__(self):
        self.directories = {}

    def entries(self, directory):
        if directory not in self.directories:
            entries = {}
            try:
                for path in directory.iterdir():
                    entries.setdefault(path.name.casefold(), []).append(path)
            except (FileNotFoundError, NotADirectoryError):
                pass
            self.directories[directory] = entries
        return self.directories[directory]

    def resolve(self, directory, name):
        path = directory
        for part in Path(name.replace('\\', '/')).parts:
            if part == '.':
                continue
            if part == '..':
                path = path.parent
                continue
            if part == '/':
                path = Path('/')
                continue
            matches = self.entries(path).get(part.casefold(), [])
            if len(matches) > 1:
                raise ValueError(f'ambiguous Windows include path: {path / part}')
            if not matches:
                return None
            path = matches[0]
        return path.resolve() if path.is_file() else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--include', action='append', default=[], type=Path)
    parser.add_argument('sources', nargs='+', type=Path)
    args = parser.parse_args()
    parsed, closed = {}, {}
    resolver = IncludeResolver()
    pattern = re.compile(r'^\s*#\s*include\s*(["<])([^">]+)[">]', re.M)

    def children(path):
        if path not in parsed:
            result = []
            for opening, name in pattern.findall(path.read_text(errors='replace')):
                directories = ([path.parent] if opening == '"' else [])
                directories += args.include
                for directory in directories:
                    candidate = resolver.resolve(directory, name)
                    if candidate is not None:
                        result.append(candidate)
                        break
            parsed[path] = result
        return parsed[path]

    def closure(path, active):
        if path in closed:
            return closed[path], False
        if path in active:
            return set(), True
        active.add(path)
        result, cyclic = {path}, False
        for child in children(path):
            descendants, cycle = closure(child, active)
            result.update(descendants)
            cyclic |= cycle
        active.remove(path)
        # Don't cache an incomplete closure through a cycle. Parsing is still
        # shared, and ordinary acyclic SDK/header closures are computed once.
        if not cyclic:
            closed[path] = result
        return result, cyclic

    lines = []
    try:
        for source in args.sources:
            dependencies, _ = closure(source.resolve(), set())
            target = args.object_root / (str(source) + '.obj')
            lines.append(escape(target) + ': ' + ' '.join(map(escape, sorted(dependencies))))
    except ValueError as error:
        parser.error(str(error))
    # Header edits may add further includes, so refresh this scan as well.
    lines.append(escape(args.output) + ': ' + ' '.join(map(escape, sorted(parsed))))
    # Like -MP, permit removal of an old include without a premature make error.
    lines += [escape(path) + ':' for path in sorted(parsed)]
    args.output.write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
