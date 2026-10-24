"""Decode every original FILE checkpoint, retaining unopened vs opened-empty."""
import re


def original_buffers(events, expected_count):
    rows = re.findall(r'^buffer (\d+) (.*)$', events, re.M)
    assert len(rows) == expected_count, 'missing original buffer observations'
    buffers = []
    for index, (number, payload) in enumerate(rows):
        assert int(number) == index, 'duplicate/out-of-order original buffer observation'
        if payload == 'unopened':
            buffers.append(None)
            continue
        match = re.fullmatch(r'(\d+) ([0-9a-f]*)', payload)
        assert match, 'unreadable/invalid original FILE buffer'
        count = int(match[1])
        assert count <= 4096 and len(match[2]) == 2 * count, 'short/invalid original buffer read'
        bytes.fromhex(match[2])
        buffers.append(match[2])
    return buffers
