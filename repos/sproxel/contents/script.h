#ifndef __SPROXEL_SCRIPT_H__
#define __SPROXEL_SCRIPT_H__


#include <Python.h>


void init_script(const char *exe_path);
void close_script();

void script_set_main_window(class MainWindow *);

void scan_plugins();
void register_plugins();
void unregister_plugins();

bool run_script(const class QString &filename);


#define GLUE(a, b) a##b
#define TOPYT(cls) PyObject * GLUE(cls, _toPy ) (class cls *);
#define TOCPP(cls) class cls* GLUE(cls, _toCpp) (PyObject *);
#include "glue/classGlue.h"
#undef GLUE


extern class QDir exe_dir;

extern PyObject *py_save_project, *py_load_project;


PyObject* qstr_to_py(const QString &str);

bool py_to_qstr(PyObject *o, QString &str);


#endif
