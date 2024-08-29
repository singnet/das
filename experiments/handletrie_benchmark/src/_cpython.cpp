#include <Python.h>
#include "HandleTrie.h"
#include <iostream>

using namespace attention_broker_server;
using namespace std;

typedef struct
{
    PyObject_HEAD HandleTrie *trie;
} PyHandleTrieObject;

class Value : public HandleTrie::TrieValue
{
public:
    PyObject *value_;
    Value(PyObject *value)
    {
        this->value_ = value;
        Py_XINCREF(value_);
    }
    void merge(TrieValue *other)
    {
    }
    ~Value()
    {
        Py_XDECREF(value_);
    }

    std::string to_string()
    {
    }
};

static int PyHandleTrie_init(PyHandleTrieObject *self, PyObject *args)
{
    int key_size;
    if (!PyArg_ParseTuple(args, "I", &key_size))
    {
        return -1;
    }
    self->trie = new HandleTrie(key_size);
    return 0;
}

static void PyHandleTrie_dealloc(PyHandleTrieObject *self)
{
    // Clean up the HandleTrie instance
    delete self->trie;
    // Call the base class's deallocation function
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *PyHandleTrie_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyHandleTrieObject *self;
    self = (PyHandleTrieObject *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->trie = NULL; // Initialize the point member to NULL
    }
    return (PyObject *)self;
}

static PyObject *PyHandleTrie_insert(PyHandleTrieObject *self, PyObject *args)
{
    PyObject *value;
    const char *key;
    if (!PyArg_ParseTuple(args, "sO", &key, &value))
    {
        return NULL;
    }
    string s = key;
    self->trie->insert(s, new Value(value));
    Py_RETURN_NONE;
}

static PyObject *generate_word(PyHandleTrieObject *self, PyObject *args)
{
    int key_size;
    char buffer[1000];
    if (!PyArg_ParseTuple(args, "i", &key_size))
    {
        return nullptr; // Return NULL on failure
    }
    char R_TLB[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (unsigned int j = 0; j < key_size; j++)
    {
        buffer[j] = R_TLB[(rand() % 16)];
    }
    buffer[key_size] = 0;
    string s = buffer;
    return PyUnicode_FromString(s.c_str());
}

static PyObject *PyHandleTrie_lookup(PyHandleTrieObject *self, PyObject *args)
{
    const char *key;
    if (!PyArg_ParseTuple(args, "s", &key))
    {
        return NULL;
    }
    string k = key;
    Value *value = (Value *)self->trie->lookup(k);
    if (value == NULL)
    {
        Py_RETURN_NONE;
    }
    return value->value_;
}

static PyMethodDef PyHandleTrie_methods[] = {
    {"insert", (PyCFunction)PyHandleTrie_insert, METH_VARARGS, ""},
    {"lookup", (PyCFunction)PyHandleTrie_lookup, METH_VARARGS, ""},
    {NULL} // Sentinel
};

static PyMethodDef OtherMethods[] = {
    {"generate_word", (PyCFunction)generate_word, METH_VARARGS, ""},
    {nullptr, nullptr, 0, nullptr} // Sentinel
};

static PyTypeObject PyHandleTrieType = {
    PyVarObject_HEAD_INIT(NULL, 0) "handletrie_cpython.HandleTrie", // tp_name
    sizeof(PyHandleTrieObject),                                     // tp_basicsize
    0,                                                              // tp_itemsize
    (destructor)PyHandleTrie_dealloc,                               // tp_dealloc
    0,                                                              // tp_vectorcall_offset
    NULL,                                                           // tp_getattr
    NULL,                                                           // tp_setattr
    NULL,                                                           // tp_as_async
    NULL,                                                           // tp_repr
    NULL,                                                           // tp_as_number
    NULL,                                                           // tp_as_sequence
    NULL,                                                           // tp_as_mapping
    NULL,                                                           // tp_hash
    NULL,                                                           // tp_call
    NULL,                                                           // tp_str
    NULL,                                                           // tp_getattro
    NULL,                                                           // tp_setattro
    NULL,                                                           // tp_as_buffer
    NULL,                                                           // tp_flags
    "HandleTrie objects",                                           // tp_doc
    NULL,                                                           // tp_traverse
    NULL,                                                           // tp_clear
    NULL,                                                           // tp_richcompare
    0,                                                              // tp_weaklistoffset
    NULL,                                                           // tp_iter
    NULL,                                                           // tp_iternext
    PyHandleTrie_methods,                                           // tp_methods
    NULL,                                                           // tp_members
    NULL,                                                           // tp_getset
    NULL,                                                           // tp_base
    NULL,                                                           // tp_dict
    NULL,                                                           // tp_descr_get
    NULL,                                                           // tp_descr_set
    0,                                                              // tp_dictoffset
    (initproc)PyHandleTrie_init,                                    // tp_init
    NULL,                                                           // tp_alloc
    PyHandleTrie_new,                                               // tp_new
};

static PyModuleDef handletrie = {
    PyModuleDef_HEAD_INIT,
    "handletrie_cpython",                       // m_name
    "Example module that creates a HandleTrie", // m_doc
    -1,                                         // m_size
    OtherMethods,                               // m_methods
};

PyMODINIT_FUNC PyInit_handletrie_cpython(void)
{
    PyObject *m;
    if (PyType_Ready(&PyHandleTrieType) < 0)
    {
        return NULL;
    }
    m = PyModule_Create(&handletrie);
    if (m == NULL)
    {
        return NULL;
    }
    Py_INCREF(&PyHandleTrieType);
    PyModule_AddObject(m, "HandleTrie", (PyObject *)&PyHandleTrieType);
    return m;
}
