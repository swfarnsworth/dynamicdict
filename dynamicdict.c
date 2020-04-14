#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

/* dynamicdict type *********************************************************/

typedef struct {
    PyDictObject dict;
    PyObject *default_factory;
} dydictobject;

static PyTypeObject dydict_type; /* Forward */

PyDoc_STRVAR(dydict_missing_doc,
"__missing__(key) # Called by __getitem__ for missing key; pseudo-code:\n\
  if self.default_factory is None: raise KeyError((key,))\n\
  self[key] = value = self.default_factory(key)\n\
  return value\n\
");

static PyObject* dydict_missing(dydictobject *dyd, PyObject *key) {
    PyObject *factory = dyd->default_factory;
    PyObject *value;

    PyObject *tup = PyTuple_Pack(1, key);
    if (!tup) return NULL;

    if (factory == NULL || factory == Py_None) {
        /* XXX Call dict.__missing__(key) */
        PyErr_SetObject(PyExc_KeyError, tup);
        Py_DECREF(tup);
        return NULL;
    }
    value = PyObject_Call(factory, tup, NULL);
    if (value == NULL)
        return value;
    if (PyObject_SetItem((PyObject *)dyd, key, value) < 0) {
        Py_DECREF(value);
        return NULL;
    }
    return value;
}

static inline PyObject* new_dydict(dydictobject *dyd, PyObject *arg) {
    return PyObject_CallFunctionObjArgs(
        (PyObject*) Py_TYPE(dyd),
        dyd->default_factory ? dyd->default_factory : Py_None, arg, NULL
    );
}

PyDoc_STRVAR(dydict_copy_doc, "D.copy() -> a shallow copy of D.");

static PyObject* dydict_copy(dydictobject *dyd, PyObject *Py_UNUSED(ignored)) {
    /* This calls the object's class.  That only works for subclasses
       whose class constructor has the same signature.  Subclasses that
       define a different constructor signature must override copy().
    */
    return new_dydict(dyd, (PyObject*) dyd);
}

static PyObject* dydict_reduce(dydictobject *dyd, PyObject *Py_UNUSED(ignored)) {
    /* __reduce__ must return a 5-tuple as follows:

       - factory function
       - tuple of args for the factory function
       - additional state (here None)
       - sequence iterator (here None)
       - dictionary iterator (yielding successive (key, value) pairs

       This API is used by pickle.py and copy.py.

       For this to be useful with pickle.py, the default_factory
       must be picklable; e.g., None, a built-in, or a global
       function in a module or package.

       Both shallow and deep copying are supported, but for deep
       copying, the default_factory must be deep-copyable; e.g. None,
       or a built-in (functions are not copyable at this time).

       This only works for subclasses as long as their constructor
       signature is compatible; the first argument must be the
       optional default_factory, defaulting to None.
    */
    PyObject *args;
    PyObject *items;
    PyObject *iter;
    PyObject *result;
    _Py_IDENTIFIER(items);

    if (dyd->default_factory == NULL || dyd->default_factory == Py_None)
        args = PyTuple_New(0);
    else
        args = PyTuple_Pack(1, dyd->default_factory);
    if (args == NULL)
        return NULL;
    items = _PyObject_CallMethodId((PyObject *)dyd, &PyId_items, NULL);
    if (items == NULL) {
        Py_DECREF(args);
        return NULL;
    }
    iter = PyObject_GetIter(items);
    if (iter == NULL) {
        Py_DECREF(items);
        Py_DECREF(args);
        return NULL;
    }
    result = PyTuple_Pack(5, Py_TYPE(dyd), args,
                          Py_None, Py_None, iter);
    Py_DECREF(iter);
    Py_DECREF(items);
    Py_DECREF(args);
    return result;
}

static PyMethodDef dydict_methods[] = {
        {"__missing__", (PyCFunction)dydict_missing, METH_O,
                dydict_missing_doc},
        {"copy", (PyCFunction)dydict_copy, METH_NOARGS,
                dydict_copy_doc},
        {"__copy__", (PyCFunction)dydict_copy, METH_NOARGS,
                dydict_copy_doc},
        {"__reduce__", (PyCFunction)dydict_reduce, METH_NOARGS,
                reduce_doc},
        {NULL}
};

static PyMemberDef dydict_members[] = {
        {"default_factory", T_OBJECT,
                offsetof(dydictobject, default_factory), 0,
                PyDoc_STR("Factory for default value called by __missing__().")},
        {NULL}
};

static void dydict_dealloc(dydictobject *dyd) {
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(dyd);
    Py_CLEAR(dyd->default_factory);
    PyDict_Type.tp_dealloc((PyObject *)dyd);
}

static PyObject* dydict_repr(dydictobject *dyd) {
    PyObject *baserepr;
    PyObject *defrepr;
    PyObject *result;
    baserepr = PyDict_Type.tp_repr((PyObject *)dyd);
    if (baserepr == NULL)
        return NULL;
    if (dyd->default_factory == NULL)
        defrepr = PyUnicode_FromString("None");
    else {
        int status = Py_ReprEnter(dyd->default_factory);
        if (status != 0) {
            if (status < 0) {
                Py_DECREF(baserepr);
                return NULL;
            }
            defrepr = PyUnicode_FromString("...");
        }
        else
            defrepr = PyObject_Repr(dyd->default_factory);
        Py_ReprLeave(dyd->default_factory);
    }
    if (defrepr == NULL) {
        Py_DECREF(baserepr);
        return NULL;
    }
    result = PyUnicode_FromFormat("%s(%U, %U)",
                                  _PyType_Name(Py_TYPE(dyd)),
                                  defrepr, baserepr);
    Py_DECREF(defrepr);
    Py_DECREF(baserepr);
    return result;
}

static PyObject* dydict_or(PyObject* left, PyObject* right) {
    PyObject *self, *other;
    if (PyObject_TypeCheck(left, &dydict_type)) {
        self = left;
        other = right;
    }
    else {
        self = right;
        other = left;
    }
    if (!PyDict_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    // Like copy(), this calls the object's class.
    // Override __or__/__ror__ for subclasses with different constructors.
    PyObject *new = new_dydict((dydictobject*)self, left);
    if (!new) {
        return NULL;
    }
    if (PyDict_Update(new, right)) {
        Py_DECREF(new);
        return NULL;
    }
    return new;
}

static PyNumberMethods dydict_as_number = {
        .nb_or = dydict_or,
};

static int dydict_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(((dydictobject *)self)->default_factory);
    return PyDict_Type.tp_traverse(self, visit, arg);
}

static int dydict_tp_clear(dydictobject *dyd) {
    Py_CLEAR(dyd->default_factory);
    return PyDict_Type.tp_clear((PyObject *)dyd);
}

static int dydict_init(PyObject *self, PyObject *args, PyObject *kwds) {
    dydictobject *dyd = (dydictobject *)self;
    PyObject *olddefault = dyd->default_factory;
    PyObject *newdefault = NULL;
    PyObject *newargs;
    int result;
    if (args == NULL || !PyTuple_Check(args))
        newargs = PyTuple_New(0);
    else {
        Py_ssize_t n = PyTuple_GET_SIZE(args);
        if (n > 0) {
            newdefault = PyTuple_GET_ITEM(args, 0);
            if (!PyCallable_Check(newdefault) && newdefault != Py_None) {
                PyErr_SetString(PyExc_TypeError,
                                "first argument must be callable or None");
                return -1;
            }
        }
        newargs = PySequence_GetSlice(args, 1, n);
    }
    if (newargs == NULL)
        return -1;
    Py_XINCREF(newdefault);
    dyd->default_factory = newdefault;
    result = PyDict_Type.tp_init(self, newargs, kwds);
    Py_DECREF(newargs);
    Py_XDECREF(olddefault);
    return result;
}

PyDoc_STRVAR(dydict_doc,
"dynamicdict(default_factory[, ...]) --> dict with default factory\n\
\n\
The default factory is called with the given key as an argument to produce\n\
a new value when a key is not present, in __getitem__ only.\n\
A dynamicdict compares equal to a dict with the same items.\n\
All remaining arguments are treated the same as if they were\n\
passed to the dict constructor, including keyword arguments.\n\
");

/* See comment in xxsubtype.c */
#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject dydict_type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "dynamicdict",
        .tp_basicsize = sizeof(dydictobject),
        .tp_itemsize = 0,
        .tp_dealloc = (destructor)dydict_dealloc,
        .tp_repr = (reprfunc) dydict_repr,
        .tp_as_number = &dydict_as_number,
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
        .tp_doc = dydict_doc,
        .tp_traverse = dydict_traverse,
        .tp_clear = (inquiry)dydict_tp_clear,
        .tp_methods = dydict_methods,
        .tp_members = dydict_members,
        .tp_base = &PyDict_Type,
        .tp_init = dydict_init,
        .tp_alloc = PyType_GenericAlloc,
        .tp_free = PyObject_GC_Del,
};

/* Module code */

static PyMethodDef dydict_module_methods[] = {
    {NULL}  /* Sentinel */
};

static struct PyModuleDef dydict_module = {
    PyModuleDef_HEAD_INIT,
    "dynamicdict",
    "Module containing dynamicdict",
    -1,
    dydict_module_methods
};

PyMODINIT_FUNC PyInit_dynamicdict(void) {
    PyObject* m;
    m = PyModule_Create(&dydict_module);
    if (m == NULL)
        return NULL;

    if (PyType_Ready(&dydict_type) < 0)
        return NULL;
        
    Py_INCREF(&dydict_type);
    if (PyModule_AddObject(m, "dynamicdict", (PyObject*) &dydict_type) < 0) {
        Py_DECREF(&dydict_type);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
