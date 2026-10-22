"""Fixture clock fields: shared archive/COFF parser plus ordinary PE header."""
from importlib.machinery import SourceFileLoader
from pathlib import Path
_base=SourceFileLoader('link_debug_artifacts',str(Path(__file__).resolve().parent.parent/'link-debug-dll/artifacts.py')).load_module()
def timestamp_fields(data,kind):
    if kind=='plain-pe':
        pe=_base.u32(data,60)
        assert data[:2]==b'MZ' and data[pe:pe+4]==b'PE\0\0'
        assert _base.u16(data,pe+4)==0x14c
        return [dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp')]
    return _base.timestamp_fields(data,kind)
def normalize_artifact(data,kind):
    image=bytearray(data)
    for field in timestamp_fields(image,kind):
        image[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    return bytes(image)
