"""Error-workflow artifacts are compared byte-for-byte, with no normalization."""
def timestamp_fields(data,kind):
    assert kind=='raw'
    return []


def normalized(data,fields):
    assert fields==[]
    return bytes(data)
