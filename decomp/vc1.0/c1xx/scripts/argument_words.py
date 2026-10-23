"""Validate explicitly requested, successfully read original ABI stack words."""
import re


def observation(events, requested):
    assert requested in (4, 6), 'unsupported original argument-word count'
    rows = re.findall(r'^argument (\d+) ([0-9a-f]{8}|unreadable)$', events, re.M)
    assert [int(index) for index, _ in rows] == list(range(requested)), 'missing or short original argument observation'
    assert all(value != 'unreadable' for _, value in rows), 'original argument read failed'
    summaries = re.findall(r'^argument-words requested (\d+) actual (\d+) context (\d+)$', events, re.M)
    assert summaries == [(str(requested), str(requested), '1')], 'original argument count/read receipt differs'
    return dict(requested_words=requested, actual_words=len(rows), context_read=True,
                all_reads_succeeded=True)
