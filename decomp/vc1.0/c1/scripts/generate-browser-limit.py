#!/usr/bin/env python3
"""Generate the public bulk registry input deterministically; never read a PE."""
import argparse
import hashlib
import json
from pathlib import Path

BINARY = Path(__file__).resolve().parents[1]
COUNT = 66000

def content():
    rows = ['/* Generated ordinary external registry declarations. */\n',
            '#ifndef BULK_REGISTRY_H\n#define BULK_REGISTRY_H\n']
    rows.extend('extern unsigned registry_slot_%05d;\n' % index for index in range(COUNT))
    rows.append('#endif\n')
    return ''.join(rows).encode('ascii')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='Generated source path in scratch or .build')
    options = parser.parse_args()
    data = content()
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_bytes(data)
    print(json.dumps(dict(path=str(options.output), declarations=COUNT,
                         order='ascending decimal indices 00000 through 65999',
                         bytes=len(data), sha256=hashlib.sha256(data).hexdigest())))

if __name__ == '__main__':
    main()
