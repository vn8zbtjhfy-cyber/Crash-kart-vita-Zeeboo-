"""Host-only safety test of the source code distribution validator."""
import importlib.util
import pathlib
import tempfile
import zipfile

HERE = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('validator', HERE / 'scripts' / 'verificar_fuentes_limpias.py')
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)
required = sorted(validator.REQUIRED)

def assert_rejected(name):
    with tempfile.NamedTemporaryFile(suffix='.zip') as tmp:
        with zipfile.ZipFile(tmp.name, 'w') as out:
            for file in required:
                out.writestr(file, 'x')
            out.writestr(name, 'forbidden')
        try:
            validator.audit(tmp.name)
        except ValueError:
            return
        raise AssertionError(f'unsafe ZIP accepted: {name}')

with tempfile.NamedTemporaryFile(suffix='.zip') as tmp:
    with zipfile.ZipFile(tmp.name, 'w') as out:
        for file in required:
            out.writestr(file, 'x')
    assert validator.audit(tmp.name) == len(required)
for unsafe in ('data.vfs', 'assets/cnk2.mod', 'assets/cnk2.sig', 'src/../../private.txt', 'assets/photo.png', 'data.zip'):
    assert_rejected(unsafe)
print('PASS: source ZIP sanitizer rejects original assets and unsafe paths')
