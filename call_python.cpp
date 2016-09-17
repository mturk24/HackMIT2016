#include “stdafx.h”
#include <iostream>
#include <Python.h>
// paste into save screenshot

Py_Initialize();
PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pValue;
pName = PyString_FromString("label.py");
pModule = PyImport_Import(pName);
pDict = PyModule_GetDict(pModule);
pFunc = PyDict_GetItemString(pDict, "is_parking_lot");
pArgs = PyTuple_New(1);
pValue = PyString_FromString(szScreenshotPath);
PyTuple_SetItem(pArgs, 0, pValue);
PyTuple_SetItem(pArgs, 1, pValue);
PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
long result = PyInt_AsLong(pResult);
Py_Finalize();
if (result == 1) {
    //upload gps location
}
