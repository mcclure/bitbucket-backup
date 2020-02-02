#ifndef __SPROXEL_PY_BINDINGS_H__
#define __SPROXEL_PY_BINDINGS_H__


#include "script.h"
#include "SproxelProject.h"
#include "UndoManager.h"


PyObject* sprite_to_py(VoxelGridGroupPtr sprite);
PyObject* project_to_py(SproxelProjectPtr proj);
PyObject* undo_manager_to_py(UndoManager *);


PyObject* PySproxel_registerImporter(PyObject*, PyObject *arg);
PyObject* PySproxel_unregisterImporter(PyObject*, PyObject *arg);
PyObject* PySproxel_registerExporter(PyObject*, PyObject *arg);
PyObject* PySproxel_unregisterExporter(PyObject*, PyObject *arg);


#endif
