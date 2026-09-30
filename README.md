# Hash C Python
The goal is to write a Hash object in C, and then figure out how to make it a library that will be usable in Python. Create custom endpoints for the API. Compare my custom library against Python's dict

## Python bindings

Requires Python headers (Fedora: 'sudo dnf install python3-devel').

Build:

    `python setup.py build_ext --inplace`

Usage:

    ```
    import hashmod

    h = hashmod.HashTable(8)    # initial capacity
    h["a"] = 1
    print(h["a"], len(h))
    del h["a"]
    ```

Keys are 'str', values are C 'int. Missing keys raise 'KeyError'.

Tests:

    `make test`                     # C tests (adjust to your Makefile target)
    `python tests/test_hashmod.py`  # Python tests
