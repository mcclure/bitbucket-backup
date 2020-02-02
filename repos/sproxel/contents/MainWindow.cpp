#include <QtGui>

#include <iostream>

#include "Global.h"
#include "GLCamera.h"
#include "MainWindow.h"
#include "GLModelWidget.h"
#include "PreferencesDialog.h"
#include "ConsoleWidget.h"
#include "pyConsole.h"
#include "ImportExport.h"

#include <QFileDialog>
#include <QColorDialog>

#define DEFAULT_VOXGRID_SZ (8)

MainWindow::MainWindow(const QString& initialFilename, QWidget *parent) :
    QMainWindow(parent),
    m_appSettings("OpenSource", "Sproxel"),
    m_activeFilename(""),
    m_project(new SproxelProject())
{
    // Project
    VoxelGridGroupPtr sprite(new VoxelGridGroup(Imath::V3i(DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ),
      ColorPalettePtr()));
    sprite->setName("unnamed");
    m_project->sprites.push_back(sprite);

    // Windows
    m_glModelWidget = new GLModelWidget(this, &m_appSettings, &m_undoManager, sprite);
    setCentralWidget(m_glModelWidget);

    // The docking palette widget
    m_paletteDocker = new QDockWidget(tr("Palette"), this);
    m_paletteDocker->setObjectName("paletteDocker");
    m_paletteDocker->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_paletteWidget = new PaletteWidget(this, &m_undoManager);
    m_paletteDocker->setWidget(m_paletteWidget);
    m_paletteWidget->setPalette(m_project->mainPalette);
    addDockWidget(Qt::RightDockWidgetArea, m_paletteDocker);

    // The docking project widget
    m_projectDocker=new QDockWidget(tr("Project"), this);
    m_projectDocker->setObjectName("projectDocker");
    m_projectWidget=new ProjectWidget(this, &m_undoManager, &m_appSettings);
    m_projectDocker->setWidget(m_projectWidget);
    m_projectWidget->setProject(m_project);
    addDockWidget(Qt::RightDockWidgetArea, m_projectDocker);

    // The docking layers widget
    //m_layersDocker = new QDockWidget(tr("Layers"), this);
    //m_layersDocker->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    //m_layersWidget = new LayersWidget(this);
    //m_layersDocker->setWidget(m_layersWidget);
    //addDockWidget(Qt::RightDockWidgetArea, m_layersDocker);

    // Connect some window signals together
    QObject::connect(m_paletteWidget, SIGNAL(activeColorChanged(Imath::Color4f, int)),
                     m_glModelWidget, SLOT(setActiveColor(Imath::Color4f, int)));
    QObject::connect(m_glModelWidget, SIGNAL(colorSampled(Imath::Color4f, int)),
                     m_paletteWidget, SLOT(setActiveColor(Imath::Color4f, int)));
    QObject::connect(&m_undoManager, SIGNAL(cleanChanged(bool)),
                     this, SLOT(reactToModified(bool)));

    QObject::connect(m_projectWidget, SIGNAL(spriteSelected(VoxelGridGroupPtr)),
      m_glModelWidget, SLOT(setSprite(VoxelGridGroupPtr)));


    // Toolbar
    m_toolbar = new QToolBar("Tools", this);
    m_toolbar->setObjectName("toolbar");
    m_toolbar->setOrientation(Qt::Vertical);
    addToolBar(Qt::LeftToolBarArea, m_toolbar);


    // Actions & Menus
    menuBar()->show();
    m_menuFile = menuBar()->addMenu("Fi&le");

    m_actFileNew = new QAction("&New", this);
    m_actFileNew->setShortcut(Qt::CTRL + Qt::Key_N);
    m_menuFile->addAction(m_actFileNew);
    connect(m_actFileNew, SIGNAL(triggered()),
            this, SLOT(newGrid()));

    m_menuFile->addSeparator();

    m_actFileOpen = new QAction("&Open", this);
    m_actFileOpen->setShortcut(Qt::CTRL + Qt::Key_O);
    m_menuFile->addAction(m_actFileOpen);
    connect(m_actFileOpen, SIGNAL(triggered()),
            this, SLOT(openFile()));

    m_actFileSave = new QAction("&Save", this);
    m_actFileSave->setShortcut(Qt::CTRL + Qt::Key_S);
    m_menuFile->addAction(m_actFileSave);
    connect(m_actFileSave, SIGNAL(triggered()),
            this, SLOT(saveFile()));

    m_actFileSaveAs = new QAction("Save &As", this);
    m_menuFile->addAction(m_actFileSaveAs);
    connect(m_actFileSaveAs, SIGNAL(triggered()),
            this, SLOT(saveFileAs()));

    m_menuFile->addSeparator();

    m_actFileImport = new QAction("&Import...", this);
    m_menuFile->addAction(m_actFileImport);
    connect(m_actFileImport, SIGNAL(triggered()),
            this, SLOT(import()));

    m_actFileExportGrid = new QAction("&Export...", this);
    m_menuFile->addAction(m_actFileExportGrid);
    connect(m_actFileExportGrid, SIGNAL(triggered()),
            this, SLOT(exportGrid()));

    m_menuFile->addSeparator();

    m_actQuit = new QAction("&Quit", this);
    m_actQuit->setShortcut(Qt::CTRL + Qt::Key_Q);
    m_menuFile->addAction(m_actQuit);
    connect(m_actQuit, SIGNAL(triggered()),
            this, SLOT(close()));


    // ------ edit menu
    m_menuEdit = menuBar()->addMenu("&Edit");

    m_actUndo=m_undoManager.createUndoAction(this, "Undo");
    m_actUndo->setShortcut(Qt::CTRL + Qt::Key_Z);
    m_menuEdit->addAction(m_actUndo);

    m_actRedo=m_undoManager.createRedoAction(this, "Redo");
    m_actRedo->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
    m_menuEdit->addAction(m_actRedo);

    m_menuEdit->addSeparator();

    m_actShiftUp = new QAction("Shift up", this);
    m_actShiftUp->setShortcut(Qt::CTRL + Qt::Key_BracketRight);
    m_menuEdit->addAction(m_actShiftUp);
    connect(m_actShiftUp, SIGNAL(triggered()),
            this, SLOT(shiftUp()));

    m_actShiftDown = new QAction("Shift down", this);
    m_actShiftDown->setShortcut(Qt::CTRL + Qt::Key_BracketLeft);
    m_menuEdit->addAction(m_actShiftDown);
    connect(m_actShiftDown, SIGNAL(triggered()),
            this, SLOT(shiftDown()));

    m_actShiftWrap = new QAction("Wrap shift ops", this);
    m_actShiftWrap->setCheckable(true);
    m_actShiftWrap->setChecked(m_glModelWidget->shiftWrap());
    m_menuEdit->addAction(m_actShiftWrap);
    connect(m_actShiftWrap, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setShiftWrap(bool)));

    m_actRotateCw = new QAction("Rotate clockwise", this);
    m_actRotateCw->setShortcut(Qt::CTRL + Qt::Key_Greater);
    m_menuEdit->addAction(m_actRotateCw);
    connect(m_actRotateCw, SIGNAL(triggered()),
            this, SLOT(rotateCw()));

    m_actRotateCcw = new QAction("Rotate counter-clockwise", this);
    m_actRotateCcw->setShortcut(Qt::CTRL + Qt::Key_Less);
    m_menuEdit->addAction(m_actRotateCcw);
    connect(m_actRotateCcw, SIGNAL(triggered()),
            this, SLOT(rotateCcw()));

    m_actMirror = new QAction("Mirror", this);
    m_actMirror->setShortcut(Qt::CTRL + Qt::Key_M);
    m_menuEdit->addAction(m_actMirror);
    connect(m_actMirror, SIGNAL(triggered()),
            this, SLOT(mirror()));

    m_menuEdit->addSeparator();

    m_actPreferences = new QAction("Preferences...", this);
    m_menuEdit->addAction(m_actPreferences);
    connect(m_actPreferences, SIGNAL(triggered()),
            this, SLOT(editPreferences()));


    // ------ grid menu
    m_menuGrid = menuBar()->addMenu("&Grid");

    m_actExtendUp = new QAction("Extend grid dimension up", this);
    m_actExtendUp->setShortcut(Qt::CTRL + Qt::Key_Plus);
    m_menuGrid->addAction(m_actExtendUp);
    connect(m_actExtendUp, SIGNAL(triggered()),
            this, SLOT(extendUp()));

    m_actExtendDown = new QAction("Extend grid dimension down", this);
    m_actExtendDown->setShortcut(Qt::CTRL + Qt::Key_Minus);
    m_menuGrid->addAction(m_actExtendDown);
    connect(m_actExtendDown, SIGNAL(triggered()),
            this, SLOT(extendDown()));

    m_actContractUp = new QAction("Contract grid dimension from above", this);
    m_menuGrid->addAction(m_actContractUp);
    connect(m_actContractUp, SIGNAL(triggered()),
            this, SLOT(contractUp()));

    m_actContractDown = new QAction("Contract grid dimension from below", this);
    m_menuGrid->addAction(m_actContractDown);
    connect(m_actContractDown, SIGNAL(triggered()),
            this, SLOT(contractDown()));

    m_menuGrid->addSeparator();

    m_actUpRes = new QAction("Double grid resolution", this);
    m_menuGrid->addAction(m_actUpRes);
    connect(m_actUpRes, SIGNAL(triggered()),
            this, SLOT(upRes()));

    m_actDownRes = new QAction("Half grid resolution", this);
    m_menuGrid->addAction(m_actDownRes);
    connect(m_actDownRes, SIGNAL(triggered()),
            this, SLOT(downRes()));


    // ------ view menu
    m_menuView = menuBar()->addMenu("&View");

    QAction *action=new QAction(tr("Frame sprite"), this);
    action->setShortcut(Qt::Key_Z);
    m_menuView->addAction(action);
    connect(action, SIGNAL(triggered()),
      m_glModelWidget, SLOT(frameFull()));

    m_actViewGrid = new QAction("View Grid", this);
    m_actViewGrid->setShortcut(Qt::CTRL + Qt::Key_G);
    m_actViewGrid->setCheckable(true);
    m_actViewGrid->setChecked(m_glModelWidget->drawGrid());
    m_menuView->addAction(m_actViewGrid);
    connect(m_actViewGrid, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawGrid(bool)));

    m_actViewVoxgrid = new QAction("Voxel Grid", this);
    m_actViewVoxgrid->setShortcut(Qt::Key_G);
    m_actViewVoxgrid->setCheckable(true);
    m_actViewVoxgrid->setChecked(m_glModelWidget->drawVoxelGrid());
    m_menuView->addAction(m_actViewVoxgrid);
    connect(m_actViewVoxgrid, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawVoxelGrid(bool)));

    m_actViewBBox = new QAction("Bounding Box", this);
    m_actViewBBox->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actViewBBox->setCheckable(true);
    m_actViewBBox->setChecked(m_glModelWidget->drawBoundingBox());
    m_menuView->addAction(m_actViewBBox);
    connect(m_actViewBBox, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawBoundingBox(bool)));

    action=new QAction("Sprite Bounds", this);
    action->setShortcut(Qt::Key_B);
    action->setCheckable(true);
    action->setChecked(m_glModelWidget->drawSpriteBounds());
    m_menuView->addAction(action);
    connect(action, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawSpriteBounds(bool)));


    // ------ window menu
    m_menuWindow = menuBar()->addMenu("&Window");
    m_menuWindow->addAction(m_toolbar->toggleViewAction());
    m_menuWindow->addAction(m_paletteDocker->toggleViewAction());
    m_menuWindow->addAction(m_projectDocker->toggleViewAction());
    //m_menuWindow->addAction(m_layersDocker->toggleViewAction());
    m_menuWindow->addAction(get_python_console_widget()->toggleViewAction());
    get_python_console_widget()->toggleViewAction()->setChecked(false);


    // ------ toolbar hookups
    // Icons from the brilliant icon pack located at : http://pen-art.ru/
    m_toolbarActionGroup = new QActionGroup(this);

    m_actToolSplat = new QAction("Splat", m_toolbarActionGroup);
    m_actToolSplat->setIcon(QIcon(QPixmap(":/icons/splat.png")));
    m_actToolSplat->setCheckable(true);
    connect(m_actToolSplat, SIGNAL(toggled(bool)), this, SLOT(setToolSplat(bool)));

    m_actToolReplace = new QAction("Replace", m_toolbarActionGroup);
    m_actToolReplace->setIcon(QIcon(QPixmap(":/icons/pencil.png")));
    m_actToolReplace->setCheckable(true);
    connect(m_actToolReplace, SIGNAL(toggled(bool)), this, SLOT(setToolReplace(bool)));

    m_actToolFlood = new QAction("Flood", m_toolbarActionGroup);
    m_actToolFlood->setIcon(QIcon(QPixmap(":/icons/paintBucket.png")));
    m_actToolFlood->setCheckable(true);
    connect(m_actToolFlood, SIGNAL(toggled(bool)), this, SLOT(setToolFlood(bool)));

    m_actToolDropper = new QAction("Dropper", m_toolbarActionGroup);
    m_actToolDropper->setIcon(QIcon(QPixmap(":/icons/eyeDropper.png")));
    m_actToolDropper->setCheckable(true);
    connect(m_actToolDropper, SIGNAL(toggled(bool)), this, SLOT(setToolDropper(bool)));

    m_actToolEraser = new QAction("Eraser", m_toolbarActionGroup);
    m_actToolEraser->setIcon(QIcon(QPixmap(":/icons/eraser.png")));
    m_actToolEraser->setCheckable(true);
    connect(m_actToolEraser, SIGNAL(toggled(bool)), this, SLOT(setToolEraser(bool)));

    m_actToolSlab = new QAction("Slab", m_toolbarActionGroup);
    m_actToolSlab->setIcon(QIcon(QPixmap(":/icons/slab.png")));
    m_actToolSlab->setCheckable(true);
    connect(m_actToolSlab, SIGNAL(toggled(bool)), this, SLOT(setToolSlab(bool)));

    m_actToolLine = new QAction("Line", m_toolbarActionGroup);
    m_actToolLine->setIcon(QIcon(QPixmap(":/icons/line.png")));
    m_actToolLine->setCheckable(true);
    connect(m_actToolLine, SIGNAL(toggled(bool)), this, SLOT(setToolLine(bool)));

    //m_actToolRay = new QAction("Ray", this);

    m_actToolSplat->setChecked(true);
    m_toolbar->addActions(m_toolbarActionGroup->actions());


    // Remaining verbosity
    setWindowTitle(BASE_WINDOW_TITLE);
    statusBar()->showMessage(tr("Ready"));

    // Load up some settings
    if (m_appSettings.value("saveUILayout", true).toBool())
    {
        resize(m_appSettings.value("MainWindow/size", QSize(546, 427)).toSize());
        move(m_appSettings.value("MainWindow/position", QPoint(200, 200)).toPoint());
        setWindowState((Qt::WindowStates)m_appSettings.value("MainWindow/windowState", Qt::WindowActive).toInt());
        restoreState(m_appSettings.value("MainWindow/widgetsState").toByteArray());
        m_toolbar->setVisible(m_appSettings.value("toolbar/visibility", true).toBool());
        m_paletteDocker->setVisible(m_appSettings.value("paletteWindow/visibility", true).toBool());
        m_projectDocker->setVisible(m_appSettings.value("projectWindow/visibility", true).toBool());
        //m_layersDocker->setVisible(m_appSettings.value("layersWindow/visibility", true).toBool());
    }

    // Load the commandline supplied filename
    if (initialFilename != "")
    {
        openFile(initialFilename);
    }

    // Better way to keep the state in one place
    //std::cout << (m_toolbarActionGroup->checkedAction()->text() == "Splat") << std::endl;
    //std::cout << qPrintable(m_toolbarActionGroup->checkedAction()->text()) << std::endl;

    // Start things off focused on the GLWidget
    m_glModelWidget->setFocus();
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    // Confirmation dialog
    if (!m_undoManager.isClean())
    {
        switch (fileModifiedDialog())
        {
            case QMessageBox::Save: saveFile(); event->accept(); break;
            case QMessageBox::Discard:          event->accept(); break;
            case QMessageBox::Cancel:           event->ignore(); break;
            default: event->ignore(); break;
        }
    }
    else
    {
        event->accept();
    }

    m_glModelWidget->saveSettings();

    // Save some window settings on exit (if requested)
    if (m_appSettings.value("saveUILayout", true).toBool())
    {
        m_appSettings.setValue("MainWindow/size", size());
        m_appSettings.setValue("MainWindow/position", pos());
        m_appSettings.setValue("MainWindow/windowState", (int)windowState());
        m_appSettings.setValue("MainWindow/widgetsState", saveState());
        m_appSettings.setValue("toolbar/visibility", m_toolbar->isVisible());
        m_appSettings.setValue("paletteWindow/visibility", m_paletteDocker->isVisible());
        m_appSettings.setValue("projectWindow/visibility", m_projectDocker->isVisible());
        //m_appSettings.setValue("layersWindow/visibility", m_layersDocker->isVisible());
    }

    if (event->isAccepted()) close_python_console();
}


void MainWindow::keyPressEvent(QKeyEvent* event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;
    //const bool shiftDown = event->modifiers() & Qt::ShiftModifier;

    if (altDown && event->key() == Qt::Key_X)
    {
        m_glModelWidget->setCurrentAxis(X_AXIS);
    }
    else if (altDown && event->key() == Qt::Key_Y)
    {
        m_glModelWidget->setCurrentAxis(Y_AXIS);
    }
    else if (altDown && event->key() == Qt::Key_Z)
    {
        m_glModelWidget->setCurrentAxis(Z_AXIS);
    }
    else if (ctrlDown && event->key() == Qt::Key_C)
    {
        QColor color = QColorDialog::getColor(Qt::white, this);
        m_paletteWidget->setActiveColor(Imath::Color4f((float)color.red()/255.0f,
                                                       (float)color.green()/255.0f,
                                                       (float)color.blue()/255.0f,
                                                       (float)color.alpha()/255.0f),
                                                       -1);
    }
    else if (ctrlDown && event->key() == Qt::Key_F)
    {
        // Frame the full extents no matter what
        m_glModelWidget->frame(true);
    }
    else if (event->key() == Qt::Key_F)
    {
        // Frame the data if it exists
        m_glModelWidget->frame(false);
    }
    else if (event->key() == Qt::Key_X)
    {
        m_paletteWidget->swapColors();
    }
    //else if (event->key() == Qt::Key_D)
    //{
    //    m_paletteWidget->setDefaultColors();
    //}

    else if (event->key() == Qt::Key_Q) m_actToolSplat->setChecked(true);
    else if (event->key() == Qt::Key_W) m_actToolReplace->setChecked(true);
    else if (event->key() == Qt::Key_E) m_actToolFlood->setChecked(true);
    else if (event->key() == Qt::Key_R) m_actToolDropper->setChecked(true);
    else if (event->key() == Qt::Key_T) m_actToolEraser->setChecked(true);
    else if (event->key() == Qt::Key_Y) m_actToolSlab->setChecked(true);
    else if (event->key() == Qt::Key_U) m_actToolLine->setChecked(true);

    else if (event->key() >= Qt::Key_Left && event->key() <= Qt::Key_PageDown)
    {
        m_glModelWidget->handleArrows(event);
    }
    else if (event->key() == Qt::Key_Space)
    {
        // It's okay to call setVoxelColor once on the model widget, but any more requires an internal wrapper
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(),
                                       m_glModelWidget->activeColor(),
                                       m_glModelWidget->activeIndex());
        m_glModelWidget->updateGL();
    }
    else if (event->key() == Qt::Key_Delete)
    {
        // It's okay to call setVoxelColor once on the model widget, but any more requires an internal wrapper
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(),
                                       Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f),
                                       0);
        m_glModelWidget->updateGL();
    }
}


int MainWindow::fileModifiedDialog()
{
    QMessageBox msgBox;
    msgBox.setText("The document has been modified.");
    msgBox.setInformativeText("Do you want to save your changes?");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    return msgBox.exec();
}


void MainWindow::newGrid()
{
    // Confirmation dialog
    if (!m_undoManager.isClean())
    {
        switch (fileModifiedDialog())
        {
            case QMessageBox::Save: saveFile(); break;
            case QMessageBox::Discard: break;
            case QMessageBox::Cancel: return; break;
        }
    }

    NewGridDialog dlg(this);
    dlg.setModal(true);
    if (dlg.exec())
    {
        m_activeFilename = "";
        m_undoManager.clear();
        setWindowTitle(BASE_WINDOW_TITLE + " - " + m_activeFilename);  // TODO: Functionize (resetWindowTitle)

        // create new project
        m_project=new SproxelProject();
        VoxelGridGroupPtr sprite(new VoxelGridGroup(dlg.getVoxelSize(),
          dlg.isIndexed()?m_project->mainPalette:ColorPalettePtr()));
        sprite->setName("unnamed");
        m_project->sprites.push_back(sprite);

        m_glModelWidget->setSprite(sprite);
        m_paletteWidget->setPalette(m_project->mainPalette);
        m_projectWidget->setProject(m_project);
    }
}


void MainWindow::saveFile()
{
    if (m_undoManager.isClean())
        return;

    if (m_activeFilename == "")
        return saveFileAs();

    bool success = save_project(m_activeFilename, m_project);

    if (success)
      m_undoManager.setClean();
    else
      QMessageBox::critical(this, "Sproxel Error", QString("Error saving project to file ")+m_activeFilename);
}


void MainWindow::saveFileAs()
{
    QFileDialog fd(this, "Save voxel file as...");
    fd.setFilter(tr("Sproxel project (*.sxl)"));
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.exec();
    QStringList qsl = fd.selectedFiles();
    if (qsl.isEmpty()) return;
    if (QFileInfo(qsl[0]).isDir()) return;  // It returns the directory if you press Cancel

    QString filename = qsl[0];
    QString activeFilter = fd.selectedNameFilter();

    // Switch on save type
    bool success = false;
    if (!filename.endsWith(".sxl", Qt::CaseInsensitive))
        filename.append(".sxl");
    success = save_project(filename, m_project);

    if (success)
    {
        m_undoManager.setClean();
        m_activeFilename = filename;
        setWindowTitle(BASE_WINDOW_TITLE + " - " + m_activeFilename);  // TODO: Functionize (resetWindowTitle)
    }
    else
      QMessageBox::critical(this, "Sproxel Error", QString("Error saving project to file ")+filename);
}


void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Select file to Open..."),
        QString(),
        tr("Sproxel projects (*.sxl)"));
    if (filename.isEmpty())
        return;

    openFile(filename);
}


void MainWindow::openFile(QString filename)
{
    // Confirmation dialog
    if (!m_undoManager.isClean())
    {
        switch (fileModifiedDialog())
        {
            case QMessageBox::Save: saveFile(); break;
            case QMessageBox::Discard: break;
            case QMessageBox::Cancel: return; break;
        }
    }

    bool success = false;
    SproxelProjectPtr project=load_project(filename);
    if (project) { m_project=project; success=true; }

    if (success)
    {
        m_undoManager.clear();

        if (m_project->sprites.empty())
        {
          VoxelGridGroupPtr sprite(new VoxelGridGroup(
            Imath::V3i(DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ, DEFAULT_VOXGRID_SZ),
            ColorPalettePtr()));
          sprite->setName("unnamed");
          m_project->sprites.push_back(sprite);
        }

        m_glModelWidget->setSprite(m_project->sprites[0]);
        m_paletteWidget->setPalette(m_project->mainPalette);
        m_projectWidget->setProject(m_project);

        m_activeFilename = filename;
        setWindowTitle(BASE_WINDOW_TITLE + " - " + m_activeFilename);  // TODO: Functionize (resetWindowTitle)
        if (m_appSettings.value("frameOnOpen", false).toBool())
            m_glModelWidget->frame(true);
    }
    else
      QMessageBox::critical(this, "Sproxel Error", QString("Error loading project from file ")+filename);
}


void MainWindow::import()
{
  const QList<Importer*> &importers=get_importers();

  QStringList filters;
  filters.reserve(importers.size());
  foreach (Importer *imp, importers) filters += imp->name()+" ("+imp->filter()+")";

  QFileDialog fd(this, "Import file...");
  fd.setNameFilters(filters);
  fd.setAcceptMode(QFileDialog::AcceptOpen);
  fd.setFileMode(QFileDialog::ExistingFiles);
  if (!fd.exec()) return;

  QStringList files=fd.selectedFiles();
  if (files.isEmpty()) return;

  QString activeFilter=fd.selectedNameFilter();
  Importer *activeImporter=NULL;
  for (int i=0; i<filters.size(); ++i)
    if (filters[i]==activeFilter) { activeImporter=importers[i]; break; }

  if (!activeImporter) return;

  foreach (const QString &filename, files)
  {
    if (QFileInfo(filename).isDir()) continue;
    activeImporter->doImport(filename, &m_undoManager, m_project, m_glModelWidget->getSprite());
  }
}


void MainWindow::exportGrid()
{
  const QList<Exporter*> &exporters=get_exporters();

  QStringList filters;
  filters.reserve(exporters.size());
  foreach (Exporter *exp, exporters) filters += exp->name()+" ("+exp->filter()+")";

  QFileDialog fd(this, "Export file...");
  fd.setNameFilters(filters);
  fd.setAcceptMode(QFileDialog::AcceptSave);
  fd.selectNameFilter(m_appSettings.value("lastExportFilter", QString()).toString());
  fd.selectFile(m_appSettings.value("lastExportFile", QString()).toString());
  if (!fd.exec()) return;

  QStringList files=fd.selectedFiles();
  if (files.isEmpty()) return;

  QString filename=files[0];
  if (QFileInfo(filename).isDir()) return;

  QString activeFilter=fd.selectedNameFilter();
  Exporter *activeExporter=NULL;
  for (int i=0; i<filters.size(); ++i)
    if (filters[i]==activeFilter) { activeExporter=exporters[i]; break; }

  if (!activeExporter) return;

  if (!activeExporter->doExport(filename, m_project, m_glModelWidget->getSprite()))
    QMessageBox::critical(this, "Sproxel Error", QString("Failed to export ")+filename);

  m_appSettings.setValue("lastExportFile", filename);
  m_appSettings.setValue("lastExportFilter", activeFilter);
}


void MainWindow::editPreferences()
{
    PreferencesDialog dlg(this, &m_appSettings);
    dlg.setModal(true);
    QObject::connect(&dlg, SIGNAL(preferenceChanged()), m_glModelWidget, SLOT(updateGL()));
    dlg.exec();
}


// Trampoline functions because QSignalMapper can't do complex args
// Search for QBoundMethod for a custom approach, but I'm too lazy to include it for now.
void MainWindow::shiftUp()
{
    m_glModelWidget->shiftVoxels(m_glModelWidget->currentAxis(),
                                 true, m_glModelWidget->shiftWrap());
}
void MainWindow::shiftDown()
{
    m_glModelWidget->shiftVoxels(m_glModelWidget->currentAxis(),
                                 false, m_glModelWidget->shiftWrap());
}
void MainWindow::rotateCw()
{
    m_glModelWidget->rotateVoxels(m_glModelWidget->currentAxis(), 1);
}
void MainWindow::rotateCcw()
{
    m_glModelWidget->rotateVoxels(m_glModelWidget->currentAxis(), -1);
}
void MainWindow::mirror()
{
    m_glModelWidget->mirrorVoxels(m_glModelWidget->currentAxis());
}

void MainWindow::extendUp()
{
    Imath::V3i sizeInc(0,0,0);
    switch (m_glModelWidget->currentAxis())
    {
        case X_AXIS: sizeInc.x += 1; break;
        case Y_AXIS: sizeInc.y += 1; break;
        case Z_AXIS: sizeInc.z += 1; break;
    }
    m_glModelWidget->resizeAndShiftVoxelGrid(sizeInc, Imath::V3i(0,0,0));
}

void MainWindow::extendDown()
{
    Imath::V3i shift(0,0,0);
    Imath::V3i sizeInc(0,0,0);
    switch (m_glModelWidget->currentAxis())
    {
        case X_AXIS: sizeInc.x += 1; shift.x += 1; break;
        case Y_AXIS: sizeInc.y += 1; shift.y += 1; break;
        case Z_AXIS: sizeInc.z += 1; shift.z += 1; break;
    }
    m_glModelWidget->resizeAndShiftVoxelGrid(sizeInc, shift);
}

void MainWindow::contractUp()
{
    Imath::V3i sizeInc(0,0,0);
    switch (m_glModelWidget->currentAxis())
    {
        case X_AXIS: sizeInc.x -= 1; break;
        case Y_AXIS: sizeInc.y -= 1; break;
        case Z_AXIS: sizeInc.z -= 1; break;
    }
    m_glModelWidget->resizeAndShiftVoxelGrid(sizeInc, Imath::V3i(0,0,0));
}

void MainWindow::contractDown()
{
    Imath::V3i shift(0,0,0);
    Imath::V3i sizeInc(0,0,0);
    switch (m_glModelWidget->currentAxis())
    {
        case X_AXIS: sizeInc.x -= 1; shift.x -= 1; break;
        case Y_AXIS: sizeInc.y -= 1; shift.y -= 1; break;
        case Z_AXIS: sizeInc.z -= 1; shift.z -= 1; break;
    }
    m_glModelWidget->resizeAndShiftVoxelGrid(sizeInc, shift);
}

void MainWindow::upRes()   { m_glModelWidget->reresVoxelGrid(2.0f); }
void MainWindow::downRes() { m_glModelWidget->reresVoxelGrid(0.5f); }

void MainWindow::setToolSplat(bool stat)   { if (stat) m_glModelWidget->setActiveTool(TOOL_SPLAT); }
void MainWindow::setToolFlood(bool stat)   { if (stat) m_glModelWidget->setActiveTool(TOOL_FLOOD); }
void MainWindow::setToolRay(bool stat)     { if (stat) m_glModelWidget->setActiveTool(TOOL_RAY); }
void MainWindow::setToolDropper(bool stat) { if (stat) m_glModelWidget->setActiveTool(TOOL_DROPPER); }
void MainWindow::setToolEraser(bool stat)  { if (stat) m_glModelWidget->setActiveTool(TOOL_ERASER); }
void MainWindow::setToolReplace(bool stat) { if (stat) m_glModelWidget->setActiveTool(TOOL_REPLACE); }
void MainWindow::setToolSlab(bool stat)    { if (stat) m_glModelWidget->setActiveTool(TOOL_SLAB); }
void MainWindow::setToolLine(bool stat)    { if (stat) m_glModelWidget->setActiveTool(TOOL_LINE); }

void MainWindow::reactToModified(bool clean)
{
    QString current = windowTitle();

    if (!clean)
    {
        if (current.endsWith("*"))
            return;
        setWindowTitle(current + "*");
    }
    else
    {
        current.chop(1);
        setWindowTitle(current);
    }
}
