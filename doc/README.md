# API documentation

Doxygen HTML/XML for public headers under `modules/nfc_pn7160/include/`.

```bash
# Local build (requires doxygen + pip install -r doc/requirements.txt)
make -C doc html
make -C doc doxygen-coverage-json
```

HTML output: `doc/_build/doxygen/html/index.html`

Coverage JSON (for CI delta checks): `doc/_build/doc-coverage.json`
