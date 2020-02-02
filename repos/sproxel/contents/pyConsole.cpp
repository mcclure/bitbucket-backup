#include <Python.h>
#include <QPlainTextEdit>
#include <QAction>
#include <QDir>
#include <QApplication>
#include <stdarg.h>
#include "pyConsole.h"
#include "ConsoleWidget.h"
#include "MainWindow.h"
#include "script.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static FILE *logFile=NULL;


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static ConsoleWidget *console=NULL;


ConsoleWidget::ConsoleWidget(const QString &title, QWidget *parent)
  : QPlainTextEdit(parent), viewAction(NULL)
{
  setWindowTitle(title);
  setReadOnly(true);
  setFont(QFont("Lucida Console", 10, QFont::Bold));

  setStyleSheet("background: black; color: #1e1;");

  viewAction=new QAction(title, this);
  viewAction->setCheckable(true);
  viewAction->setChecked(true);
  connect(viewAction, SIGNAL(toggled(bool)), this, SLOT(setVisible(bool)));
}


QSize ConsoleWidget::sizeHint() const
{
  return QSize(800, 600);
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void pycon_raw(const char *s)
{
  if (logFile)
  {
    fputs(s, logFile);
    fflush(logFile);
  }

  if (console) console->insertPlainText(s);
}


void pycon(const char *fmt, ...)
{
  char buf[4096];

  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  pycon_raw(buf);
  pycon_raw("\n");
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static PyObject* console_write(PyObject *, PyObject *args)
{
  const char *str;
  if (!PyArg_ParseTuple(args, "s", &str)) return NULL;
  pycon_raw(str);
  Py_RETURN_NONE;
}


static PyMethodDef methods[]=
{
  { "write", console_write, METH_VARARGS, "Write to Sproxel console." },
  { NULL, NULL, 0, NULL }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void init_python_console()
{
  //logFile=_wfopen((wchar_t*)exe_dir.filePath("log.txt").unicode(), L"wt");

  console=new ConsoleWidget("Sproxel Python console");
  console->show();
  pycon("Starting Sproxel " SPROXEL_VERSION);

  Py_InitModule("sproxelConsole", methods);

  PyRun_SimpleString(
    "import sys\n"
    "import sproxelConsole\n"
    "\n"
    "class SproxelConsoleIO(object):\n"
    "  def write(self, s):\n"
    "    sproxelConsole.write(s)\n"
    "\n"
    "sys.stdout=SproxelConsoleIO()\n"
    "sys.stderr=sys.stdout\n"
  );
}


void close_python_console()
{
  if (console) { delete console; console=NULL; }

  if (logFile) { fclose(logFile); logFile=NULL; }
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


ConsoleWidget* get_python_console_widget()
{
  return console;
}
