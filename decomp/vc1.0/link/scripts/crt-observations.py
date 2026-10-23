"""Validate a declared, nonempty public CRT observation inventory; no execution."""
def validate(expected,original,native):
    assert expected and original and native, 'CRT boundary inventory must be nonempty on all sides'
    assert len(expected)==len(original)==len(native), 'CRT boundary observation count differs'
    for planned,left,right in zip(expected,original,native):
        assert planned['phase'] and planned['call']>=1, 'Undeclared CRT call/phase'
        actual={key:planned[key] for key in ('before','after')}
        assert left==right==actual, 'Public CRT boundary state differs'
    return len(expected)
