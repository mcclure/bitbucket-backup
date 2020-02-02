#include "MainWindow.h"
#include "ImportExport.h"
#include "script.h"
#include "ConsoleWidget.h"
#include "pyConsole.h"

#include <QtGui>
#include <QApplication>


MainWindow *main_window=NULL;


int main(int argc, char *argv[])
{
    QString filename = "";
    if (argc > 1)
        filename = argv[1];

    QApplication a(argc, argv);

    register_builtin_importers_exporters();

    init_script(argc>=1 ? argv[0] : "sproxel.exe");

    MainWindow window(filename);
    main_window=&window;
    window.show();

    script_set_main_window(&window);
    scan_plugins();
    register_plugins();
    run_script("test.py");
    //get_python_console_widget()->toggleViewAction()->setChecked(true);

    int r=a.exec();
    unregister_plugins();
    close_script();
    main_window=NULL;
    return r;
}
