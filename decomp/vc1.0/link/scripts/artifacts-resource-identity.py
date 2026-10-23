"""Pure resource-image clock parser plus the observed empty fatal temporary file."""
import hashlib
import importlib.util
from pathlib import Path

source=Path(__file__).with_name('artifacts-resources.py')
assert hashlib.sha256(source.read_bytes()).hexdigest()=='a4cd8686adf1b291d5e7ef5e76cfac127e41b4dc2e4fd283dba8391ce123b2a8'
spec=importlib.util.spec_from_file_location('resource_identity_clocks',source)
parser=importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)


def timestamp_fields(data,kind):
    if kind=='empty-resource-temporary':
        assert data==b'', 'Fatal conversion temporary must be exactly empty'
        return []
    return parser.timestamp_fields(data,kind)


def normalized(data,fields):
    return parser.normalized(data,fields)
