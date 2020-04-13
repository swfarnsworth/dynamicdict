//static PyMethodDef dydict_methods[] = {
//    {NULL}
//};


static struct PyModuleDef dynamicdictmodule = {
    PyModuleDef_HEAD_INIT,
    "dynamicdict",
    "Gives you a dynamicdict",
    -1,
    dydict_methods
};


PyDoc_STRVAR(module_doc,
"dynamicdict: dict subclass with a default value factory based on the missing key\n"
);

//static struct PyMethodDef module_functions[] = {
//    _COLLECTIONS__COUNT_ELEMENTS_METHODDEF
//    {NULL,       NULL}          /* sentinel */
//};

static struct PyModuleDef dydict_module = {
    PyModuleDef_HEAD_INIT,
    "dynamicdict",
    module_doc,
    -1,
    dydict_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC init_dydict_module(void)
{
    PyObject *m;

    m = PyModule_Create(&dydict_module);
    if (m == NULL)
        return NULL;

    dydict_type.tp_base = &PyDict_Type;
    if (PyType_Ready(&dydict_type) < 0)
        return NULL;

    Py_INCREF(&dydict_type);
    PyModule_AddObject(m, "dynamicdict", (PyObject *)&dydict_type);

    return m;
}

//PyMODINIT_FUNC PyInit_dydict(void) {
//    return PyModule_Create(&dynamicdictmodule);
//}