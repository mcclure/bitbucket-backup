#include <pyside_qtgui_python.h>


#define GLUE(a, b) a##b


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


#define TOPYT(cls) \
  static PyObject* GLUE(cls, _toPy) (PyObject*, PyObject *args) \
  { \
    PyObject *c=PyTuple_GetItem(args, 0); \
    if (!c) return NULL; \
    void *p=PyCapsule_GetPointer(c, NULL); \
    if (!p) return NULL; \
    return Shiboken::Converter<cls*>::toPython((cls*)p); \
  }


#define TOCPP(cls) \
  static PyObject* GLUE(cls, _toCpp) (PyObject*, PyObject *args) \
  { \
    PyObject *w=PyTuple_GetItem(args, 0); \
    if (!w) return NULL; \
    cls *p=Shiboken::Converter<cls*>::toCpp(w); \
    if (!p) { PyErr_SetString(PyExc_StandardError, "NULL pointer in " #cls "_toCpp"); return NULL; } \
    return PyCapsule_New(p, NULL, NULL); \
  }


#include "classGlue.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyMethodDef methods[]=
{
  #define TOPYT(cls) { #cls "_toPy" , GLUE(cls, _toPy ), METH_VARARGS, "Convert " #cls " to Python." },
  #define TOCPP(cls) { #cls "_toCpp", GLUE(cls, _toCpp), METH_VARARGS, "Convert " #cls " to C++."    },
  #include "classGlue.h"
  { NULL, NULL, 0, NULL }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


PyTypeObject** SbkPySide_QtCoreTypes=NULL;
PyTypeObject** SbkPySide_QtGuiTypes=NULL;


PyMODINIT_FUNC initSproxelGlue()
{
  if (!Shiboken::importModule("PySide.QtCore", &SbkPySide_QtCoreTypes))
  {
    PyErr_SetString(PyExc_ImportError, "Could not import PySide.QtCore");
    return;
  }

  if (!Shiboken::importModule("PySide.QtGui", &SbkPySide_QtGuiTypes))
  {
    PyErr_SetString(PyExc_ImportError, "Could not import PySide.QtGui");
    return;
  }

  Py_InitModule("PySide.SproxelGlue", methods);
}
