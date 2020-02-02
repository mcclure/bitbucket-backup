#include "pyBindings.h"
#include "ImportExport.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class PyImporter : public Importer
{
public:
  PyObject *obj;

  PyImporter(PyObject *o) : obj(o) {}

  ~PyImporter() { Py_XDECREF(obj); obj=NULL; }


  virtual QString name()
  {
    PyObject *o=PyObject_CallMethod(obj, "name", NULL);
    if (!o) { PyErr_Print(); return QString(); }

    QString s;
    py_to_qstr(o, s);
    Py_DECREF(o);
    return s;
  }


  virtual QString filter()
  {
    PyObject *o=PyObject_CallMethod(obj, "filter", NULL);
    if (!o) { PyErr_Print(); return QString(); }

    QString s;
    py_to_qstr(o, s);
    Py_DECREF(o);
    return s;
  }


  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr spr)
  {
    PyObject *name=PyString_FromString("doImport");
    PyObject *o=PyObject_CallMethodObjArgs(obj, name,
      qstr_to_py(filename), undo_manager_to_py(um),
      project_to_py(project), sprite_to_py(spr), NULL);
    Py_DECREF(name);
    if (!o) { PyErr_Print(); return false; }

    bool res=PyObject_IsTrue(o);
    Py_DECREF(o);
    return res;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class PyExporter : public Exporter
{
public:
  PyObject *obj;

  PyExporter(PyObject *o) : obj(o) {}

  ~PyExporter() { Py_XDECREF(obj); obj=NULL; }


  virtual QString name()
  {
    PyObject *o=PyObject_CallMethod(obj, "name", NULL);
    if (!o) { PyErr_Print(); return QString(); }

    QString s;
    py_to_qstr(o, s);
    Py_DECREF(o);
    return s;
  }


  virtual QString filter()
  {
    PyObject *o=PyObject_CallMethod(obj, "filter", NULL);
    if (!o) { PyErr_Print(); return QString(); }

    QString s;
    py_to_qstr(o, s);
    Py_DECREF(o);
    return s;
  }


  virtual bool doExport(const QString &filename,
    SproxelProjectPtr project, VoxelGridGroupPtr spr)
  {
    PyObject *name=PyString_FromString("doExport");
    PyObject *o=PyObject_CallMethodObjArgs(obj, name,
      qstr_to_py(filename), project_to_py(project), sprite_to_py(spr), NULL);
    Py_DECREF(name);
    if (!o) { PyErr_Print(); return false; }

    bool res=PyObject_IsTrue(o);
    Py_DECREF(o);
    return res;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static QList<PyImporter*> pyimps;


PyObject* PySproxel_registerImporter(PyObject*, PyObject *arg)
{
  foreach (PyImporter *p, pyimps)
    if (p->obj==arg) Py_RETURN_FALSE;

  PyImporter *p=new PyImporter(arg);
  pyimps.append(p);
  register_importer(p);
  Py_RETURN_TRUE;
}


PyObject* PySproxel_unregisterImporter(PyObject*, PyObject *arg)
{
  for (int i=0; i<pyimps.size(); ++i)
    if (pyimps[i]->obj==arg)
    {
      PyImporter *p=pyimps[i];
      pyimps.removeAt(i);
      unregister_importer(p);
      delete p;
      Py_RETURN_TRUE;
    }

  Py_RETURN_FALSE;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static QList<PyExporter*> pyexps;


PyObject* PySproxel_registerExporter(PyObject*, PyObject *arg)
{
  foreach (PyExporter *p, pyexps)
    if (p->obj==arg) Py_RETURN_FALSE;

  PyExporter *p=new PyExporter(arg);
  pyexps.append(p);
  register_exporter(p);
  Py_RETURN_TRUE;
}


PyObject* PySproxel_unregisterExporter(PyObject*, PyObject *arg)
{
  for (int i=0; i<pyexps.size(); ++i)
    if (pyexps[i]->obj==arg)
    {
      PyExporter *p=pyexps[i];
      pyexps.removeAt(i);
      unregister_exporter(p);
      delete p;
      Py_RETURN_TRUE;
    }

  Py_RETURN_FALSE;
}
