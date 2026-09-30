#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <limits.h>
#include "hashtable.h"

typedef struct {
    PyObject_HEAD
    Hash *ht;
} PyHash;

static PyObject *PyHash_new(PyTypeObject *type, PyObject *args, PyObject *kw) {
    PyHash *self = (PyHash *)type->tp_alloc(type, 0);
    if (self) self->ht = NULL;
    return (PyObject *)self;
}

static int PyHash_init(PyHash *self, PyObject *args, PyObject *kw) {
    Py_ssize_t cap = 8;
    if (!PyArg_ParseTuple(args, "|n", &cap)) return -1;
    if (cap <= 0) {
        PyErr_SetString(PyExc_ValueError, "capacity must be positive");
        return -1;
    }
    self->ht = ht_create((size_t)cap);
    if (!self->ht) { PyErr_NoMemory(); return -1; }
    return 0;
}

static void PyHash_dealloc(PyHash *self) {
    ht_destroy(self->ht);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyHash_insert(PyHash *self, PyObject *args) {
    const char *key;
    int value;
    if (!PyArg_ParseTuple(args, "si", &key, &value)) return NULL;
    if (!ht_insert(self->ht, key, value)) {
        PyErr_NoMemory();
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *PyHash_get(PyHash *self, PyObject *args) {
    const char *key;
    int value;
    if (!PyArg_ParseTuple(args, "s", &key)) return NULL;
    if (!ht_get(self->ht, key, &value)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return NULL;
    }
    return PyLong_FromLong(value);
}

static PyObject *PyHash_delete(PyHash *self, PyObject *args) {
    const char *key;
    if (!PyArg_ParseTuple(args, "s", &key)) return NULL;
    if (!ht_delete(self->ht, key)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return NULL;
    }
    Py_RETURN_NONE;
}

static Py_ssize_t PyHash_length(PyHash *self) {
    return (Py_ssize_t)ht_size(self->ht);
}

/* h["a"] */
static PyObject *PyHash_subscript(PyHash *self, PyObject *key) {
    const char *k = PyUnicode_AsUTF8(key);
    if (!k) return NULL;                       /* not a str -> TypeError already set */
    int value;
    if (!ht_get(self->ht, k, &value)) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    return PyLong_FromLong(value);
}

/* h["a"] = 1   and   del h["a"]  (value == NULL means delete) */
static int PyHash_ass_subscript(PyHash *self, PyObject *key, PyObject *value) {
    const char *k = PyUnicode_AsUTF8(key);
    if (!k) return -1;

    if (value == NULL) {
        if (!ht_delete(self->ht, k)) {
            PyErr_SetObject(PyExc_KeyError, key);
            return -1;
        }
        return 0;
    }

    long v = PyLong_AsLong(value);
    if (v == -1 && PyErr_Occurred()) return -1;
    if (v > INT_MAX || v < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError, "value does not fit in a C int");
        return -1;
    }
    if (!ht_insert(self->ht, k, (int)v)) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static PyMappingMethods PyHash_mapping = {
    .mp_length        = (lenfunc)PyHash_length,
    .mp_subscript     = (binaryfunc)PyHash_subscript,
    .mp_ass_subscript = (objobjargproc)PyHash_ass_subscript,
};

static PyMethodDef PyHash_methods[] = {
    {"insert", (PyCFunction)PyHash_insert, METH_VARARGS, "insert(key, value)"},
    {"get",    (PyCFunction)PyHash_get,    METH_VARARGS, "get(key) -> value"},
    {"delete",  (PyCFunction)PyHash_delete,METH_VARARGS, "delete(key)"},
    {NULL}
};

static PyTypeObject PyHashType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "hashmod.HashTable",
    .tp_basicsize = sizeof(PyHash),
    .tp_flags     = Py_TPFLAGS_DEFAULT,
    .tp_new       = PyHash_new,
    .tp_init      = (initproc)PyHash_init,
    .tp_dealloc   = (destructor)PyHash_dealloc,
    .tp_methods   = PyHash_methods,
    .tp_as_mapping = &PyHash_mapping,
};

static struct PyModuleDef hashmod = {
    PyModuleDef_HEAD_INIT, "hashmod", NULL, -1, NULL
};

PyMODINIT_FUNC PyInit_hashmod(void) {
    if (PyType_Ready(&PyHashType) < 0) return NULL;
    PyObject *m = PyModule_Create(&hashmod);
    if (!m) return NULL;
    Py_INCREF(&PyHashType);
    if (PyModule_AddObject(m, "HashTable", (PyObject *)&PyHashType) < 0) {
        Py_DECREF(&PyHashType);
        Py_DECREF(m);
        return NULL;
    }
    return m;
}