"""Paths for the ordered C1XX frontend reconstruction cases."""
from pathlib import Path

BINARY=Path(__file__).resolve().parents[1]


def paths(config,name):
    case=config['cases'][name]
    fixture=(BINARY/case.get('fixture','../fixtures/cpp-basic')).resolve()
    reference=BINARY/case.get('native_reference','cases/native-cpp-basic')
    frontiers=BINARY/case.get('frontiers','cases/cpp-basic-frontiers')
    return case,fixture,reference,frontiers
