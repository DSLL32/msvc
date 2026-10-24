"""Explicit target namespace for the reached progress-feedback case only."""
def apply(record, environment):
    target = record.get('target_environment', {})
    assert not target or (record['name'] == 'dos-mapped-growth-feedback' and
                          target == {'_MSC_IDE_FLAGS': '-FEEDBACK fixture'}), 'unapproved target environment'
    environment.update(target)
    return dict(target)
