"""Authenticate a private immutable isolated API capture before host enabling."""
import hashlib
import importlib.util
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(original, output):
    checker = Path(__file__).with_name('check-isolated-locale.py')
    spec = importlib.util.spec_from_file_location('isolated_locale_reference', checker)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    evidence = module.check(original)
    # The original public API contract is enabled, not pre-applied. The actual
    # target call must still verify initial state and perform the transition.
    (output / 'locale-original-validation.json').write_text(json.dumps(evidence, indent=2) + '\n')
    for path, name in ((checker, 'locale-original-checker.py'), (Path(__file__), 'locale-reference-source.py')):
        (output / name).write_bytes(path.read_bytes())
    binding = dict(kind='enable actual isolated LC_CTYPE empty selection; no bind-time locale mutation', original_directory=str(original.resolve()), original_receipt_sha256=evidence['original_receipt_sha256'], provider_sha256=evidence['provider_sha256'], original_trial_sha256=evidence['trial_sha256'], current_native_trial_sha256=sha(output / 'trial.json'), original_validation_sha256=sha(output / 'locale-original-validation.json'), checker_sha256=sha(checker), factory_sha256=sha(Path(__file__)))
    (output / 'locale-selection-binding.json').write_text(json.dumps(binding, indent=2) + '\n')
    return evidence['original_receipt_sha256']
