import os
import sys

# hashmod.*.so is built into the project root, one level up from tests/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import hashmod

passed = 0
failed = 0


def check(name, fn):
    global passed, failed
    try:
        fn()
        print(f"  [PASS] {name}")
        passed += 1
    except Exception as e:
        print(f"  [FAIL] {name}  ({type(e).__name__}: {e})")
        failed += 1


def expect_raises(exc, fn):
    try:
        fn()
    except exc:
        return
    raise AssertionError(f"expected {exc.__name__}")


def t_create():
    hashmod.HashTable(8)
    hashmod.HashTable()          # default capacity


def t_bad_capacity():
    expect_raises(ValueError, lambda: hashmod.HashTable(0))


def t_insert():
    h = hashmod.HashTable(8)
    assert h.insert("a", 1) is None


def t_insert_bad_args():
    h = hashmod.HashTable(8)
    expect_raises(TypeError, lambda: h.insert(5, 1))       # key must be str
    expect_raises(TypeError, lambda: h.insert("a", "x"))   # value must be int


def t_get():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    h.insert("b", 2)
    assert h.get("a") == 1
    assert h.get("b") == 2


def t_update():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    h.insert("a", 99)
    assert h.get("a") == 99


def t_missing():
    h = hashmod.HashTable(8)
    expect_raises(KeyError, lambda: h.get("nope"))


def t_many():
    h = hashmod.HashTable(2)     # forces resizes
    for i in range(1000):
        h.insert(f"key{i}", i)
    assert all(h.get(f"key{i}") == i for i in range(1000))


def t_many_tables():
    # create and drop lots of tables (exercises dealloc)
    for _ in range(1000):
        h = hashmod.HashTable(4)
        h.insert("x", 1)
        del h


def t_delete():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    h.insert("b", 2)
    assert h.delete("a") is None
    expect_raises(KeyError, lambda: h.get("a"))
    assert h.get("b") == 2                    # neighbor survives


def t_delete_twice():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    h.delete("a")
    expect_raises(KeyError, lambda: h.delete("a"))


def t_delete_missing():
    h = hashmod.HashTable(8)
    expect_raises(KeyError, lambda: h.delete("nope"))


def t_delete_bad_args():
    h = hashmod.HashTable(8)
    expect_raises(TypeError, lambda: h.delete(5))
    expect_raises(TypeError, lambda: h.delete())


def t_reinsert_after_delete():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    h.delete("a")
    h.insert("a", 2)
    assert h.get("a") == 2


def t_delete_many():
    h = hashmod.HashTable(2)
    for i in range(1000):
        h.insert(f"key{i}", i)
    for i in range(0, 1000, 2):               # delete the even keys
        h.delete(f"key{i}")
    for i in range(1000):
        if i % 2 == 0:
            expect_raises(KeyError, lambda: h.get(f"key{i}"))
        else:
            assert h.get(f"key{i}") == i


def t_dict_setget():
    h = hashmod.HashTable(8)
    h["a"] = 1
    h["b"] = 2
    assert h["a"] == 1 and h["b"] == 2
    h["a"] = 10                               # overwrite
    assert h["a"] == 10 and len(h) == 2


def t_dict_len():
    h = hashmod.HashTable(2)
    assert len(h) == 0
    for i in range(100):
        h[f"k{i}"] = i
    assert len(h) == 100
    del h["k0"]
    assert len(h) == 99


def t_dict_del():
    h = hashmod.HashTable(8)
    h["a"] = 1
    h["b"] = 2
    del h["a"]
    expect_raises(KeyError, lambda: h["a"])
    assert h["b"] == 2
    expect_raises(KeyError, lambda: h.__delitem__("a"))   # already gone


def t_dict_missing():
    h = hashmod.HashTable(8)
    expect_raises(KeyError, lambda: h["nope"])


def t_dict_bad_types():
    h = hashmod.HashTable(8)
    expect_raises(TypeError, lambda: h.__getitem__(5))            # key not str
    expect_raises(TypeError, lambda: h.__setitem__(5, 1))
    expect_raises(TypeError, lambda: h.__setitem__("a", "x"))     # value not int
    expect_raises(OverflowError, lambda: h.__setitem__("a", 2**40))


def t_dict_matches_methods():
    h = hashmod.HashTable(8)
    h.insert("a", 1)
    assert h["a"] == 1
    h["b"] = 2
    assert h.get("b") == 2
    h.delete("a")
    expect_raises(KeyError, lambda: h["a"])


print("hashmod Python tests")
check("create", t_create)
check("capacity 0 raises ValueError", t_bad_capacity)
check("insert returns None", t_insert)
check("insert rejects bad arg types", t_insert_bad_args)
check("get returns inserted values", t_get)
check("re-insert updates value", t_update)
check("get missing raises KeyError", t_missing)
check("1000 keys through resizes", t_many)
check("create/destroy 1000 tables", t_many_tables)
check("delete removes key, keeps neighbors", t_delete)
check("delete twice raises KeyError", t_delete_twice)
check("delete missing raises KeyError", t_delete_missing)
check("delete rejects bad args", t_delete_bad_args)
check("re-insert after delete", t_reinsert_after_delete)
check("delete 500 of 1000 keys", t_delete_many)
check("h[k] = v and h[k]", t_dict_setget)
check("len(h) tracks size", t_dict_len)
check("del h[k]", t_dict_del)
check("h[missing] raises KeyError", t_dict_missing)
check("dict syntax rejects bad types", t_dict_bad_types)
check("dict syntax matches insert/get/delete", t_dict_matches_methods)

print(f"\n  {passed} / {passed + failed} passed")
sys.exit(0 if failed == 0 else 1)